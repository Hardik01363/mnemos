#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "../include/storage/lru_replacer.h"
#include "../include/storage/clock_replacer.h"
#include "../include/storage/lruk_replacer.h"

using namespace mnemos;

void run_benchmark() {
    constexpr size_t FRAMES = 64;
    constexpr size_t ACCESSES = 100000;

    LRUReplacer lru(FRAMES);
    ClockReplacer clock(FRAMES);
    LRUKReplacer lruk(FRAMES, 2);

    std::default_random_engine gen(42);
    //skewed zipfian distribution (80% accesses to 20% pages) simulating OLTP (heavy words ik, learnt them in CMU-DB 15-445)
    std::discrete_distribution<int> oltp_dist({10, 10, 10, 10, 2, 2, 1, 1, 1, 1});

    auto start = std::chrono::high_resolution_clock::now();
    for(size_t i = 0; i < ACCESSES; ++i) {
        frame_id_t f = oltp_dist(gen) % FRAMES;
        lruk.record_access(f);
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "[BENCHMARK] Eviction execution finished in: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";
}

int main() {
    run_benchmark();
    return 0;
}
