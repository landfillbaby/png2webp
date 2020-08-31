// vi: sw=2
#ifdef PAM
#define Z "pam"
#else
#include <png.h>
#define Z "png"
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
#define P(x) fputs(x "\n", stderr)
#define PF(x, ...) fprintf(stderr, x "\n", __VA_ARGS__)
#define PV(x) \
  if(verbose) { P(x); }
#define PFV(x, ...) \
  if(verbose) { PF(x, __VA_ARGS__); }
#define E(f, s, ...) \
  if(!(f)) { \
    PF("ERROR: " s, __VA_ARGS__); \
    return 1; \
  }
#define HELP \
  P("usage:\n\
" INEXT "2" OUTEXT " [-fv] [--] file." INEXT " ...\n\
" INEXT "2" OUTEXT " [-p[fv] [--] [{infile." INEXT "|-} [outfile." OUTEXT \
    "|-]]]\n\
\n\
for each file." INEXT ", outputs an equivalent file." OUTEXT "\n\
\n\
-f: force overwrite of output files (has no effect on stdout, see below).\n\
-v: be verbose.\n\
-p: default without arguments.\n\
    work with a single file, allowing piping from stdin or to stdout,\n\
    or using a different output filename to the input.\n\
    infile." INEXT " and outfile." OUTEXT \
    " default to stdin or stdout respectively,\n\
    or explicitly as \"-\".\n\
    will error if stdin/stdout is used and is a terminal."); \
  return -1;
#define B(x, y, o) \
  if(o || (unsigned)argc <= x || (*argv[x] == '-' && !argv[x][1])) { \
    if(isatty(x)) { \
      PF("ERROR: std%s is a terminal", #y); \
      HELP \
    } \
    E(_setmode(x, _O_BINARY) != -1, "setting std%s to binary mode", #y); \
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
      default: HELP \
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
	default: HELP \
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
    B(0, in, 1); \
  } else { \
    FLAGLOOP \
    if(usepipe) { \
      if(argc > 2) { HELP } \
      B(0, in, 0); \
    } else { \
      if(!argc) { HELP } \
    } \
    if(!usestdin) { \
      PFV("%scoding \"%s\"...", "de", *argv); \
      E(fd = fopen(*argv, "rb"), "opening \"%s\" for %s: %s", *argv, \
	"reading", strerror(errno)); \
    } \
  }
#define GETINFILE \
  if(!usestdout) { \
    E(!fclose(fd), "closing %s: %s", outname, strerror(errno)); \
  } \
  if(usepipe || !--argc) { return 0; } \
  argv++; \
  PFV("%scoding \"%s\"...", "de", *argv); \
  E(fd = fopen(*argv, "rb"), "opening \"%s\" for %s: %s", *argv, "reading", \
    strerror(errno));
#define GETOUTFILE \
  if(!usestdin) { E(!fclose(fd), "closing %s: %s", *argv, strerror(errno)); } \
  if(usepipe) { \
    B(1, out, 0) else { outname = argv[1]; } \
  } else { \
    char *dot = strrchr(*argv, '.'); \
    if(dot && dot != *argv && !strcasecmp(dot + 1, INEXT)) { \
      outname = realloc(outname, dot - *argv + 1 + sizeof(OUTEXT)); \
      memcpy(outname, *argv, dot - *argv + 1); \
      memcpy(outname + (dot - *argv + 1), OUTEXT, sizeof(OUTEXT)); \
    } else { \
      size_t len = strlen(*argv); \
      outname = realloc(outname, len + 1 + sizeof(OUTEXT)); \
      memcpy(outname, *argv, len); \
      memcpy(outname + len, "." OUTEXT, sizeof(OUTEXT) + 1); \
    } \
  } \
  if(!usestdout) { \
    char wx[] = "wbx"; \
    if(force) { wx[2] = 0; } \
    PFV("%scoding \"%s\"...", "en", outname); \
    E(fd = fopen(outname, wx), "opening \"%s\" for %s: %s", outname, \
      force ? "writing" : "creation", strerror(errno)); \
  }
