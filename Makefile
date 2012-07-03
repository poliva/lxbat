DESTDIR?=/
CC?=gcc
CFLAGS = `pkg-config --cflags lxpanel --cflags gtk+-2.0` -Wall -Wextra -O -g
LDFLAGS= `pkg-config --libs lxpanel --libs gtk+-2.0`
INSTALL = /usr/bin/install -c
SHLIB=lxbat.so
MACHINE := $(shell uname -m)

srcdir = .
prefix = $(DESTDIR)
bindir = $(prefix)/usr/bin
docdir = $(prefix)/usr/share/doc
mandir = $(prefix)/usr/share/man
libdir = $(prefix)/usr/lib
ifeq ($(MACHINE), x86_64)
libdir = $(prefix)/usr/lib64
endif

all: $(SHLIB)

lxbat.so: lxbat.c
	$(CC) $(CFLAGS) -shared -fPIC lxbat.c -o $(SHLIB) $(LDFLAGS)

clean:
	rm -f $(SHLIB)

install:  all
	mkdir -p $(libdir)/lxpanel/plugins
	$(INSTALL) -m 644 $(srcdir)/$(SHLIB) $(libdir)/lxpanel/plugins/

uninstall:
	rm -rf $(libdir)/lxpanel/plugins/$(SHLIB)
