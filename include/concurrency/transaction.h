#pragma once

#include <cstdint>
#include <unordered_set>
#include "../config.h"

namespace mnemos {
    enum class TransactionState {
        GROWING,
        SHRINKING,
        COMMITTED,
        ABORTED
    };

    class Transaction {
    private:
        txn_id_t txn_id;
        TransactionState state;
        lsn_t prev_lsn{0};

    public:
        explicit Transaction(txn_id_t tid) : txn_id(tid), state(TransactionState::GROWING) {}

        txn_id_t get_txn_id() const { return txn_id; }
        TransactionState get_state() const { return state; }
        void set_state(TransactionState s) { state = s; }

        lsn_t get_prev_lsn() const { return prev_lsn; }
        void set_prev_lsn(lsn_t lsn) { prev_lsn = lsn; }
    };
}
