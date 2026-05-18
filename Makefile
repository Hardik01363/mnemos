CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

SRC = src/main.cpp \
      src/storage/page.cpp \
      src/storage/disk_manager.cpp \
      src/storage/buffer_pool.cpp \
      src/storage/lru_replacer.cpp \
      src/storage/clock_replacer.cpp \
      src/storage/lruk_replacer.cpp \
      src/index/btree.cpp \
      src/concurrency/wal.cpp \
      src/concurrency/transaction.cpp \
      src/concurrency/lock_manager.cpp \
      src/concurrency/mvcc.cpp \
      src/concurrency/deadlock_detector.cpp

TARGET = mnemos

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
