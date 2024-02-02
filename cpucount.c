#define _GNU_SOURCE
#include <stdio.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#ifndef NOSCHED
#include <sched.h>
#endif
#ifdef NOGETNPROCS
#include <unistd.h>
#else
#include <sys/sysinfo.h>
#endif
#endif
static int cpucount(void) {
#ifdef _WIN32
  // TODO: GetProcessAffinityMask like sched_getaffinity
  SYSTEM_INFO x;
  GetSystemInfo(&x); // TODO: GetActiveProcessorCount instead ?
  return x.dwNumberOfProcessors;
#else
#ifndef NOSCHED
  cpu_set_t x;
  if(!sched_getaffinity(0, sizeof x, &x)) return CPU_COUNT(&x);
#endif
#ifdef NOGETNPROCS
  int y = (int)sysconf(
#if defined __arm__ || defined __aarch64__
      _SC_NPROCESSORS_CONF
#else
      _SC_NPROCESSORS_ONLN
#endif
  );
  return y < 1 ? 1 : y;
#elif defined __arm__ || defined __aarch64__
  return get_nprocs_conf();
#else
  return get_nprocs();
#endif
#endif
}

int main(void) { printf("%d\n", cpucount()); }
