#
# Copyright (c) 2012, Takashi TOYOSHIMA <toyoshim@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# - Neither the name of the authors nor the names of its contributors may be
#   used to endorse or promote products derived from this software with out
#   specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUE
# NTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.
#
OSNAME	:= $(shell python $(NACL_SDK_ROOT)/tools/getos.py)
#TODO: Support glibc toolchain.
TC_TYPE	:= newlib
TC_PATH	:= $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_x86_$(TC_TYPE)
ifeq ($(findstring 32, $(MAKECMDGOALS)), 32)
  ARCH		:= i686
  ARCH_FLAGS	:= -m32
else
  ARCH		:= x86_64
  ARCH_FLAGS	:= -m64
endif

CYGWIN ?= nodosfilewarning
export CYGWIN

CXX	:= $(TC_PATH)/bin/$(ARCH)-nacl-g++
AR	:= $(TC_PATH)/bin/$(ARCH)-nacl-ar
RANLIB	:= $(TC_PATH)/bin/$(ARCH)-nacl-ranlib
CFLAGS	:= $(ARCH_FLAGS) -O9 -g -Wall -pthread -Isrc
OBJ_OUT	:= obj/$(TC_TYPE)-$(ARCH)
HTML	:= html/$(TC_TYPE)
SRCS	:= src/wrap.cc src/naclfs.cc src/filesystem.cc src/port_filesystem.cc \
	   src/html5_filesystem.cc
OBJS	:= $(patsubst src/%.cc, $(OBJ_OUT)/%.o, $(SRCS))
LIBS	:= `./bin/naclfs-config --libs $(TC_TYPE)-$(ARCH)` \
	   -lppapi -lppapi_cpp -lpthread
LDFLAGS	:=

all:
	@$(MAKE) newlib32test

.PHONY: help clean
help:
	@echo "make target"
	@echo "target:"
#	@echo "  all          ... builds all libraries and tests"
	@echo "  clean        ... cleans output files"
#	@echo "  glibc32      ... builds 32-bit library for glibc toolchain"
#	@echo "  glibc64      ... builds 64-bit library for glibc toolchain"
#	@echo "  glibc32test  ... builds 32-bit test for glibc toolchain"
#	@echo "  glibc64test  ... builds 64-bit test for glibc toolchain"
	@echo "  newlib       ... builds libraries for newlib toolchain"
	@echo "  newlib32     ... builds 32-bit library for newlib toolchain"
	@echo "  newlib64     ... builds 64-bit library for newlib toolchain"
	@echo "  newlib32test ... builds 32-bit test for newlib toolchain"
	@echo "  newlib64test ... builds 64-bit test for newlib toolchain"
	@echo

clean:
	@rm -rf obj $(HTML)/*.nexe

.PHONY: newlib32 newlib32test newlib64 newlib64test lib-message test-message
newlib:
	@$(MAKE) newlib32
	@$(MAKE) newlib64
newlib32: lib-message $(OBJ_OUT)/libnaclfs.a
newlib64: lib-message $(OBJ_OUT)/libnaclfs.a
newlibtest:
	@$(MAKE) newlib32test
	@$(MAKE) newlib64test
newlib32test: newlib32 test-message $(HTML)/naclfs_tests_x86_32.nexe
newlib64test: newlib64 test-message $(HTML)/naclfs_tests_x86_64.nexe

lib-message:
	@echo "*** building library to $(OBJ_OUT) ***"

test-message:
	@echo "*** building test to $(OBJ_OUT) ***"

$(OBJ_OUT)/libnaclfs.a: $(OBJS)
	@echo "linking $@ ..."
	@$(AR) rcs $@ $(OBJS)
	@$(RANLIB) $@

$(OBJ_OUT)/%.o: src/%.cc
	@echo "compiling ... " $<
	@mkdir -p $(@D)
	@$(CXX) -c $(CFLAGS) -o $@ -c $<

$(OBJ_OUT)/%.o: test/%.cc
	@echo "compiling ... " $<
	@mkdir -p $(@D)
	@$(CXX) -c $(CFLAGS) -o $@ -c $<

$(HTML)/naclfs_tests_x86_32.nexe: $(OBJ_OUT)/naclfs_tests.o
	@echo "linking $@ ..."
	@$(CXX) $(LDFLAGS) -o $@ $< $(LIBS)

$(HTML)/naclfs_tests_x86_64.nexe: $(OBJ_OUT)/naclfs_tests.o
	@echo "linking $@ ..."
	@$(CXX) $(LDFLAGS) -o $@ $< $(LIBS)
