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

#ifndef NACLFS_HTML5_FILESYSTEM_H_
#define NACLFS_HTML5_FILESYSTEM_H_
#pragma once

#include <pthread.h>

#include "filesystem.h"
#include "ppapi/c/pp_file_info.h"

namespace pp {

class FileIO;
class FileRef;
class FileSystem;
class Var;

};  // namespace pp

namespace naclfs {

class NaClFs;

class Html5FileSystem : public FileSystem::Delegate {
 public:
  Html5FileSystem(NaClFs* naclfs);
  virtual ~Html5FileSystem();

  virtual int OpenCall(Arguments* arguments, const char* path, int oflag, ...);
  virtual ssize_t ReadCall(Arguments* arguments, void* buf, size_t nbytes);
  virtual ssize_t WriteCall(
      Arguments* arguments, const void* buf, size_t nbytes);
  virtual off_t LseekCall(Arguments* arguments, off_t offset, int whence);
  virtual int FcntlCall(Arguments* arguments, int cmd, ...);
  virtual int CloseCall(Arguments* arguments);
  virtual int StatCall(
      Arguments* arguments, const char* path, struct stat* buf);
  virtual DIR* OpenDirCall(Arguments* arguments, const char* dirname);
  virtual void RewindDirCall(Arguments* arguments, DIR* dirp);
  virtual struct dirent* ReadDirCall(Arguments* arguments, DIR* dirp);
  virtual int CloseDirCall(Arguments* arguments, DIR* dirp);
  static bool HandleMessage(const pp::Var& message);

 private:
  int Initialize(Arguments* arguments);

  static pp::FileSystem* filesystem_;

  pp::FileRef* file_ref_;
  pp::FileIO* file_io_;
  PP_FileInfo file_info_;
  NaClFs* naclfs_;
  bool waiting_;
  bool querying_;
  off_t offset_;
};

}  // namespace naclfs

#endif  // NACLFS_HTML5_FILESYSTEM_H_
