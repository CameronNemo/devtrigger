CC=gcc
CFLAGS=-Wall -Werror

all: devtrigger

devtrigger: devtrigger.c

install: all
	install devtrigger $(INSTALL_PREFIX)/sbin/devtrigger

clean:
	rm -f devtrigger
