#pragma once
#include <cstdint>
#include "../config.h"

namespace mnemos {

    struct TupleVersionHeader {
        txn_id_t xmin; //creating txn ID
        txn_id_t xmax; //deleting/superceding txn ID (0 if active)
        page_id_t prev_version_page;
        slot_id_t prev_version_slot;
    };

    class MVCCManager {
    public:
        static bool is_visible(txn_id_t reader_txn_id, const TupleVersionHeader& header) {
            //snapshot isolation rule: Visible if created before reader and not marked deleted (really good line from CMU-DB 15-445 thatstuck with me)
            if(header.xmin <= reader_txn_id && (header.xmax == 0 || header.xmax > reader_txn_id)) {
                return true;
            }
            return false;
        }
    };
}
