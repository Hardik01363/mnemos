#include <iostream>
#include <cassert>
#include <string>
#include "../include/config.h"
#include "../include/storage/disk_manager.h"
#include "../include/storage/lruk_replacer.h"
#include "../include/storage/buffer_pool.h"
#include "../include/index/btree.h"
#include "../include/concurrency/wal.h"
#include "../include/concurrency/lock_manager.h"

using namespace mnemos;

//im trying out a new way of printing success/error messages (a format i learnt in linux device driver tutorials)

int main() {
    std::cout << "=========================================================\n";
    std::cout << "          MNEMOS DATABASE ENGINE SYSTEM TEST             \n";
    std::cout << "=========================================================\n";

    //booting the storage & buffer pool subsystem
    auto* disk_mgr = new DiskManager(DEFAULT_DB_FILE);
    auto* replacer = new LRUKReplacer(POOL_SIZE, LRUK_K);
    auto* bpm = new BufferPoolManager(POOL_SIZE, disk_mgr, replacer);
    auto* log_mgr = new LogManager("mnemos.log");

    std::cout << "[+] Storage Engine & Buffer Pool initialized.\n";

    //testing WAL subsystem
    LogRecord rec;
    rec.txn_id = 101;
    rec.type = LogRecordType::BEGIN;
    lsn_t lsn1 = log_mgr->append_record(rec);
    log_mgr->flush();
    std::cout << "[+] WAL Append & Flush successful. Assigned LSN: " << lsn1 << "\n";

    //testing concurrent B+ Tree operations
    BPlusTree tree(bpm);
    std::cout << "[+] B+ Tree Index initialized.\n";

    std::cout << "[*] Inserting 100 entries into B+ Tree...\n";
    for(uint64_t i = 1; i <= 100; ++i) {
        tree.insert(i, static_cast<page_id_t>(i * 2), static_cast<slot_id_t>(i));
    }

    page_id_t found_pid;
    slot_id_t found_sid;
    bool search_res = tree.find(42, found_pid, found_sid);

    assert(search_res == true);
    assert(found_pid == 84);
    assert(found_sid == 42);
    std::cout << "[+] B+ Tree Point Lookup Verified (Key 42 -> Page 84, Slot 42).\n";

    //testing lock manager
    LockManager lock_mgr;
    lock_mgr.acquire_lock(101, 10, LockMode::EXCLUSIVE);
    std::cout << "[+] Lock Manager: Exclusive Lock Granted on Page 10 to Txn 101.\n";
    lock_mgr.unlock(101, 10);
    std::cout << "[+] Lock Manager: Page 10 Unlocked.\n";

    //cleaning up
    delete log_mgr;
    delete bpm;
    delete replacer;
    delete disk_mgr;

    std::cout << "=========================================================\n";
    std::cout << "          ALL SYSTEM TESTS PASSED SUCCESSFULLY!          \n";
    std::cout << "=========================================================\n";

    return 0;
}
