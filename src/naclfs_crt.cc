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

#include <stdio.h>
#include <sys/fcntl.h>

#include "naclfs.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"

extern "C" int naclfs_main(int argc, const char *argv[]);

namespace naclfs {

class NaClFsInstance: public pp::Instance {
 public:
  NaClFsInstance(PP_Instance instance)
      : pp::Instance(instance),
        naclfs_(new NaClFs(this)) {
  }
  virtual ~NaClFsInstance() {
    pthread_join(thread_, NULL);
    delete naclfs_;
  }
  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    argc_ = argc;
    argv_ = argv;
    open("/dev/stdin", O_RDONLY);
    open("/dev/stdout", O_WRONLY);
    open("/dev/stderr", O_WRONLY);
    naclfs_->set_trace(true);
    puts("xxx");
    pthread_create(&thread_, NULL, ThreadMain, this);
    return true;
  }
  virtual void HandleMessage(const pp::Var& var_message) {
    if (!naclfs_->HandleMessage(var_message))
      naclfs_->Log("unknown message\n");
  }
  static void* ThreadMain(void* param) {
    NaClFsInstance* instance = static_cast<NaClFsInstance*>(param);
    naclfs_main(instance->argc_, instance->argv_);
    return NULL;
  }
 private:
  naclfs::NaClFs* naclfs_;
  pthread_t thread_;
  uint32_t argc_;
  const char** argv_;
};

class NaClFsModule : public pp::Module {
 public:
  NaClFsModule() : pp::Module() {}
  virtual ~NaClFsModule() {}
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    if (instance_)
      return NULL;
    instance_ = new NaClFsInstance(instance);
    return instance_;
  }
 private:
  static NaClFsInstance* instance_;
};

NaClFsInstance* NaClFsModule::instance_ = NULL;

}  // namespace nacpfs

namespace pp {
  Module* CreateModule() {
    return new naclfs::NaClFsModule();
  }
}  // namespace pp
