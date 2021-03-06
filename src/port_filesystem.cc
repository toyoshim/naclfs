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

#include "port_filesystem.h"

#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <sstream>

#include "naclfs.h"
#include "ppapi/cpp/var.h"

namespace naclfs {

static const char kStdInPath[] = "/dev/stdin";
static const char kStdOutPath[] = "/dev/stdout";
static const char kStdErrPath[] = "/dev/stderr";

std::vector<uint8_t> PortFileSystem::buffer_;

PortFileSystem::PortFileSystem(NaClFs* naclfs)
    : naclfs_(naclfs),
      readable_(false),
      writable_(false),
      blocking_(true),
      oflag_(0),
      id_(-1) {
}

PortFileSystem::~PortFileSystem() {
}

int PortFileSystem::Open(const char* path, int oflag, mode_t cmode) {
  oflag_ = oflag;
  if (!strcmp(path, kStdInPath)) {
    id_ = STDIN_FILENO;
    readable_ = true;
    if (oflag != O_RDONLY)
      naclfs_->Log(
          "PortFileSystem: ignore oflag which doesn't match for stdin.\n");
  } else if (!strcmp(path, kStdOutPath)) {
    id_ = STDOUT_FILENO;
    writable_ = true;
    if (oflag != O_WRONLY)
      naclfs_->Log(
          "PortFileSystem: ignore oflag which doesn't match for stdout.\n");
  } else if (!strcmp(path, kStdErrPath)) {
    id_ = STDERR_FILENO;
    writable_ = true;
    if (oflag != O_WRONLY)
      naclfs_->Log(
          "PortFileSystem: ignore oflag which doesn't match for stderr.\n");
  } else {
    std::stringstream ss;
    ss << "PortFileSystem: can not open unknown device name: " << path
       << std::endl;
    naclfs_->Log(ss.str().c_str());
    return ENODEV;
  }
  return 0;
}

int PortFileSystem::Stat(const char* path, struct stat* buf) {
  if (strcmp(path, kStdInPath) &&
      strcmp(path, kStdOutPath) &&
      strcmp(path, kStdErrPath)) {
    std::stringstream ss;
    ss << "PortFileSystem: can not open unknown device name: " << path
       << std::endl;
    naclfs_->Log(ss.str().c_str());
    return ENODEV;
  }
  memset(buf, 0, sizeof(struct stat));
  buf->st_mode = S_IFCHR | S_IRUSR | S_IWUSR;
  return 0;
}

int PortFileSystem::Close() {
  if (id_ < 0)
    return -1;
  return 0;
}

int PortFileSystem::Fstat(struct stat* buf) {
  memset(buf, 0, sizeof(struct stat));
  buf->st_mode = S_IFCHR | S_IRUSR | S_IWUSR;
  return 0;
}

ssize_t PortFileSystem::Read(void* buf, size_t nbytes) {
  if (id_ < 0 || !readable_)
    return -1;
  size_t read_size = 0;
  uint8_t* write_ptr = static_cast<uint8_t*>(buf);
  while (read_size < nbytes && !buffer_.empty()) {
    *write_ptr++ = buffer_.front();
    read_size++;
    buffer_.erase(buffer_.begin());
  }
  if (read_size == 0) {
    if (blocking_)
      return -2;
    else
      return -1;
  }
  return static_cast<ssize_t>(read_size);
}

ssize_t PortFileSystem::Write(const void* buf, size_t nbytes) {
  if (id_ < 0 || !writable_)
    return -1;
  std::stringstream ss;
  ss << "S" << id_ << std::string(static_cast<const char*>(buf), nbytes);
  naclfs_->PostMessage(pp::Var(ss.str()));
  return nbytes;
}

off_t PortFileSystem::Seek(off_t offset, int whence) {
  naclfs_->Log("PortFileSystem::Seek not supported.\n");
  return 0;
}

int PortFileSystem::Fcntl(int cmd, va_list* ap) {
  if (cmd == F_SETFL) {
    long flag = va_arg(*ap, long);
    blocking_ = !(flag & O_NONBLOCK);
    if (flag & ~O_NONBLOCK) {
      std::stringstream ss;
      ss << "PortFileSystem::Fcntl F_SETFL unknown flag: ";
      ss << flag << std::endl;
      naclfs_->Log(ss.str().c_str());
    }
  } else if (cmd == F_GETFL) {
    return oflag_;
  } else {
    naclfs_->Log("PortFileSystem::Fcntl not supported.\n");
    errno = ENOSYS;
    return -1;
  }
  errno = 0;
  return 0;
}

bool PortFileSystem::HandleMessage(const pp::Var& message) {
  if (!message.is_string())
    return false;
  std::string s = message.AsString();
  if (s[0] != 'S')
    return false;
  PortFileSystem::buffer_.push_back(s[2]);
  return true;
}

}  // namespace naclfs
