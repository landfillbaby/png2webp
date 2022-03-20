// anti-copyright Lucy Phipps 2020
// vi: sw=2 tw=80
#define VERSION "v0.8"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#define O(x) _##x
#define M _S_IREAD | _S_IWRITE
#else
#include <unistd.h>
#define _setmode(x, y) 0
#define _O_BINARY 0
#define O(x) x
#define M 0666
#endif
#ifndef ISINEXT
#ifndef INEXTCHK
#define INEXTCHK "." INEXT
#endif
#define ISINEXT \
	if(len >= sizeof INEXT) { \
		uint32_t ext, extmask, extmatch; \
		memcpy(&ext, *argv + len - 4, 4); \
		memcpy(&extmask, (char[4]){(sizeof INEXT > 4) * 32, 32, 32, \
			32}, 4); \
		memcpy(&extmatch, (char[4]){INEXTCHK}, 4); \
		if((sizeof INEXT < 5 || argv[0][len - 5] == '.') && \
			(ext | extmask) == extmatch) len -= sizeof INEXT; \
	}
#endif
#ifndef OUTEXTCHK
#define OUTEXTCHK "." OUTEXT
#endif
#define P(x) fputs(x "\n", stderr)
#define PF(x, ...) fprintf(stderr, x "\n", __VA_ARGS__)
#define PV(x) if(verbose) P(x);
#define PFV(...) if(verbose) PF(__VA_ARGS__);
#define E(f, ...) \
	if(!(f)) { \
		PF("ERROR " __VA_ARGS__); \
		return 1; \
	}
#ifndef EXTRAHELP
#define EXTRALETTERS
#define EXTRAHELP
#define EXTRAFLAGS
#endif
#define EO(x) E(x, "opening \"%s\" for %s: %s", outname, \
	force ? "writing" : "creation", strerror(errno))
#if __STDC_VERSION__ < 201112L || defined(NOFOPENX)
#include <fcntl.h>
#include <sys/stat.h>
#define OPENW \
	int fd = O(open)(outname, O(O_WRONLY) | O(O_CREAT) | _O_BINARY | \
		O(O_TRUNC) | (!force * O(O_EXCL)), M); \
	EO(fd != -1 && (fp = O(fdopen)(fd, "wb")));
#else
#define OPENW EO(fp = fopen(outname, force ? "wb" : "wbx"));
#endif
#define HELP \
  P(INEXT "2" OUTEXT " " VERSION "\n\nUsage:\n" INEXT "2" OUTEXT \
    " [-b" EXTRALETTERS "fv-] infile." INEXT " ...\n" INEXT "2" OUTEXT \
    " [-p" EXTRALETTERS "fv-] [{infile." INEXT "|-} [outfile." OUTEXT \
    "|-]]\n\n-b: Work with many input files (Batch mode).\n" \
    "    Constructs output filenames by removing the ." INEXT \
    " extension if possible,\n    and appending \"." OUTEXT "\".\n" \
    "-p: Work with a single file, allowing Piping from stdin or to stdout,\n" \
    "    or using a different output filename to the input.\n    infile." \
    INEXT " and outfile." OUTEXT \
    " default to stdin and stdout respectively,\n" \
    "    or explicitly as \"-\".\n" \
    "    Will show this message if stdin/stdout is used and is a terminal.\n" \
    EXTRAHELP \
    "-f: Force overwrite of output files (has no effect on stdout).\n" \
    "-v: Be verbose.\n--: Explicitly stop parsing options.\n\n" \
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
		goto endflagloop; \
	  } /* fall through */ \
	default: HELP \
    } \
  endflagloop:
#endif
#define OPENR(x) \
	PFV("%scoding %s ...", "De", x ? "stdin" : *argv); \
	if(x) fp = stdin; \
	else E(fp = fopen(*argv, "rb"), "opening \"%s\" for %s: %s", *argv, \
		"reading", strerror(errno));
#define PIPEARG(x) (*argv[x] == '-' && !argv[x][1])
#define PIPECHK(x, y) \
  if(use##y) { \
	if(!(x && skipstdoutchk) && O(isatty)(x)) { HELP } \
	E(_setmode(x, _O_BINARY) != -1, "setting %s to binary mode", #y); \
  }
#define URGC (unsigned)argc
#define GETARGS \
  bool usepipe = 0, usestdin = 0, usestdout = 0, force = 0, verbose = 0, \
	chosen = 0, skipstdoutchk = 0; \
  char* outname = 0; \
  FLAGLOOP \
  if(chosen && (usepipe ? URGC > 2 : !argc)) { HELP } \
  if(usepipe) { \
    usestdin = !argc || PIPEARG(0); \
    usestdout = URGC < 2 || PIPEARG(1); \
  } else if(!chosen && URGC < 3) { \
    usestdin = !argc || PIPEARG(0); \
    usestdout = argc == 2 ? PIPEARG(1) : usestdin; \
    if(!(usepipe = usestdin || usestdout)) { \
	PF("Warning: %d file%s given and neither -b or -p specified.", argc, \
		argc == 1 ? "" : "s"); \
	if(argc == 1) { \
		if(!O(isatty)(1)) usepipe = usestdout = skipstdoutchk = 1; \
	} else { \
		size_t len = strlen(argv[1]); \
		if(len >= sizeof OUTEXT) { \
			uint32_t ext, extmask, extmatch; \
			memcpy(&ext, argv[1] + len - 4, 4); \
			memcpy(&extmask, (char[4]){(sizeof OUTEXT > 4) * 32, \
				32, 32, 32}, 4); \
			memcpy(&extmatch, (char[4]){OUTEXTCHK}, 4); \
			usepipe = (sizeof OUTEXT < 5 || \
					argv[1][len - 5] == '.') && \
				(ext | extmask) == extmatch; \
	}	} \
	PF("Guessed -%c.", usepipe ? 'p' : 'b'); \
  } } \
  PIPECHK(0, stdin); \
  PIPECHK(1, stdout); \
  OPENR(usestdin);
#define EC(x) E(!fclose(fp), "closing %s: %s", x, strerror(errno))
#define GETOUTFILE \
  EC(usestdin ? "stdin" : *argv); \
  if(usestdout) { \
	PFV("%scoding %s ...", "En", "stdout"); \
	fp = stdout; \
  } else { \
    if(usepipe) outname = argv[1]; \
    else { \
	size_t len = strlen(*argv); \
	ISINEXT; \
	outname = malloc(len + sizeof "." OUTEXT); \
	E(outname, "adding ." OUTEXT " extension to %s: out of RAM", *argv); \
	memcpy(outname, *argv, len); \
	memcpy(outname + len, "." OUTEXT, sizeof "." OUTEXT); \
    } \
    PFV("%scoding %s ...", "En", outname); \
    OPENW \
  }
#define GETINFILE \
	EC(usestdout ? "stdout" : outname); \
	if(usepipe || !--argc) return 0; \
	if(outname) { \
		free(outname); \
		outname = 0; \
	} \
	argv++; \
	OPENR(0);
