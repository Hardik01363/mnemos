#include "../../include/concurrency/deadlock_detector.h"

namespace mnemos {

    void DeadlockDetector::add_edge(txn_id_t t1, txn_id_t t2) {
        wait_for_graph[t1].push_back(t2);
    }

    void DeadlockDetector::remove_edge(txn_id_t t1, txn_id_t t2) {
        auto& neighbors = wait_for_graph[t1];
        for(auto it = neighbors.begin(); it != neighbors.end(); ++it) {
            if(*it == t2) {
                neighbors.erase(it);
                break;
            }
        }
    }

    bool DeadlockDetector::dfs(txn_id_t current, std::unordered_set<txn_id_t>& visited, std::unordered_set<txn_id_t>& rec_stack, txn_id_t& victim) {
        visited.insert(current);
        rec_stack.insert(current);

        for(txn_id_t neighbor : wait_for_graph[current]) {
            if(rec_stack.count(neighbor)) {
                victim = current; // Pick current as cycle victim
                return true;
            }
            if(!visited.count(neighbor)) {
                if (dfs(neighbor, visited, rec_stack, victim)) return true;
            }
        }

        rec_stack.erase(current);
        return false;
    }

    bool DeadlockDetector::detect_deadlock(txn_id_t& victim_txn_id) {
        std::unordered_set<txn_id_t> visited;
        std::unordered_set<txn_id_t> rec_stack;

        for(auto const& [node, neighbors] : wait_for_graph) {
            if(!visited.count(node)) {
                if(dfs(node, visited, rec_stack, victim_txn_id)) {return true;}
            }
        }
        return false;
    }
}
