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
#include <stdarg.h>

#include <sstream>

#include "filesystem.h"
#include "naclfs.h"

int __wrap_open(const char* path, int oflag, mode_t cmode, int* newfd) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter open:" << std::endl;
    ss << " path=" << path << std::endl;
    ss << " oflag=" << oflag << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Open(path, oflag, cmode, newfd);
}

int __wrap_stat(const char* path, struct stat* buf) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter stat:" << std::endl;
    ss << " path=" << path << std::endl;
    ss << " buf=" << buf << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Stat(path, buf);
}

int __wrap_close(int fildes) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter close:" << std::endl;
    ss << " fildes=" << fildes << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->Close(fildes);
}

int __wrap_read(int fildes, void* buf, size_t nbytes, size_t* nread) {
  if (fildes > 2 && naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter read:" << std::endl;
    ss << " fildes=" << fildes << std::endl;
    ss << " buf=" << buf << std::endl;
    ss << " nbytes=" << nbytes << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  *nread = naclfs::NaClFs::GetFileSystem()->Read(fildes, buf, nbytes);
  // TODO: Handle returning errono.
  return 0;
}

int __wrap_write(int fildes, const void* buf, size_t nbytes, size_t* nwrote) {
  if (fildes > 2 && naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter write:" << std::endl;
    ss << " fildes=" << fildes << std::endl;
    ss << " buf=" << buf << std::endl;
    ss << " nbytes=" << nbytes << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  *nwrote = naclfs::NaClFs::GetFileSystem()->Write(fildes, buf, nbytes);
  // TODO: Handle returning errono.
  return 0;
}

int __wrap_seek(int fildes, off_t offset, int whence, off_t* new_offset) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter seek:" << std::endl;
    ss << " fildes=" << fildes << std::endl;
    ss << " offset=" << offset << std::endl;
    ss << " whence=" << whence << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  *new_offset = naclfs::NaClFs::GetFileSystem()->Seek(fildes, offset, whence);
  // TODO: Handle returning errono.
  return 0;
}

extern "C" int fcntl(int fildes, int cmd, ...) {
  va_list ap;
  va_start(ap, cmd);
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter fcntl:" << std::endl;
    ss << " fildes=" << fildes << std::endl;
    ss << " cmd=" << cmd << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  int result = naclfs::NaClFs::GetFileSystem()->Fcntl(fildes, cmd, &ap);
  va_end(ap);
  return result;
}

extern "C" int mkdir(const char* path, mode_t mode) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter mkdir:" << std::endl;
    ss << " path=" << path << std::endl;
    ss << " mode=" << mode << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->MkDir(path, mode);
}

extern "C" DIR* opendir(const char* dirname) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter opendir:" << std::endl;
    ss << " dirname=" << dirname << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->OpenDir(dirname);
}

extern "C" void rewinddir(DIR* dirp) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter rewinddir:" << std::endl;
    ss << " dirp=" << dirp << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  naclfs::NaClFs::GetFileSystem()->RewindDir(dirp);
}

extern "C" struct dirent* readdir(DIR* dirp) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter readdir:" << std::endl;
    ss << " dirp=" << dirp << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->ReadDir(dirp);
}

extern "C" int closedir(DIR* dirp) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter closedir:" << std::endl;
    ss << " dirp=" << dirp << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->CloseDir(dirp);
}

extern "C" int chdir(const char* path) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter chdir:" << std::endl;
    ss << " path=" << path << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->ChDir(path);
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
