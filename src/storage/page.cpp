//include statements

namespace mnemos {

    slot_id_t Page::insert_slot(uint8_t* input_data, uint16_t length) {
        uint16_t slot_cnt = get_slot_cnt();
        uint16_t fsp = get_fsp();
        
        if(length + sizeof(SlotEntry) > fsp - PAGE_HEADER_SIZE - slot_cnt * sizeof(SlotEntry)) return INVALID_SLOT_ID;
        
        SlotEntry entry;
        entry.offset = fsp - length;
        entry.length = length;

        *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE/8 + slot_cnt * sizeof(SlotEntry)]) = entry.offset;
        *reinterpret_cast<uint16_t*>(&data[PAGE_HEADER_SIZE/8 + slot_cnt * sizeof(SlotEntry) + sizeof(entry.offset)]) = entry.length;

        set_slot_cnt(slot_cnt + 1);
        

    }

    bool Page::read_slot(slot_id_t id, uint8_t* buffer) {
        
    }

    bool Page::update_slot(slot_id_t id, uint8_t* data, uint16_t length) {
        
    }

    bool Page::delete_slot(slot_id_t id) {
        
    }

    bool Page::compact() {
        
    }
}
