#pragma once

#include "../config.h"

namespace mnemos{
    class DiskManager {
        public:
            DiskManager(std::string filename);
            bool readpg(page_id_t, std::byte* buffer_ptr);
            void wrtpg(page_id_t, std::byte* buffer_ptr);
            ~DiskManager();
    };
}
