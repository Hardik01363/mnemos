#pragma once

//include statements

struct Page {
    uint8_t data[PAGE_SIZE];

    page_id_t get_page_id() {
    return *reinterpret_cast<page_id_t*>(&data[0]);
    }

    uint16_t get_fsp() {
    return *reinterpret_cast<uint16_t*>(&data[4]);
    }

    uint16_t get_slot_cnt() {
    return *reinterpret_cast<uint16_t*>(&data[6]);
    }

    bool get_is_dirty() {
    return *reinterpret_cast<bool*>(&data[8]);
    }

    //1 byte padding here as uint16_t would be referenced faster if the offset is divisible by 2
    
    uint16_t get_pin_cnt() {
    return *reinterpret_cast<uint16_t*>(&data[10]);
    }
    
    //12 bytes of padding is applied here in the page header for some future extensions (if needed). Header size is now a total of 24 bytes.
    
   void set_page_id(page_id_t id) {
        *reinterpret_cast<page_id_t*>(&data[0]) = id;
    }

    void set_fsp(uint16_t fsp) {
        *reinterpret_cast<uint16_t*>(&data[4]) = fsp;
    }

    void set_slot_cnt(uint16_t cnt) {
        *reinterpret_cast<uint16_t*>(&data[6]) = cnt;
    }

    void set_is_dirty(bool dirty) {
        *reinterpret_cast<bool*>(&data[8]) = dirty;
    }

    void set_pin_cnt(uint16_t cnt) {
        *reinterpret_cast<uint16_t*>(&data[10]) = cnt;
    }
}
