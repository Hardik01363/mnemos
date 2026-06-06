#include "../../include/storage/buffer_pool_manager.h"

namespace mnemos {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager* disk_manager, LRUKReplacer* replacer) {
    this->pool_size = pool_size;
    this->disk_manager = disk_manager;
    this->replacer = replacer;
    this->pool = new Page[pool_size];
    this->next_page_id = 0;

    for (size_t i = 0; i < pool_size; ++i) {
        free_list.push_back(static_cast<frame_id_t>(i));
    }
}

BufferPoolManager::~BufferPoolManager() {
    flush_all_pages();
    delete[] pool;
}

size_t BufferPoolManager::get_shard_index(page_id_t page_id) {
    return page_id % PAGE_TABLE_SHARDS;
}

bool BufferPoolManager::find_frame(page_id_t page_id, frame_id_t* frame_id) {
    size_t shard_idx = get_shard_index(page_id);
    std::shared_lock<std::shared_mutex> guard(page_table[shard_idx].shard_lock);
    
    auto it = page_table[shard_idx].shard_map.find(page_id);
    if (it == page_table[shard_idx].shard_map.end()) {
        return false;
    }
    *frame_id = it->second;
    return true;
}

Page* BufferPoolManager::fetch_page(page_id_t page_id) {
    frame_id_t frame_id;
    
    if (find_frame(page_id, &frame_id)) {
        size_t shard_idx = get_shard_index(page_id);
        std::unique_lock<std::shared_mutex> shard_guard(page_table[shard_idx].shard_lock);
        
        uint16_t current_pin = pool[frame_id].get_pin_cnt();
        pool[frame_id].set_pin_cnt(current_pin + 1);
        replacer->record_access(frame_id);
        replacer->set_evictable(frame_id, false);
        return &pool[frame_id];
    }

    std::unique_lock<std::mutex> free_guard(free_list_lock);
    if (!free_list.empty()) {
        frame_id = free_list.back();
        free_list.pop_back();
        free_guard.unlock();
    } else {
        free_guard.unlock();
        if (!replacer->evict(&frame_id)) {
            return nullptr;
        }

        page_id_t victim_page_id = pool[frame_id].get_page_id();
        if (pool[frame_id].get_is_dirty()) {
            disk_manager->wrtpg(victim_page_id, reinterpret_cast<std::byte*>(pool[frame_id].data));
        }

        size_t victim_shard_idx = get_shard_index(victim_page_id);
        std::unique_lock<std::shared_mutex> victim_shard_guard(page_table[victim_shard_idx].shard_lock);
        page_table[victim_shard_idx].shard_map.erase(victim_page_id);
        victim_shard_guard.unlock();
    }

    size_t shard_idx = get_shard_index(page_id);
    std::unique_lock<std::shared_mutex> shard_guard(page_table[shard_idx].shard_lock);
    
    page_table[shard_idx].shard_map[page_id] = frame_id;
    
    pool[frame_id].set_page_id(page_id);
    pool[frame_id].set_pin_cnt(1);
    pool[frame_id].set_is_dirty(false);
    
    disk_manager->readpg(page_id, reinterpret_cast<std::byte*>(pool[frame_id].data));
    
    replacer->record_access(frame_id);
    replacer->set_evictable(frame_id, false);
    
    return &pool[frame_id];
}

bool BufferPoolManager::unpin_page(page_id_t page_id, bool is_dirty) {
    frame_id_t frame_id;
    if (!find_frame(page_id, &frame_id)) {
        return false;
    }

    size_t shard_idx = get_shard_index(page_id);
    std::unique_lock<std::shared_mutex> shard_guard(page_table[shard_idx].shard_lock);

    uint16_t current_pin = pool[frame_id].get_pin_cnt();
    if (current_pin == 0) {
        return false;
    }

    if (is_dirty) {
        pool[frame_id].set_is_dirty(true);
    }

    pool[frame_id].set_pin_cnt(current_pin - 1);
    if (current_pin - 1 == 0) {
        replacer->set_evictable(frame_id, true);
    }

    return true;
}

bool BufferPoolManager::flush_page(page_id_t page_id) {
    frame_id_t frame_id;
    if (!find_frame(page_id, &frame_id)) {
        return false;
    }

    size_t shard_idx = get_shard_index(page_id);
    std::shared_lock<std::shared_mutex> shard_guard(page_table[shard_idx].shard_lock);

    disk_manager->wrtpg(page_id, reinterpret_cast<std::byte*>(pool[frame_id].data));
    pool[frame_id].set_is_dirty(false);

    return true;
}

Page* BufferPoolManager::new_page(page_id_t* page_id) {
    std::unique_lock<std::mutex> id_guard(next_page_id_lock);
    page_id_t assigned_id = next_page_id;
    next_page_id++;
    id_guard.unlock();

    frame_id_t frame_id;
    std::unique_lock<std::mutex> free_guard(free_list_lock);
    if (!free_list.empty()) {
        frame_id = free_list.back();
        free_list.pop_back();
        free_guard.unlock();
    } else {
        free_guard.unlock();
        if (!replacer->evict(&frame_id)) {
            return nullptr;
        }

        page_id_t victim_page_id = pool[frame_id].get_page_id();
        if (pool[frame_id].get_is_dirty()) {
            disk_manager->wrtpg(victim_page_id, reinterpret_cast<std::byte*>(pool[frame_id].data));
        }

        size_t victim_shard_idx = get_shard_index(victim_page_id);
        std::unique_lock<std::shared_mutex> victim_shard_guard(page_table[victim_shard_idx].shard_lock);
        page_table[victim_shard_idx].shard_map.erase(victim_page_id);
        victim_shard_guard.unlock();
    }

    size_t shard_idx = get_shard_index(assigned_id);
    std::unique_lock<std::shared_mutex> shard_guard(page_table[shard_idx].shard_lock);
    
    page_table[shard_idx].shard_map[assigned_id] = frame_id;
    
    std::uint8_t zero_buffer[PAGE_SIZE] = {0};
    std::memcpy(pool[frame_id].data, zero_buffer, PAGE_SIZE);
    
    pool[frame_id].set_page_id(assigned_id);
    pool[frame_id].set_fsp(PAGE_SIZE);
    pool[frame_id].set_slot_cnt(0);
    pool[frame_id].set_pin_cnt(1);
    pool[frame_id].set_is_dirty(true);

    replacer->record_access(frame_id);
    replacer->set_evictable(frame_id, false);

    *page_id = assigned_id;
    return &pool[frame_id];
}

bool BufferPoolManager::delete_page(page_id_t page_id) {
    frame_id_t frame_id;
    if (!find_frame(page_id, &frame_id)) {
        return true;
    }

    size_t shard_idx = get_shard_index(page_id);
    std::unique_lock<std::shared_mutex> shard_guard(page_table[shard_idx].shard_lock);

    if (pool[frame_id].get_pin_cnt() > 0) {
        return false;
    }

    page_table[shard_idx].shard_map.erase(page_id);
    shard_guard.unlock();

    replacer->remove(frame_id);

    pool[frame_id].set_page_id(INVALID_PAGE_ID);
    pool[frame_id].set_is_dirty(false);
    pool[frame_id].set_pin_cnt(0);
    
    std::uint8_t zero_buffer[PAGE_SIZE] = {0};
    std::memcpy(pool[frame_id].data, zero_buffer, PAGE_SIZE);

    std::unique_lock<std::mutex> free_guard(free_list_lock);
    free_list.push_back(frame_id);

    return true;
}

void BufferPoolManager::flush_all_pages() {
    for (size_t i = 0; i < PAGE_TABLE_SHARDS; ++i) {
        std::shared_lock<std::shared_mutex> shard_guard(page_table[i].shard_lock);
        for (auto const& [page_id, frame_id] : page_table[i].shard_map) {
            if (pool[frame_id].get_is_dirty()) {
                disk_manager->wrtpg(page_id, reinterpret_cast<std::byte*>(pool[frame_id].data));
                pool[frame_id].set_is_dirty(false);
            }
        }
    }
}
}
