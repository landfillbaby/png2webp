// vi: sw=2 tw=80
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
// WIP based loosely on coreutils cp copy_reg()
// no ACLs or xattrs or security contexts or anything
// also no optional overwriting symlinks (yet)
int main(void) {
  int fd = open("test.txt", O_RDONLY /*| O_BINARY*/);
  if(fd == -1) {
    perror("open(in)");
    return 1;
  }
  struct stat s;
  if(fstat(fd, &s)) {
    perror("fstat(in)");
    close(fd);
    return 1;
  }
  char b[64];
  ssize_t c = read(fd, b, 64); // would actually be a loop
  if(c == -1) {
    perror("read(in)");
    close(fd);
    return 1;
  }
  if(close(fd)) {
    perror("close(in)");
    return 1;
  }
  fd = open("test2.txt", O_WRONLY /*| O_BINARY*/ | O_CREAT | O_TRUNC,
      s.st_mode | S_IWUSR);
  if(fd == -1) {
    perror("open(out)");
    return 1;
  }
#ifdef AVOID_CHOWN
  struct stat o;
  if(fstat(fd, &o)) {
    perror("fstat(out)");
    close(fd);
    return 1;
  }
#endif
  if(write(fd, b, c) != c) { // see read
    perror("write(out)");
    close(fd);
    return 1;
  }
  int r = 0;
  if(futimens(fd,
	 offsetof(struct stat, st_mtim) - offsetof(struct stat, st_atim)
		 == sizeof(struct timespec)
	     ? &s.st_atim
	     : (const struct timespec[2]){
		   {.tv_sec = s.st_atim.tv_sec, .tv_nsec = s.st_atim.tv_nsec},
		   {.tv_sec = s.st_mtim.tv_sec,
		       .tv_nsec = s.st_mtim.tv_nsec}})) {
    perror("futimens(out)");
    r = 1;
  }
  if(
#ifdef AVOID_CHOWN
      // "Avoid calling chown if we know it's not necessary"
      (o.st_uid != s.st_uid || o.st_gid != s.st_gid) &&
#endif
      fchown(fd, s.st_uid, s.st_gid)) {
    // TODO: allow failure?
    perror("fchown(out)");
    r = 1;
  }
  if(fchmod(fd, s.st_mode)) {
    perror("fchmod(out)");
    r = 1;
  }
  // TODO: futimens last?
  if(close(fd)) {
    perror("close(out)");
    r = 1;
  }
  return r;
}
