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

#include "naclfs.h"

#include <pthread.h>
#include <sstream>

#include "filesystem.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/core.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"

#include "wrap.h"

namespace naclfs {

NaClFs* NaClFs::single_instance_ = NULL;
  bool NaClFs::trace_ = false;

NaClFs::NaClFs(pp::Instance* instance)
    : filesystem_(new FileSystem(this)),
      core_(pp::Module::Get()->core()),
      instance_(instance) {
  single_instance_ = this;
  pthread_mutex_init(&mutex_, NULL);
  pthread_cond_init(&cond_, NULL);
}

NaClFs::~NaClFs() {
  single_instance_ = NULL;
  pthread_mutex_destroy(&mutex_);
  pthread_cond_destroy(&cond_);
}

void NaClFs::Log(const char* message) {
  std::stringstream ss;
  ss << "E" << message;
  write_to_real_stderr(ss.str().c_str(), ss.str().size());
  if (single_instance_)
    NaClFs::PostMessage(pp::Var(ss.str()));
}

void NaClFs::PostMessage(const pp::Var& message) {
  if (single_instance_->core_->IsMainThread()) {
    single_instance_->instance_->PostMessage(message);
  } else {
    pthread_mutex_lock(&single_instance_->mutex_);
    single_instance_->core_->CallOnMainThread(
        0, pp::CompletionCallback(PostMessageFromMainThread,
                                  const_cast<pp::Var*>(&message)));
    pthread_cond_wait(&single_instance_->cond_, &single_instance_->mutex_);
    pthread_mutex_unlock(&single_instance_->mutex_);
  }
}

bool NaClFs::HandleMessage(const pp::Var& message) {
  return filesystem_->HandleMessage(message);
}

void NaClFs::PostMessageFromMainThread(void* param, int32_t result) {
  const pp::Var* message = static_cast<pp::Var*>(param);
  single_instance_->instance_->PostMessage(*message);
  pthread_mutex_lock(&single_instance_->mutex_);
  pthread_cond_signal(&single_instance_->cond_);
  pthread_mutex_unlock(&single_instance_->mutex_);
}

}  // namespace naclfs
