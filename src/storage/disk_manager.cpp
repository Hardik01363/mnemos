//include statements

bool readpg(page_id_t, std::byte* buffer_ptr) {
    uint64_t byte_offset = page_id_t*PAGE_SIZE;

    std::lock_guard<std::mutex> guard(lock);
    open_file_handle.seekg(byte_offset);
    open_file_handle.read(reinterpret_cast<char*>(buffer_ptr), PAGE_SIZE);
    
    if(open_file_handle.good()) return true;
    else return false;
}

void wrtpg(page_id_t, std::byte* buffer_ptr) {
    uint64_t byte_offset = page_id_t*PAGE_SIZE;

    std::lock_guard<std::mutex> guard(lock);
    open_file_handle.seekp(byte_offset);
    open_file_handle.write(reinterpret_cast<char*>(buffer_ptr), PAGE_SIZE);
    open_file_handle.flush()

    if(open_file_handle.good()) return;
    else std::abort();
}

DiskManager(std::string filename) {
    
}
