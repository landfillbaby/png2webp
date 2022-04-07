SHELL = /bin/sh
PREFIX ?= /usr/local
INSTALL ?= install
CFLAGS ?= -O3 -Wall -Wextra -s
MAKEFLAGS += -r
.PHONY: default install pam install-pam all install-all clean
default install: png2webp webp2png
pam install-pam: pam2webp webp2pam
all install-all: png2webp webp2png pam2webp webp2pam
png2webp webp2png: LDLIBS += -lpng
webp2png webp2pam: CPPFLAGS += -DFROMWEBP
pam2webp webp2pam: CPPFLAGS += -DPAM
pam2webp: LDLIBS += -lnetpbm
png2webp webp2png pam2webp webp2pam: png2webp.c
	$(LINK.c) $< $(LOADLIBES) -lwebp $(LDLIBS) -o $@
install install-pam install-all:
	$(INSTALL) $^ $(DESTDIR)$(PREFIX)/bin/
clean:
	$(RM) pam2webp png2webp webp2pam webp2png
