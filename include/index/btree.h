#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include "../config.h"
#include "../storage/buffer_pool.h"

namespace mnemos {

    enum class NodeType : uint8_t {
        InternalNode = 0,
        LeafNode = 1
    };

    struct NodeHeader {
        NodeType node_type;
        uint8_t padding[1];
        uint16_t num_keys;
        page_id_t parent_page_id;
        page_id_t next_leaf_page_id;
    };

    struct LeafEntry {
        uint64_t key;
        page_id_t page_id;
        slot_id_t slot_id;
        uint8_t padding[2];
    };

    struct BPlusTreeLeafNode {
        NodeHeader header;
        LeafEntry entries[BTREE_LEAF_MAX_ENTRIES];
    };

    struct BPlusTreeInternalNode {
        NodeHeader header;
        uint64_t keys[BTREE_INTERNAL_MAX_KEYS];
        page_id_t children[BTREE_INTERNAL_MAX_CHILDREN];
    };

    class BPlusTree {
    private:
        page_id_t root_page_id;
        BufferPoolManager* bpm;

        Page* get_node_page(page_id_t page_id);
        void create_new_tree(uint64_t key, page_id_t page_id, slot_id_t slot_id);
        
        // Node Manipulation Helpers
        bool leaf_insert(BPlusTreeLeafNode* leaf, uint64_t key, page_id_t pid, slot_id_t sid);
        void split_leaf(Page* leaf_page, BPlusTreeLeafNode* leaf, page_id_t leaf_id);
        void split_internal(Page* internal_page, BPlusTreeInternalNode* internal, page_id_t internal_id);
        void insert_into_parent(page_id_t left_id, uint64_t key, page_id_t right_id);

    public:
        explicit BPlusTree(BufferPoolManager* bpm, page_id_t root_id = INVALID_PAGE_ID);

        page_id_t get_root_page_id() const { return root_page_id; }
        
        bool find(uint64_t key, page_id_t& out_page_id, slot_id_t& out_slot_id);
        bool insert(uint64_t key, page_id_t page_id, slot_id_t slot_id);
        bool remove(uint64_t key);
        
        std::vector<std::pair<page_id_t, slot_id_t>> range_scan(uint64_t start_key, uint64_t end_key);
    };
}
