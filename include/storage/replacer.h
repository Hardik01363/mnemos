#pragma once

#include <cstddef>
#include "../config.h"

namespace mnemos {
    class Replacer {
    public:
        virtual ~Replacer() = default;
        virtual bool evict(frame_id_t* frame_id) = 0;
        virtual void record_access(frame_id_t frame_id) = 0;
        virtual void set_evictable(frame_id_t frame_id, bool set_evictable) = 0;
        virtual void remove(frame_id_t frame_id) = 0;
        virtual size_t size() = 0;
    };
}
