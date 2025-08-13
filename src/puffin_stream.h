// Copyright 2017 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_PUFFIN_STREAM_H_
#define SRC_PUFFIN_STREAM_H_

#include <filesystem>
#include <list>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "puffin/src/include/puffin/common.h"
#include "puffin/src/include/puffin/huffer.h"
#include "puffin/src/include/puffin/puffer.h"
#include "puffin/src/include/puffin/stream.h"

namespace puffin {

class LRUCache {
 public:
  using Key = int;
  using Value = std::shared_ptr<Buffer>;
  typedef typename std::pair<Key, Value> key_value_pair_t;
  typedef typename std::list<key_value_pair_t>::iterator iterator;

  LRUCache(size_t max_size);
  ~LRUCache();
  LRUCache(const LRUCache&) = delete;

  void put(const Key& key, const Value& value);

  bool EvictLRUItem();

  // Ensure that cache has sufficient space for a new item of |size| bytes
  void EnsureSpace(size_t size);

  const Value get(const Key& key);

  bool exists(const Key& key) const {
    return items_map_.find(key) != items_map_.end();
  }

  size_t size() const { return cache_size_; }
  size_t capacity() const { return max_size_; }

 private:
  bool WriteToDisk(const Key& key, const Value& value);
  Value ReadFromDisk(const Key& key);
  std::list<key_value_pair_t> items_list_;
  std::unordered_map<Key, iterator> items_map_;
  size_t cache_size_ = 0;
  size_t max_size_;
  std::filesystem::path tmpdir_;
  std::set<Key> ondisk_items_;
};

// A class for puffing a deflate stream and huffing into a deflate stream. The
// puff stream is "imaginary", which means it doesn't really exists; It is build
// and used on demand. This class uses a given deflate stream, and puffs the
// deflate buffers in the stream as needed or vice versa.  An object of this
// class can be used for reading and writing puff data but should not be used
// for both reading and writing using the same instance. In theory we can
// separate this class into two classes, namely |PuffStream| and |HuffStream|,
// but they are sharing a lot of codes which might be inconvenient and
// unnecessary to do so. In this implementation, there is no protection against
// reading and writing at the same time.
class PuffinStream : public StreamInterface {
 public:
  ~PuffinStream() override = default;

  // Creates a |PuffinStream| for reading puff buffers from a deflate stream.
  // |stream|    IN  The deflate stream.
  // |puffer|    IN  The |Puffer| used for puffing the stream.
  // |puff_size| IN  The size of the puff stream (assuming |stream| has been
  //                 completely puffed.
  // |deflates|  IN  The location of deflates in |stream|.
  // |puffs|     IN  The location of puffs into the final puff stream.
  // |max_cache_size| IN  The amount of memory to use for caching puff buffers.
  //                      If the mount is smaller than the maximum puff buffer
  //                      size in |puffs|, then its value will be set to zero
  //                      and no puff will be cached.
  static UniqueStreamPtr CreateForPuff(UniqueStreamPtr stream,
                                       std::shared_ptr<Puffer> puffer,
                                       uint64_t puff_size,
                                       const std::vector<BitExtent>& deflates,
                                       const std::vector<ByteExtent>& puffs,
                                       size_t max_cache_size = 0);

  // Creates a |PuffinStream| for writing puff buffers into a deflate stream.
  // |stream|    IN  The deflate stream.
  // |huffer|    IN  The |Huffer| used for huffing into the |stream|.
  // |puff_size| IN  The size of the puff stream (assuming |stream| has been
  //                 completely puffed.
  // |deflates|  IN  The location of deflates in |stream|.
  // |puffs|     IN  The location of puffs into the input puff stream.
  static UniqueStreamPtr CreateForHuff(UniqueStreamPtr stream,
                                       std::shared_ptr<Huffer> huffer,
                                       uint64_t puff_size,
                                       const std::vector<BitExtent>& deflates,
                                       const std::vector<ByteExtent>& puffs);

  bool GetSize(uint64_t* size) const override;

  // Returns the current offset in the imaginary puff stream.
  bool GetOffset(uint64_t* offset) const override;

  // Sets the current offset in the imaginary puff stream.
  bool Seek(uint64_t offset) override;

  // Reads from the deflate stream |stream_| and writes the puff stream into
  // |buffer|.
  bool Read(void* buffer, size_t length) override;

  // Reads the puff stream from |buffer|, huffs it and writes it into the
  // deflate stream |stream_|. The current assumption for write is that data is
  // wrote from beginning to end with no retraction or random change of offset.
  // This function, writes non-puff data directly to |stream_| and caches the
  // puff data into |puff_buffer_|. When |puff_buffer_| is full, it huffs it
  // into |deflate_buffer_| and writes it to |stream_|.
  bool Write(const void* buffer, size_t length) override;

  bool Close() override;

 protected:
  // The non-public internal Ctor.
  PuffinStream(UniqueStreamPtr stream,
               std::shared_ptr<Puffer> puffer,
               std::shared_ptr<Huffer> huffer,
               uint64_t puff_size,
               const std::vector<BitExtent>& deflates,
               const std::vector<ByteExtent>& puffs,
               size_t max_cache_size);

 private:
  // See |extra_byte_|.
  bool SetExtraByte();

  // Returns the cache for the |puff_id|th puff. If it does not find it, either
  // returns the least accessed cached (if cache is full) or creates a new empty
  // buffer. It returns false if it cannot find the |puff_id|th puff cache.
  bool GetPuffCache(int puff_id,
                    uint64_t puff_size,
                    std::shared_ptr<Buffer>* buffer);

  UniqueStreamPtr stream_;

  std::shared_ptr<Puffer> puffer_;
  std::shared_ptr<Huffer> huffer_;

  // The size of the imaginary puff stream.
  uint64_t puff_stream_size_;

  std::vector<BitExtent> deflates_;
  // The current deflate is being processed.
  std::vector<BitExtent>::iterator cur_deflate_;

  std::vector<ByteExtent> puffs_;
  // The current puff is being processed.
  std::vector<ByteExtent>::iterator cur_puff_;

  std::vector<uint64_t> upper_bounds_;

  // The current offset in the imaginary puff stream is |puff_pos_| +
  // |skip_bytes_|
  uint64_t puff_pos_;
  uint64_t skip_bytes_;

  // The current bit offset in |stream_|.
  uint64_t deflate_bit_pos_;

  // This value caches the first or last byte of a deflate stream. This is
  // needed when two deflate stream end on the same byte (with greater than zero
  // bit offset difference) or a deflate starts from middle of the byte. We need
  // to cache the value in here before we have the rest of the puff buffer to
  // make the deflate.
  uint8_t last_byte_;

  // We have to figure out if we need to cache an extra puff byte for the last
  // byte of the deflate. This is only needed if the last bit of the current
  // deflate is not in the same byte as the first bit of the next deflate. The
  // value is either 0 or 1. If 1.
  size_t extra_byte_;

  // True if the stream is only for puffing. False if for huffing.
  bool is_for_puff_;

  // True if the |Close()| is called.
  bool closed_;

  std::unique_ptr<Buffer> deflate_buffer_;
  std::shared_ptr<Buffer> puff_buffer_;

  // The LRU cache for holding puff data
  LRUCache lru_cache_;

  DISALLOW_COPY_AND_ASSIGN(PuffinStream);
};

}  // namespace puffin

#endif  // SRC_PUFFIN_STREAM_H_
