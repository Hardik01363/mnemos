#pragma once
#include "replacer.h"
#include <list>
#include <unordered_map>
#include <mutex>

namespace mnemos {
    class LRUReplacer : public Replacer {
    private:
        size_t capacity;
        std::list<frame_id_t> lru_list;
        std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> lru_map;
        std::mutex lock;

    public:
        explicit LRUReplacer(size_t num_pages) : capacity(num_pages) {}
        bool evict(frame_id_t* frame_id) override;
        void record_access(frame_id_t frame_id) override;
        void set_evictable(frame_id_t frame_id, bool set_evictable) override;
        void remove(frame_id_t frame_id) override;
        size_t size() override;
    };
}
