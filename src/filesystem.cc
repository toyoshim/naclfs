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

#include <sstream>
#include <string.h>

#include "naclfs.h"
#include "port_filesystem.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/core.h"
#include "ppapi/cpp/module.h"

namespace naclfs {

static const char kPortFileSystemPrefix[] = "/dev/std";
static size_t kPortFileSystemPrefixSize = sizeof(kPortFileSystemPrefix) - 1;

FileSystem::FileSystem(NaClFs* naclfs) : naclfs_(naclfs) {
  pthread_mutex_init(&call_mutex_, NULL);
  pthread_mutex_init(&proxy_sync_mutex_, NULL);
  pthread_mutex_lock(&proxy_sync_mutex_);
  pthread_mutex_init(&block_mutex_, NULL);
  pthread_mutex_lock(&block_mutex_);
  core_ = pp::Module::Get()->core();
}

FileSystem::~FileSystem() {
  pthread_mutex_destroy(&call_mutex_);
  pthread_mutex_destroy(&proxy_sync_mutex_);
  pthread_mutex_destroy(&block_mutex_);
}

int FileSystem::Open(const char* path, int oflag, ...) {
  Arguments arguments;
  arguments.delegate = CreateDelegate(path);
  if (!arguments.delegate) {
    naclfs_->Log("FileSystem: can not create delegate\n");
    return -1;
  }
  arguments.function = OPEN;
  arguments.u.open.path = path;
  arguments.u.open.oflag = oflag;
  Call(arguments);
  if (arguments.result.open < 0) {
    delete arguments.delegate;
    return arguments.result.open;
  }
  return BindToDescriptor(arguments.delegate);
}

ssize_t FileSystem::Read(int fildes, void* buf, size_t nbytes) {
  Arguments arguments;
  arguments.delegate = GetDelegate(fildes);
  arguments.function = READ;
  arguments.u.read.buf = buf;
  arguments.u.read.nbytes = nbytes;
  for (;;) {
    Call(arguments);
    if (0 != arguments.result.read)
      break;
    pthread_mutex_lock(&block_mutex_);
  }
  return arguments.result.read;
}

ssize_t FileSystem::Write(int fildes, const void* buf, size_t nbytes) {
  Arguments arguments;
  arguments.delegate = GetDelegate(fildes);
  arguments.function = WRITE;
  arguments.u.write.buf = buf;
  arguments.u.write.nbytes = nbytes;
  Call(arguments);
  return arguments.result.write;
}

off_t FileSystem::Lseek(int fildes, off_t offset, int whence) {
  Arguments arguments;
  arguments.delegate = GetDelegate(fildes);
  arguments.function = LSEEK;
  arguments.u.lseek.offset = offset;
  arguments.u.lseek.whence = whence;
  Call(arguments);
  return arguments.result.lseek;
}

int FileSystem::Close(int fildes) {
  Arguments arguments;
  arguments.delegate = GetDelegate(fildes);
  arguments.function = CLOSE;
  Call(arguments);
  if (arguments.result.close != 0)
    return arguments.result.close;
  DeleteDescriptor(fildes);
  return 0;
}

bool FileSystem::HandleMessage(const pp::Var& message) {
  bool result = PortFileSystem::HandleMessage(message);
  pthread_mutex_unlock(&NaClFs::GetFileSystem()->block_mutex_);
  return result;
}

FileSystem::Delegate* FileSystem::CreateDelegate(const char* path) {
  if (strncmp(path, kPortFileSystemPrefix, kPortFileSystemPrefixSize) == 0)
    return new PortFileSystem(naclfs_);
  naclfs_->Log("FileSystem: no matching delegate\n");
  return NULL;
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

void FileSystem::Call(Arguments& arguments) {
  pthread_mutex_lock(&call_mutex_);
  if (core_->IsMainThread())
    Switch(&arguments);
  else
    SwitchViaProxy(arguments);
  pthread_mutex_unlock(&call_mutex_);
}

void FileSystem::SwitchViaProxy(Arguments& arguments) {
  arguments.self = this;
  core_->CallOnMainThread(0, pp::CompletionCallback(Proxy, &arguments));
  pthread_mutex_lock(&proxy_sync_mutex_);
}

void FileSystem::Proxy(void* param, int32_t result) {
  Arguments* arguments = static_cast<Arguments*>(param);
  FileSystem* self = arguments->self;
  self->Switch(arguments);
  pthread_mutex_unlock(&self->proxy_sync_mutex_);
}

void FileSystem::Switch(Arguments* arguments) {
  switch (arguments->function) {
    case OPEN:
      arguments->result.open =
          arguments->delegate->Open(arguments->u.open.path,
                                    arguments->u.open.oflag);
      break;
    case READ:
      arguments->result.read =
          arguments->delegate->Read(arguments->u.read.buf,
                                    arguments->u.read.nbytes);
      break;
    case WRITE:
      arguments->result.write =
          arguments->delegate->Write(arguments->u.write.buf,
                                     arguments->u.write.nbytes);
      break;
    case LSEEK:
      arguments->result.lseek =
          arguments->delegate->Lseek(arguments->u.lseek.offset,
                                     arguments->u.lseek.whence);
      break;
    case CLOSE:
      arguments->result.close =
          arguments->delegate->Close();
      break;
  }
}

}  // namespace naclfs

