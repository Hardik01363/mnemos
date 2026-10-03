# Mnemos

Mnemos is a small disk based database storage engine written in C++17. It uses no outside libraries. I built it to learn how a real database engine works, starting from the disk and going up one layer at a time.

## What it has today

Storage layer

* A disk manager that reads and writes fixed size 4096 byte pages in one database file.
* A slotted page format for storing variable length tuples.
* A buffer pool manager with pinning, dirty page tracking and a sharded page table.
* An LRU K replacer for choosing which page to evict. Simple LRU and Clock replacers are also in the code for comparing policies later.

Index layer

* A B+ tree that maps a 64 bit key to a tuple address (page id and slot id).
* Point lookup, insert with leaf and internal node splits, and a range scan that walks the leaf chain.

Concurrency layer

* A write ahead log that writes records to a log file.
* A lock manager with shared and exclusive page locks.
* A wait for graph with cycle detection for finding deadlocks.
* Files for transactions and MVCC exist, but MVCC is still empty.

## Status

Working and tested by the system test in `src/main.cpp`:

* Disk manager, buffer pool, LRU K replacer
* WAL append and flush
* B+ tree insert and point lookup for a small number of keys
* Lock manager grant and release for one transaction

Written but not tested enough yet:

* B+ tree node splits (the test never fills a node)
* Range scan
* Lock manager with many transactions at the same time
* Deadlock detector (not connected to the lock manager yet)

Not done yet:

* B+ tree delete with merge and rebalance
* Crash recovery from the log
* MVCC and version chains
* A table layer that stores tuples in slotted pages and connects them to the index

See `DESIGN.md` for the reasons behind the choices and the full list of open work.

## Project layout

```
include/config.h             page size, pool size, B+ tree limits, id types
include/storage/             disk manager, page, buffer pool, replacers
include/index/               B+ tree
include/concurrency/         WAL, lock manager, transactions, deadlock detector, MVCC
src/                         matching .cpp files, plus main.cpp
benchmarks/                  benchmark code
docs/                        extra notes
Makefile                     build rules
```

## Build and run

You need `g++` with C++17 support and `make`.

```
make
./mnemos
```

The program creates `mnemos.db` and `mnemos.log` in the current folder. If everything works, the last line it prints is:

```
ALL SYSTEM TESTS PASSED SUCCESSFULLY!
```

For a clean start, run `make clean` and also delete `mnemos.log` by hand. Page ids start from zero on every run, so an old database file will confuse a new run.

## What the system test checks

`src/main.cpp` starts the storage engine, then checks each part once:

1. The buffer pool and disk manager start without errors.
2. The log accepts a BEGIN record, flushes it and returns an LSN.
3. The B+ tree takes 100 inserts, and a lookup of key 42 returns page 84 and slot 42.
4. The lock manager grants and releases an exclusive lock on one page.

This is a smoke test. It is not a full test suite yet.
