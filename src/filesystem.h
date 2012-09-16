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

#ifndef NACLFS_FILESYSTEM_H_
#define NACLFS_FILESYSTEM_H_
#pragma once

#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>

#include <string>
#include <vector>

#include "ppapi/cpp/completion_callback.h"

namespace pp {

class Core;
class Var;

};  // namespace pp

namespace naclfs {

class NaClFs;

class FileSystem {
 public:
  class Delegate {
   protected:
    enum Function {
      OPEN,
      STAT,
      CLOSE,
      FSTAT,
      READ,
      WRITE,
      SEEK,
      ISATTY,
      FCNTL,
      MKDIR,
      OPENDIR,
      REWINDDIR,
      READDIR,
      CLOSEDIR
    };  // enum Function
    typedef struct _Arguments {
      enum Function function;
      Delegate* delegate;
      bool chaining;
      union {
        struct {
          const char* path;
          int oflag;
          mode_t cmode;
        } open;
        struct {
          const char* path;
          struct stat* buf;
        } stat;
        struct {
          struct stat* buf;
        } fstat;
        struct {
          void* buf;
          size_t nbytes;
        } read;
        struct {
          const void* buf;
          size_t nbytes;
        } write;
        struct {
          off_t offset;
          int whence;
        } seek;
        struct {
          int cmd;
          va_list* ap;
        } fcntl;
        struct {
          const char* path;
          mode_t mode;
        } mkdir;
        struct {
          const char* dirname;
        } opendir;
        struct {
          DIR* dirp;
        } rewinddir;
        struct {
          DIR* dirp;
        } readdir;
        struct {
          DIR* dirp;
        } closedir;
      } u;
      union {
        int32_t callback;
        int open;
        int stat;
        int close;
        int fstat;
        ssize_t read;
        ssize_t write;
        off_t seek;
        int isatty;
        int fcntl;
        int mkdir;
        DIR* opendir;
        struct dirent* readdir;
        int closedir;
      } result;
    } Arguments;

   public:
    Delegate();
    virtual int Open(const char* path, int oflag, mode_t cmode);
    virtual int Stat(const char* path, struct stat* buf);
    virtual int Close();
    virtual int Fstat(struct stat* buf);
    virtual ssize_t Read(void* buf, size_t nbytes);
    virtual ssize_t Write(const void* buf, size_t nbytes);
    virtual off_t Seek(off_t offset, int whence);
    virtual int IsATty();
    virtual int Fcntl(int cmd, va_list* ap);
    virtual int MkDir(const char* path, mode_t mode);
    virtual DIR* OpenDir(const char* dirname);
    virtual void RewindDir(DIR* dirp);
    virtual struct dirent* ReadDir(DIR* dirp);
    virtual int CloseDir(DIR* dirp);

    virtual int OpenCall(Arguments* arguments,
                         const char* path,
                         int oflag,
                         mode_t cmode) { return ENOSYS; }
    virtual int StatCall(Arguments* arguments,
                         const char* path,
                         struct stat* buf) { return ENOSYS; }
    virtual int CloseCall(Arguments* arguments) { return ENOSYS; }
    virtual int FstatCall(Arguments* arguments,
                          struct stat* buf) { return ENOSYS; }
    virtual ssize_t ReadCall(Arguments* arguments,
                             void* buf,
                             size_t nbytes) { return -1; }
    virtual ssize_t WriteCall(Arguments* arguments,
                              const void* buf,
                              size_t nbytes) { return -1; }
    virtual off_t SeekCall(Arguments* arguments,
                           off_t offset,
                           int whence) { return -1; }
    virtual int IsATtyCall(Arguments* arguments) { return ENOSYS; }
    virtual int FcntlCall(Arguments* arguments,
                          int cmd,
                          va_list* ap) { return -1; }
    virtual int MkDirCall(Arguments* arguments,
                          const char* path,
                          mode_t mode) { return -1; }
    virtual DIR* OpenDirCall(Arguments* arguments,
                             const char* dirname) { return NULL; }
    virtual void RewindDirCall(Arguments* arguments, DIR* dirp) {}
    virtual struct dirent* ReadDirCall(Arguments* arguments,
                                       DIR* dirp) { return NULL; }
    virtual int CloseDirCall(Arguments* arguments, DIR* dirp) { return -1; }

   protected:
    pp::CompletionCallback callback_;

   private:
    void Wait();
    void Signal();

    void Call(Arguments& arguments);
    static void Proxy(void* param, int32_t result);
    static void Switch(Arguments* arguments);

    static bool initialized_;
    static pthread_mutex_t mutex_;
    static pthread_cond_t cond_;
    static pp::Core* core_;
  };  // class FileSystem::Delegate

  class Dir {
   public:
    Dir(Delegate* delegate) : delegate_(delegate) {}
    virtual ~Dir() {}

    Delegate* delegate() { return delegate_; }

   protected:
    Delegate* delegate_;
  };  // class FileSystem::Dir

  FileSystem(NaClFs* naclfs);
  ~FileSystem();

  int Open(const char* path, int oflag, mode_t cmode, int* newfd);
  int Stat(const char* path, struct stat* buf);
  int Close(int fildes);
  int Fstat(int fildes, struct stat* buf);
  ssize_t Read(int fildes, void* buf, size_t nbytes);
  ssize_t Write(int fildes, const void* buf, size_t nbytes);
  off_t Seek(int fildes, off_t offset, int whence);
  int IsATty(int fildes);
  int Fcntl(int fildes, int cmd, va_list* ap);
  int MkDir(const char* path, mode_t mode);
  DIR* OpenDir(const char* dirname);
  void RewindDir(DIR* dirp);
  struct dirent* ReadDir(DIR* dirp);
  int CloseDir(DIR* dirp);
  int ChDir(const char* path);
  char* GetCwd(char* buf, size_t size);

  static bool HandleMessage(const pp::Var& message);

 private:
  void CreateFullpath(const char* path, std::string* fullpath);
  Delegate* CreateDelegate(const char* path);
  int BindToDescriptor(Delegate* delegate);
  Delegate* GetDelegate(int fildes);
  void DeleteDescriptor(int fildes);

  std::vector<Delegate*> descriptors_;
  std::string cwd_;
  pthread_mutex_t mutex_;
  pp::Core* core_;
  NaClFs* naclfs_;
};  // class FileSystem

}  // namespace naclfs

#endif  // NACLFS_FILESYSTEM_H_
