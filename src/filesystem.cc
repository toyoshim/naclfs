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
#include <sstream>
#include <string.h>

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
sem_t FileSystem::Delegate::sem_;
pp::Core* FileSystem::Delegate::core_;

FileSystem::Delegate::Delegate() {
  if (initialized_)
    return;
  initialized_ = true;
  sem_init(&sem_, 0, 0);
  core_ = pp::Module::Get()->core();
}

int FileSystem::Delegate::Open(const char* path, int oflag, ...) {
  if (core_->IsMainThread())
    return -1;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = OPEN;
  arguments.u.open.path = path;
  arguments.u.open.oflag = oflag;
  Call(arguments);
  return arguments.result.open;
}

int FileSystem::Delegate::Stat(const char* path, struct stat* buf) {
  if (core_->IsMainThread())
    return -1;
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
    return -1;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = CLOSE;
  Call(arguments);
  return arguments.result.close;
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

int FileSystem::Delegate::Fcntl(int cmd, ...) {
  va_list ap;
  va_start(ap, cmd);
  int arg1 = va_arg(ap, int);
  va_end(ap);
  if (core_->IsMainThread())
    return -1;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = FCNTL;
  arguments.u.fcntl.cmd = cmd;
  arguments.u.fcntl.arg1 = arg1;
  Call(arguments);
  return arguments.result.fcntl;
}

int FileSystem::Delegate::MkDir(const char* path, mode_t mode) {
  if (core_->IsMainThread())
    return NULL;
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
    return -1;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = CLOSEDIR;
  arguments.u.closedir.dirp = dirp;
  Call(arguments);
  return arguments.result.closedir;
}

void FileSystem::Delegate::Lock() {
  sem_wait(&sem_);
}

void FileSystem::Delegate::Unlock() {
  sem_post(&sem_);
}

void FileSystem::Delegate::Call(Arguments& arguments) {
  callback_ = pp::CompletionCallback(Proxy, &arguments);
  core_->CallOnMainThread(0, pp::CompletionCallback(Proxy, &arguments));
  Lock();
}

void FileSystem::Delegate::Proxy(void* param, int32_t result) {
  Arguments* arguments = static_cast<Arguments*>(param);
  arguments->result.callback = result;
  arguments->chaining = false;
  arguments->delegate->Switch(arguments);
  if (!arguments->chaining)
    arguments->delegate->Unlock();
}

void FileSystem::Delegate::Switch(Arguments* arguments) {
  switch (arguments->function) {
    case OPEN:
      arguments->result.open =
          arguments->delegate->OpenCall(arguments,
                                        arguments->u.open.path,
                                        arguments->u.open.oflag);
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
                                         arguments->u.fcntl.arg1);
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
  cwd_ = "/";
}

FileSystem::~FileSystem() {
  pthread_mutex_destroy(&mutex_);
}

int FileSystem::Open(const char* path, int oflag, ...) {
  if (!path)
    return -1;
  std::string fullpath;
  CreateFullpath(path, &fullpath);
  Delegate* delegate = CreateDelegate(fullpath.c_str());
  if (!delegate) {
    naclfs_->Log("FileSystem: can not create delegate\n");
    return -1;
  }
  int result = delegate->Open(fullpath.c_str(), oflag);
  if (result < 0) {
    delete delegate;
    return result;
  }
  return BindToDescriptor(delegate);
}

int FileSystem::Stat(const char* path, struct stat* buf) {
  if (!path)
    return -1;
  std::string fullpath;
  CreateFullpath(path, &fullpath);
  Delegate* delegate = CreateDelegate(fullpath.c_str());
  if (!delegate) {
    naclfs_->Log("FileSystem: can not create delegate\n");
    return -1;
  }
  int result = delegate->Stat(fullpath.c_str(), buf);
  delete delegate;
  return result;
}

int FileSystem::Close(int fildes) {
  Delegate* delegate = GetDelegate(fildes);
  if (!delegate)
    return -1;
  int result = delegate->Close();
  if (result != 0)
    return result;
  DeleteDescriptor(fildes);
  return result;
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

int FileSystem::Fcntl(int fildes, int cmd, ...) {
  va_list ap;
  va_start(ap, cmd);
  int arg1 = va_arg(ap, int);
  va_end(ap);
  Delegate* delegate = GetDelegate(fildes);
  if (!delegate)
    return -1;
  return delegate->Fcntl(cmd, arg1);
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
  // TODO: Handle directories.
  if (path[0] == '/') {
    *fullpath = path;
  } else {
    *fullpath = cwd_;
    *fullpath += path;
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
