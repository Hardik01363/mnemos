#pragma once

#include <cstdint>
#include <cstddef>

namespace mnemos {
    static constexpr size_t PAGE_SIZE = 4096;
    static constexpr size_t PAGE_HEADER_SIZE = 24;
    static constexpr uint32_t INVALID_PAGE_ID = UINT32_MAX;

    static constexpr size_t POOL_SIZE = 256;
    static constexpr size_t PAGE_TABLE_SHARDS = 16;
    static constexpr int LRUK_K = 2;

    static constexpr const char* DEFAULT_DB_FILE = "mnemos.db";

    using page_id_t = uint32_t;
    using frame_id_t = uint32_t;
    using slot_id_t = uint16_t;
    using txn_id_t = uint64_t;
}
