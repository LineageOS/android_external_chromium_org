// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/sandbox_init.h"

#if defined(OS_LINUX) && defined(__x86_64__)

#include <asm/unistd.h>
#include <errno.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <signal.h>
#include <string.h>
#include <sys/prctl.h>
#include <ucontext.h>
#include <unistd.h>

#include <vector>

#include "base/command_line.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/time.h"
#include "content/public/common/content_switches.h"

#ifndef PR_SET_NO_NEW_PRIVS
  #define PR_SET_NO_NEW_PRIVS 38
#endif

#ifndef SYS_SECCOMP
  #define SYS_SECCOMP 1
#endif

#ifndef __NR_eventfd2
  #define __NR_eventfd2 290
#endif

// Constants from very new header files that we can't yet include.
#ifndef SECCOMP_MODE_FILTER
  #define SECCOMP_MODE_FILTER 2
  #define SECCOMP_RET_KILL        0x00000000U
  #define SECCOMP_RET_TRAP        0x00030000U
  #define SECCOMP_RET_ERRNO       0x00050000U
  #define SECCOMP_RET_ALLOW       0x7fff0000U
#endif


namespace {

static void CheckSingleThreaded() {
  int num_threads = file_util::CountFilesCreatedAfter(
      FilePath("/proc/self/task"), base::Time::UnixEpoch());
  // Possibly racy, but it's ok because this is more of a debug check to catch
  // new threaded situations arising during development.
  CHECK_EQ(num_threads, 1);
}

static void SIGSYS_Handler(int signal, siginfo_t* info, void* void_context) {
  if (signal != SIGSYS)
    return;
  if (info->si_code != SYS_SECCOMP)
    return;
  if (!void_context)
    return;
  ucontext_t* context = reinterpret_cast<ucontext_t*>(void_context);
  unsigned int syscall = context->uc_mcontext.gregs[REG_RAX];
  if (syscall >= 1024)
    syscall = 0;
  // Purposefully dereference the syscall as an address so it'll show up very
  // clearly and easily in crash dumps.
  volatile char* addr = reinterpret_cast<volatile char*>(syscall);
  *addr = '\0';
  _exit(1);
}

static void InstallSIGSYSHandler() {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = SIGSYS_Handler;
  int ret = sigaction(SIGSYS, &sa, NULL);
  PLOG_IF(FATAL, ret != 0) << "Failed to install SIGSYS handler.";
}

static void EmitLoad(int offset, std::vector<struct sock_filter>* program) {
  struct sock_filter filter;
  filter.code = BPF_LD+BPF_W+BPF_ABS;
  filter.jt = 0;
  filter.jf = 0;
  filter.k = offset;
  program->push_back(filter);
}

static void EmitJEQJT1(int value, std::vector<struct sock_filter>* program) {
  struct sock_filter filter;
  filter.code = BPF_JMP+BPF_JEQ+BPF_K;
  filter.jt = 1;
  filter.jf = 0;
  filter.k = value;
  program->push_back(filter);
}

static void EmitJEQJF1(int value, std::vector<struct sock_filter>* program) {
  struct sock_filter filter;
  filter.code = BPF_JMP+BPF_JEQ+BPF_K;
  filter.jt = 0;
  filter.jf = 1;
  filter.k = value;
  program->push_back(filter);
}

static void EmitRet(int value, std::vector<struct sock_filter>* program) {
  struct sock_filter filter;
  filter.code = BPF_RET+BPF_K;
  filter.jt = 0;
  filter.jf = 0;
  filter.k = value;
  program->push_back(filter);
}

static void EmitPreamble(std::vector<struct sock_filter>* program) {
  // First, check correct syscall arch.
  // "4" is magic offset for the arch number.
  EmitLoad(4, program);
  EmitJEQJT1(AUDIT_ARCH_X86_64, program);
  EmitRet(SECCOMP_RET_KILL, program);

  // Load the syscall number.
  // "0" is magic offset for the syscall number.
  EmitLoad(0, program);
}

static void EmitAllowSyscall(int nr, std::vector<struct sock_filter>* program) {
  EmitJEQJF1(nr, program);
  EmitRet(SECCOMP_RET_ALLOW, program);
}

static void EmitFailSyscall(int nr, int err,
                            std::vector<struct sock_filter>* program) {
  EmitJEQJF1(nr, program);
  EmitRet(SECCOMP_RET_ERRNO | err, program);
}

static void EmitTrap(std::vector<struct sock_filter>* program) {
  EmitRet(SECCOMP_RET_TRAP, program);
}

static void ApplyGPUPolicy(std::vector<struct sock_filter>* program) {
  // "Hot" syscalls go first.
  EmitAllowSyscall(__NR_read, program);
  EmitAllowSyscall(__NR_ioctl, program);
  EmitAllowSyscall(__NR_poll, program);
  EmitAllowSyscall(__NR_epoll_wait, program);
  EmitAllowSyscall(__NR_recvfrom, program);
  EmitAllowSyscall(__NR_write, program);
  EmitAllowSyscall(__NR_writev, program);
  EmitAllowSyscall(__NR_gettid, program);

  // Less hot syscalls.
  EmitAllowSyscall(__NR_futex, program);
  EmitAllowSyscall(__NR_madvise, program);
  EmitAllowSyscall(__NR_sendmsg, program);
  EmitAllowSyscall(__NR_recvmsg, program);
  EmitAllowSyscall(__NR_eventfd2, program);
  EmitAllowSyscall(__NR_pipe, program);
  EmitAllowSyscall(__NR_mmap, program);
  EmitAllowSyscall(__NR_mprotect, program);
  EmitAllowSyscall(__NR_clone, program);
  EmitAllowSyscall(__NR_set_robust_list, program);
  EmitAllowSyscall(__NR_getuid, program);
  EmitAllowSyscall(__NR_geteuid, program);
  EmitAllowSyscall(__NR_getgid, program);
  EmitAllowSyscall(__NR_getegid, program);
  EmitAllowSyscall(__NR_epoll_create, program);
  EmitAllowSyscall(__NR_fcntl, program);
  EmitAllowSyscall(__NR_socketpair, program);
  EmitAllowSyscall(__NR_epoll_ctl, program);
  EmitAllowSyscall(__NR_prctl, program);
  EmitAllowSyscall(__NR_fstat, program);
  EmitAllowSyscall(__NR_close, program);
  EmitAllowSyscall(__NR_restart_syscall, program);
  EmitAllowSyscall(__NR_rt_sigreturn, program);
  EmitAllowSyscall(__NR_brk, program);
  EmitAllowSyscall(__NR_rt_sigprocmask, program);
  EmitAllowSyscall(__NR_munmap, program);
  EmitAllowSyscall(__NR_dup, program);
  EmitAllowSyscall(__NR_mlock, program);
  EmitAllowSyscall(__NR_munlock, program);
  EmitAllowSyscall(__NR_exit, program);
  EmitAllowSyscall(__NR_exit_group, program);

  EmitFailSyscall(__NR_open, ENOENT, program);
  EmitFailSyscall(__NR_access, ENOENT, program);
}

static bool CanUseSeccompFilters() {
  int ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, 0, 0, 0);
  if (ret != 0 && errno == EFAULT)
    return true;
  return false;
}

static void InstallFilter(const std::vector<struct sock_filter>& program) {
  int ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
  PLOG_IF(FATAL, ret != 0) << "prctl(PR_SET_NO_NEW_PRIVS) failed";

  struct sock_fprog fprog;
  fprog.len = program.size();
  fprog.filter = const_cast<struct sock_filter*>(&program[0]);

  ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &fprog, 0, 0);
  PLOG_IF(FATAL, ret != 0) << "Failed to install filter.";
}

}  // anonymous namespace

namespace content {

void InitializeSandbox() {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kNoSandbox))
    return;

  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);
  if (process_type == switches::kGpuProcess &&
      command_line.HasSwitch(switches::kDisableGpuSandbox))
    return;

  if (!CanUseSeccompFilters())
    return;

  CheckSingleThreaded();

  std::vector<struct sock_filter> program;
  EmitPreamble(&program);

  if (process_type == switches::kGpuProcess) {
    ApplyGPUPolicy(&program);
  } else {
    NOTREACHED();
  }

  EmitTrap(&program);

  InstallSIGSYSHandler();
  InstallFilter(program);
}

}  // namespace content

#else

namespace content {

void InitializeSandbox() {
}

}  // namespace content

#endif

