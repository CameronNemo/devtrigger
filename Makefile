ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

BINDIR=$(DESTDIR)$(PREFIX)/sbin
MANDIR=$(DESTDIR)$(PREFIX)/share/man/man1

MANFLAGS=--no-discard-stderr --no-info -h -h -v -v
CFLAGS=-Wall -Werror

all: devtrigger man

devtrigger: devtrigger.c

man: devtrigger
	test -n $(which help2man) || { echo "help2man not installed"; exit 1; }
	help2man $(MANFLAGS) ./devtrigger | gzip - >devtrigger.1.gz

install: all
	install -dZ $(BINDIR)
	install -DZ devtrigger -t $(BINDIR)
	install -dZ $(MANDIR)
	install -DZ devtrigger.1.gz -t $(MANDIR)

uninstall:
	rm -rf $(BINDIR)/devtrigger
	rm -rf $(MANDIR)/devtrigger.1.gz

clean:
	rm -f devtrigger devtrigger.1.gz
