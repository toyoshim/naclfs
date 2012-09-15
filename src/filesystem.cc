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

#include "filesystem.h"

#include <stdarg.h>
#include <string.h>

#include <sstream>
#include <vector>

#include "html5_filesystem.h"
#include "naclfs.h"
#include "port_filesystem.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/core.h"
#include "ppapi/cpp/module.h"

namespace naclfs {

static const char kPortFileSystemPrefix[] = "/dev/std";
static size_t kPortFileSystemPrefixSize = sizeof(kPortFileSystemPrefix) - 1;

bool FileSystem::Delegate::initialized_ = false;
pthread_mutex_t FileSystem::Delegate::mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t FileSystem::Delegate::cond_ = PTHREAD_COND_INITIALIZER;
pp::Core* FileSystem::Delegate::core_;

FileSystem::Delegate::Delegate() {
  if (initialized_)
    return;
  initialized_ = true;
  core_ = pp::Module::Get()->core();
}

int FileSystem::Delegate::Open(const char* path,
                               int oflag,
                               mode_t cmode) {
  if (core_->IsMainThread())
    return EIO;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = OPEN;
  arguments.u.open.path = path;
  arguments.u.open.oflag = oflag;
  arguments.u.open.cmode = cmode;
  Call(arguments);
  return arguments.result.open;
}

int FileSystem::Delegate::Stat(const char* path, struct stat* buf) {
  if (core_->IsMainThread())
    return EIO;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = STAT;
  arguments.u.stat.path = path;
  arguments.u.stat.buf = buf;
  Call(arguments);
  return arguments.result.stat;
}

int FileSystem::Delegate::Close() {
  if (core_->IsMainThread())
    return EIO;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = CLOSE;
  Call(arguments);
  return arguments.result.close;
}

int FileSystem::Delegate::Fstat(struct stat* buf) {
  if (core_->IsMainThread())
    return EIO;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = FSTAT;
  arguments.u.fstat.buf = buf;
  Call(arguments);
  return arguments.result.fstat;
}

ssize_t FileSystem::Delegate::Read(void* buf, size_t nbytes) {
  if (core_->IsMainThread())
    return -1;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = READ;
  arguments.u.read.buf = buf;
  arguments.u.read.nbytes = nbytes;
  Call(arguments);
  return arguments.result.read;
}

ssize_t FileSystem::Delegate::Write(const void* buf, size_t nbytes) {
  if (core_->IsMainThread())
    return -1;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = WRITE;
  arguments.u.write.buf = buf;
  arguments.u.write.nbytes = nbytes;
  Call(arguments);
  return arguments.result.write;
}

off_t FileSystem::Delegate::Seek(off_t offset, int whence) {
  if (core_->IsMainThread())
    return -1;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = SEEK;
  arguments.u.seek.offset = offset;
  arguments.u.seek.whence = whence;
  Call(arguments);
  return arguments.result.seek;
}

int FileSystem::Delegate::Fcntl(int cmd, va_list* ap) {
  if (core_->IsMainThread())
    return EIO;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = FCNTL;
  arguments.u.fcntl.cmd = cmd;
  arguments.u.fcntl.ap = ap;
  Call(arguments);
  return arguments.result.fcntl;
}

int FileSystem::Delegate::MkDir(const char* path, mode_t mode) {
  if (core_->IsMainThread())
    return EIO;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = MKDIR;
  arguments.u.mkdir.path = path;
  arguments.u.mkdir.mode = mode;
  Call(arguments);
  return arguments.result.mkdir;
}

DIR* FileSystem::Delegate::OpenDir(const char* dirname) {
  if (core_->IsMainThread())
    return NULL;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = OPENDIR;
  arguments.u.opendir.dirname = dirname;
  Call(arguments);
  return arguments.result.opendir;
}

void FileSystem::Delegate::RewindDir(DIR* dirp) {
  if (core_->IsMainThread())
    return;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = REWINDDIR;
  arguments.u.rewinddir.dirp = dirp;
  Call(arguments);
}

struct dirent* FileSystem::Delegate::ReadDir(DIR* dirp) {
  if (core_->IsMainThread())
    return NULL;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = READDIR;
  arguments.u.readdir.dirp = dirp;
  Call(arguments);
  return arguments.result.readdir;
}

int FileSystem::Delegate::CloseDir(DIR* dirp) {
  if (core_->IsMainThread())
    return EIO;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = CLOSEDIR;
  arguments.u.closedir.dirp = dirp;
  Call(arguments);
  return arguments.result.closedir;
}

void FileSystem::Delegate::Wait() {
  pthread_mutex_lock(&mutex_);
  pthread_cond_wait(&cond_, &mutex_);
  pthread_mutex_unlock(&mutex_);
}

void FileSystem::Delegate::Signal() {
  pthread_mutex_lock(&mutex_);
  pthread_cond_signal(&cond_);
  pthread_mutex_unlock(&mutex_);
}

void FileSystem::Delegate::Call(Arguments& arguments) {
  callback_ = pp::CompletionCallback(Proxy, &arguments);
  core_->CallOnMainThread(0, pp::CompletionCallback(Proxy, &arguments));
  Wait();
}

void FileSystem::Delegate::Proxy(void* param, int32_t result) {
  Arguments* arguments = static_cast<Arguments*>(param);
  arguments->result.callback = result;
  arguments->chaining = false;
  arguments->delegate->Switch(arguments);
  if (!arguments->chaining)
    arguments->delegate->Signal();
}

void FileSystem::Delegate::Switch(Arguments* arguments) {
  switch (arguments->function) {
    case OPEN:
      arguments->result.open =
          arguments->delegate->OpenCall(arguments,
                                        arguments->u.open.path,
                                        arguments->u.open.oflag,
                                        arguments->u.open.cmode);
      break;
    case STAT:
      arguments->result.stat =
          arguments->delegate->StatCall(arguments,
                                        arguments->u.stat.path,
                                        arguments->u.stat.buf);
      break;
    case CLOSE:
      arguments->result.close =
          arguments->delegate->CloseCall(arguments);
      break;
    case FSTAT:
      arguments->result.fstat =
          arguments->delegate->FstatCall(arguments,
                                         arguments->u.fstat.buf);
      break;
    case READ:
      arguments->result.read =
          arguments->delegate->ReadCall(arguments,
                                        arguments->u.read.buf,
                                        arguments->u.read.nbytes);
      break;
    case WRITE:
      arguments->result.write =
          arguments->delegate->WriteCall(arguments,
                                         arguments->u.write.buf,
                                         arguments->u.write.nbytes);
      break;
    case SEEK:
      arguments->result.seek =
          arguments->delegate->SeekCall(arguments,
                                        arguments->u.seek.offset,
                                        arguments->u.seek.whence);
      break;
    case FCNTL:
      arguments->result.fcntl =
          arguments->delegate->FcntlCall(arguments,
                                         arguments->u.fcntl.cmd,
                                         arguments->u.fcntl.ap);
      break;
    case MKDIR:
      arguments->result.mkdir =
          arguments->delegate->MkDirCall(arguments,
                                         arguments->u.mkdir.path,
                                         arguments->u.mkdir.mode);
      break;
    case OPENDIR:
      arguments->result.opendir =
          arguments->delegate->OpenDirCall(arguments,
                                           arguments->u.opendir.dirname);
      break;
    case REWINDDIR:
      arguments->delegate->RewindDirCall(arguments,
                                         arguments->u.rewinddir.dirp);
      break;
    case READDIR:
      arguments->result.readdir =
          arguments->delegate->ReadDirCall(arguments,
                                           arguments->u.readdir.dirp);
      break;
    case CLOSEDIR:
      arguments->result.closedir =
          arguments->delegate->CloseDirCall(arguments,
                                            arguments->u.closedir.dirp);
      break;
  }
}

FileSystem::FileSystem(NaClFs* naclfs) : naclfs_(naclfs) {
  pthread_mutex_init(&mutex_, NULL);
  pthread_mutex_lock(&mutex_);
  core_ = pp::Module::Get()->core();
  // Current path which doesn't contain the first slash.
  // E.g., "" means "/".
  cwd_ = "";
}

FileSystem::~FileSystem() {
  pthread_mutex_destroy(&mutex_);
}

int FileSystem::Open(const char* path, int oflag, mode_t cmode, int* newfd) {
  if (!path)
    return EFAULT;
  std::string fullpath;
  CreateFullpath(path, &fullpath);
  Delegate* delegate = CreateDelegate(fullpath.c_str());
  if (!delegate) {
    naclfs_->Log("FileSystem: can not create delegate\n");
    return ENODEV;
  }
  int result = delegate->Open(fullpath.c_str(), oflag, cmode);
  if (result) {
    delete delegate;
    return result;
  }
  *newfd = BindToDescriptor(delegate);
  return 0;
}

int FileSystem::Stat(const char* path, struct stat* buf) {
  std::string fullpath;
  CreateFullpath(path, &fullpath);
  Delegate* delegate = CreateDelegate(fullpath.c_str());
  if (!delegate) {
    naclfs_->Log("FileSystem: can not create delegate\n");
    return EBADF;
  }
  int result = delegate->Stat(fullpath.c_str(), buf);
  delete delegate;
  return result;
}

int FileSystem::Close(int fildes) {
  Delegate* delegate = GetDelegate(fildes);
  if (!delegate)
    return EBADF;
  int result = delegate->Close();
  if (result != 0)
    return result;
  DeleteDescriptor(fildes);
  return result;
}

int FileSystem::Fstat(int fildes, struct stat* buf) {
  Delegate* delegate = GetDelegate(fildes);
  if (!delegate)
    return EBADF;
  return delegate->Fstat(buf);
}

ssize_t FileSystem::Read(int fildes, void* buf, size_t nbytes) {
  Delegate* delegate = GetDelegate(fildes);
  if (!delegate)
    return -1;
  for (;;) {
    // TODO: Move lock related code into PortFileSystem.
    // Shoud remove magic number -2 for blocking.
    ssize_t result = delegate->Read(buf, nbytes);
    if (result != -2)
      return result;
    pthread_mutex_lock(&mutex_);
  }
  return 0;
}

ssize_t FileSystem::Write(int fildes, const void* buf, size_t nbytes) {
  Delegate* delegate = GetDelegate(fildes);
  if (!delegate)
    return -1;
  return delegate->Write(buf, nbytes);
}

off_t FileSystem::Seek(int fildes, off_t offset, int whence) {
  Delegate* delegate = GetDelegate(fildes);
  if (!delegate)
    return -1;
  return delegate->Seek(offset, whence);
}

int FileSystem::Fcntl(int fildes, int cmd, va_list* ap) {
  Delegate* delegate = GetDelegate(fildes);
  if (!delegate)
    return EBADF;
  return delegate->Fcntl(cmd, ap);
}

int FileSystem::MkDir(const char* path, mode_t mode) {
  if (!path)
    return -1;
  std::string fullpath;
  CreateFullpath(path, &fullpath);
  Delegate* delegate = CreateDelegate(fullpath.c_str());
  if (!delegate) {
    naclfs_->Log("FileSystem: can not create delegate\n");
    return -1;
  }
  int result = delegate->MkDir(fullpath.c_str(), mode);
  delete delegate;
  return result;
}

DIR* FileSystem::OpenDir(const char* dirname) {
  if (!dirname)
    return NULL;
  std::string fullpath;
  CreateFullpath(dirname, &fullpath);
  Delegate* delegate = CreateDelegate(fullpath.c_str());
  if (!delegate) {
    naclfs_->Log("FileSystem: can not create delegate\n");
    return NULL;
  }
  DIR* result = delegate->OpenDir(fullpath.c_str());
  if (!result)
    delete delegate;
  return result;
}

void FileSystem::RewindDir(DIR* dirp) {
  Delegate* delegate = reinterpret_cast<Dir*>(dirp)->delegate();
  if (!delegate)
    return;
  delegate->RewindDir(dirp);
}

struct dirent* FileSystem::ReadDir(DIR* dirp) {
  Delegate* delegate = reinterpret_cast<Dir*>(dirp)->delegate();
  if (!delegate)
    return NULL;
  return delegate->ReadDir(dirp);
}

int FileSystem::CloseDir(DIR* dirp) {
  Delegate* delegate = reinterpret_cast<Dir*>(dirp)->delegate();
  if (!delegate)
    return -1;
  int result = delegate->CloseDir(dirp);
  if (!result)
    delete delegate;
  return result;
}

int FileSystem::ChDir(const char* path) {
  struct stat buf;
  int result = Stat(path, &buf);
  if (result)
    return result;
  if (!S_ISDIR(buf.st_mode)) {
    errno = ENOTDIR;
    return -1;
  }
  std::string fullpath;
  CreateFullpath(path, &fullpath);
  cwd_ = fullpath.substr(1);
  return 0;
}

char* FileSystem::GetCwd(char* buf, size_t size) {
  if (!buf || !size) {
    errno = EINVAL;
    return NULL;
  }
  size_t cwd_size = cwd_.size() + 2;
  if (size < cwd_size) {
    errno = ERANGE;
    return NULL;
  }
  buf[0] = '/';
  strcpy(&buf[1], cwd_.c_str());
  return buf;
}

bool FileSystem::HandleMessage(const pp::Var& message) {
  std::stringstream ss;
  ss << "HandleMessage: " << message.AsString().c_str();
  NaClFs::Log(ss.str().c_str());
  bool result = PortFileSystem::HandleMessage(message);
  if (result)
    pthread_mutex_unlock(&NaClFs::GetFileSystem()->mutex_);
  else
    result = Html5FileSystem::HandleMessage(message);
  return result;
}

void FileSystem::CreateFullpath(const char* path, std::string* fullpath) {
  // Insert current path to a relative path.
  std::vector<std::string> paths;
  if (*path != '/' && cwd_.size()) {
    size_t offset = 0;
    for (;;) {
      size_t next_offset = cwd_.find("/", offset);
      if (next_offset == std::string::npos) {
        paths.push_back(cwd_.substr(offset));
        break;
      }
      paths.push_back(cwd_.substr(offset, next_offset - offset));
      offset = next_offset + 1;
    }
  }

  // Split path into a string vector.
  for (const char* caret = path; *caret != 0; ) {
    const char* delim = index(caret, '/');
    if (NULL == delim) {
      paths.push_back(std::string(caret));
      break;
    } else {
      int size = delim - caret;
      if (size != 0 && !(size == 1 && *caret == '.'))  // Trim '' and '.'.
        paths.push_back(std::string(caret, size));
      caret = delim + 1;
    }
  }

  // Normalize '..'.
  for (std::vector<std::string>::iterator iter = paths.begin();
      iter != paths.end();
      ++iter) {
    if (iter->compare(".."))
      continue;
    if (iter != paths.begin())
      paths.erase(iter--);
    paths.erase(iter--);
  }

  // Join paths.
  if (!paths.size())
    *fullpath = "/";
  else for (std::vector<std::string>::iterator iter = paths.begin();
      iter != paths.end();
      ++iter) {
    fullpath->append("/");
    fullpath->append(*iter);
  }

  if (naclfs::NaClFs::trace()) {
    std::stringstream ss;
    ss << "CreateFullpath from '" << path << "' with cwd '/" << cwd_.c_str()
       << "' -> '" << fullpath->c_str() << "'" << std::endl;
    naclfs_->Log(ss.str().c_str());
  }
}

FileSystem::Delegate* FileSystem::CreateDelegate(const char* path) {
  if (strncmp(path, kPortFileSystemPrefix, kPortFileSystemPrefixSize) == 0)
    return new PortFileSystem(naclfs_);
  return new Html5FileSystem(naclfs_);
}

int FileSystem::BindToDescriptor(Delegate* delegate) {
  descriptors_.push_back(delegate);
  return descriptors_.size() - 1;
}

FileSystem::Delegate* FileSystem::GetDelegate(int fildes) {
  if (fildes < 0 || descriptors_.size() <= (uint)fildes)
    return NULL;
  return descriptors_[fildes];
}

void FileSystem::DeleteDescriptor(int fildes) {
  Delegate* delegate = GetDelegate(fildes);
  if (NULL == delegate)
    return;
  delete delegate;
  descriptors_[fildes] = NULL;
}

}  // namespace naclfs
