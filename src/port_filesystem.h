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

#ifndef NACLFS_PORT_FILESYSTEM_H_
#define NACLFS_PORT_FILESYSTEM_H_
#pragma once

#include <vector>

#include "filesystem.h"

namespace pp {

class Var;

};  // namespace pp

namespace naclfs {

class NaClFs;

class PortFileSystem : public FileSystem::Delegate {
 public:
  PortFileSystem(NaClFs* naclfs);
  virtual ~PortFileSystem();
  virtual int Open(const char* path, int oflag, mode_t cmode);
  virtual int Stat(const char* path, struct stat* buf);
  virtual int Close();
  virtual int Fstat(struct stat* buf);
  virtual ssize_t Read(void* buf, size_t nbytes);
  virtual ssize_t Write(const void* buf, size_t nbytes);
  virtual off_t Seek(off_t offset, int whence);
  virtual int IsATty() { return 0; }
  virtual int Fcntl(int cmd, va_list* ap);
  virtual int MkDir(const char* path, mode_t mode) { return -1; }
  virtual DIR* OpenDir(const char* dirname) { return NULL; }
  virtual void RewindDir(DIR* dirp) {}
  virtual struct dirent* ReadDir(DIR* dirp) { return NULL; }
  virtual int CloseDir(DIR* dirp) { return -1; }
  static bool HandleMessage(const pp::Var& message);

 private:
  static std::vector<uint8_t> buffer_;

  NaClFs* naclfs_;
  bool readable_;
  bool writable_;
  bool blocking_;
  int oflag_;
  int id_;
};

}  // namespace naclfs

#endif  // NACLFS_PORT_FILESYSTEM_H_
