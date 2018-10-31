// LevelDB Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// See port_example.h for documentation for the following types/functions.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of the University of California, Berkeley nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef STORAGE_LEVELDB_PORT_PORT_WIN_H_
#define STORAGE_LEVELDB_PORT_PORT_WIN_H_

#define OS_WIN 1
#define COMPILER_MSVC 1
#define ARCH_CPU_X86_FAMILY 1 

#ifdef _MSC_VER
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
#define close _close
#define fread_unlocked _fread_nolock
#endif

#include <unistd.h>

#include <string>
#include <stdint.h>
#ifdef SNAPPY
#include <snappy.h>
#endif

#if defined(HAVE_CRC32C)
#include <crc32c/crc32c.h>
#endif  // defined(HAVE_CRC32C)

#include <stddef.h>
#include <stdint.h>
#include <cassert>
#include <condition_variable>  // NOLINT
#include <mutex>               // NOLINT
#include <string>
#include "port/atomic_pointer.h"
#include "port/thread_annotations.h"

namespace leveldb {
namespace port {

// Windows is little endian (for now :p)
static const bool kLittleEndian = true;

class CondVar;

// Thinly wraps std::mutex.
class LOCKABLE Mutex {
public:
	Mutex() = default;
	~Mutex() = default;

	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;

	void Lock() EXCLUSIVE_LOCK_FUNCTION() { mu_.lock(); }
	void Unlock() UNLOCK_FUNCTION() { mu_.unlock(); }
	void AssertHeld() ASSERT_EXCLUSIVE_LOCK() { }

private:
	friend class CondVar;
	std::mutex mu_;
};

// Thinly wraps std::condition_variable.
class CondVar {
public:
	explicit CondVar(Mutex* mu) : mu_(mu) { assert(mu != nullptr); }
	~CondVar() = default;

	CondVar(const CondVar&) = delete;
	CondVar& operator=(const CondVar&) = delete;

	void Wait() {
		std::unique_lock<std::mutex> lock(mu_->mu_, std::adopt_lock);
		cv_.wait(lock);
		lock.release();
	}
	void Signal() { cv_.notify_one(); }
	void SignalAll() { cv_.notify_all(); }
private:
	std::condition_variable cv_;
	Mutex* const mu_;
};

class OnceType {
public:
//    OnceType() : init_(false) {}
    OnceType(const OnceType &once) : init_(once.init_) {}
    OnceType(bool f) : init_(f) {}
    void InitOnce(void (*initializer)()) {
        mutex_.Lock();
        if (!init_) {
            init_ = true;
            initializer();
        }
        mutex_.Unlock();
    }

private:
    bool init_;
    Mutex mutex_;
};

#define LEVELDB_ONCE_INIT false
inline void InitOnce(OnceType* once, void (*initializer)()) {
  once->InitOnce(initializer);
}

inline bool Snappy_Compress(const char* input, size_t length,
                            ::std::string* output) {
#ifdef SNAPPY
  output->resize(snappy::MaxCompressedLength(length));
  size_t outlen;
  snappy::RawCompress(input, length, &(*output)[0], &outlen);
  output->resize(outlen);
  return true;
#endif

  return false;
}

inline bool Snappy_GetUncompressedLength(const char* input, size_t length,
                                         size_t* result) {
#ifdef SNAPPY
  return snappy::GetUncompressedLength(input, length, result);
#else
  return false;
#endif
}

inline bool Snappy_Uncompress(const char* input, size_t length,
                              char* output) {
#ifdef SNAPPY
  return snappy::RawUncompress(input, length, output);
#else
  return false;
#endif
}

inline bool GetHeapProfile(void (*func)(void*, const char*, int), void* arg) {
  return false;
}

inline uint32_t AcceleratedCRC32C(uint32_t crc, const char* buf, size_t size) {
#if HAVE_CRC32C
  return ::crc32c::Extend(crc, reinterpret_cast<const uint8_t*>(buf), size);
#else
  return 0;
#endif  // HAVE_CRC32C
}

}
}

#endif  // STORAGE_LEVELDB_PORT_PORT_WIN_H_
