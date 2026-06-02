#include "../../include/storage/page.h"
#include <cstring>
#include <utility>

namespace mnemos {

    slot_id_t Page::insert_slot(uint8_t* input_data, uint16_t length) {
        uint16_t slot_cnt = get_slot_cnt();
        uint16_t new_slot_id = slot_cnt;
        uint16_t fsp = get_fsp();
        
        if(length + sizeof(SlotEntry) > fsp - PAGE_HEADER_SIZE - slot_cnt * sizeof(SlotEntry)) return INVALID_SLOT_ID;
        
        SlotEntry entry;
        entry.offset = fsp - length;
        entry.length = length;

        memcpy(&data[PAGE_HEADER_SIZE + slot_cnt * sizeof(SlotEntry)], &entry, sizeof(SlotEntry));

        set_slot_cnt(slot_cnt + 1);
        memcpy(&data[entry.offset], input_data, length);
        set_fsp(fsp - length);
        return new_slot_id;
    }

    std::pair<bool, uint16_t> Page::read_slot(slot_id_t id, uint8_t* buffer) {
        uint16_t slot_cnt = get_slot_cnt();
        if(id >= slot_cnt || *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE + id * sizeof(SlotEntry)]) == 0) return std::make_pair(false, 0);
        
        uint16_t tuple_offset = *reinterpret_cast<uint16_t*>(PAGE_HEADER_SIZE + id * sizeof(SlotEntry));
        uint16_t read_data_length = *reinterpret_cast<uint16_t*>(PAGE_HEADER_SIZE + id * sizeof(SlotEntry) + 2);
        memcpy(buffer, &data[tuple_offset], read_data_length);
        return std::make_pair(true, read_data_length);
    }

    bool Page::update_slot(slot_id_t id, uint8_t* input_data, uint16_t length) {
        uint16_t slot_cnt = get_slot_cnt();

        if(id >= slot_cnt || *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE + id * sizeof(SlotEntry)]) == 0) return false;
        
        uint16_t old_offset = *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE + id * sizeof(SlotEntry)]);
        uint16_t old_length = *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE + id * sizeof(SlotEntry) + 2]);
        
        if(length <= old_length) {
            memcpy(&data[old_offset], input_data, length);
            *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE + id * sizeof(SlotEntry) + 2]) = length;
            return true;
        }
        
        if(length > fsp - PAGE_HEADER_SIZE - slot_cnt * sizeof(SlotEntry)) return false;

        *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE + id * sizeof(SlotEntry)]) = fsp - length;
        *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE + id * sizeof(SlotEntry]) + 2]) = length;
        memcpy(&data[fsp - length], input_data, length);
        set_fsp(fsp - length);
        return true;
    }

    bool Page::delete_slot(slot_id_t id) {
        uint16_t slot_cnt = get_slot_cnt();
        if(id >= slot_cnt) return false;
        
        *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE + id * sizeof(SlotEntry)]) = 0;
        *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE + id * sizeof(SlotEntry) + 2]) = 0;
        return true;
    }

    bool Page::compact() {
        uint8_t page[PAGE_SIZE];
        uint16_t slot_cnt = get_slot_cnt();
        uint16_t page_fsp = PAGE_SIZE;
        memcpy(&page[0], &data[0], PAGE_SIZE);

        for(int i = 0; i < slot_cnt; i++) {
            uint16_t i_offset = *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE + i * sizeof(SlotEntry)]);
            uint16_t i_length = *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE + i * sizeof(SlotEntry) + 2]);

            if(i_offset == 0) continue;

            *reinterpret_cast<uint16_t*>(&page[PAGE_HEADER_SIZE + i * sizeof(SlotEntry)]) = page_fsp - i_length;
            *reinterpret_cast<uint16_t*>(&page[PAGE_HEADER_SIZE + i * sizeof(SlotEntry) + 2]) = i_length;

            memcpy(&page[page_fsp - i_length], &data[i_offset], i_length);
            page_fsp = page_fsp - i_length;
        }

        memcpy(&data[PAGE_HEADER_SIZE], &page[PAGE_HEADER_SIZE], PAGE_SIZE - PAGE_HEADER_SIZE);
        set_fsp(page_fsp);
        return true;
    }
}
