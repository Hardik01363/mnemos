#include "../../include/index/btree.h"
#include <cstring>
#include <iostream>

namespace mnemos {
    //constructor for the BPlusTree class, initialising member variables
    BPlusTree::BPlusTree(BufferPoolManager* bpm, page_id_t root_id)
        : root_page_id(root_id), bpm(bpm) {}
    
    Page *BPlusTree::get_node_page(page_id_t page_id) {
        return bpm->fetch_page(page_id);
    }

    void BPlusTree::create_new_tree(uint64_t key, page_id_t page_id, slot_id_t slot_id) {
        Page *root_page = bpm->new_page(&root_page_id);
        root_page->WLock();
        
        auto* leaf = reinterpret_cast<BPlusTreeLeafNode*>(&root_page->data[PAGE_HEADER_SIZE]);
        leaf->header.node_type = NodeType::LeafNode;
        leaf->header.num_keys = 1;
        leaf->header.parent_page_id = INVALID_PAGE_ID;
        leaf->header.next_leaf_page_id = INVALID_PAGE_ID;

        leaf->entries[0] = {key, page_id, slot_id, {0, 0}};

        root_page->WUnlock();
        bpm->unpin_page(root_page_id, true);
    }

    bool BPlusTree::find(uint64_t key, page_id_t& out_page_id, slot_id_t& out_slot_id) {
        if(root_page_id == INVALID_PAGE_ID) {return false;}

        page_id_t curr_id = root_page_id;
        Page* curr_page = bpm->fetch_page(curr_id);
        curr_page->RLock();

        auto* header = reinterpret_cast<NodeHeader*>(&curr_page->data[PAGE_HEADER_SIZE]);

        while (header->node_type == NodeType::InternalNode) {
            auto* internal = reinterpret_cast<BPlusTreeInternalNode*>(header);
            
            //binary searching keys within internal node
            int low = 0, high = internal->header.num_keys - 1;
            int child_idx = internal->header.num_keys; //rightmost is set as default
            
            while(low <= high) {
                int mid = low + (high - low) / 2;
                if(internal->keys[mid] > key) {
                    child_idx = mid;
                    high = mid - 1;
                } else {
                    low = mid + 1;
                }
            }

            page_id_t next_id = internal->children[child_idx];
            Page* next_page = bpm->fetch_page(next_id);
            next_page->RLock();
            
            curr_page->RUnlock();
            bpm->unpin_page(curr_id, false);

            curr_id = next_id;
            curr_page = next_page;
            header = reinterpret_cast<NodeHeader*>(&curr_page->data[PAGE_HEADER_SIZE]);
        }

        //at the leaf node now
        auto* leaf = reinterpret_cast<BPlusTreeLeafNode*>(header);
        bool found = false;

        for (uint16_t i = 0; i < leaf->header.num_keys; ++i) {
            if(leaf->entries[i].key == key) {
                out_page_id = leaf->entries[i].page_id;
                out_slot_id = leaf->entries[i].slot_id;
                found = true;
                break;
            }
        }

        curr_page->RUnlock();
        bpm->unpin_page(curr_id, false);
        return found;
    }

    bool BPlusTree::insert(uint64_t key, page_id_t page_id, slot_id_t slot_id) {
        if(root_page_id == INVALID_PAGE_ID) {
            create_new_tree(key, page_id, slot_id);
            return true;
        }

        page_id_t curr_id = root_page_id;
        Page* curr_page = bpm->fetch_page(curr_id);
        curr_page->WLock();

        auto* header = reinterpret_cast<NodeHeader*>(&curr_page->data[PAGE_HEADER_SIZE]);

        while(header->node_type == NodeType::InternalNode) {
            auto* internal = reinterpret_cast<BPlusTreeInternalNode*>(header);
            
            int low = 0, high = internal->header.num_keys - 1;
            int child_idx = internal->header.num_keys;
            
            while(low <= high) {
                int mid = low + (high - low) / 2;
                if(internal->keys[mid] > key) {
                    child_idx = mid;
                    high = mid - 1;
                }
                else {
                    low = mid + 1;
                }
            }

            page_id_t next_id = internal->children[child_idx];
            Page* next_page = bpm->fetch_page(next_id);
            next_page->WLock();

            curr_page->WUnlock();
            bpm->unpin_page(curr_id, false);

            curr_id = next_id;
            curr_page = next_page;
            header = reinterpret_cast<NodeHeader*>(&curr_page->data[PAGE_HEADER_SIZE]);
        }

        auto* leaf = reinterpret_cast<BPlusTreeLeafNode*>(header);

        //if node still has space, insert into it
        if(leaf->header.num_keys < BTREE_LEAF_MAX_ENTRIES) {
            leaf_insert(leaf, key, page_id, slot_id);
            curr_page->WUnlock();
            bpm->unpin_page(curr_id, true);
            return true;
        }

        //if node is full, split
        split_leaf(curr_page, leaf, curr_id);
        return true;
    }

    bool BPlusTree::leaf_insert(BPlusTreeLeafNode* leaf, uint64_t key, page_id_t pid, slot_id_t sid) {
        int i = leaf->header.num_keys - 1;
        while(i >= 0 && leaf->entries[i].key > key) {
            leaf->entries[i + 1] = leaf->entries[i];
            i--;
        }
        leaf->entries[i + 1] = {key, pid, sid, {0, 0}};
        leaf->header.num_keys++;
        return true;
    }

    void BPlusTree::split_leaf(Page* leaf_page, BPlusTreeLeafNode* leaf, page_id_t leaf_id) {
        page_id_t new_leaf_id;
        Page* new_leaf_page = bpm->new_page(&new_leaf_id);
        new_leaf_page->WLock();

        auto* new_leaf = reinterpret_cast<BPlusTreeLeafNode*>(&new_leaf_page->data[PAGE_HEADER_SIZE]);
        new_leaf->header.node_type = NodeType::LeafNode;
        new_leaf->header.parent_page_id = leaf->header.parent_page_id;
        new_leaf->header.next_leaf_page_id = leaf->header.next_leaf_page_id;
        leaf->header.next_leaf_page_id = new_leaf_id;

        uint16_t split_index = leaf->header.num_keys / 2;
        uint16_t move_cnt = leaf->header.num_keys - split_index;

        for(uint16_t i = 0; i < move_cnt; ++i) {
            new_leaf->entries[i] = leaf->entries[split_index + i];
        }

        new_leaf->header.num_keys = move_cnt;
        leaf->header.num_keys = split_index;

        uint64_t split_key = new_leaf->entries[0].key;
        page_id_t parent_id = leaf->header.parent_page_id;

        new_leaf_page->WUnlock();
        bpm->unpin_page(new_leaf_id, true);

        leaf_page->WUnlock();
        bpm->unpin_page(leaf_id, true);

        if(parent_id == INVALID_PAGE_ID) {
            //a root split
            Page* new_root_page = bpm->new_page(&root_page_id);
            new_root_page->WLock();

            auto* new_root = reinterpret_cast<BPlusTreeInternalNode*>(&new_root_page->data[PAGE_HEADER_SIZE]);
            new_root->header.node_type = NodeType::InternalNode;
            new_root->header.num_keys = 1;
            new_root->header.parent_page_id = INVALID_PAGE_ID;
            new_root->keys[0] = split_key;
            new_root->children[0] = leaf_id;
            new_root->children[1] = new_leaf_id;

            //updating the children parent pointers
            Page* l_pg = bpm->fetch_page(leaf_id);
            l_pg->WLock();
            reinterpret_cast<NodeHeader*>(&l_pg->data[PAGE_HEADER_SIZE])->parent_page_id = root_page_id;
            l_pg->WUnlock();
            bpm->unpin_page(leaf_id, true);

            Page* r_pg = bpm->fetch_page(new_leaf_id);
            r_pg->WLock();
            reinterpret_cast<NodeHeader*>(&r_pg->data[PAGE_HEADER_SIZE])->parent_page_id = root_page_id;
            r_pg->WUnlock();
            bpm->unpin_page(new_leaf_id, true);

            new_root_page->WUnlock();
            bpm->unpin_page(root_page_id, true);
        }
        else {
            insert_into_parent(leaf_id, split_key, new_leaf_id);
        }
    }

    void BPlusTree::insert_into_parent(page_id_t left_id, uint64_t key, page_id_t right_id) {
        Page* left_pg = bpm->fetch_page(left_id);
        left_pg->RLock();
        page_id_t parent_id = reinterpret_cast<NodeHeader*>(&left_pg->data[PAGE_HEADER_SIZE])->parent_page_id;
        left_pg->RUnlock();
        bpm->unpin_page(left_id, false);

        Page* parent_pg = bpm->fetch_page(parent_id);
        parent_pg->WLock();
        auto* parent = reinterpret_cast<BPlusTreeInternalNode*>(&parent_pg->data[PAGE_HEADER_SIZE]);

        if(parent->header.num_keys < BTREE_INTERNAL_MAX_KEYS) {
            int i = parent->header.num_keys - 1;
            while(i >= 0 && parent->keys[i] > key) {
                parent->keys[i + 1] = parent->keys[i];
                parent->children[i + 2] = parent->children[i + 1];
                i--;
            }
            parent->keys[i + 1] = key;
            parent->children[i + 2] = right_id;
            parent->header.num_keys++;

            parent_pg->WUnlock();
            bpm->unpin_page(parent_id, true);
        }
        else {
            split_internal(parent_pg, parent, parent_id);
        }
    }

    void BPlusTree::split_internal(Page* internal_page, BPlusTreeInternalNode* internal, page_id_t internal_id) {
        page_id_t new_internal_id;
        Page* new_internal_pg = bpm->new_page(&new_internal_id);
        new_internal_pg->WLock();

        auto* new_internal = reinterpret_cast<BPlusTreeInternalNode*>(&new_internal_pg->data[PAGE_HEADER_SIZE]);
        new_internal->header.node_type = NodeType::InternalNode;
        new_internal->header.parent_page_id = internal->header.parent_page_id;

        uint16_t split_index = internal->header.num_keys / 2;
        uint64_t parent_up_key = internal->keys[split_index];

        uint16_t move_cnt = internal->header.num_keys - split_index - 1;

        for(uint16_t i = 0; i < move_cnt; ++i) {
            new_internal->keys[i] = internal->keys[split_index + 1 + i];
            new_internal->children[i] = internal->children[split_index + 1 + i];
        }
        new_internal->children[move_cnt] = internal->children[internal->header.num_keys];

        new_internal->header.num_keys = move_cnt;
        internal->header.num_keys = split_index;

        new_internal_pg->WUnlock();
        bpm->unpin_page(new_internal_id, true);

        internal_page->WUnlock();
        bpm->unpin_page(internal_id, true);

        if(internal->header.parent_page_id == INVALID_PAGE_ID) {
            Page* new_root_pg = bpm->new_page(&root_page_id);
            new_root_pg->WLock();
            auto* root = reinterpret_cast<BPlusTreeInternalNode*>(&new_root_pg->data[PAGE_HEADER_SIZE]);
            
            root->header.node_type = NodeType::InternalNode;
            root->header.num_keys = 1;
            root->header.parent_page_id = INVALID_PAGE_ID;
            root->keys[0] = parent_up_key;
            root->children[0] = internal_id;
            root->children[1] = new_internal_id;
            
            new_root_pg->WUnlock();
            bpm->unpin_page(root_page_id, true);
        }
        else {
            insert_into_parent(internal_id, parent_up_key, new_internal_id);
        }
    }

    bool BPlusTree::remove(uint64_t key) {
        //unfilled merge/redistribute stub
        page_id_t leaf_id, dummy_pid;
        slot_id_t dummy_sid;
        if(!find(key, dummy_pid, dummy_sid)) {return false;}

        Page* leaf_pg = bpm->fetch_page(leaf_id);
        leaf_pg->WLock();
        auto* leaf = reinterpret_cast<BPlusTreeLeafNode*>(&leaf_pg->data[PAGE_HEADER_SIZE]);

        int idx = -1;
        for(int i = 0; i < leaf->header.num_keys; ++i) {
            if(leaf->entries[i].key == key) {
                idx = i;
                break;
            }
        }

        if(idx != -1) {
            for(int i = idx; i < leaf->header.num_keys - 1; ++i) {
                leaf->entries[i] = leaf->entries[i + 1];
            }
            leaf->header.num_keys--;
        }

        leaf_pg->WUnlock();
        bpm->unpin_page(leaf_id, true);
        return true;
    }

    std::vector<std::pair<page_id_t, slot_id_t>> BPlusTree::range_scan(uint64_t start_key, uint64_t end_key) {
        std::vector<std::pair<page_id_t, slot_id_t>> results;
        page_id_t p_id;
        slot_id_t s_id;
        if (!find(start_key, p_id, s_id)) return results;

        //traversing the leaf chain
        page_id_t curr_leaf_id = p_id;
        while(curr_leaf_id != INVALID_PAGE_ID) {
            Page* page = bpm->fetch_page(curr_leaf_id);
            page->RLock();
            auto* leaf = reinterpret_cast<BPlusTreeLeafNode*>(&page->data[PAGE_HEADER_SIZE]);

            for(uint16_t i = 0; i < leaf->header.num_keys; ++i) {
                if(leaf->entries[i].key >= start_key && leaf->entries[i].key <= end_key) {
                    results.push_back({leaf->entries[i].page_id, leaf->entries[i].slot_id});
                }
                if(leaf->entries[i].key > end_key) {
                    page->RUnlock();
                    bpm->unpin_page(curr_leaf_id, false);
                    return results;
                }
            }

            page_id_t next_id = leaf->header.next_leaf_page_id;
            page->RUnlock();
            bpm->unpin_page(curr_leaf_id, false);
            curr_leaf_id = next_id;
        }

        return results;
    }
}
