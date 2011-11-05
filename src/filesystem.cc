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
pthread_mutex_t FileSystem::Delegate::mutex_;
pp::Core* FileSystem::Delegate::core_;

FileSystem::Delegate::Delegate() {
  if (initialized_)
    return;
  initialized_ = true;
  pthread_mutex_init(&mutex_, NULL);
  Lock();
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

off_t FileSystem::Delegate::Lseek(off_t offset, int whence) {
  if (core_->IsMainThread())
    return -1;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = LSEEK;
  arguments.u.lseek.offset = offset;
  arguments.u.lseek.whence = whence;
  Call(arguments);
  return arguments.result.lseek;
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

int FileSystem::Delegate::Close() {
  if (core_->IsMainThread())
    return -1;
  Arguments arguments;
  arguments.delegate = this;
  arguments.function = CLOSE;
  Call(arguments);
  return arguments.result.close;
}

void FileSystem::Delegate::Lock() {
  pthread_mutex_lock(&mutex_);
}

void FileSystem::Delegate::Unlock() {
  pthread_mutex_unlock(&mutex_);
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
    case LSEEK:
      arguments->result.lseek =
          arguments->delegate->LseekCall(arguments,
                                         arguments->u.lseek.offset,
                                         arguments->u.lseek.whence);
      break;
    case FCNTL:
      arguments->result.fcntl =
          arguments->delegate->FcntlCall(arguments,
                                         arguments->u.fcntl.cmd,
                                         arguments->u.fcntl.arg1);
      break;
    case CLOSE:
      arguments->result.close =
          arguments->delegate->CloseCall(arguments);
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
  if (path[0] == '/') {
    fullpath = path;
  } else {
    fullpath = cwd_;
    fullpath += path;
  }
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

off_t FileSystem::Lseek(int fildes, off_t offset, int whence) {
  Delegate* delegate = GetDelegate(fildes);
  if (!delegate)
    return -1;
  return delegate->Lseek(offset, whence);
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

bool FileSystem::HandleMessage(const pp::Var& message) {
  bool result = PortFileSystem::HandleMessage(message);
  pthread_mutex_unlock(&NaClFs::GetFileSystem()->mutex_);
  return result;
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

