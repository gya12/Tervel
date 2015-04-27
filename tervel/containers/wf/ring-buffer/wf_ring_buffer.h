/*
The MIT License (MIT)

Copyright (c) 2015 University of Central Florida's Computer Software Engineering
Scalable & Secure Systems (CSE - S3) Lab

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef TERVEL_CONTAINERS_WF_RINGBUFFER_RINGBUFFER_H_
#define TERVEL_CONTAINERS_WF_RINGBUFFER_RINGBUFFER_H_

#include <atomic>
#include <assert.h>
#include <cstddef>
#include <memory>
#include <thread>
#include <string>

namespace tervel {
namespace containers {
namespace wf {

/**
 * @brief This is a non-blocking FIFO ring buffer design
 * that was made wait-free by applying a progress assurance framework to it.
 *
 * TODO(steven): implement the progress assurance changes
 * If isDelayed never returns true then it will not use additional memory.
 * However, if it does, then it will allocate some objects which are protected
 * by hazard pointer protection.
 *
 * @details This ring buffer is implemented on a statically sized array.
 * It can only store pointer sized objects that extend the Value class.
 * Further it reserves the 3 LSB for type identification, which makes it
 * compatible only with 64 bit systems
 * TODO(steven): add compile to check to ensure this.
 *
 * It supports enqueue, dequeue, isFull, and isEmpty operations
 *
 * @tparam T The type of information stored, must be a pointer and the class
 * must extend RingBuffer::Value.
 */
template<typename T>
class RingBuffer {
  static const uintptr_t num_lsb = 3;
  static const uintptr_t mark_lsb = 0x1;
  static const uintptr_t emptynode_lsb = 0x2;
  static const uintptr_t oprec_lsb = 0x4;
  static const uintptr_t clear_lsb = 7;

  static_assert(sizeof(T) == sizeof(uintptr_t) &&
    sizeof(uintptr_t) == sizeof(uint64_t), " Pointers muse be 64 bits");

 public:
  /**
   * @brief RingBuffer value class, values stored in the class must extend it.
   * @details
   * This class is necessary to provide the FIFO property.
   * It adds a sequence identifier to each value stored.
   * Using this identifier, we are apply generate FIFO valid sequential history
   * from a concurrent history.
   */
  class Value {
   public:
    /**
     * @brief Empty constructor
     * @details Empty Constructor
     */
    Value() {};
    friend RingBuffer;
   private:
    /**
     * @brief Returns the items seqid
     * @details Returns the items seqid
     * @return Returns the items seqid
     */
    int64_t func_seqid() {
      assert(seqid_ != -1);
      return seqid_;
    }

    /**
     * @brief Sets the items seqid
     * @details Sets the items seqid
     * @return Sets the items seqid
     */
    void func_seqid(int64_t s) {
      seqid_ = s;
    }
    int64_t seqid_{-1};
  };

  /**
   * @brief Ring Buffer constructor
   * @details This constructs and initializes the ring buffer object
   *
   * @param capacity the length of the internal array to allocate.
   */
  RingBuffer(size_t capacity);

  /**
   * @brief Returns whether or not the ring buffer is full.
   * @details Returns whether or not the ring buffer is full.
   * @return Returns whether or not the ring buffer is full.
   */
  bool isFull();

  /**
   * @brief Returns whether or not the ring buffer is empty.
   * @details Returns whether or not the ring buffer is empty.
   * @return Returns whether or not the ring buffer is empty.
   */
  bool isEmpty();

  /**
   * @brief Enqueues the passed value into the buffer
   * @details This function attempts to enqueue the passed value.
   * It returns false in the event the ring buffer is full.
   * Internally it assigned a sequence number to the value.
   *
   * @param value The value to enqueue.
   * @return whether or not the value was enqueued.
   */
  bool enqueue(T value);

  /**
   * @brief Dequeues a value from the buffer
   * @details This function attempts to dequeue a value.
   * It returns false in the event the ring buffer is empty.
   *
   * @param value A variable to store the dequeued value.
   * @return whether or not a value was dequeued.
   */
  bool dequeue(T &value);

  /**
   * @brief This function returns a string debugging information
   * @details This information includes
   * Value at each potion, head, tail, capacity, and someother stuff.
   * @return debugging info string
   */
  std::string debug_string();

 private:
  /**
   * @brief Creates a uintptr_t that represents an emptynode
   * @details the uintptr_t is composed by
   * seqid << num_lsb | empty_node_lsb
   *
   * @param seqid the seqid for this emptynode
   * @return Returns uintptr_t that represents an emptynode
   */
  uintptr_t EmptyNode(int64_t seqid);

  /**
   * @brief Returns the seqid encoded in the passed value
   * @details Returns the seqid encoded in the passed value
   *
   * seqid = val >> num_lsb
   *
   * @param val a unitptr_t loaded from the buffer
   * @return its encoded seqid
   */
  int64_t getEmptyNodeSeqId(uintptr_t val);

  /**
   * @brief Creates a uintptr_t that represents an ValueType
   * @details the uintptr_t is composed by casting value to a uintptr_t.
   *
   * Internally it calls value->func_seqid(seqid) to set the seqid.
   *
   * @param value a pointer type to enqueue
   * @param seqid the sequence id assigned to this value
   * @return Returns uintptr_t that represents an ValueType
   */
  uintptr_t ValueType(T value, int64_t seqid);

  /**
   * @brief Returns the value type from a uintptr
   * @details clears any delayed marks then casts it to type T.
   *
   * @param val The value to cast
   * @return val cast to type T
   */
  T getValueType(uintptr_t val);

  /**
   * @brief Returns the seqid of the passed value
   * @details Returns the seqid of the passed value, first it casts val to type
   * T by calling getValyeType then it calls val->func_seqid();
   *
   * @param val a value read from the ring buffer that has been determined to
   * be a ValueType
   * @return The values seqid
   */
  int64_t getValueTypeSeqId(uintptr_t val);

  /**
   * @brief Takes a uintptr_t and places a bitmark on the mark_lsb
   * @details Takes a uintptr_t and places a bitmark on the mark_lsb
   *
   * @param node The value to mark
   * @return A marked value
   */
  uintptr_t MarkNode(uintptr_t node);

  /**
   * @brief This function is used to get information from a value read from the
   * ring buffer
   * @details This function is used to get information from a value read from the
   * ring buffer. This information is then assigned to the arguments.
   *
   * @param val a value read from a position on the ring buffer
   * @param val_seqid The seqid associated with val
   * @param val_isValueType Whether or not val is a ValueType
   * @param val_isMarked Whether or not val has a delay mark.
   */
  void getInfo(uintptr_t val, int64_t &val_seqid,
    bool &val_isValueType, bool &val_isMarked);

  /**
   * @brief returns whether or not p represents an EmptyNode
   * @details returns whether or not p represents an EmptyNode by examining
   * the emptynode_lsb. If it is 1 then it is an EmptyNode type.
   *
   * @param p the value to examine
   * @return whether or not it is an EmptyNode
   */
  bool isEmptyNode(uintptr_t p);

  /**
   * @brief returns whether or not p represents an ValueType
   * @details returns whether or not p represents an ValueType by examining
   * the emptynode_lsb. If it is 0 then it is an ValueType type.
   *
   * @param p the value to examine
   * @return whether or not it is an ValueType
   */
  bool isValueType(uintptr_t p);

  /**
   * @brief returns whether or not p has a delay mark
   * @details returns whether or not p has a delay mark by examining
   * the mark_lsb. If it is 1 then it is delayed
   *
   * @param p the value to examine
   * @return whether or not it is has a delay mark.
   */
  bool isDelayedMarked(uintptr_t p);

  /**
   * @brief performs a fetch-and-add on the head counter
   * @details atomically increments the head
   * @return returns the pre-incremented value of the head counter.
   */
  int64_t nextHead();

  /**
   * @brief performs a fetch-and-add on the tail counter
   * @details atomically increments the tail
   * @return returns the pre-incremented value of the tail counter.
   */
  int64_t nextTail();

  /**
   * @brief Returns the next seqid
   * @details Returns the next seqid, which is seqid+capacity_
   *
   * @param seqid The current seqid
   * @return the next seqid
   */
  int64_t nextSeqId(int64_t seqid);

  /**
   * @brief Returns the position a seqid belongs at
   * @details Returns the position a seqid belongs at, determined by
   * seqid % capacity_
   *
   * @param seqid the seqid
   * @return the position the seqid belongs at
   */
  int64_t getPos(int64_t seqid);

  /**
   * @brief A backoff routine in the event of thread delay
   * @details This function is called in the event the value at position on the
   * ringbuffer is lagging behind as a result of a delayed thread.
   *
   * It calls tervel::util::backoff() and upon its return checks whether or the value at
   * address has changed.
   * If it has, it returns true, Else it returns false.
   * If the value has changed the new value is assigned to val
   *
   * @param address The address val was read from
   * @param val The last read value from address. If this function returns true
   * then val is updated, otherwise it is left unchanged.
   *
   * @return whether or not the val changed.
   */
  bool backoff(std::atomic<uintptr_t> *address, uintptr_t &val);

  /**
   * @brief This function places a bitmark on the value held at address
   * @details This function places a bitmark on the value held at address by
   * calling address->fetch_or(mark_lsb) and then it loads the new current value
   * and assigns it to val.
   *
   * @param address The address to perfom the blind bitmark.
   * @return the new value at address
   */
  uintptr_t atomic_delay_mark(std::atomic<uintptr_t> *address);

  /**
   * @brief This prints out debugging information.
   * @details This prints out debugging information.
   *
   * @param val a value loaded from a position on the buffer
   * @return a string representation the contents of val.
   */
  std::string debug_string(uintptr_t val);

  const int64_t capacity_;
  std::atomic<int64_t> head {0};
  std::atomic<int64_t> tail {0};
  std::unique_ptr<std::atomic<uintptr_t>[]> array_;

};  // class RingBuffer<Value>



}  // namespace wf
}  // namespace containers
}  // namespace tervel

#include <tervel/containers/wf/ring-buffer/wf_ring_buffer_imp.h>

#endif  // TERVEL_CONTAINERS_WF_RINGBUFFER_RINGBUFFER_H_