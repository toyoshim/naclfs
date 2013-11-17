//
// Copyright (c) 2012, Takashi TOYOSHIMA <toyoshim@gmail.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// - Neither the name of the authors nor the names of its contributors may be
//   used to endorse or promote products derived from this software with out
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUE
// NTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//

#ifndef NACLFS_NACLFS_H_
#define NACLFS_NACLFS_H_
#pragma once

#include <pthread.h>
#include <stdint.h>

#include <string>

namespace pp {

class Instance;
class Core;
class Var;

}  // namespace naclfs

namespace naclfs {

class FileSystem;

class NaClFs {
 public:
  NaClFs(pp::Instance* instance);
  ~NaClFs();

  // TODO: Make these function thread safe
  static bool trace() { return trace_; }
  static void set_trace(bool enable) { trace_ = enable; }
  static void Log(const char* message);

  static FileSystem* GetFileSystem() { return single_instance_->filesystem_; }
  static pp::Instance* GetInstance() { return single_instance_->instance_; }

  static void PostMessage(const pp::Var& message);
  bool HandleMessage(const pp::Var& message);

 private:
  static void PostMessageFromMainThread(void* param, int32_t result);

  static NaClFs* single_instance_;
  static bool trace_;
  FileSystem* filesystem_;
  pp::Core* core_;
  pp::Instance* instance_;
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
};

}  // namespace naclfs

#endif  // NACLFS_NACLFS_H_
