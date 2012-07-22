/*
 * Copyright (c) 2012, Takashi TOYOSHIMA <toyoshim@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of the authors nor the names of its contributors may be
 *   used to endorse or promote products derived from this software with out
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUE
 * NTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "wrap.h"

#include <sstream>

#include "filesystem.h"
#include "naclfs.h"

int __wrap_open(const char* path, int oflag, ...) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter open:" << "\n path=" << path << "\n oflag=" << oflag << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Open(path, oflag);
}

ssize_t __wrap_read(int fildes, void* buf, size_t nbytes) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter read:" << "\n fildes=" << fildes << "\n buf=" << buf <<
        "\n nbytes=" << nbytes << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Read(fildes, buf, nbytes);
}

ssize_t __wrap_write(int fildes, const void* buf, size_t nbytes) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter write:" << "\n fildes=" << fildes << "\n buf=" << buf <<
        "\n nbytes=" << nbytes << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Write(fildes, buf, nbytes);
}

off_t __wrap_lseek(int fildes, off_t offset, int whence) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter lseek:" << "\n fildes=" << fildes << "\n offset=" << offset <<
        "\n whence=" << whence << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Lseek(fildes, offset, whence);
}

int __wrap_fcntl(int fildes, int cmd, ...) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter fcntl:" << "\n fildes=" << fildes << "\n cmd=" << cmd << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Fcntl(fildes, cmd);
}

int __wrap_close(int fildes) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter close:" << "\n fildes=" << fildes << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Close(fildes);
}

int __wrap_stat(const char* path, struct stat* buf) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter stat:" << "\n path=" << path << "\n buf=" << buf << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Stat(path, buf);
}
