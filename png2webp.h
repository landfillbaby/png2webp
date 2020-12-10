// © 2020 Lucy Phipps; zlib license
// vi: sw=2
#define VERSION "v0.7"
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
#define O(x) _##x
#define S_IRUSR _S_IREAD
#define S_IWUSR _S_IWRITE
#else
#include <unistd.h>
#define _setmode(x, y) 0
#define _O_BINARY 0
#define O(x) x
#endif
#define P(x) fputs(x "\n", stderr)
#define PF(x, ...) fprintf(stderr, x "\n", __VA_ARGS__)
#define PV(x) if(verbose) P(x);
#define PFV(x, ...) if(verbose) PF(x, __VA_ARGS__);
#define E(f, s, ...) \
  if(!(f)) { \
    PF("ERROR " s, __VA_ARGS__); \
    return 1; \
  }
#define Q(x) \
  if(outnamealloced) free(outname); \
  return x
#define ED(f, s, ...) \
  if(!(f)) { \
    PF("ERROR " s, __VA_ARGS__); \
    Q(1); \
  }
#ifndef EXTRAHELP
#define EXTRALETTERS
#define EXTRAHELP
#define EXTRAFLAGS
#endif
#define EO(x) E(x, "opening \"%s\" for %s: %s", outname, \
    force ? "writing" : "creation", strerror(errno));
#if __STDC_VERSION__ < 201112L || defined(NOFOPENX)
#include <fcntl.h>
#include <sys/stat.h>
#define OPENW \
  int fd = O(open)(outname, \
      O(O_WRONLY) | O(O_CREAT) | _O_BINARY | O(O_TRUNC) | \
	  (force ? 0 : O(O_EXCL)), \
      S_IRUSR | S_IWUSR); \
  EO(fd != -1); \
  EO(fp = O(fdopen)(fd, "wb"));
#else
#define OPENW EO(fp = fopen(outname, force ? "wb" : "wbx"));
#endif
#define HELP \
  P(INEXT "2" OUTEXT " " VERSION ", by Lucy Phipps\n\n" \
    "Usage:\n" \
    INEXT "2" OUTEXT " [-b" EXTRALETTERS "fv-] infile." INEXT " ...\n" \
    INEXT "2" OUTEXT " [-p" EXTRALETTERS "fv-] [{infile." INEXT \
    "|-} [outfile." OUTEXT "|-]]\n\n" \
    "-b: Work with many input files (Batch mode).\n" \
    "    Constructs output filenames by removing the ." INEXT \
    " extension if possible,\n" \
    "    and appending \"." OUTEXT "\".\n" \
    "-p: Work with a single file, allowing Piping from stdin or to stdout,\n" \
    "    or using a different output filename to the input.\n" \
    "    infile." INEXT " and outfile." OUTEXT \
    " default to stdin and stdout respectively,\n" \
    "    or explicitly as \"-\"." \
    " Will error if stdin/stdout is used and is a terminal.\n" \
    EXTRAHELP \
    "-f: Force overwrite of output files (has no effect on stdout).\n" \
    "-v: Be verbose.\n" \
    "--: Explicitly stop parsing options.\n\n" \
    "Without -b or -p, and with 1 or 2 filenames, there is some ambiguity.\n" \
    "In this case it will tell you what its guess is."); \
  return -1;
#define FLAGLIST \
  case 'p': usepipe = 1; /* fall through */ \
  case 'b': \
    if(chosen) { HELP } \
    chosen = 1; \
    break; \
  EXTRAFLAGS \
  case 'f': force = 1; break; \
  case 'v': verbose = 1; break;
#ifdef USEGETOPT
#define FLAGLOOP \
  int c; \
  while((c = getopt(argc, argv, ":bp" EXTRALETTERS "fv")) != -1) switch(c) { \
      FLAGLIST \
      default: HELP \
    } \
  argc -= optind; \
  argv += optind;
#else
#define FLAGLOOP \
  while(--argc && **++argv == '-' && argv[0][1]) \
    while(*++*argv) switch(**argv) { \
	FLAGLIST \
	case '-': \
	  if(!argv[0][1]) { \
	    argc--; \
	    argv++; \
	    goto endflagloop; /* break nested or fall through */ \
	  } \
	default: HELP \
      } \
  endflagloop:
#endif
#define OPENR(x) \
  PFV("%scoding %s ...", "De", x ? "stdin" : *argv); \
  if(x) fp = stdin; \
  else \
    E(fp = fopen(*argv, "rb"), "opening \"%s\" for %s: %s", *argv, "reading", \
	strerror(errno));
#define PIPEARG(x) (*argv[x] == '-' && !argv[x][1])
#define PIPECHK(x, y) \
  if(use##y) { \
    E((x && skipstdoutchk) || !O(isatty)(x), "%s is a terminal", #y); \
    E(_setmode(x, _O_BINARY) != -1, "setting %s to binary mode", #y); \
  }
#define URGC (unsigned)argc
#define GETARGS \
  bool usepipe = 0, usestdin = 0, usestdout = 0, force = 0, verbose = 0, \
       chosen = 0, outnamealloced = 0, skipstdoutchk = 0; \
  char *outname; \
  FLAGLOOP \
  if(chosen && ((usepipe && URGC > 2) || (!usepipe && !argc))) { HELP } \
  if(usepipe) { \
    usestdin = !argc || PIPEARG(0); \
    usestdout = URGC < 2 || PIPEARG(1); \
  } else if(!chosen && URGC < 3) { \
    usestdin = !argc || PIPEARG(0); \
    usestdout = (usestdin && URGC < 2) || (argc == 2 && PIPEARG(1)); \
    usepipe = usestdin || usestdout; \
    if(!usepipe) { \
      PF("Warning: %d file%s given and neither -b or -p specified.", argc, \
	  argc == 1 ? "" : "s"); \
      if(argc == 1) { \
	if(!O(isatty)(1)) usepipe = usestdout = skipstdoutchk = 1; \
      } else /* argc == 2: -p if argv[1] ends in "." OUTEXT */ \
	for(size_t len = 0; argv[1][len];) { \
	  if(argv[1][len] != '.') { \
	    len++; \
	    continue; \
	  } \
	  for(size_t extlen = 0; extlen < sizeof(OUTEXT) - 1; extlen++) \
	    if((argv[1][len + extlen + 1] | 32) != OUTEXT[extlen]) { \
	      len += extlen + 1; \
	      goto endtestext; \
	    } \
	  if(argv[1][len + sizeof(OUTEXT)]) { \
	    len += sizeof(OUTEXT); \
	    continue; \
	  } \
	  usepipe = 1; \
	  break; \
	endtestext:; \
	} \
      PF("Guessed -%c.", usepipe ? 'p' : 'b'); \
  } } \
  PIPECHK(0, stdin); \
  PIPECHK(1, stdout); \
  OPENR(usestdin);
#define EC(x) E(!fclose(fp), "closing %s: %s", x, strerror(errno))
#ifndef GETEXT
#define GETEXT \
  for(size_t extlen = 0; extlen < sizeof(INEXT) - 1; extlen++) \
    if((argv[0][len + extlen + 1] | 32) != INEXT[extlen]) { \
      len += extlen + 1; \
      goto endgetext; \
    }
#endif
#define GETOUTFILE \
  EC(usestdin ? "stdin" : *argv); \
  if(usestdout) { \
    PFV("%scoding %s ...", "En", "stdout"); \
    fp = stdout; \
  } else { \
    if(usepipe) outname = argv[1]; \
    else { \
      size_t len = 0; \
      while(argv[0][len]) { \
	if(argv[0][len] != '.') { \
	  len++; \
	  continue; \
	} \
	GETEXT \
	if(argv[0][len + sizeof(INEXT)]) { \
	  len += sizeof(INEXT); \
	  continue; \
	} \
	break; \
      endgetext:; \
      } \
      outname = malloc(len + sizeof("." OUTEXT)); \
      E(outname, "adding ." OUTEXT " extension to %s: out of RAM", *argv); \
      outnamealloced = 1; \
      memcpy(outname, *argv, len); \
      memcpy(outname + len, "." OUTEXT, sizeof("." OUTEXT)); \
    } \
    PFV("%scoding %s ...", "En", outname); \
    OPENW \
  }
#define GETINFILE \
  EC(usestdout ? "stdout" : outname); \
  if(outnamealloced) { \
    free(outname); \
    outnamealloced = 0; \
  } \
  if(usepipe || !--argc) return 0; \
  argv++; \
  OPENR(0);
