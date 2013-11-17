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

# LIBC_TYPE: newlib | glibc
# LIBC_CFLAGS: ...
# TC_CXX: clang++ | g++
# TC_TYPE: pnacl | x86_newlib | x86_glibc
# ARCH_TYPE: pnacl | i686-nacl | x86_64-nacl
# ARCH_CFLAGS: ...
# TARGET_TYPE: pnacl | newlib-i686 | newlib-x86_64 | glibc-i686 | glibc-x86_64
ifeq ($(findstring pnacl, $(MAKECMDGOALS)), pnacl)
  LIBC_TYPE	:= newlib
  LIBC_CFLAGS	:=
  TC_CXX	:= clang++
  TC_TYPE	:= pnacl
  ARCH_TYPE	:= pnacl
  ARCH_CFLAGS	:=
  TARGET_TYPE	:= pnacl
else
  ifeq ($(findstring newlib, $(MAKECMDGOALS)), newlib)
    LIBC_TYPE	:= newlib
    LIBC_CFLAGS	:=
  else
    LIBC_TYPE	:= glibc
    LIBC_CFLAGS	:= -fPIC
  endif
  TC_CXX	:= g++
  TC_TYPE	:= x86_$(LIBC_TYPE)
  ifeq ($(findstring 32, $(MAKECMDGOALS)), 32)
    ARCH_TYPE	:= i686-nacl
    ARCH_CFLAGS	:= -m32
    TARGET_TYPE	:= $(LIBC_TYPE)-i686
  else
    ARCH_TYPE	:= x86_64-nacl
    ARCH_CFLAGS	:= -m64
    TARGET_TYPE	:= $(LIBC_TYPE)-x86_64
  endif
endif
OSNAME	:= $(shell python $(NACL_SDK_ROOT)/tools/getos.py)
TC_PATH	:= $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_$(TC_TYPE)

USR32_PATH	:= $(TC_PATH)/i686-nacl/usr
USR64_PATH	:= $(TC_PATH)/x86_64-nacl/usr
USRPNACL_PATH	:= $(TC_PATH)/usr

CYGWIN ?= nodosfilewarning
export CYGWIN

CXX	:= $(TC_PATH)/bin/$(ARCH_TYPE)-$(TC_CXX)
AR	:= $(TC_PATH)/bin/$(ARCH_TYPE)-ar
RANLIB	:= $(TC_PATH)/bin/$(ARCH_TYPE)-ranlib
OBJDUMP	:= $(TC_PATH)/bin/$(ARCH_TYPE)-objdump
PNACLFN	:= $(TC_PATH)/bin/$(ARCH_TYPE)-finalize
PNACLTR	:= $(TC_PATH)/bin/$(ARCH_TYPE)-translate
NMFGEN	:= $(NACL_SDK_ROOT)/tools/create_nmf.py
CFLAGS	:= $(LIBC_CFLAGS) $(ARCH_CFLAGS) -O9 -g -Wall \
	   -I$(NACL_SDK_ROOT)/include \
	   `./bin/naclfs-config --cflags $(TARGET_TYPE)`
OBJ_OUT	:= obj/$(TARGET_TYPE)
HTML	:= html/$(TC_TYPE)
SRCS	:= src/wrap.cc src/naclfs.cc src/filesystem.cc src/port_filesystem.cc \
	   src/html5_filesystem.cc
CRT_SRC	:= src/naclfs_crt.cc
OBJS	:= $(patsubst src/%.cc, $(OBJ_OUT)/%.o, $(SRCS))
CRT_OBJ	:= $(OBJ_OUT)/naclfs_crt.o
CRT_LIB	:= `./bin/naclfs-config --crt $(TARGET_TYPE)`
LDFLAGS	:= `./bin/naclfs-config --libs $(TARGET_TYPE)`

.PHONY: all clean install glibcinstall newlibinstall default help
default: help

all:
	@$(MAKE) glibctest
	@$(MAKE) newlibtest
	@$(MAKE) pnacltest

clean:
	@rm -rf obj html/*/*.nexe html/*/*.pexe html/*/*.bc \
		html/x86_glibc/lib32 html/x86_glibc/lib64 \
		html/x86_glibc/naclfs_tests.nmf html/x86_glibc/tests.nmf \
		html/x86_glibc/hello.nmf

install:
	@$(MAKE) glibcinstall
	@$(MAKE) newlibinstall
	@$(MAKE) pnaclinstall

.PHONY: _glibc_install_message _newlibc_install_message _pnacl_install _common_install
glibcinstall: glibc _glibc_install_message _common_install
newlibinstall: newlib _newlib_install_message _common_install
pnaclinstall: pnacl _pnacl_install_message _pnacl_install

_glibc_install_message:
	@echo "--- installing glibc libraries and tools ---"

_newlib_install_message:
	@echo "--- installing newlib libraries and tools ---"

_pnacl_install_message:
	@echo "--- installing pnacl libraries and tools ---"

_common_install:
	@echo "--- installing 32-bit library and tool ---"
	@install -d $(USR32_PATH)/bin
	@install -d $(USR32_PATH)/lib/naclfs
	@install obj/$(LIBC_TYPE)-i686/libnaclfs.a $(USR32_PATH)/lib
	@if [ -f obj/$(LIBC_TYPE)-i686/libnaclfs.so ]; then \
	  install obj/$(LIBC_TYPE)-i686/libnaclfs.so $(USR32_PATH)/lib; \
	fi
	@install bin/install-naclfs-config $(USR32_PATH)/bin/naclfs-config
	@install html/naclfs.js $(USR32_PATH)/lib/naclfs
	@echo "--- installing 64-bit library and tool ---"
	@install -d $(USR64_PATH)/bin
	@install -d $(USR64_PATH)/lib/naclfs
	@install obj/$(LIBC_TYPE)-x86_64/libnaclfs.a $(USR64_PATH)/lib
	@if [ -f obj/$(LIBC_TYPE)-x86_64/libnaclfs.so ]; then \
	  install obj/$(LIBC_TYPE)-x86_64/libnaclfs.so $(USR64_PATH)/lib; \
	fi
	@install bin/install-naclfs-config $(USR64_PATH)/bin/naclfs-config
	@install html/naclfs.js $(USR64_PATH)/lib/naclfs

_pnacl_install:
	@echo "--- installing pnacl library and tool ---"
	@install -d $(USRPNACL_PATH)/bin
	@install -d $(USRPNACL_PATH)/lib/naclfs
	@install obj/pnacl/libnaclfs.a $(USRPNACL_PATH)/lib
	@if [ -f obj/pnacl/libnaclfs.so ]; then \
	  install obj/pnacl/libnaclfs.so $(USRPNACL_PATH)/lib; \
	fi
	@install bin/install-naclfs-config $(USRPNACL_PATH)/bin/naclfs-config
	@install html/naclfs.js $(USRPNACL_PATH)/lib/naclfs

help:
	@echo "make target"
	@echo "target:"
	@echo "  all           ... builds all libraries and tests"
	@echo "  clean         ... cleans output files"
	@echo "  install       ... install libraries and tools"
	@echo "  glibcinstall  ... install glibc library and tool"
	@echo "  newlibinstall ... install newlib library and tool"
	@echo "  pnaclinstall  ... install pnacl library and tool"
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
	@echo "  pnacl         ... builds libraries for pnacl toolchain"
	@echo "  pnacltest     ... builds test for pnacl toolchain"
	@echo

.PHONY: glibc glibc32 glibc64 newlib newlib32 newlib64 pnal _lib_message
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
pnacl: _lib_message $(OBJ_OUT)/libnaclfs.a $(CRT_OBJ)
_lib_message:
	@echo "--- building library to $(OBJ_OUT) ---"

.PHONY: glibctest glibc32test glibc64test \
	newlibtest newlib32test newlib64test pnacltest _test_message
glibctest:
	@$(MAKE) glibc32test
	@$(MAKE) glibc64test
	@echo "--- generating nmf for glibc ---"
	@$(NMFGEN) -o html/x86_glibc/naclfs_tests.nmf \
		`./bin/naclfs-config --nmf` \
		html/x86_glibc/naclfs_tests_x86_32.nexe \
		html/x86_glibc/naclfs_tests_x86_64.nexe \
		-s html/x86_glibc
	@$(NMFGEN) -o html/x86_glibc/tests.nmf \
		`./bin/naclfs-config --nmf` \
		html/x86_glibc/tests_x86_32.nexe \
		html/x86_glibc/tests_x86_64.nexe \
		-s html/x86_glibc
	@$(NMFGEN) -o html/x86_glibc/hello.nmf \
		`./bin/naclfs-config --nmf` \
		html/x86_glibc/hello_x86_32.nexe \
		html/x86_glibc/hello_x86_64.nexe \
		-s html/x86_glibc
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
pnacltest: pnacl _test_message \
	$(HTML)/naclfs_tests_arm.nexe \
	$(HTML)/tests_arm.nexe $(HTML)/hello_arm.nexe
_test_message:
	@echo "--- building test to $(OBJ_OUT) ---"

$(OBJ_OUT)/libnaclfs.so: $(OBJS)
	@echo "linking shared library $@ ..."
	@$(CXX) -shared -Wl,-soname,libnaclfs.so -o $@ $(OBJS)

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
	@$(CXX) -o $@ $< $(LDFLAGS)

$(HTML)/tests_x86_32.nexe: $(OBJ_OUT)/tests.o $(CRT_OBJ) $(OBJS)
	@echo "linking $@ ..."
	@$(CXX) -o $@ $< $(CRT_LIB) $(LDFLAGS)

$(HTML)/hello_x86_32.nexe: $(OBJ_OUT)/hello.o $(CRT_OBJ) $(OBJS)
	@echo "linking $@ ..."
	@$(CXX) -o $@ $< $(CRT_LIB) $(LDFLAGS)

$(HTML)/naclfs_tests_x86_64.nexe: $(OBJ_OUT)/naclfs_tests.o
	@echo "linking $@ ..."
	@$(CXX) -o $@ $< $(LDFLAGS)

$(HTML)/tests_x86_64.nexe: $(OBJ_OUT)/tests.o $(CRT_OBJ) $(OBJS)
	@echo "linking $@ ..."
	@$(CXX) -o $@ $< $(CRT_LIB) $(LDFLAGS)

$(HTML)/hello_x86_64.nexe: $(OBJ_OUT)/hello.o $(CRT_OBJ) $(OBJS)
	@echo "linking $@ ..."
	@$(CXX) -o $@ $< $(CRT_LIB) $(LDFLAGS)

$(HTML)/naclfs_tests_arm.nexe: $(HTML)/naclfs_tests.pexe
	@echo "translating $@ ..."
	@$(PNACLTR) -arch arm -O3 -o $@ $<

$(HTML)/naclfs_tests.pexe: $(HTML)/naclfs_tests.bc
	@echo "finalizing $@ ..."
	@$(PNACLFN) -o $@ $<

$(HTML)/naclfs_tests.bc: $(OBJ_OUT)/naclfs_tests.o
	@echo "linking $@ ..."
	@$(CXX) -O9 -o $@ $< $(LDFLAGS)

$(HTML)/tests_arm.nexe: $(HTML)/tests.pexe
	@echo "translating $@ ..."
	@$(PNACLTR) -arch arm -O3 -o $@ $<

$(HTML)/tests.pexe: $(HTML)/tests.bc
	@echo "finalizing $@ ..."
	@$(PNACLFN) -o $@ $<

$(HTML)/tests.bc: $(OBJ_OUT)/tests.o $(CRT_OBJ) $(OBJS)
	@echo "linking $@ ..."
	@$(CXX) -O9 -o $@ $< $(CRT_LIB) $(LDFLAGS)

$(HTML)/hello_arm.nexe: $(HTML)/hello.pexe
	@echo "translating $@ ..."
	@$(PNACLTR) -arch arm -O3 -o $@ $<

$(HTML)/hello.pexe: $(HTML)/hello.bc
	@echo "finalizing $@ ..."
	@$(PNACLFN) -o $@ $<

$(HTML)/hello.bc: $(OBJ_OUT)/hello.o $(CRT_OBJ) $(OBJS)
	@echo "linking $@ ..."
	@$(CXX) -O9 -o $@ $< $(CRT_LIB) $(LDFLAGS)

