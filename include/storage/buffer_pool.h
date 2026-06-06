#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include "../config.h"
#include "page.h"
#include "disk_manager.h"
#include "lruk_replacer.h"

namespace mnemos {

    class BufferPoolManager {
    private:
        struct PageTableShard {
            std::unordered_map<page_id_t, frame_id_t> shard_map;
            std::shared_mutex shard_lock;
        };

        size_t pool_size;
        DiskManager* disk_manager;
        LRUKReplacer* replacer;
        Page* pool;
        std::vector<frame_id_t> free_list;
        PageTableShard page_table[PAGE_TABLE_SHARDS];
        std::mutex free_list_lock;
        page_id_t next_page_id;
        std::mutex next_page_id_lock;

        size_t get_shard_index(page_id_t page_id);
        bool find_frame(page_id_t page_id, frame_id_t* frame_id);

    public:
        BufferPoolManager(size_t pool_size, DiskManager* disk_manager, LRUKReplacer* replacer);
        ~BufferPoolManager();

        Page* fetch_page(page_id_t page_id);
        bool unpin_page(page_id_t page_id, bool is_dirty);
        bool flush_page(page_id_t page_id);
        Page* new_page(page_id_t* page_id);
        bool delete_page(page_id_t page_id);
        void flush_all_pages();
    };
}
