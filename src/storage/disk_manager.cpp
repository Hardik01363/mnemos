#include "../../include/storage/disk_manager.h"

namespace mnemos {

    bool DiskManager::readpg(page_id_t id, std::byte* buffer_ptr) {
        uint64_t byte_offset = id * PAGE_SIZE;

        std::lock_guard<std::mutex> guard(lock);
        open_file_handle.seekg(byte_offset);
        open_file_handle.read(reinterpret_cast<char*>(buffer_ptr), PAGE_SIZE);
    
        if(open_file_handle.good()) return true;
        else return false;
    }

    void DiskManager::wrtpg(page_id_t id, std::byte* buffer_ptr) {
        uint64_t byte_offset = id * PAGE_SIZE;

        std::lock_guard<std::mutex> guard(lock);
        open_file_handle.seekp(byte_offset);
        open_file_handle.write(reinterpret_cast<char*>(buffer_ptr), PAGE_SIZE);
        open_file_handle.flush();

        if(open_file_handle.good()) return;
        else std::abort();
    }

    DiskManager::DiskManager(std::string filename) {
        open_file_handle.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        if(!open_file_handle.good()) std::abort();
        open_file_name = filename;
    }

    DiskManager::~DiskManager() {
        open_file_handle.close();
    }

}
