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

#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sstream>
#include <stdio.h>
#include <sys/stat.h>

#include "naclfs.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"

class NaClFsTestsInstance: public pp::Instance {
 public:
  NaClFsTestsInstance(PP_Instance instance)
      : pp::Instance(instance),
        naclfs_(new naclfs::NaClFs(this)) {
  }
  virtual ~NaClFsTestsInstance() {
    pthread_join(thread_, NULL);
    delete naclfs_;
  }
  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    std::stringstream ss;
    naclfs_->Log("opening stdio descriptors...\n");
    int fdin = open("/dev/stdin", O_RDONLY);
    ss.str("");
    ss << " in : " << fdin << std::endl;
    naclfs_->Log(ss.str().c_str());
    int fdout = open("/dev/stdout", O_WRONLY);
    ss.str("");
    ss << " out: " << fdout << std::endl;
    naclfs_->Log(ss.str().c_str());
    int fderr = open("/dev/stderr", O_WRONLY);
    ss.str("");
    ss << " err: " << fderr << std::endl;
    naclfs_->Log(ss.str().c_str());
    naclfs_->set_trace(true);

    BasicTest();

    pthread_create(&thread_, NULL, ThreadMain, this);

    return true;
  }
  void BasicTest() {
    write(STDOUT_FILENO, "out\n", 4);
    write(STDERR_FILENO, "err\n", 4);

    printf("hello to stdout\n");
    fprintf(stderr, "hello to stderr\n");

    FILE* fp = fopen("/dev/stdout", "a");
    fprintf(fp, "hello from fprintf\n");
    fclose(fp);
  }
  virtual void HandleMessage(const pp::Var& var_message) {
    if (!naclfs_->HandleMessage(var_message))
      naclfs_->Log("unknown message\n");
  }
  static void* ThreadMain(void* param) {
    NaClFsTestsInstance* self = static_cast<NaClFsTestsInstance*>(param);
    self->naclfs_->Log("thread start\n");
    self->BasicTest();
    self->naclfs_->Log("say hello to out.txt\n");
    FILE* out = fopen("out.txt", "a+");
    fprintf(out, "hello\n");
    fflush(out);
    fclose(out);
    self->naclfs_->Log("dump out.txt\n");
    FILE* io = fopen("out.txt", "r+");
    size_t read_size;
    do {
      uint8_t buffer[512];
      read_size = fread(buffer, 1, 512, io);
      fprintf(stderr, "fread return %d\n", read_size);
      write(STDOUT_FILENO, buffer, read_size);
    } while (read_size == 512);
    printf("stat on out.txt...\n");
    struct stat buf;
    printf("  result: %d\n", stat("out.txt", &buf));
    printf("  st_mode: %o\n", buf.st_mode);
    printf("  st_size: %Ld\n", buf.st_size);
    printf("  st_blksize: %u\n", buf.st_blksize);
    printf("  st_blocks: %d\n", buf.st_blocks);
    printf("  st_atime: %Ld\n", buf.st_atime);
    printf("  st_mtime: %Ld\n", buf.st_mtime);
    printf("  st_ctime: %Ld\n", buf.st_ctime);

    printf("call opendir which will fail\n");
    DIR* dir = opendir("foo");
    printf(" dir = %p\n", dir);

    self->naclfs_->Log("start echo\n");
    for (;;) {
      uint8_t buffer[1];
      if (read(STDIN_FILENO, buffer, 1) != 1)
        break;
      write(STDOUT_FILENO, buffer, 1);
      fwrite(buffer, 1, 1, io);
      fflush(io);
    }

    self->naclfs_->Log("thread end by EOF\n");
    return NULL;
  }
 private:
  naclfs::NaClFs* naclfs_;
  pthread_t thread_;
};

class NaClFsTestsModule : public pp::Module {
 public:
  NaClFsTestsModule() : pp::Module() {}
  virtual ~NaClFsTestsModule() {}
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    if (single_instance_)
      return NULL;
    single_instance_ = new NaClFsTestsInstance(instance);
    return single_instance_;
  }
 private:
  static NaClFsTestsInstance* single_instance_;
};

NaClFsTestsInstance* NaClFsTestsModule::single_instance_ = NULL;

namespace pp {
  Module* CreateModule() {
    return new NaClFsTestsModule();
  }
}  // namespace pp
