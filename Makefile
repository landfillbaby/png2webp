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
pam2webp webp2pam: CPPFLAGS += -DPAM
pam2webp: LDLIBS += -lnetpbm
png2webp pam2webp: png2webp.[ch]
webp2png webp2pam: webp2png.[ch]
png2webp webp2png pam2webp webp2pam:
	$(LINK.c) $< $(LOADLIBES) -lwebp $(LDLIBS) -o $@
install install-pam install-all:
	$(INSTALL) $^ $(DESTDIR)$(PREFIX)/bin/
clean:
	$(RM) pam2webp png2webp webp2pam webp2png
