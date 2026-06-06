#include "../../include/storage/lruk_replacer.h"

namespace mnemos {

    LRUKReplacer::LRUKReplacer(size_t num_frames, int k) {
        pool_capacity = num_frames;
        k_value = k;
        replacer_size = 0;
        current_timestamp = 0;
    }

    bool LRUKReplacer::evict(frame_id_t* frame_id) {
        std::lock_guard<std::mutex> guard(lock);

        for (auto it = history_list.begin(); it != history_list.end(); ++it) {
            frame_id_t f_id = *it;
            if (node_store[f_id].is_evictable) {
                *frame_id = f_id;
                history_list.erase(it);
                node_store.erase(f_id);
                replacer_size--;
                return true;
            }
        }

        for (auto it = cache_list.begin(); it != cache_list.end(); ++it) {
            frame_id_t f_id = *it;
            if (node_store[f_id].is_evictable) {
                *frame_id = f_id;
                cache_list.erase(it);
                node_store.erase(f_id);
                replacer_size--;
                return true;
            }
        }

        return false;
    }

    void LRUKReplacer::record_access(frame_id_t frame_id) {
        std::lock_guard<std::mutex> guard(lock);

        if (frame_id >= pool_capacity) {
            return;
        }

        current_timestamp++;

        if (node_store.find(frame_id) == node_store.end()) {
            FrameMetadata meta;
            meta.timestamps.push_back(current_timestamp);
            meta.is_evictable = false;
            meta.in_cache = false;
            history_list.push_back(frame_id);
            meta.list_iterator = std::prev(history_list.end());
            node_store[frame_id] = meta;
            return;
        }

        auto& meta = node_store[frame_id];
        meta.timestamps.push_back(current_timestamp);

        if (meta.in_cache) {
            if (meta.timestamps.size() > static_cast<size_t>(k_value)) {
                meta.timestamps.erase(meta.timestamps.begin());
            }
            cache_list.erase(meta.list_iterator);
            cache_list.push_back(frame_id);
            meta.list_iterator = std::prev(cache_list.end());
            return;
        }

        if (meta.timestamps.size() == static_cast<size_t>(k_value)) {
            history_list.erase(meta.list_iterator);
            meta.in_cache = true;
            cache_list.push_back(frame_id);
            meta.list_iterator = std::prev(cache_list.end());
        }
    }

    void LRUKReplacer::set_evictable(frame_id_t frame_id, bool set_evictable) {
        std::lock_guard<std::mutex> guard(lock);

        if (node_store.find(frame_id) == node_store.end()) {
            return;
        }

        auto& meta = node_store[frame_id];
        if (meta.is_evictable == set_evictable) {
            return;
        }

        meta.is_evictable = set_evictable;
        if (set_evictable) {
            replacer_size++;
        } else {
            replacer_size--;
        }
    }

    void LRUKReplacer::remove(frame_id_t frame_id) {
        std::lock_guard<std::mutex> guard(lock);

        if (node_store.find(frame_id) == node_store.end()) {
            return;
        }

        auto& meta = node_store[frame_id];
        if (!meta.is_evictable) {
            return;
        }

        if (meta.in_cache) {
            cache_list.erase(meta.list_iterator);
      } else {
            history_list.erase(meta.list_iterator);
        }

        node_store.erase(frame_id);
        replacer_size--;
    }

    size_t LRUKReplacer::size() {
        std::lock_guard<std::mutex> guard(lock);
        return replacer_size;
    }
}
