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
#include <string>
#include <vector>

#include "ppapi/cpp/completion_callback.h"

namespace pp {

class Core;
class Var;

};  // namespace pp

namespace naclfs {

class NaClFs;

class FileSystem {
 public:
  class Delegate {
   protected:
    enum Function {
      OPEN,
      READ,
      WRITE,
      LSEEK,
      CLOSE,
    };
    typedef struct _Arguments {
      enum Function function;
      Delegate* delegate;
      bool chaining;
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
        int32_t callback;
        int open;
        ssize_t read;
        ssize_t write;
        off_t lseek;
        int close;
      } result;
    } Arguments;

   public:
    Delegate();
    virtual int Open(const char* path, int oflag, ...);
    virtual ssize_t Read(void* buf, size_t nbytes);
    virtual ssize_t Write(const void* buf, size_t nbytes);
    virtual off_t Lseek(off_t offset, int whence);
    virtual int Close();

    virtual int OpenCall(
        Arguments* arguments, const char* path, int oflag, ...) { return 0; };
    virtual ssize_t ReadCall(
        Arguments* arguments, void* buf, size_t nbytes) { return 0; };
    virtual ssize_t WriteCall(
        Arguments* arguments, const void* buf, size_t nbytes) { return 0; };
    virtual off_t LseekCall(
        Arguments* arguments, off_t offset, int whence) { return 0; };
    virtual int CloseCall(Arguments* arguments) { return 0; };

   protected:
    pp::CompletionCallback callback_;

   private:
    void Lock();
    void Unlock();

    void Call(Arguments& arguments);
    static void Proxy(void* param, int32_t result);
    static void Switch(Arguments* arguments);

    static bool initialized_;
    static pthread_mutex_t mutex_;
    static pp::Core* core_;
  };

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

  std::vector<Delegate*> descriptors_;
  std::string cwd_;
  pthread_mutex_t mutex_;
  pp::Core* core_;
  NaClFs* naclfs_;
};

}  // namespace naclfs

#endif  // NACLFS_FILESYSTEM_H_
