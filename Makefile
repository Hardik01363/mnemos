# Makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -g

SRCS = src/storage/disk_manager.cpp \
       src/storage/page.cpp \
       src/storage/buffer_pool.cpp \
       src/storage/lruk_replacer.cpp \
       src/storage/lru_replacer.cpp \
       src/storage/clock_replacer.cpp \
       src/index/btree.cpp \
       src/concurrency/wal.cpp \
       src/concurrency/transaction.cpp \
       src/concurrency/lock_manager.cpp \
       src/concurrency/deadlock_detector.cpp \
       src/concurrency/mvcc.cpp \
       src/main.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = mnemos

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o src/*/*.o $(TARGET) mnemos.db

.PHONY: all clean
