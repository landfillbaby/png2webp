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
#define strcasecmp _stricmp
#define O(x) _##x
#define S_IRUSR _S_IREAD
#define S_IWUSR _S_IWRITE
#else
#include <strings.h>
#include <unistd.h>
#define _setmode(x, y) 0
#define _O_BINARY 0
#define O(x) x
#endif
#define P(x) fputs(x "\n", stderr)
#define PF(x, ...) fprintf(stderr, x "\n", __VA_ARGS__)
#define PV(x) \
  if(verbose) { P(x); }
#define PFV(x, ...) \
  if(verbose) { PF(x, __VA_ARGS__); }
#define E(f, s, ...) \
  if(!(f)) { \
    PF("ERROR " s, __VA_ARGS__); \
    return 1; \
  }
#if __STDC_VERSION__ < 201112L
#ifndef NOFOPENX
#define NOFOPENX
#endif
#endif
#define EO(x) \
  E(x, "opening \"%s\" for %s: %s", outname, force ? "writing" : "creation", \
    strerror(errno));
#ifdef NOFOPENX
#include <fcntl.h>
#include <sys/stat.h>
#define OPENW \
  PFV("%scoding \"%s\"...", "En", outname); \
  int fd = O(open)(outname, \
		   O(O_WRONLY) | O(O_CREAT) | _O_BINARY | O(O_TRUNC) | \
		       (force ? 0 : O(O_EXCL)), \
		   S_IRUSR | S_IWUSR); \
  EO(fd != -1); \
  EO(fp = O(fdopen)(fd, "wb"));
#else
#define OPENW \
  char wx[] = "wbx"; \
  if(force) { wx[2] = 0; } \
  PFV("%scoding \"%s\"...", "En", outname); \
  EO(fp = fopen(outname, wx));
#endif
#define HELP \
  P("Usage:\n" \
    INEXT "2" OUTEXT " [-bfv-] infile." INEXT " ...\n" \
    INEXT "2" OUTEXT " [-pfv-] [{infile." INEXT "|-} [outfile." OUTEXT \
    "|-]]\n\n" \
    "-b: Default when at least 1 file is given.\n" \
    "    Work with many input files (Batch mode).\n" \
    "    Constructs output filenames by removing the \"." INEXT \
    "\" extension if possible,\n" \
    "    and appending \"." OUTEXT "\".\n" \
    "-p: Default when no files are given.\n" \
    "    Work with a single file, allowing Piping from stdin or to stdout,\n" \
    "    or using a different output filename to the input.\n" \
    "    \"infile." INEXT "\" and \"outfile." OUTEXT \
    "\" default to stdin and stdout respectively,\n" \
    "    or explicitly as \"-\".\n" \
    "    Will error if stdin/stdout is used and is a terminal.\n" \
    "-f: Force overwrite of output files (has no effect on stdout).\n" \
    "-v: Be verbose.\n" \
    "--: Explicitly stop parsing options."); \
  return -1;
#define B(x, y) \
  if((unsigned)argc <= x || (*argv[x] == '-' && !argv[x][1])) { \
    if(O(isatty)(x)) { HELP } \
    E(_setmode(x, _O_BINARY) != -1, "setting std%s to binary mode", #y); \
    fp = std##y; \
    usestd##y = 1; \
  }
#define FLAGLIST \
  case 'p': usepipe = 1; /* fall through */ \
  case 'b': \
    if(chosen) { HELP } \
    chosen = 1; \
    break; \
  case 'f': force = 1; break; \
  case 'v': \
    verbose = 1; \
    break;
#ifdef USEGETOPT
#define FLAGLOOP \
  int c; \
  while((c = getopt(argc, argv, ":bpfv")) != -1) { \
    switch(c) { \
      FLAGLIST \
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
	FLAGLIST \
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
  bool usepipe = 0, usestdin = 0, usestdout = 0, force = 0, verbose = 0, \
       chosen = 0; \
  char *outname = 0; \
  FLAGLOOP \
  if(!chosen && !argc) { usepipe = 1; } \
  if(usepipe) { \
    if(argc > 2) { HELP } \
    B(0, in); \
  } \
  if(!usestdin) { \
    if(!argc) { HELP } \
    PFV("%scoding \"%s\"...", "De", *argv); \
    E(fp = fopen(*argv, "rb"), "opening \"%s\" for %s: %s", *argv, "reading", \
      strerror(errno)); \
  }
#define GETINFILE \
  if(!usestdout) { \
    E(!fclose(fp), "closing %s: %s", outname, strerror(errno)); \
  } \
  if(usepipe || !--argc) { return 0; } \
  argv++; \
  PFV("%scoding \"%s\"...", "De", *argv); \
  E(fp = fopen(*argv, "rb"), "opening \"%s\" for %s: %s", *argv, "reading", \
    strerror(errno));
#define GETOUTFILE \
  if(!usestdin) { E(!fclose(fp), "closing %s: %s", *argv, strerror(errno)); } \
  if(usepipe) { \
    B(1, out) else { outname = argv[1]; } \
  } else { \
    char *dot = strrchr(*argv, '.'); \
    size_t len; \
    if(dot && dot != *argv && !strcasecmp(dot + 1, INEXT)) { \
      len = dot - *argv; \
    } else { \
      len = strlen(*argv); \
    } \
    outname = realloc(outname, len + sizeof("." OUTEXT)); \
    memcpy(outname, *argv, len); \
    memcpy(outname + len, "." OUTEXT, sizeof("." OUTEXT)); \
  } \
  if(!usestdout) { OPENW }
