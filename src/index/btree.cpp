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
        
        //do some stuff

        root_page->WUnlock();
        //do some stuff
    }
}
