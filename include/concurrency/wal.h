#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include "../config.h"

namespace mnemos {
    enum class LogRecordType : uint8_t {
        INVALID = 0,
        BEGIN,
        COMMIT,
        ABORT,
        INSERT,
        UPDATE,
        DELETE,
        CLR //(Compensation Log Record) for UNDO recovery
    };

    struct LogRecord {
        lsn_t lsn{INVALID_LSN};
        lsn_t prev_lsn{INVALID_LSN};
        txn_id_t txn_id{0};
        LogRecordType type{LogRecordType::INVALID};
        
        page_id_t page_id{INVALID_PAGE_ID};
        slot_id_t slot_id{INVALID_SLOT_ID};
        
        uint16_t before_len{0};
        uint16_t after_len{0};
        std::vector<uint8_t> before_image;
        std::vector<uint8_t> after_image;

        lsn_t undo_next_lsn{INVALID_LSN}; //used only for CLR records
    };

    class LogManager {
    private:
        std::string log_filename;
        std::fstream log_file;
        std::mutex lock;
        
        lsn_t next_lsn{1};
        lsn_t persistent_lsn{0};

        char log_buffer[4096];
        size_t buffer_offset{0};

    public:
        explicit LogManager(std::string log_filename = "mnemos.log");
        ~LogManager();

        lsn_t append_record(LogRecord& record);
        void flush();

        lsn_t get_persistent_lsn() {
            std::lock_guard<std::mutex> guard(lock);
            return persistent_lsn;
        }

        //ARIES Recovery Reader Interface
        std::vector<LogRecord> read_all_records();
    };
}
