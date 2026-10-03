#include "../../include/concurrency/lock_manager.h"

namespace mnemos {
    bool LockManager::acquire_lock(txn_id_t txn_id, page_id_t page_id, LockMode mode) {
        std::unique_lock<std::mutex> global_guard(global_latch);

        if(lock_table.find(page_id) == lock_table.end()) {
            lock_table[page_id] = new LockHead();
        }

        LockHead* lock_head = lock_table[page_id];
        global_guard.unlock();

        std::unique_lock<std::mutex> lock_guard(lock_head->lock);
        
        lock_head->request_queue.push_back({txn_id, mode, false});
        auto req_it = std::prev(lock_head->request_queue.end());

        lock_head->cv.wait(lock_guard, [&]() {
            if(req_it == lock_head->request_queue.begin()) {return true;}
            if(mode == LockMode::SHARED && lock_head->current_mode == LockMode::SHARED) {return true;}
            return false;
        });

        req_it->granted = true;
        lock_head->current_mode = mode;
        return true;
    }

    bool LockManager::unlock(txn_id_t txn_id, page_id_t page_id) {
        std::unique_lock<std::mutex> global_guard(global_latch);
        if(lock_table.find(page_id) == lock_table.end()) {return false;}

        LockHead* lock_head = lock_table[page_id];
        global_guard.unlock();

        std::unique_lock<std::mutex> lock_guard(lock_head->lock);

        for(auto it = lock_head->request_queue.begin(); it != lock_head->request_queue.end(); ++it) {
            if(it->txn_id == txn_id) {
                lock_head->request_queue.erase(it);
                break;
            }
        }

        lock_head->cv.notify_all();
        return true;
    }
}
