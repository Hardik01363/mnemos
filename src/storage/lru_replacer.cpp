#include "../../include/storage/lru_replacer.h"

namespace mnemos {
    bool LRUReplacer::evict(frame_id_t* frame_id) {
        std::lock_guard<std::mutex> guard(lock);
        if(lru_list.empty()) {return false;}
        *frame_id = lru_list.back();
        lru_map.erase(*frame_id);
        lru_list.pop_back();
        return true;
    }

    void LRUReplacer::record_access(frame_id_t frame_id) {
        std::lock_guard<std::mutex> guard(lock);
        if(lru_map.count(frame_id)) {
            lru_list.erase(lru_map[frame_id]);
        }
        lru_list.push_front(frame_id);
        lru_map[frame_id] = lru_list.begin();
    }

    void LRUReplacer::set_evictable(frame_id_t, bool) {}
    void LRUReplacer::remove(frame_id_t frame_id) {
        std::lock_guard<std::mutex> guard(lock);
        if(lru_map.count(frame_id)) {
            lru_list.erase(lru_map[frame_id]);
            lru_map.erase(frame_id);
        }
    }
    size_t LRUReplacer::size() {
        std::lock_guard<std::mutex> guard(lock);
        return lru_list.size();
    }
}
