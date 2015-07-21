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
#ifndef TERVEL_UTIL_TERVEL_H
#define TERVEL_UTIL_TERVEL_H

#include <tervel/util/util.h>
#include <tervel/util/thread_context.h>
#include <tervel/util/progress_assurance.h>
#include <tervel/util/memory/hp/hazard_pointer.h>
#include <tervel/util/memory/rc/pool_manager.h>

namespace tervel {

/**
 * Contains shared information that should be accessible by all threads.
 */
class Tervel {
 public:
  explicit Tervel(size_t num_threads)
      : num_threads_  {num_threads}
      , hazard_pointer_(num_threads)
      , rc_pool_manager_(num_threads)
      , progress_assurance_(num_threads) {}

  ~Tervel() {
    // Notice: The destructor of the member variables are called when this
    // object is freed.
  }

 private:
  uint64_t get_thread_id() {
    return active_threads_.fetch_add(1);
  }

  // The total number of expected threads in the system.
  const uint64_t num_threads_;

  // The number of threads which have been assigned an thread_id
  std::atomic<uint64_t> active_threads_ {0};

  // The shared hazard_pointer object
  util::memory::hp::HazardPointer hazard_pointer_;

  // Shared RC Descriptor Pool Manager
  util::memory::rc::PoolManager rc_pool_manager_;

  // Shared Progress Assurance Object
  util::ProgressAssurance progress_assurance_;

  friend ThreadContext;

  DISALLOW_COPY_AND_ASSIGN(Tervel);
};

}  // namespace tervel
#endif  // TERVEL_UTIL_TERVEL_H