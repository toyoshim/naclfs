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

#include <irt.h>

#include <sstream>

#include "filesystem.h"
#include "naclfs.h"

int __wrap_open(const char* path, int oflag, mode_t cmode, int* newfd) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter open:" << "\n path=" << path << "\n oflag=" << oflag << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  *newfd = naclfs::NaClFs::GetFileSystem()->Open(path, oflag);
  // TODO: Handle cmode, and returning errno.
  return 0;
}

int __wrap_stat(const char* path, struct stat* buf) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter stat:" << "\n path=" << path << "\n buf=" << buf << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Stat(path, buf);
}

int __wrap_close(int fildes) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter close:" << "\n fildes=" << fildes << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Close(fildes);
}

int __wrap_read(int fildes, void* buf, size_t nbytes, size_t* nread) {
  if (fildes > 2 && naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter read:" << "\n fildes=" << fildes << "\n buf=" << buf <<
        "\n nbytes=" << nbytes << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  *nread = naclfs::NaClFs::GetFileSystem()->Read(fildes, buf, nbytes);
  // TODO: Handle returning errono.
  return 0;
}

int __wrap_write(int fildes, const void* buf, size_t nbytes, size_t* nwrote) {
  if (fildes > 2 && naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter write:" << "\n fildes=" << fildes << "\n buf=" << buf <<
        "\n nbytes=" << nbytes << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  *nwrote = naclfs::NaClFs::GetFileSystem()->Write(fildes, buf, nbytes);
  // TODO: Handle returning errono.
  return 0;
}

int __wrap_seek(int fildes, off_t offset, int whence, off_t* new_offset) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter seek:" << "\n fildes=" << fildes << "\n offset=" << offset <<
        "\n whence=" << whence << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  // TODO: Rename Lseek to Seek.
  *new_offset = naclfs::NaClFs::GetFileSystem()->Lseek(fildes, offset, whence);
  // TODO: Handle returning errono.
  return 0;
}

int __wrap_fcntl(int fildes, int cmd, ...) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter fcntl:" << "\n fildes=" << fildes << "\n cmd=" << cmd << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Fcntl(fildes, cmd);
}

DIR* __wrap_opendir(const char* dirname) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter opendir:" << "\n dirname=" << dirname << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->OpenDir(dirname);
}

void __wrap_rewinddir(DIR* dirp) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter rewinddir:" << "\n dirp=" << dirp << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  naclfs::NaClFs::GetFileSystem()->RewindDir(dirp);
}

struct dirent* __wrap_readdir(DIR* dirp) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter readdir:" << "\n dirp=" << dirp << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->ReadDir(dirp);
}

int __wrap_closedir(DIR* dirp) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter closedir:" << "\n dirp=" << dirp << "\n";
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->CloseDir(dirp);
}

extern "C" struct nacl_irt_filename __libnacl_irt_filename;
extern "C" struct nacl_irt_fdio __libnacl_irt_fdio;
void do_wrap(void) {
  __libnacl_irt_filename.open = __wrap_open;
  __libnacl_irt_filename.stat = __wrap_stat;

  __libnacl_irt_fdio.close = __wrap_close;
  //__libnacl_irt_fdio.dup = __wrap_dup;
  //__libnacl_irt_fdio.dup2 = __wrap_dup2;
  __libnacl_irt_fdio.read = __wrap_read;
  __libnacl_irt_fdio.write = __wrap_write;
  __libnacl_irt_fdio.seek = __wrap_seek;
  //__libnacl_irt_fdio.fstat = __wrap_fstat;
  //__libnacl_irt_fdio.getdents = __wrap_getdents;
}
