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
ifeq ($(findstring newlib, $(MAKECMDGOALS)), newlib)
  TC_TYPE	:= newlib
  ARCH_CFLAGS	:=
else
  TC_TYPE	:= glibc
  ARCH_CFLAGS	:= -fPIC
endif
OSNAME	:= $(shell python $(NACL_SDK_ROOT)/tools/getos.py)
TC_PATH	:= $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_x86_$(TC_TYPE)
USR32_PATH	:= $(TC_PATH)/i686-nacl/usr
USR64_PATH	:= $(TC_PATH)/x86_64-nacl/usr
ifeq ($(findstring 32, $(MAKECMDGOALS)), 32)
  ARCH		:= i686
  ARCH_CFLAGS	+= -m32
else
  ARCH		:= x86_64
  ARCH_CFLAGS	+= -m64
endif
CYGWIN ?= nodosfilewarning
export CYGWIN

CXX	:= $(TC_PATH)/bin/$(ARCH)-nacl-g++
AR	:= $(TC_PATH)/bin/$(ARCH)-nacl-ar
RANLIB	:= $(TC_PATH)/bin/$(ARCH)-nacl-ranlib
OBJDUMP	:= $(TC_PATH)/bin/$(ARCH)-nacl-objdump
NMFGEN	:= $(NACL_SDK_ROOT)/tools/create_nmf.py
CFLAGS	:= $(ARCH_CFLAGS) -O9 -g -Wall -pthread `./bin/naclfs-config --cflags`
OBJ_OUT	:= obj/$(TC_TYPE)-$(ARCH)
HTML	:= html/$(TC_TYPE)
SRCS	:= src/wrap.cc src/naclfs.cc src/filesystem.cc src/port_filesystem.cc \
	   src/html5_filesystem.cc
CRT_SRC	:= src/naclfs_crt.cc
OBJS	:= $(patsubst src/%.cc, $(OBJ_OUT)/%.o, $(SRCS))
CRT_OBJ	:= $(OBJ_OUT)/naclfs_crt.o
LIBS	:= `./bin/naclfs-config --libs $(TC_TYPE)-$(ARCH)` \
	   -lppapi -lppapi_cpp -lpthread
CRT_LIB	:= `./bin/naclfs-config --crt $(TC_TYPE)-$(ARCH)`
LDFLAGS	:=

.PHONY: all clean install glibcinstall newlibinstall default help
default: help

all:
	@$(MAKE) glibctest
	@$(MAKE) newlibtest

clean:
	@rm -rf obj html/*/*.nexe html/glibc/naclfs_tests.nmf \
		html/glibc/tests.nmf html/glibc/hello.nmf \
		html/glibc/lib32 html/glibc/lib64

install:
	@$(MAKE) glibcinstall
	@$(MAKE) newlibinstall

.PHONY: _glibc_install_message _newlibc_install_message _common_install
glibcinstall: glibc _glibc_install_message _common_install
newlibinstall: newlib _newlib_install_message _common_install

_glibc_install_message:
	@echo "*** installing glibc libraries and tools ***"

_newlib_install_message:
	@echo "*** installing newlib libraries and tools ***"

_common_install:
	@echo "*** installing 32-bit library and tool ***"
	@install -d $(USR32_PATH)/bin
	@install -d $(USR32_PATH)/lib/naclfs
	@install obj/$(TC_TYPE)-i686/libnaclfs.a $(USR32_PATH)/lib
	@if [ -f obj/$(TC_TYPE)-i686/libnaclfs.so ]; then \
		install obj/$(TC_TYPE)-i686/libnaclfs.so $(USR32_PATH)/lib; \
	fi
	@install bin/install-naclfs-config $(USR32_PATH)/bin/naclfs-config
	@install html/naclfs.js $(USR32_PATH)/lib/naclfs
	@echo "*** installing 64-bit library and tool ***"
	@install -d $(USR64_PATH)/bin
	@install -d $(USR64_PATH)/lib/naclfs
	@install obj/$(TC_TYPE)-x86_64/libnaclfs.a $(USR64_PATH)/lib
	@if [ -f obj/$(TC_TYPE)-x86_64/libnaclfs.so ]; then \
		install obj/$(TC_TYPE)-x86_64/libnaclfs.so $(USR64_PATH)/lib; \
	fi
	@install bin/install-naclfs-config $(USR64_PATH)/bin/naclfs-config
	@install html/naclfs.js $(USR64_PATH)/lib/naclfs

help:
	@echo "make target"
	@echo "target:"
	@echo "  all           ... builds all libraries and tests"
	@echo "  clean         ... cleans output files"
	@echo "  install       ... install libraries and tools"
	@echo "  glibcinstall  ... install glibc library and tool"
	@echo "  newlibinstall ... install newlib library and tool"
	@echo "  glibc         ... builds libraries for glibc toolchain"
	@echo "  glibc32       ... builds 32-bit library for glibc toolchain"
	@echo "  glibc64       ... builds 64-bit library for glibc toolchain"
	@echo "  glibctest     ... builds tests for glibc toolchain"
	@echo "  glibc32test   ... builds 32-bit test for glibc toolchain"
	@echo "  glibc64test   ... builds 64-bit test for glibc toolchain"
	@echo "  newlib        ... builds libraries for newlib toolchain"
	@echo "  newlib32      ... builds 32-bit library for newlib toolchain"
	@echo "  newlib64      ... builds 64-bit library for newlib toolchain"
	@echo "  newlibtest    ... builds tests for newlib toolchain"
	@echo "  newlib32test  ... builds 32-bit test for newlib toolchain"
	@echo "  newlib64test  ... builds 64-bit test for newlib toolchain"
	@echo

.PHONY: glibc glibc32 glibc64 newlib newlib32 newlib64 _lib_message
glibc:
	@$(MAKE) glibc32
	@$(MAKE) glibc64
glibc32: _lib_message $(OBJ_OUT)/libnaclfs.so $(OBJ_OUT)/libnaclfs.a $(CRT_OBJ)
glibc64: _lib_message $(OBJ_OUT)/libnaclfs.so $(OBJ_OUT)/libnaclfs.a $(CRT_OBJ)
newlib:
	@$(MAKE) newlib32
	@$(MAKE) newlib64
newlib32: _lib_message $(OBJ_OUT)/libnaclfs.a $(CRT_OBJ)
newlib64: _lib_message $(OBJ_OUT)/libnaclfs.a $(CRT_OBJ)
_lib_message:
	@echo "*** building library to $(OBJ_OUT) ***"

.PHONY: glibctest glibc32test glibc64test \
	newlibtest newlib32test newlib64test _test_message
glibctest:
	@$(MAKE) glibc32test
	@$(MAKE) glibc64test
	@echo "*** generating nmf for glibc ***"
	@$(NMFGEN) -D $(OBJDUMP) -o html/glibc/naclfs_tests.nmf \
		-L $(TC_PATH)/x86_64-nacl/lib32 -L $(TC_PATH)/x86_64-nacl/lib \
		`./bin/naclfs-config --nmf` \
		html/glibc/naclfs_tests_x86_32.nexe \
		html/glibc/naclfs_tests_x86_64.nexe \
		-t glibc -s html/glibc
	@$(NMFGEN) -D $(OBJDUMP) -o html/glibc/tests.nmf \
		-L $(TC_PATH)/x86_64-nacl/lib32 -L $(TC_PATH)/x86_64-nacl/lib \
		`./bin/naclfs-config --nmf` \
		html/glibc/tests_x86_32.nexe \
		html/glibc/tests_x86_64.nexe \
		-t glibc -s html/glibc
	@$(NMFGEN) -D $(OBJDUMP) -o html/glibc/hello.nmf \
		-L $(TC_PATH)/x86_64-nacl/lib32 -L $(TC_PATH)/x86_64-nacl/lib \
		`./bin/naclfs-config --nmf` \
		html/glibc/hello_x86_32.nexe \
		html/glibc/hello_x86_64.nexe \
		-t glibc -s html/glibc
	@rm -r html/glibc/html
glibc32test: glibc32 _test_message \
	$(HTML)/naclfs_tests_x86_32.nexe \
	$(HTML)/tests_x86_32.nexe $(HTML)/hello_x86_32.nexe
glibc64test: glibc64 _test_message \
	$(HTML)/naclfs_tests_x86_64.nexe \
	$(HTML)/tests_x86_64.nexe $(HTML)/hello_x86_64.nexe
newlibtest:
	@$(MAKE) newlib32test
	@$(MAKE) newlib64test
newlib32test: newlib32 _test_message \
	$(HTML)/naclfs_tests_x86_32.nexe \
	$(HTML)/tests_x86_32.nexe $(HTML)/hello_x86_32.nexe
newlib64test: newlib64 _test_message \
	$(HTML)/naclfs_tests_x86_64.nexe \
	$(HTML)/tests_x86_64.nexe $(HTML)/hello_x86_64.nexe
_test_message:
	@echo "*** building test to $(OBJ_OUT) ***"

$(OBJ_OUT)/libnaclfs.so: $(OBJS)
	@echo "linking shared library $@ ..."
	@$(CXX) -shared -Wl,-soname,libnaclfs.so -o $@ $(OBJS) \
		-lppapi_cpp -lpthread

$(OBJ_OUT)/libnaclfs.a: $(OBJS)
	@echo "linking static library $@ ..."
	@$(AR) rcs $@ $(OBJS)
	@$(RANLIB) $@

$(OBJ_OUT)/%.o: src/%.cc
	@echo "compiling ..." $<
	@mkdir -p $(@D)
	@$(CXX) -c $(CFLAGS) -o $@ -c $<

$(OBJ_OUT)/%.o: test/%.cc
	@echo "compiling ..." $<
	@mkdir -p $(@D)
	@$(CXX) -c $(CFLAGS) -o $@ -c $<

$(HTML)/naclfs_tests_x86_32.nexe: $(OBJ_OUT)/naclfs_tests.o
	@echo "linking $@ ..."
	@$(CXX) $(LDFLAGS) -o $@ $< $(LIBS)

$(HTML)/tests_x86_32.nexe: $(OBJ_OUT)/tests.o
	@echo "linking $@ ..."
	@$(CXX) $(LDFLAGS) -o $@ $< $(CRT_LIB) $(LIBS)

$(HTML)/hello_x86_32.nexe: $(OBJ_OUT)/hello.o
	@echo "linking $@ ..."
	@$(CXX) $(LDFLAGS) -o $@ $< $(CRT_LIB) $(LIBS)

$(HTML)/naclfs_tests_x86_64.nexe: $(OBJ_OUT)/naclfs_tests.o
	@echo "linking $@ ..."
	@$(CXX) $(LDFLAGS) -o $@ $< $(LIBS)

$(HTML)/tests_x86_64.nexe: $(OBJ_OUT)/tests.o
	@echo "linking $@ ..."
	@$(CXX) $(LDFLAGS) -o $@ $< $(CRT_LIB) $(LIBS)

$(HTML)/hello_x86_64.nexe: $(OBJ_OUT)/hello.o
	@echo "linking $@ ..."
	@$(CXX) $(LDFLAGS) -o $@ $< $(CRT_LIB) $(LIBS)
