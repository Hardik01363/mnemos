#pragma once

#include "../config.h"
#include <fstream>
#include <mutex>

namespace mnemos{
    class DiskManager {
        private:
            std::string open_file_name;
            std::fstream open_file_handle;
            std::mutex lock;

        public:
            DiskManager(std::string filename);
            bool readpg(page_id_t id, std::byte* buffer_ptr);
            void wrtpg(page_id_t id, std::byte* buffer_ptr);
            ~DiskManager();
    };
}
