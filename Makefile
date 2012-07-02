CC?=gcc
CFLAGS = `pkg-config --cflags lxpanel --cflags gtk+-2.0` -Wall -Wextra -O -g
LDFLAGS= `pkg-config --libs lxpanel --libs gtk+-2.0`
INSTALL = /usr/bin/install -c
SHLIB=lxbat.so

all: $(SHLIB)

lxbat.so: lxbat.c
	$(CC) $(CFLAGS) -shared -fPIC lxbat.c -o $(SHLIB) $(LDFLAGS)

clean:
	rm -f $(SHLIB)

install:  all
	if [ -d "/usr/lib/lxpanel/plugins" ];   then $(INSTALL) -m 644 $(SHLIB) /usr/lib/lxpanel/plugins/   ; fi
	if [ -d "/usr/lib64/lxpanel/plugins" ]; then $(INSTALL) -m 644 $(SHLIB) /usr/lib64/lxpanel/plugins/ ; fi
