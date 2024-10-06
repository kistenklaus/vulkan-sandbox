#pragma once
#include <cstddef>
#include <algorithm>
#include <memory>
#include <vector>

namespace wie {

namespace internal {

template <size_t BLOCK_SIZE> union LinkedPoolBucketBlock {
  static_assert(BLOCK_SIZE > 0);
  std::byte value[BLOCK_SIZE];
  union LinkedPoolBucketBlock *next;
};

template <size_t BLOCK_SIZE, size_t CHUNK_SIZE> struct LinkedPoolBucketChunk {
  static_assert(CHUNK_SIZE >= 1);
  LinkedPoolBucketBlock<BLOCK_SIZE> m_mem[CHUNK_SIZE];

  LinkedPoolBucketChunk() {
    for (size_t i = 0; i < CHUNK_SIZE - 1; ++i) {
      m_mem[i].next = m_mem + i + 1;
    }
    m_mem[CHUNK_SIZE - 1].next = nullptr;
  }
};

} // namespace internal

template <size_t BLOCK_SIZE,
          size_t CHUNK_SIZE = std::max((size_t)1, 4096 / BLOCK_SIZE * 8),
          typename ChunkAllocator = std::allocator<
              internal::LinkedPoolBucketChunk<BLOCK_SIZE, CHUNK_SIZE>>>
struct FreelistPool {
public:

  FreelistPool() {
    m_chunkAllocator = ChunkAllocator();
    grow();
  }

  FreelistPool(const ChunkAllocator &chunkAllocator)
      : m_chunkAllocator(chunkAllocator) {
    grow();
  }

  ~FreelistPool() {
    for (Chunk *chunk : m_chunks) {
      m_chunkAllocator.deallocate(chunk, 1);
    }
  }

  constexpr size_t blockSize() const { return BLOCK_SIZE; }
  constexpr size_t chunkSize() const { return CHUNK_SIZE; }

  void *allocate() {
    if (m_freeList == nullptr) {
      grow();
    }
    Block *next = m_freeList->next;
    void *mem = reinterpret_cast<void *>(&m_freeList->value);
    m_freeList = next;
    return mem;
  }

  void deallocate(void *p) {
    Block *b = reinterpret_cast<Block *>(p);
    b->next = m_freeList;
    m_freeList = b;
  }

private:

  using Block = internal::LinkedPoolBucketBlock<BLOCK_SIZE>;
  using Chunk = internal::LinkedPoolBucketChunk<BLOCK_SIZE, CHUNK_SIZE>;
  void grow() {
    Chunk *new_chunk = new (m_chunkAllocator.allocate(1)) Chunk();
    m_chunks.push_back(new_chunk);
    m_freeList = new_chunk->m_mem;
  }

  ChunkAllocator m_chunkAllocator;
  Block *m_freeList;
  std::vector<Chunk *> m_chunks;
};


} // namespace wie
