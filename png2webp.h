// vi: sw=2
#ifdef PAM
#define z "pam"
#else
#include <png.h>
#define z "png"
#endif
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#define isatty _isatty
#define strcasecmp _stricmp
#else
#include <strings.h>
#include <unistd.h>
#define _setmode(x, y) 0
#endif
#define p(x) fputs(x "\n", stderr)
#define pf(x, ...) fprintf(stderr, x "\n", __VA_ARGS__)
#define pv(x) \
  if(verbose) { p(x); }
#define pfv(x, ...) \
  if(verbose) { pf(x, __VA_ARGS__); }
#define e(f, s, ...) \
  if(!(f)) { \
    pf("ERROR: " s, __VA_ARGS__); \
    return 1; \
  }
#define help \
  p("usage:\n\
" inext "2" outext " [-fv] [--] file." inext " ...\n\
" inext "2" outext " [-p[fv] [--] [{infile." inext "|-} [outfile." outext \
    "|-]]]\n\
\n\
for each file." inext ", outputs an equivalent file." outext "\n\
\n\
-f: force overwrite of output files (has no effect on stdout, see below).\n\
-v: be verbose.\n\
-p: default without arguments.\n\
    work with a single file, allowing piping from stdin or to stdout,\n\
    or using a different output filename to the input.\n\
    infile." inext " and outfile." outext \
    " default to stdin or stdout respectively,\n\
    or explicitly as \"-\".\n\
    will error if stdin/stdout is used and is a terminal."); \
  return -1;
#define b(x, y, o) \
  if(o || (unsigned)argc <= x || (*argv[x] == '-' && !argv[x][1])) { \
    if(isatty(x)) { \
      pf("ERROR: std%s is a terminal", #y); \
      help \
    } \
    e(_setmode(x, _O_BINARY) != -1, "setting std%s to binary mode", #y); \
    fd = std##y; \
    usestd##y = 1; \
  }
#ifdef USEGETOPT
#define FLAGLOOP \
  int c; \
  while((c = getopt(argc, argv, ":pfv")) != -1) { \
    switch(c) { \
      case 'p': usepipe = 1; break; \
      case 'f': force = 1; break; \
      case 'v': verbose = 1; break; \
      default: help \
    } \
  } \
  argc -= optind; \
  argv += optind;
#else
#define FLAGLOOP \
  while(--argc && **++argv == '-' && argv[0][1]) { \
    while(*++*argv) { \
      switch(**argv) { \
	case 'p': usepipe = 1; break; \
	case 'f': force = 1; break; \
	case 'v': verbose = 1; break; \
	case '-': \
	  if(!argv[0][1]) { \
	    argc--; \
	    argv++; \
	    goto endflagloop; /*break nested or fall through*/ \
	  } \
	default: help \
      } \
    } \
  } \
  endflagloop:
#endif
#define GETARGS \
  bool force = 0, usepipe = 0, verbose = 0, usestdin = 0, usestdout = 0; \
  char *outname; \
  if(argc < 2) { \
    usepipe = 1; \
    b(0, in, 1); \
  } else { \
    FLAGLOOP \
    if(usepipe) { \
      if(argc > 2) { help } \
      b(0, in, 0); \
    } else { \
      if(!argc) { help } \
    } \
    if(!usestdin) { \
      pfv("%scoding \"%s\"...", "de", *argv); \
      e(fd = fopen(*argv, "rb"), "opening \"%s\" for %s: %s", *argv, \
	"reading", strerror(errno)); \
    } \
  }
#define GETINFILE \
  if(!usestdout) { \
    e(!fclose(fd), "closing %s: %s", outname, strerror(errno)); \
  } \
  if(usepipe || !--argc) { return 0; } \
  argv++; \
  pfv("%scoding \"%s\"...", "de", *argv); \
  e(fd = fopen(*argv, "rb"), "opening \"%s\" for %s: %s", *argv, "reading", \
    strerror(errno));
#define GETOUTFILE \
  if(!usestdin) { e(!fclose(fd), "closing %s: %s", *argv, strerror(errno)); } \
  if(usepipe) { \
    b(1, out, 0) else { outname = argv[1]; } \
  } else { \
    char *dot = strrchr(*argv, '.'); \
    if(dot && dot != *argv && !strcasecmp(dot + 1, inext)) { \
      outname = realloc(outname, dot - *argv + 1 + sizeof(outext)); \
      memcpy(outname, *argv, dot - *argv + 1); \
      memcpy(outname + (dot - *argv + 1), outext, sizeof(outext)); \
    } else { \
      size_t len = strlen(*argv); \
      outname = realloc(outname, len + 1 + sizeof(outext)); \
      memcpy(outname, *argv, len); \
      memcpy(outname + len, "." outext, sizeof(outext) + 1); \
    } \
  } \
  if(!usestdout) { \
    char wx[] = "wbx"; \
    if(force) { wx[2] = 0; } \
    pfv("%scoding \"%s\"...", "en", outname); \
    e(fd = fopen(outname, wx), "opening \"%s\" for %s: %s", outname, \
      force ? "writing" : "creation", strerror(errno)); \
  }
