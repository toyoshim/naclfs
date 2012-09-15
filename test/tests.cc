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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <vector>

static int g_argc;
static char** g_argv;
typedef struct test {
  bool (*test)(void);
  const char* name;
} test;
static std::vector<test> g_tests;
static test g_current_test;

#define SKIP_REGISTER_TEST(group, item)

#define REGISTER_TEST(group, item) { \
  test new_test; \
  new_test.test = test_##group##_##item; \
  new_test.name = #group "." #item; \
  g_tests.push_back(new_test); \
}

#define ERROR(message) { \
  printf("**** ERROR **** : %s : %s\n", \
      g_current_test.name, message); \
  fprintf(stderr, "**** ERROR **** : %s : %s\n", \
      g_current_test.name, message); \
  return false; \
}

int run_tests() {
  int i = 1;
  std::vector<test> failed;
  for (std::vector<test>::iterator it = g_tests.begin();
      it != g_tests.end();
      ++it, ++i) {
    g_current_test = *it;
    bool result = g_current_test.test();
    if (!result)
      failed.push_back(g_current_test);
    printf("%3d/%3d [%s] %s\n",
        i, g_tests.size(),
        result ? "OK" : "NG",
        g_current_test.name);
  }
  if (!failed.size()) {
    puts("All tests pass");
    fprintf(stderr, "No error\n");
  } else {
    int passed = g_tests.size() - failed.size();
    printf("%d test%s pass\n\n",
        passed,
        (passed > 1) ? "s" : "");
    fprintf(stderr, "\n");
    for (std::vector<test>::iterator it = failed.begin();
        it != failed.end();
        ++it) {
      printf("%s\n", (*it).name);
      fprintf(stderr, "%s\n", (*it).name);
    }
    printf("%d test%s failed\n",
        failed.size(),
        (failed.size() == 1) ? "" : "s");
    fprintf(stderr, "%d test%s failed\n",
        failed.size(),
        (failed.size() == 1) ? "" : "s");
  }
  return 0;
}

bool test_SystemCall_OpenAndCloseStandards() {
  int fd;
  fd = open("/dev/stdin", O_RDONLY);
  if (fd < 0)
    ERROR("can not open /dev/stdin");
  if (close(fd))
    ERROR("close /dev/stdin failed");
  if (fd != 3)
    ERROR("unexpected descriptor id");

  fd = open("/dev/stdout", O_WRONLY);
  if (fd < 0)
    ERROR("can not open /dev/stdout");
  if (close(fd))
    ERROR("close /dev/stdout failed");

  fd = open("/dev/stderr", O_WRONLY);
  if (fd < 0)
    ERROR("can not open /dev/stderr");
  if (close(fd))
    ERROR("close /dev/stderr failed");

  return true;
}

bool test_SystemCall_WriteStandards() {
  if (5 != write(STDOUT_FILENO, "*****", 5))
    ERROR("can not write to STDOUT_FILENO");
  if (5 != write(STDERR_FILENO, "*****", 5))
    ERROR("can not write to STDERR_FILENO");

  int fd;
  fd = open("/dev/stdout", O_WRONLY);
  if (fd < 0)
    ERROR("can not open /dev/stdout");
  if (6 != write(fd, "+++++\n", 6))
    ERROR("can not write to /dev/stdout");
  if (close(fd))
    ERROR("close /dev/stdout failed");

  fd = open("/dev/stderr", O_WRONLY);
  if (fd < 0)
    ERROR("can not open /dev/stderr");
  if (6 != write(fd, "+++++\n", 6))
    ERROR("can not write to /dev/stderr");
  if (close(fd))
    ERROR("close /dev/stderr failed");

  return true;
}

bool test_SystemCall_StatStandards() {
  // TODO: Doesn't work.
  struct stat buf;
  if (stat("/dev/stdin", &buf))
    ERROR("can not stat on /dev/stdin");

  return true;
}

bool test_SystemCall_ReadLseekAndWriteFile() {
  const char* fname = "/test_create";
  int fd = open(fname, O_RDWR | O_CREAT | O_TRUNC);
  if (fd < 0)
    ERROR("can not create /test_create");

  char buf[1024];
  if (0 != read(fd, buf, 1024))
    ERROR("unexpected read success");

  if (5 != write(fd, "@@@@@", 5))
    ERROR("can not write to /test_create");

  if (5 != lseek(fd, 0, SEEK_CUR))
    ERROR("seek with SEEK_CUR failed");

  if (0 != read(fd, buf, 1024))
    ERROR("unexpected read success");

  if (4 != lseek(fd, -1, SEEK_END))
    ERROR("seek with SEEK_END failed");

  if (1 != read(fd, buf, 1024))
    ERROR("unexpected read success");

  if (2 != lseek(fd, 2, SEEK_SET))
    ERROR("seek with SEEK_SET failed");

  if (3 != read(fd, buf, 1024))
    ERROR("can not read from /test_create");

  if (close(fd))
    ERROR("close /test_create failed");

  return true;
}

bool test_SystemCall_CreateAndStatFile() {
  const char* fname = "/test_create";
  int fd = open(fname, O_RDWR | O_CREAT | O_TRUNC);
  if (fd < 0)
    ERROR("can not create /test_create");

  if (5 != write(fd, "@@@@@", 5))
    ERROR("can not write to /test_create");

  if (close(fd))
    ERROR("close /test_create failed");

  struct stat buf;
  if (stat(fname, &buf))
    ERROR("can not stat on /test_create");

  mode_t expected_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IXUSR;
  if (buf.st_mode != expected_mode)
    ERROR("invalid st_mode");
  if (5 != buf.st_size)
    ERROR("invalid st_size");
  if (512 != buf.st_blksize)
    ERROR("invalid st_blksize");
  if (1 != buf.st_blocks)
    ERROR("invalid blocks");

  if (-1 != stat("/test_create_ng", &buf))
    ERROR("unexpected successful stat on /test_create_ng");
  // TODO: errno should be set correctly.

  return true;
}

bool test_SystemCall_CreateAndStatDirectory() {
  const char* fpath1 = "/test_path";
  const char* fpath2 = "/test_path/child_dir";
  mkdir(fpath1, S_IRUSR | S_IWUSR);
  mkdir(fpath2, S_IRUSR | S_IWUSR);
  // TODO: Test creation

  struct stat buf;
  mode_t expected_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR;
  if (stat(fpath1, &buf))
    ERROR("stat on /test_path directory failed");
  if (buf.st_mode != expected_mode)
    ERROR("invalid st_mode on /test_path");
  if (stat(fpath2, &buf))
    ERROR("stat on /test_path/child_dir directory failed");
  if (buf.st_mode != expected_mode)
    ERROR("invalid st_mode on /test_path/child_dir");

  return true;
}

bool test_SystemCall_Chdir() {
  const char* fpath1 = "/test_path";
  const char* fpath2 = "/test_path/child_dir";
  const char* fname = "/test_path/hello";
  mkdir(fpath1, S_IRUSR | S_IWUSR);
  mkdir(fpath2, S_IRUSR | S_IWUSR);
  fclose(fopen(fname, "a+"));

  if (chdir("/"))
    ERROR("chdir to / failed");
  if (chdir("test_path/child_dir"))
    ERROR("chdir to test_path/child_dir");
  if (chdir("../..")) {
    chdir("/");
    ERROR("chdir to ../..");
  }
  if (chdir("test_path/child_dir/"))
    ERROR("chdir to test_path/child_dir/");
  struct stat buf;
  // TODO: "../hello" doesn't work.
  //if (stat("../hello", &buf))
  //  ERROR("stat on ../hello failed");
  // TODO: chdir("..") doesn't work.
  if (chdir("/test_path")) { // work around
    chdir("/");
    ERROR("chdir to ..");
  }
  if (stat("hello", &buf)) {
    chdir("/");
    ERROR("stat on hello failed");
  }

  chdir("/");
  return true;
}

bool test_POSIX_OpenAndCloseStandards() {
  FILE* fp;
  fp = fopen("/dev/stdin", "r");
  if (!fp)
    ERROR("can not fopen /dev/stdin");
  if (fclose(fp))
    ERROR("can not fclose /dev/stdin");

  fp = fopen("/dev/stdout", "w+");
  if (!fp)
    ERROR("can not fopen /dev/stdout");
  if (fclose(fp))
    ERROR("can not fclose /dev/stdout");

  fp = fopen("/dev/stdin", "w+");
  if (!fp)
    ERROR("can not fopen /dev/stderr");
  if (fclose(fp))
    ERROR("can not fclose /dev/stderr");

  return true;
}

bool test_POSIX_WriteStandards() {
  if (5 != fprintf(stdout, "%s", "+++++"))
    ERROR("can not fprintf to stdout");
  if (5 != fprintf(stderr, "%s", "+++++"))
    ERROR("can not fprintf to stderr");

  FILE* fp;
  fp = fopen("/dev/stdout", "a");
  if (6 != fprintf(fp, "%s", "*****\n"))
    ERROR("can not fprintf to /dev/stdout");
  if (fclose(fp))
    ERROR("fclose /dev/stdout failed");
  fp = fopen("/dev/stderr", "a");
  if (6 != fprintf(fp, "%s", "*****\n"))
    ERROR("can not fprintf to /dev/stderr");
  if (fclose(fp))
    ERROR("fclose /dev/stderr failed");

  return true;
}

bool test_Internal_PathNormalization() {
  mkdir("/test_path", S_IRUSR | S_IWUSR);
  fclose(fopen("/test_path/hello", "a+"));

  struct stat buf;
  if (stat("/test_path/hello", &buf))
    ERROR("can not stat on /test_path/hello");
  if (stat("test_path/../test_path/hello", &buf))
    ERROR("can not stat on test_path/../test_path/hello");
  if (stat("/test_path/../test_path/hello", &buf))
    ERROR("can not stat on /test_path/../test_path/hello");
  if (stat("/test_path/../test_path", &buf))
    ERROR("can not stat on /test_path/../test_path");
  if (stat("/test_path/../test_path/", &buf))
    ERROR("can not stat on /test_path/../test_path/");

  return true;
}

extern "C" int naclfs_main(int argc, char** argv) {
  g_argc = argc;
  g_argv = argv;

  REGISTER_TEST(SystemCall, OpenAndCloseStandards);
  REGISTER_TEST(SystemCall, WriteStandards);
  SKIP_REGISTER_TEST(SystemCall, StatStandards);
  REGISTER_TEST(SystemCall, ReadLseekAndWriteFile);
  REGISTER_TEST(SystemCall, CreateAndStatFile);
  REGISTER_TEST(SystemCall, CreateAndStatDirectory);
  REGISTER_TEST(SystemCall, Chdir);
  // TODO: OpenAndUnlink, OpenWithVariousModes, MkdirAndRmdir
  REGISTER_TEST(POSIX, OpenAndCloseStandards);
  REGISTER_TEST(POSIX, WriteStandards);
  // TODO: fstat, fcntl
  // TODO: dirent(opendir, rewinddir, readdir, closedir)
  REGISTER_TEST(Internal, PathNormalization);

  return run_tests();
}
