#pragma once

#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <list>
#include <vector>
#include <mutex>
#include "../config.h"

namespace mnemos {

    class LRUKReplacer {
    private:
        struct FrameMetadata {
            std::list<frame_id_t>::iterator list_iterator;
            std::vector<uint64_t> timestamps;
            bool is_evictable;
            bool in_cache;
        };

        size_t replacer_size;
        size_t pool_capacity;
        int k_value;
        uint64_t current_timestamp;
        std::list<frame_id_t> history_list;
        std::list<frame_id_t> cache_list;
        std::unordered_map<frame_id_t, FrameMetadata> node_store;
        std::mutex lock;

    public:
        explicit LRUKReplacer(size_t num_frames, int k = 2);
        ~LRUKReplacer() = default;

        bool evict(frame_id_t* frame_id);
        void record_access(frame_id_t frame_id);
        void set_evictable(frame_id_t frame_id, bool set_evictable);
        void remove(frame_id_t frame_id);
        size_t size();
    };
}
