#include "../../include/concurrency/wal.h"
#include <cstring>
#include <iostream>

namespace mnemos {

    LogManager::LogManager(std::string filename) : log_filename(filename) {
        log_file.open(log_filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::app);
        if (!log_file.good()) {
            log_file.open(log_filename, std::ios::out | std::ios::binary);
            log_file.close();
            log_file.open(log_filename, std::ios::in | std::ios::out | std::ios::binary);
        }
    }

    LogManager::~LogManager() {
        flush();
        if(log_file.is_open()) log_file.close();
    }

    lsn_t LogManager::append_record(LogRecord& record) {
        std::lock_guard<std::mutex> guard(lock);
        record.lsn = next_lsn++;

        //serializing Log Record directly into disk stream
        log_file.write(reinterpret_cast<const char*>(&record.lsn), sizeof(lsn_t));
        log_file.write(reinterpret_cast<const char*>(&record.prev_lsn), sizeof(lsn_t));
        log_file.write(reinterpret_cast<const char*>(&record.txn_id), sizeof(txn_id_t));
        log_file.write(reinterpret_cast<const char*>(&record.type), sizeof(LogRecordType));
        log_file.write(reinterpret_cast<const char*>(&record.page_id), sizeof(page_id_t));
        log_file.write(reinterpret_cast<const char*>(&record.slot_id), sizeof(slot_id_t));

        log_file.write(reinterpret_cast<const char*>(&record.before_len), sizeof(uint16_t));
        if (record.before_len > 0) {
            log_file.write(reinterpret_cast<const char*>(record.before_image.data()), record.before_len);
        }

        log_file.write(reinterpret_cast<const char*>(&record.after_len), sizeof(uint16_t));
        if (record.after_len > 0) {
            log_file.write(reinterpret_cast<const char*>(record.after_image.data()), record.after_len);
        }

        return record.lsn;
    }

    void LogManager::flush() {
        std::lock_guard<std::mutex> guard(lock);
        log_file.flush();
        persistent_lsn = next_lsn - 1;
    }

    std::vector<LogRecord> LogManager::read_all_records() {
        std::lock_guard<std::mutex> guard(lock);
        log_file.seekg(0, std::ios::beg);
        std::vector<LogRecord> records;

        while (log_file.peek() != EOF) {
            LogRecord rec;
            log_file.read(reinterpret_cast<char*>(&rec.lsn), sizeof(lsn_t));
            log_file.read(reinterpret_cast<char*>(&rec.prev_lsn), sizeof(lsn_t));
            log_file.read(reinterpret_cast<char*>(&rec.txn_id), sizeof(txn_id_t));
            log_file.read(reinterpret_cast<char*>(&rec.type), sizeof(LogRecordType));
            log_file.read(reinterpret_cast<char*>(&rec.page_id), sizeof(page_id_t));
            log_file.read(reinterpret_cast<char*>(&rec.slot_id), sizeof(slot_id_t));

            log_file.read(reinterpret_cast<char*>(&rec.before_len), sizeof(uint16_t));
            if (rec.before_len > 0) {
                rec.before_image.resize(rec.before_len);
                log_file.read(reinterpret_cast<char*>(rec.before_image.data()), rec.before_len);
            }

            log_file.read(reinterpret_cast<char*>(&rec.after_len), sizeof(uint16_t));
            if (rec.after_len > 0) {
                rec.after_image.resize(rec.after_len);
                log_file.read(reinterpret_cast<char*>(rec.after_image.data()), rec.after_len);
            }

            records.push_back(rec);
        }
        return records;
    }
}
