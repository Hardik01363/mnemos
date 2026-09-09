#pragma once

#include "../config.h"

namespace mnemos {
    enum class NodeType {InternalNode, LeafNode};

    struct NodeHeader {
        NodeType node_type;
        uint16_t num_keys;
        page_id_t parent_page_id;
        page_id_t next_leaf_page_id;
    };

}
