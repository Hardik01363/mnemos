#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../config.h"

namespace mnemos {
    class DeadlockDetector {
    private:
        std::unordered_map<txn_id_t, std::vector<txn_id_t>> wait_for_graph;
        
        bool dfs(txn_id_t current, std::unordered_set<txn_id_t>& visited, std::unordered_set<txn_id_t>& rec_stack, txn_id_t& victim);

    public:
        void add_edge(txn_id_t t1, txn_id_t t2);
        void remove_edge(txn_id_t t1, txn_id_t t2);
        bool detect_deadlock(txn_id_t& victim_txn_id);
    };
}
