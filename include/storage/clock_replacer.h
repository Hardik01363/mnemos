#pragma once
#include "replacer.h"
#include <vector>
#include <mutex>

namespace mnemos {
    class ClockReplacer : public Replacer {
    private:
        struct ClockNode {
            bool ref_bit{false};
            bool evictable{false};
        };
        std::vector<ClockNode> clock_ring;
        size_t hand{0};
        std::mutex lock;

    public:
        explicit ClockReplacer(size_t num_pages) : clock_ring(num_pages) {}
        bool evict(frame_id_t* frame_id) override;
        void record_access(frame_id_t frame_id) override;
        void set_evictable(frame_id_t frame_id, bool set_evictable) override;
        void remove(frame_id_t frame_id) override;
        size_t size() override;
    };
}
