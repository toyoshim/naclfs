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

#include <assert.h>
#include <errno.h>
#include <irt.h>
#if defined(__GLIBC__)
#  include <irt_syscalls.h>
#endif  // defined(__GLIBC__)
#include <stdarg.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

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
  if (!path)
    return EFAULT;
  if (!path[0])
    return ENOENT;
  int result = naclfs::NaClFs::GetFileSystem()->Open(path, oflag, cmode, newfd);
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "leave open: " << *newfd << std::endl;
    ss << " errno=" << errno << "; " << strerror(errno) << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return result;
}

#if defined(__GLIBC__)
int __wrap_stat(const char* path, struct nacl_abi_stat* nacl_buf) {
  struct stat libc_buf;
  struct stat* buf = NULL;
  if (NULL != nacl_buf)
    buf = &libc_buf;
#else  // defined(__GLIBC__)
int __wrap_stat(const char* path, struct stat* buf) {
#endif // defined(__GLIBC__)
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter stat:" << std::endl;
    ss << " path=" << path << std::endl;
    ss << " buf=" << buf << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  if (!path || !buf)
    return EFAULT;
  if (!path[0])
    return ENOENT;
  int result = naclfs::NaClFs::GetFileSystem()->Stat(path, buf);
#if defined(__GLIBC__)
  if (nacl_buf) {
    memset(nacl_buf, 0, sizeof(struct nacl_abi_stat));
    nacl_buf->nacl_abi_st_mode = buf->st_mode;
    nacl_buf->nacl_abi_st_nlink = buf->st_nlink;
    nacl_buf->nacl_abi_st_size = buf->st_size;
    nacl_buf->nacl_abi_st_blksize = buf->st_blksize;
    nacl_buf->nacl_abi_st_blocks = buf->st_blocks;
    nacl_buf->nacl_abi_st_atime = buf->st_atim.tv_sec;
    nacl_buf->nacl_abi_st_atimensec = buf->st_atim.tv_nsec;
    nacl_buf->nacl_abi_st_mtime = buf->st_mtim.tv_sec;
    nacl_buf->nacl_abi_st_mtimensec = buf->st_mtim.tv_nsec;
    nacl_buf->nacl_abi_st_ctime = buf->st_ctim.tv_sec;
    nacl_buf->nacl_abi_st_ctimensec = buf->st_mtim.tv_nsec;
  }
#endif // defined(__GLIBC__)
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "leave stat: " << result << std::endl;
    ss << " errno=" << errno << "; " << strerror(errno) << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return result;
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

int __wrap_dup(int fd, int* newfd) {
  naclfs::NaClFs::Log("XXX not impl: dup\n");
  return 0;
}

int __wrap_dup2(int fd, int newfd) {
  naclfs::NaClFs::Log("XXX not impl: dup2\n");
  return 0;
}

#if defined(__GLIBC__)
int __wrap_fstat(int fd, struct nacl_abi_stat* nacl_buf) {
  struct stat libc_buf;
  struct stat* buf = NULL;
  if (NULL != nacl_buf)
    buf = &libc_buf;
#else  // defined(__GLIBC__)
int __wrap_fstat(int fd, struct stat* buf) {
#endif // defined(__GLIBC__)
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter fstat:" << std::endl;
    ss << " fd=" << fd << std::endl;
    ss << " buf=" << buf << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  if (!buf)
    return EFAULT;
  int result = naclfs::NaClFs::GetFileSystem()->Fstat(fd, buf);
#if defined(__GLIBC__)
  if (nacl_buf) {
    memset(nacl_buf, 0, sizeof(struct nacl_abi_stat));
    nacl_buf->nacl_abi_st_mode = buf->st_mode;
    nacl_buf->nacl_abi_st_nlink = buf->st_nlink;
    nacl_buf->nacl_abi_st_size = buf->st_size;
    nacl_buf->nacl_abi_st_blksize = buf->st_blksize;
    nacl_buf->nacl_abi_st_blocks = buf->st_blocks;
    nacl_buf->nacl_abi_st_atime = buf->st_atim.tv_sec;
    nacl_buf->nacl_abi_st_atimensec = buf->st_atim.tv_nsec;
    nacl_buf->nacl_abi_st_mtime = buf->st_mtim.tv_sec;
    nacl_buf->nacl_abi_st_mtimensec = buf->st_mtim.tv_nsec;
    nacl_buf->nacl_abi_st_ctime = buf->st_ctim.tv_sec;
    nacl_buf->nacl_abi_st_ctimensec = buf->st_mtim.tv_nsec;
  }
#endif // defined(__GLIBC__)
  return result;
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

extern "C" int __wrap_access(const char* path, int amode) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter access:" << std::endl;
    ss << " path=" << path << std::endl;
    ss << " amode=" << amode << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  struct stat buf;
  int result = naclfs::NaClFs::GetFileSystem()->Stat(path, &buf);
  if (result) {
    errno = result;
    return -1;
  }
  // TODO: Currently, naclfs doesn't have the idea of file owner.
  // And stat returns the same mode for all of user, group, and other.
  mode_t mode = (mode_t)amode;
  if ((buf.st_mode & mode) != mode) {
    errno = EACCES;
    return -1;
  }
  if (naclfs::NaClFs::trace())
    naclfs::NaClFs::Log("leave access: 0");
  return 0;
}

extern "C" int __wrap_isatty(int fildes) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter isatty:" << std::endl;
    ss << " fildes=" << fildes << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  int result = naclfs::NaClFs::GetFileSystem()->IsATty(fildes);
  if (result >= 0)
    errno = result;
  else
    errno = 0;
  if (result)
    return 0;
  return 1;
}

extern "C" int __wrap_fcntl(int fildes, int cmd, ...) {
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
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "leave fcntl: " << result << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
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

extern "C" int readdir_r(DIR* dirp,
                         struct dirent* entry,
                         struct dirent** result) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter readdir_r:" << std::endl;
    ss << " dirp=" << dirp << std::endl;
    ss << " entry=" << entry << std::endl;
    ss << " result=" << result << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  if (!dirp || !entry || !result) {
    if (result)
      *result = NULL;
    return EINVAL;  // TODO: Set proper errno.
  }
  struct dirent* dir = naclfs::NaClFs::GetFileSystem()->ReadDir(dirp);
  if (!dir) {
    if (result)
      *result = NULL;
    return EFAULT;  // TODO: Set proper errno.
  }
  memcpy(entry, dir, sizeof(struct dirent));
  *result = entry;
  return 0;
}

extern "C" long telldir(DIR* dirp) {
  naclfs::NaClFs::Log("XXX not impl: telldir\n");
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter telldir:" << std::endl;
    ss << " dirp=" << dirp << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return -1;
}

extern "C" void seekdir(DIR* dirp, long offset) {
  naclfs::NaClFs::Log("XXX not impl: seekdir\n");
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter seekdir:" << std::endl;
    ss << " dirp=" << dirp << std::endl;
    ss << " offset=" << offset << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
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

extern "C" char* getcwd(char* buf, size_t size) {
  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "enter getcwd:" << std::endl;
    ss << " buf=" << buf << std::endl;
    ss << " size=" << size << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
  }
  return naclfs::NaClFs::GetFileSystem()->GetCwd(buf, size);
}

extern "C" char* getwd(char* buf) {
  return getcwd(buf, MAXPATHLEN);
}

int (*__nacl_irt_write_real)(int, const void*, size_t, size_t*);

#if defined(__GLIBC__)
__attribute__((constructor)) static void wrap() {
  __nacl_irt_open = __wrap_open;
  __nacl_irt_stat = __wrap_stat;
  __nacl_irt_close = __wrap_close;
  __nacl_irt_dup = __wrap_dup;
  __nacl_irt_dup2 = __wrap_dup2;
  __nacl_irt_read = __wrap_read;
  __nacl_irt_write_real = __nacl_irt_write;
  __nacl_irt_write = __wrap_write;
  __nacl_irt_seek = __wrap_seek;
  __nacl_irt_fstat = __wrap_fstat;

  setvbuf(stdout, NULL, _IOLBF, 4096);
  setvbuf(stderr, NULL, _IOLBF, 4096);
}

#else  // defined(__GLIBC__)
// Use IRT structure overwriting for newlib.
extern "C" struct nacl_irt_filename __libnacl_irt_dev_filename;
extern "C" struct nacl_irt_fdio __libnacl_irt_fdio;
__attribute__((constructor)) static void wrap() {
  __libnacl_irt_dev_filename.open = __wrap_open;
  __libnacl_irt_dev_filename.stat = __wrap_stat;

  __libnacl_irt_fdio.close = __wrap_close;
  //__libnacl_irt_fdio.dup = __wrap_dup;
  //__libnacl_irt_fdio.dup2 = __wrap_dup2;
  __libnacl_irt_fdio.read = __wrap_read;
  __nacl_irt_write_real = __libnacl_irt_fdio.write;
  __libnacl_irt_fdio.write = __wrap_write;
  __libnacl_irt_fdio.seek = __wrap_seek;
  __libnacl_irt_fdio.fstat = __wrap_fstat;
  //__libnacl_irt_fdio.getdents = __wrap_getdents;
}
#endif  // defined(__GLIBC__)

void write_to_real_stderr(const void* buf, size_t count) {
  size_t nwrote;
  __nacl_irt_write_real(2, buf, count, &nwrote);
}
