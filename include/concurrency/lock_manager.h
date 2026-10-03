#pragma once

#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <list>
#include "../config.h"

namespace mnemos {
    enum class LockMode { SHARED, EXCLUSIVE };

    struct LockRequest {
        txn_id_t txn_id;
        LockMode lock_mode;
        bool granted{false};
    };

    struct LockHead {
        std::mutex lock;
        std::condition_variable cv;
        std::list<LockRequest> request_queue;
        LockMode current_mode{LockMode::SHARED};
    };

    class LockManager {
    private:
        std::mutex global_latch;
        std::unordered_map<page_id_t, LockHead*> lock_table;

    public:
        bool acquire_lock(txn_id_t txn_id, page_id_t page_id, LockMode mode);
        bool unlock(txn_id_t txn_id, page_id_t page_id);
    };
}
