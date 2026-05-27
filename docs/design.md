# Mnemos

## Why this name?

In Greek mythology, Mnemosyne is the goddess of memory. Since this project is a database engine whose main job is to safely manage and remember data on a disk, "Mnemos" is a fitting name.

## What does Mnemos do?

Mnemos is a custom database storage engine that I am building from scratch using pure C++17. I am only using the standard library, with no outside tools or code. It includes a memory manager (buffer pool), a B+ tree for finding data quickly, and safe ways to handle multiple threads running at the same time.I purposely left out the parts that parse SQL text or plan queries. Processing strings takes a lot of time and doesn't help me learn the core systems concepts I care about: managing memory, testing how data is kicked out of the cache, and making sure bytes are safely saved to the disk without corruption.

## Layer 1: Disk I/O and Reliability

The DiskManager is the only part of the program that talks directly to the computer's file system. It uses standard C++ <fstream> to read and write raw bytes to the disk.I had to make a big decision about what to do if the physical disk fails—for example, if it runs out of space. If the engine just sends a normal error message, the database might keep running with modified data stuck in RAM that it can't save. To guarantee that data is never silently lost, Mnemos uses a "Fail-Fast" rule. If a write to the disk fails, the program logs a critical error and purposely forces the entire system to crash (std::abort()). This stops the system from making false promises that data was saved.

## Memory Layout: The Slotted Page

The engine organizes data into fixed chunks called pages, which are exactly 4096 bytes long. Because database records (tuples) can be different sizes, we can't just put them in a simple array without wasting space.
To solve this, I use a "Slotted Page" layout:
The first 24 bytes hold the header, which tracks basic information and includes a Free Space Pointer (FSP).
The directory (which tracks the location and size of each record) starts at the top and grows downward.
The actual record data starts at the very bottom of the page and grows upward.
The FSP points to the edge of the record data. When a new record is added, the pointer moves up using the simple math: FSP = FSP - tuple_size.
When records are deleted, the page leaves empty gaps. To fix this, the page has a compact() function that slides the remaining records back down to the bottom, grouping all the free space together.

## Buffer Pool Manager (BPM)

The BPM manages which disk pages are currently loaded into the fast RAM. To keep track of where every page is, it uses a Page Table.

### Lock Striping: 

If I used one big lock for the entire Page Table, multiple threads would constantly be waiting in line, slowing down the program. Instead, I split the Page Table into 16 separate pieces (an array of 16 hash maps). Each piece has its own lock. This means threads looking for different pages can search at the exact same time without blocking each other.

### Full Pool Protocol: 

If a thread asks for a new page, but every single spot in the memory pool is currently being used (pinned), the BPM does not put the thread to sleep to wait. That could cause threads to be stuck waiting on each other forever (a deadlock). Instead, the BPM just returns nullptr right away. This forces the higher-level code to cancel what it is doing and try again later.

### Saving Data: 
Mnemos does not immediately write data to the disk just because a task finishes. Instead, it waits until the memory pool is completely full. When it needs to kick out an old, modified page to make room for a new one, that is when the DiskManager saves it to the disk.

### Concurrency: Page-Level Latching

The split Page Table protects the directory, but it doesn't protect the actual bytes inside the pages. If one thread is writing a new record while another thread tries to read it, the program could crash or read broken data.
To stop this, every Page object has its own small lock (called a latch). A thread must get an exclusive "Write Latch" to change the data, or a shared "Read Latch" just to look at it. This keeps the data safe when many threads run at once.

### Cache Eviction: The LRU-K Algorithm

To decide which pages to keep in memory and which to kick out, Mnemos uses an algorithm called LRU-K (specifically K=2).A basic LRU algorithm is bad for databases because a massive sequential scan (reading a whole table from start to finish) will kick out all the important, frequently used data. Under LRU-K, any page that has been accessed less than 2 times is always kicked out before pages that have a longer history of use, no matter how recently they were read.
To make this incredibly fast, the code doesn't scan through long arrays. Instead, it uses a mix of hash maps and doubly linked lists: 
1. One list for pages accessed fewer than 2 times.
2. One list for pages accessed 2 or more times.
3. A hash map to instantly find where a page is in the lists.
This lets the program find, move, and kick out pages instantly in $O(1)$ time, keeping CPU usage very low.
