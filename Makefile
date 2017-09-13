CC=gcc
CFLAGS=-Wall -Werror

all: devtriggerall

devtriggerall: devtriggerall.c

install: all
	install devtriggerall $(INSTALL_PREFIX)/sbin/devtriggerall

clean:
	rm -f devtriggerall
