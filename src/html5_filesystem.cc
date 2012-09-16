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

#include "html5_filesystem.h"

#include <fcntl.h>
#include <sstream>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "naclfs.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_file_io.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/dev/directory_entry_dev.h"
#include "ppapi/cpp/dev/directory_reader_dev.h"
#include "ppapi/cpp/file_io.h"
#include "ppapi/cpp/file_ref.h"
#include "ppapi/cpp/file_system.h"
#include "ppapi/cpp/var.h"

namespace {

const int kOffCmd = 2;
const int kOffData = 3;
const char* const kRpcId = "X5";
const char* const kCmdStat = "S";
const char* const kCmdDir = "D";

int PPErrorToErrNo(int pp_error) {
  switch (pp_error) {
   case PP_ERROR_NOACCESS:
    return EACCES;
   case PP_ERROR_NOMEMORY:
    return ENOMEM;
   case PP_ERROR_NOSPACE:
    return ENOSPC;
   case PP_ERROR_NOQUOTA:
    return ENOSPC;
   case PP_ERROR_FILENOTFOUND:
    return ENOENT;
   case PP_ERROR_FILEEXISTS:
    return EEXIST;
   case PP_ERROR_FILETOOBIG:
    return EFBIG;
   default: {
    std::stringstream ss;
    ss << "Html5FileSystem internal errno conversion failed for "
       << pp_error << std::endl;
    naclfs::NaClFs::Log(ss.str().c_str());
    return ENOSYS; }
  }
}

class Html5FileSystemDir : public naclfs::FileSystem::Dir {
 public:
  Html5FileSystemDir(naclfs::Html5FileSystem* owner,
                     const pp::FileRef& directory_ref)
    : Dir(owner),
      reader_(new pp::DirectoryReader_Dev(directory_ref)),
      entries_(NULL),
      offset_(0) {}
  virtual ~Html5FileSystemDir() {
    delete reader_;
    if (entries_)
      delete entries_;
  }

  void SetEntries(std::vector<std::string>* entries) {
    entries_ = entries;
    offset_ = 0;
  }

  int32_t ReadDir(const pp::CompletionCallback& cc) {
    if (entries_)
      return PP_OK;
    return reader_->GetNextEntry(&entry_, cc);
  }

  struct dirent* ReadDirComplete() {
    memset(&dirent_, 0, sizeof(struct dirent));
    std::string name;
    if (entries_) {
      if (entries_->size() == offset_)
        return NULL;
      name = (*entries_)[offset_++].substr(1);
    } else {
      name = entry_.file_ref().GetName().AsString();
    }
    strncpy(dirent_.d_name, name.c_str(), 256);
    if (name.length() > 255)
      dirent_.d_name[255] = 0;
    return &dirent_;
  }

  void Rewind() {
    offset_ = 0;
  }

 private:
  pp::DirectoryReader_Dev* reader_;
  pp::DirectoryEntry_Dev entry_;
  struct dirent dirent_;
  std::vector<std::string>* entries_;
  size_t offset_;
};

};  // namespace

namespace naclfs {

pp::FileSystem* Html5FileSystem::filesystem_ = NULL;
// TODO: Make JavaScript RPC thread safe. We should have a map for objects
// and requests.
Html5FileSystem* Html5FileSystem::rpc_object_ = NULL;

Html5FileSystem::Html5FileSystem(NaClFs* naclfs)
    : file_ref_(NULL),
      file_io_(NULL),
      naclfs_(naclfs),
      waiting_(false),
      querying_(false),
      remoting_(false),
      offset_(0) {
}

Html5FileSystem::~Html5FileSystem() {
  // TODO: Fix memory leaks on file_io_ and file_ref_.
  delete file_io_;
  delete file_ref_;
}

// TODO: Handle cmode argument.
int Html5FileSystem::OpenCall(Arguments* arguments,
                              const char* path,
                              int oflag,
                              mode_t cmode) {
  if (!filesystem_)
    return Initialize(arguments);

  if (waiting_) {
    // Check FileIO::Open completion result.
    waiting_ = false;
    if (arguments->result.callback != 0) {
      std::stringstream ss;
      ss << "Html5FileSystem::Open failed internal FileIO::Open completion "
         << "with " << arguments->result.callback
         << " for " << path << std::endl;
      naclfs_->Log(ss.str().c_str());
      return PPErrorToErrNo(arguments->result.callback);
    }
    querying_ = true;
    // TODO: Check return value.
    file_io_->Query(&file_info_, callback_);
    arguments->chaining = true;
    return 0;
  }

  if (querying_) {
    // Check FileIO::Query completion result.
    querying_ = false;
    if (arguments->result.callback != 0) {
      std::stringstream ss;
      ss << "Html5FileSystem::Open failed internal FileIO::Query completion "
         << "with " << arguments->result.callback
         << " for " << path << std::endl;
      naclfs_->Log(ss.str().c_str());
      return PPErrorToErrNo(arguments->result.callback);
    }
    if (oflag & O_APPEND)
      offset_ = file_info_.size;
    return 0;
  }

  file_ref_ = new pp::FileRef(*filesystem_, path);
  file_io_ = new pp::FileIO(naclfs_->GetInstance());
  int32_t flags = 0;
  switch (oflag & O_ACCMODE) {
    case O_RDONLY:
      flags = PP_FILEOPENFLAG_READ;
      oflag &= ~O_ACCMODE;
      break;
    case O_WRONLY:
      flags = PP_FILEOPENFLAG_WRITE;
      oflag &= ~O_ACCMODE;
      break;
    case O_RDWR:
      flags = PP_FILEOPENFLAG_READ | PP_FILEOPENFLAG_WRITE;
      oflag &= ~O_ACCMODE;
      break;
  }
  if (oflag & O_CREAT)
    flags |= PP_FILEOPENFLAG_CREATE;
  if (oflag & O_TRUNC)
    flags |= PP_FILEOPENFLAG_TRUNCATE;
  if (oflag & O_EXCL)
    flags |= PP_FILEOPENFLAG_EXCLUSIVE;
  oflag &= ~(O_CREAT | O_TRUNC | O_EXCL | O_APPEND);
  if (oflag) {
    std::stringstream ss;
    ss << "Html5FileSystem::Open ignore unsupporting oflag: " << oflag <<
        std::endl;
    naclfs_->Log(ss.str().c_str());
  }
  int32_t result = file_io_->Open(*file_ref_, flags, callback_);
  if (result != PP_OK_COMPLETIONPENDING) {
    std::stringstream ss;
    ss << "Html5FileSystem::Open failed internal FileIO::Open with " << result
       << " for " << path << std::endl;
    naclfs_->Log(ss.str().c_str());
    return PPErrorToErrNo(arguments->result.callback);
  }
  waiting_ = true;
  arguments->chaining = true;
  return 0;
}

int Html5FileSystem::StatCall(Arguments* arguments,
                              const char* path,
                              struct stat* buf) {
  if (!filesystem_)
    return Initialize(arguments);

  if (waiting_) {
    waiting_ = false;
    if (arguments->result.callback) {
      std::stringstream ss;
      ss << "Html5FileSystem::StatCall got call back failure result "
         << arguments->result.callback << std::endl;
      naclfs_->Log(ss.str().c_str());
    }
    if (arguments->result.callback == PP_ERROR_FAILED) {
      // TODO: Queries on directories seem to fail with PP_ERROR_FAILED.
      // See, http://crbug.com/132201
      naclfs_->Log(" ... apply workaround for http://crbug.com/132201\n");
      std::stringstream ss;
      ss << kRpcId << kCmdStat << path;
      // As a workaround, request JavaScript to proxy the query.
      rpc_object_ = this;
      remoting_ = true;
      arguments->chaining = true;
      naclfs_->PostMessage(pp::Var(ss.str()));
      return 0;
    }

    if (arguments->result.callback)
      return PPErrorToErrNo(arguments->result.callback);
    querying_ = true;
    arguments->chaining = true;
    file_io_->Query(&file_info_, callback_);
    return 0;
  }

  if (remoting_) {
    remoting_ = false;
    rpc_object_ = NULL;
    memset(buf, 0, sizeof(struct stat));
    if (!arguments->result.callback)
      buf->st_mode = S_IFDIR |
                     S_IRUSR | S_IWUSR | S_IXUSR |
                     S_IRGRP | S_IWGRP | S_IXGRP |
                     S_IROTH | S_IWOTH | S_IXOTH;
    return arguments->result.callback;
  }

  if (querying_) {
    querying_ = false;
    if (arguments->result.callback)
      return PPErrorToErrNo(arguments->result.callback);
    memset(buf, 0, sizeof(struct stat));
    buf->st_mode = S_IRUSR | S_IWUSR | S_IXUSR |
                   S_IRGRP | S_IWGRP | S_IXGRP |
                   S_IROTH | S_IWOTH | S_IXOTH;
    buf->st_size = file_info_.size;
    buf->st_atime = file_info_.last_access_time;
    buf->st_mtime = file_info_.last_modified_time;
    buf->st_ctime = file_info_.creation_time;
    buf->st_blksize = 512;  // pseudo value for compatilibity
    buf->st_blocks = (file_info_.size + 511) >> 9;
    switch (file_info_.type) {
      case PP_FILETYPE_REGULAR:
        buf->st_mode |= S_IFREG;
        break;
      case PP_FILETYPE_DIRECTORY:
        buf->st_mode |= S_IFDIR;
        break;
      case PP_FILETYPE_OTHER:
      default:
        break;
    }
    return 0;
  }

  file_ref_ = new pp::FileRef(*filesystem_, path);
  file_io_ = new pp::FileIO(naclfs_->GetInstance());
  int32_t result = file_io_->Open(*file_ref_, 0, callback_);
  if (result != PP_OK_COMPLETIONPENDING) {
    naclfs_->Log(
        "Html5FileSystem::Open doesn't return PP_OK_COMPLETIONPENDING\n");
    return PPErrorToErrNo(result);
  }
  waiting_ = true;
  arguments->chaining = true;
  return 0;
}

int Html5FileSystem::CloseCall(Arguments* arguments) {
  file_io_->Close();
  return 0;
}

int Html5FileSystem::FstatCall(Arguments* arguments, struct stat* buf) {
  if (waiting_) {
    waiting_ = false;
    if (arguments->result.callback)
      PPErrorToErrNo(arguments->result.callback);
    memset(buf, 0, sizeof(struct stat));
    buf->st_mode = S_IRUSR | S_IWUSR | S_IXUSR;
    buf->st_size = file_info_.size;
    buf->st_atime = file_info_.last_access_time;
    buf->st_mtime = file_info_.last_modified_time;
    buf->st_ctime = file_info_.creation_time;
    buf->st_blksize = 512;  // pseudo value for compatilibity
    buf->st_blocks = (file_info_.size + 511) >> 9;
    switch (file_info_.type) {
      case PP_FILETYPE_REGULAR:
        buf->st_mode |= S_IFREG;
        break;
      case PP_FILETYPE_DIRECTORY:
        buf->st_mode |= S_IFDIR;
        break;
      case PP_FILETYPE_OTHER:
      default:
        break;
    }
    return 0;
  }

  int32_t result = file_io_->Query(&file_info_, callback_);
  if (result != PP_OK_COMPLETIONPENDING) {
    naclfs_->Log(
        "Html5FileSystem::Query doesn't return PP_OK_COMPLETIONPENDING\n");
    return PPErrorToErrNo(result);
  }
  waiting_ = true;
  arguments->chaining = true;
  return 0;
}

ssize_t Html5FileSystem::ReadCall(Arguments* arguments,
                                  void* buf,
                                  size_t nbytes) {
  if (waiting_) {
    waiting_ = false;
    if (arguments->result.callback > 0)
      offset_ += arguments->result.callback;
    return arguments->result.callback;
  }

  if (file_io_->Read(offset_, static_cast<char*>(buf), nbytes, callback_) !=
      PP_OK_COMPLETIONPENDING) {
    naclfs_->Log(
        "Html5FileSystem::Read doesn't return PP_OK_COMPLETIONPENDING\n");
    return -1;
  }
  waiting_ = true;
  arguments->chaining = true;
  return 0;
}

ssize_t Html5FileSystem::WriteCall(Arguments* arguments,
                                   const void* buf,
                                   size_t nbytes) {
  if (waiting_) {
    waiting_ = false;
    if (arguments->result.callback > 0)
      offset_ += arguments->result.callback;
    if (file_info_.size < offset_)
      file_info_.size = offset_;
    return arguments->result.callback;
  }

  if (file_io_->Write(
          offset_, static_cast<const char*>(buf), nbytes, callback_) !=
      PP_OK_COMPLETIONPENDING) {
    naclfs_->Log(
        "Html5FileSystem::Write doesn't return PP_OK_COMPLETIONPENDING\n");
    return -1;
  }
  waiting_ = true;
  arguments->chaining = true;
  return 0;
}

off_t Html5FileSystem::SeekCall(Arguments* arguments,
                                off_t offset,
                                int whence) {
  switch (whence) {
    case SEEK_SET:
      offset_ = offset;
      break;
    case SEEK_CUR:
      offset_ += offset;
      break;
    case SEEK_END:
      offset_ = file_info_.size + offset;
      break;
    default:
      naclfs_->Log("Html5FileSystem::Seek invalid whence\n");
      return -1;
  }
  return offset_;
}

int Html5FileSystem::FcntlCall(Arguments* arguments, int cmd, va_list* ap) {
  switch (cmd) {
    case F_GETFD: {
      // TODO: FD_CLOEXEC isn't supported.
      return 0;
    }
    case F_SETFD: {
      long arg;
      arg = va_arg(*ap, long);
      if (arg != FD_CLOEXEC) {
        std::ostringstream ss;
        ss << "Html5FileSystem::Fcntl not supported cmd=F_SETFD arg="
           << arg << "\n";
        naclfs_->Log(ss.str().c_str());
        errno = ENOSYS;
        return -1;
      }
      return 0;
    }
    default: {
      std::ostringstream ss;
      ss << "Html5FileSystem::Fcntl not supported cmd=" << cmd << "\n";
      naclfs_->Log(ss.str().c_str());
      errno = ENOSYS;
      return -1;
    }
  }
  return 0;
}

int Html5FileSystem::MkDirCall(Arguments* arguments,
                               const char* path,
                               mode_t mode) {
  if (!filesystem_)
    return Initialize(arguments);

  if (waiting_) {
    waiting_ = false;
    return arguments->result.callback;
  }

  pp::FileRef file_ref(*filesystem_, path);
  if (file_ref.MakeDirectory(callback_) != PP_OK_COMPLETIONPENDING) {
    naclfs_->Log(
        "Html5FileSystem::MkDir doesn't return PP_OK_COMPLETIONPENDING\n");
    return -1;
  }
  waiting_ = true;
  arguments->chaining = true;
  return 0;
}

DIR* Html5FileSystem::OpenDirCall(Arguments* arguments, const char* dirname) {
  if (!filesystem_) {
    Initialize(arguments);
    return NULL;
  }
  if (remoting_) {
    rpc_object_ = NULL;
    remoting_ = false;
    if (arguments->result.callback) {
      delete dirent_;
      dirent_ = NULL;
      return NULL;
    }

    pp::FileRef file_ref(*filesystem_, dirname);
    Html5FileSystemDir* dir = new Html5FileSystemDir(this, file_ref);
    if (dir)
      dir->SetEntries(dirent_);
    else
      delete dirent_;
    dirent_ = NULL;
    return reinterpret_cast<DIR*>(dir);
  }

  // Firstly, check if the directory exists.
  // TODO: This is a workaround for http://crbug.com/132201
  std::stringstream ss;
  ss << kRpcId << kCmdDir << dirname;
  rpc_object_ = this;
  remoting_ = true;
  arguments->chaining = true;
  dirent_ = new std::vector<std::string>();
  naclfs_->PostMessage(pp::Var(ss.str()));
  return NULL;
}

void Html5FileSystem::RewindDirCall(Arguments* arguments, DIR* dirp) {
  Html5FileSystemDir* dir = reinterpret_cast<Html5FileSystemDir*>(dirp);
  if (!dir)
    return;
  dir->Rewind();
}

struct dirent* Html5FileSystem::ReadDirCall(Arguments* arguments, DIR* dirp) {
  Html5FileSystemDir* dir = reinterpret_cast<Html5FileSystemDir*>(dirp);
  if (!dir)
    return NULL;

  if (waiting_) {
    waiting_ = false;
    if (arguments->result.callback) {
      // TODO: Oops, DirectoryReader doesn't work in NaCl yet.
      // See, http://crbug.com/106129
      std::stringstream ss;
      ss << "Html5FileSystem::ReadDir failed with error code " <<
          arguments->result.callback << std::endl;
      naclfs_->Log(ss.str().c_str());
      return NULL;
    }
    return dir->ReadDirComplete();
  }

  int result = dir->ReadDir(callback_);
  if (result == PP_OK)
    return dir->ReadDirComplete();

  if (result == PP_OK_COMPLETIONPENDING) {
    waiting_ = true;
    arguments->chaining = true;
  }

  naclfs_->Log(
      "Html5FileSystem::ReadDir doesn't return PP_OK_COMPLETIONPENDING\n");
  return NULL;
}

int Html5FileSystem::CloseDirCall(Arguments* arguments, DIR* dirp) {
  Html5FileSystemDir* dir = reinterpret_cast<Html5FileSystemDir*>(dirp);
  if (!dir)
    return -1;

  delete dir;
  return 0;
}

bool Html5FileSystem::HandleMessage(const pp::Var& message) {
  if (!message.is_string() || !rpc_object_)
    return false;
  std::string s = message.AsString();
  if (s.find(kRpcId) != 0)
    return false;
  if (s[kOffCmd] == kCmdStat[0]) {
    rpc_object_->callback_.Run(s[kOffData] - '0');
    return true;
  } else if (s[kOffCmd] == kCmdDir[0]) {
    if (s[kOffData] != '_') {
      if (rpc_object_->dirent_)
        rpc_object_->dirent_->push_back(s.substr(kOffData));
    } else {
      rpc_object_->callback_.Run(s[kOffData + 1] - '0');
    }
    return true;
  }
  return false;
}

int Html5FileSystem::Initialize(Arguments* arguments) {
  naclfs_->Log("Html5FileSystem: initializing file system...");
  filesystem_ = new pp::FileSystem(naclfs_->GetInstance(),
                                   PP_FILESYSTEMTYPE_LOCALTEMPORARY);
  filesystem_->Open(1024 * 1024, callback_);
  naclfs_->Log("done\n");
  arguments->chaining = true;
  return 0;
}

}  // namespace naclfs
