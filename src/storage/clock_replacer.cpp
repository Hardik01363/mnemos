#include "../../include/storage/clock_replacer.h"

namespace mnemos {
    bool ClockReplacer::evict(frame_id_t* frame_id) {
        std::lock_guard<std::mutex> guard(lock);
        size_t n = clock_ring.size();
        for(size_t i = 0; i < 2 * n; ++i) {
            if(clock_ring[hand].evictable) {
                if(clock_ring[hand].ref_bit) {
                    clock_ring[hand].ref_bit = false;
                }
                else {
                    *frame_id = static_cast<frame_id_t>(hand);
                    clock_ring[hand].evictable = false;
                    hand = (hand + 1) % n;
                    return true;
                }
            }
            hand = (hand + 1) % n;
        }
        return false;
    }

    void ClockReplacer::record_access(frame_id_t frame_id) {
        std::lock_guard<std::mutex> guard(lock);
        clock_ring[frame_id].ref_bit = true;
    }

    void ClockReplacer::set_evictable(frame_id_t frame_id, bool set_evictable) {
        std::lock_guard<std::mutex> guard(lock);
        clock_ring[frame_id].evictable = set_evictable;
    }

    void ClockReplacer::remove(frame_id_t frame_id) {
        std::lock_guard<std::mutex> guard(lock);
        clock_ring[frame_id].ref_bit = false;
        clock_ring[frame_id].evictable = false;
    }

    size_t ClockReplacer::size() { return clock_ring.size(); }
}
