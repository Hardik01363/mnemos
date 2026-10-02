#pragma once

#include <cstdint>
#include <cstddef>

namespace mnemos {
    static constexpr size_t PAGE_SIZE = 4096;
    static constexpr size_t PAGE_HEADER_SIZE = 24;
    static constexpr uint32_t INVALID_PAGE_ID = UINT32_MAX;
    static constexpr uint16_t INVALID_SLOT_ID = UINT16_MAX;

    static constexpr size_t POOL_SIZE = 256;
    static constexpr size_t PAGE_TABLE_SHARDS = 16;
    static constexpr int LRUK_K = 2;

    static constexpr const char* DEFAULT_DB_FILE = "mnemos.db";

    using page_id_t = uint32_t;
    using frame_id_t = uint32_t;
    using slot_id_t = uint16_t;
    using txn_id_t = uint64_t;
    using lsn_t = uint64_t;
    static constexpr lsn_t INVALID_LSN = 0;

    //the below BTREE values are calculated by me considering the sizes of every fiel and also onsidering padding in my calculations
    //i could have fit 291 entries in the leaf node by packing the structs manually instead of padding, so as to increase data density by 14%, but, it would incur a CPU overhead to access non-favouable memory adddresses. since the sacle is relatively small, i chose speed over data density, but, would be an easy switch to packing if i ever decide to do so.
    static constexpr size_t BTREE_INTERNAL_MAX_KEYS = 337;
    static constexpr size_t BTREE_INTERNAL_MAX_CHILDREN = 338;
    static constexpr size_t BTREE_LEAF_MAX_ENTRIES = 253;
}
