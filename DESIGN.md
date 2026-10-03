# Mnemos Design Notes

This file explains how Mnemos is built and why I made each choice. It also lists what is still open.

## Goals

* Learn the full path of a database engine: disk, memory, index, log, locks.
* Use only the C++17 standard library.
* Keep each layer small enough to read in one sitting.
* Make choices that can be measured, such as the page eviction policy.

## Layers

Mnemos has four layers. Each layer only talks to the one below it.

1. Pages and buffer pool
2. B+ tree index
3. Write ahead log and locking
4. MVCC and deadlock handling

## Layer 1: Pages and buffer pool

### Basic numbers (`include/config.h`)

* Page size is 4096 bytes. Every page has a 24 byte header.
* The pool holds 256 frames.
* The page table is split into 16 shards.
* LRU K uses K = 2.
* Page ids, frame ids and LSNs are plain integers: 32 bit for pages and frames, 16 bit for slots, 64 bit for transactions and LSNs.

### Disk manager

The disk manager treats the database file as an array of pages. The byte offset of a page is its page id times the page size. One mutex protects the file handle. A failed write stops the program, because going on after a lost write would hide data loss.

### Slotted page

A slotted page stores tuples of different sizes.

* The page header holds the page id, the free space pointer, the slot count, the pin count and the dirty flag.
* The slot directory starts right after the header and grows forward. Each slot stores an offset and a length.
* Tuple data starts at the end of the page and grows backward.
* A deleted slot has offset zero.
* `compact()` moves live tuples together to remove holes left by deletes and updates.

Each page also has a read write latch. The B+ tree uses it to protect one node at a time.

### Buffer pool manager

The buffer pool keeps pages in memory so the program does not read the disk every time.

* A free list holds frames that have never been used or were freed.
* The page table maps a page id to a frame. It is split into 16 shards, and each shard has its own shared mutex. This lowers lock fights between threads that touch different pages.
* A page with a pin count above zero cannot be evicted.
* When the pool is full, the replacer picks a victim. If the victim is dirty, it is written to disk before the frame is reused.
* `new_page` gives out the next page id, zeroes the frame and marks it dirty.
* `flush_page` always writes the page, even if it is not dirty. The caller decides when a flush is needed, so the pool does not need an extra dirty check method.
* The destructor flushes all dirty pages.

### Replacers

The buffer pool uses LRU K. A frame with fewer than K recorded accesses is evicted first, in the order it first appeared. Frames with K or more accesses sit in a second list and are evicted from the least recently used end. This is a simple version of LRU K, not the full backward distance rule.

`LRUReplacer` and `ClockReplacer` are in the code for one reason: I want to measure all three policies under a transaction style workload (many small random reads and writes) and a scan style workload (long sequential reads), and see which policy wins where. The buffer pool currently takes the LRU K class directly, so a shared replacer interface is needed before the benchmark can swap policies.

## Layer 2: B+ tree

### Node layout

B+ tree nodes do not use the slotted page. Keys are fixed size 64 bit integers, so each node is a flat sorted array placed right after the page header. A slot directory would only add cost.

Node header (12 bytes with padding): node type, number of keys, parent page id, next leaf page id.

Internal node: up to 337 keys and 338 child page ids.

Leaf node: up to 253 entries. Each entry is 16 bytes: the key, a page id, a slot id and 2 bytes of padding. The value is a (page id, slot id) pair so it points at the exact tuple.

### Padding choice

Packing the leaf entries tightly would fit 291 entries instead of 253, about 14 percent more data per page. But a 64 bit key forces 8 byte alignment, and a packed layout would put keys at unaligned addresses. I chose the padded layout for simple code and aligned memory access. If the scale grows, switching to packing is a small change in `config.h` and the entry struct.

### Operations

* `find`: walks from the root down. At each internal node it does a binary search and moves to the child. It takes a read latch on the child before letting go of the parent.
* `insert`: walks down with write latches, then inserts into the sorted leaf. If the leaf is full, `split_leaf` makes a new leaf, moves the upper half of the entries, links the leaf chain and pushes the first key of the new leaf into the parent. `split_internal` does the same for internal nodes and moves the middle key up.
* `range_scan`: finds the start leaf, then follows the next leaf links until the key goes past the end.
* `remove`: removes an entry from a leaf. It does not merge or rebalance yet.

### Open work in this layer

* Splits need tests that fill many nodes and check that every key can still be found afterwards.
* Parent pointers must stay correct after an internal split.
* Delete needs merge and redistribute.
* The tree needs a proper latching rule for writers that split. The current code lets go of the parent before it knows the child is safe.
* The root page id has to be saved somewhere on disk so the tree can be opened again after a restart.

## Layer 3: Write ahead log and locking

### Write ahead log

Each log record holds an LSN, the previous LSN of the same transaction, the transaction id, a type (begin, commit, abort, insert, update, delete, compensation), a page id, a slot id, and a before image and after image of the changed bytes. The log manager serializes records to `mnemos.log` under a mutex and can read all records back.

The rule of a write ahead log is that the log record must reach disk before the page it describes. This rule is not enforced yet, because pages do not carry an LSN and the buffer pool does not ask the log manager before it flushes.

### Lock manager

Locks are held per page. Each page has a request queue. A request waits on a condition variable until it is at the front of the queue, or until it is a shared request and the page is already locked in shared mode. Unlock removes the request and wakes the waiters.

Open work:

* Lock upgrade from shared to exclusive.
* Tracking which locks each transaction holds, so commit and abort can release them all.
* Strict two phase locking rules.
* Waiting exclusive requests should block new shared requests so they do not starve.
* Freeing lock heads that are no longer used.

### Deadlock detector

The detector keeps a wait for graph. An edge from transaction A to transaction B means A waits for B. A depth first search finds cycles and picks a victim. Nothing calls it yet. The lock manager must add and remove edges as requests wait and finish, and the graph needs its own mutex.

## Layer 4: MVCC

Not started. The file exists as an empty placeholder. The plan is a version chain per tuple with begin and end timestamps, so readers can see a consistent snapshot without blocking writers.

## Recovery

Not started. The log format has room for compensation records, which an undo pass needs. The plan follows the usual three passes: analysis, redo, undo.

## Testing

Right now `src/main.cpp` is one smoke test. Planned tests:

* Slotted page: insert, update, delete, compact, full page.
* Buffer pool: eviction under pressure, pinned pages never evicted, dirty pages written on eviction.
* B+ tree: sequential, reverse and random inserts of many thousands of keys, with a full check that every key is found and the leaf chain is sorted.
* Lock manager: several threads on the same page.
* Log: write, close, reopen, read back and compare.

## Benchmarks

The first planned benchmark compares the three replacers (LRU, Clock, LRU K) under two workloads: random point lookups, and a long scan mixed with hot pages. The question is whether LRU K protects hot pages from a scan better than the other two.

## Not planned

* A SQL parser or query planner.
* Network access. Mnemos is a library with a test driver.
