#pragma once

#include "../config.h"

namespace mnemos {

    enum NodeType {InternalNode, LeafNode};

    struct NodeHeader {
        NodeType node_type;
        uint16_t num_keys;
        page_id_t parent_page_id;
        page_id_t next_leaf_page_id;
    };

    struct BPlusTreeInternalNode {
        NodeHeader header;
        uint64_t keys[BTREE_INTERNAL_MAX_KEYS];
        page_id_t children[BTREE_INTERNAL_MAX_CHILDREN];
    };

    struct LeafEntry {
        uint64_t key;
        page_id_t page_id;
        slot_id_t slot_id;
    };

    struct BPlusTreeLeafNode {
        NodeHeader header;
        LeafEntry entries[BTREE_LEAF_MAX_ENTRIES];
    };

}
