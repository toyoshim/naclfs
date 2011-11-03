//
// Copyright (c) 2011, Takashi TOYOSHIMA <toyoshim@gmail.com>
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

#ifndef NACLFS_FILESYSTEM_H_
#define NACLFS_FILESYSTEM_H_
#pragma once

#include <pthread.h>
#include <stdio.h>
#include <vector>

namespace pp {

class Core;
class Var;

};  // namespace pp

namespace naclfs {

class NaClFs;

class FileSystem {
 public:
  enum Function {
    OPEN,
    READ,
    WRITE,
    LSEEK,
    CLOSE,
  };

 class Delegate {
   public:
    virtual int Open(const char* path, int oflag, ...) = 0;
    virtual ssize_t Read(void* buf, size_t nbytes) = 0;
    virtual ssize_t Write(const void* buf, size_t nbytes) = 0;
    virtual off_t Lseek(off_t offset, int whence) = 0;
    virtual int Close() = 0;
  };

  typedef struct _Arguments {
    enum Function function;
    Delegate* delegate;
    FileSystem* self;
    union {
      struct {
        const char* path;
        int oflag;
      } open;
      struct {
        void* buf;
        size_t nbytes;
      } read;
      struct {
        const void* buf;
        size_t nbytes;
      } write;
      struct {
        off_t offset;
        int whence;
      } lseek;
    } u;
    union {
      int open;
      ssize_t read;
      ssize_t write;
      off_t lseek;
      int close;
    } result;
  } Arguments;

  FileSystem(NaClFs* naclfs);
  ~FileSystem();

  int Open(const char* path, int oflag, ...);
  ssize_t Read(int fildes, void* buf, size_t nbytes);
  ssize_t Write(int fildes, const void* buf, size_t nbytes);
  off_t Lseek(int fildes, off_t offset, int whence);
  int Close(int fildes);

  static bool HandleMessage(const pp::Var& message);

 private:
  Delegate* CreateDelegate(const char* path);
  int BindToDescriptor(Delegate* delegate);
  Delegate* GetDelegate(int fildes);
  void DeleteDescriptor(int fildes);

  void Call(Arguments& arguments);
  void SwitchViaProxy(Arguments& arguments);
  static void Proxy(void* param, int32_t result);
  static void Switch(Arguments* arguments);

  std::vector<Delegate*> descriptors_;
  pthread_mutex_t call_mutex_;
  pthread_mutex_t proxy_sync_mutex_;
  pthread_mutex_t block_mutex_;
  pp::Core* core_;
  NaClFs* naclfs_;
};

}  // namespace naclfs

#endif  // NACLFS_FILESYSTEM_H_
