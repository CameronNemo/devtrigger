ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

BINDIR=$(DESTDIR)$(PREFIX)/sbin
MANDIR=$(DESTDIR)$(PREFIX)/share/man/man1

MANFLAGS=--no-discard-stderr --no-info -h -h -v -v
CFLAGS=-Wall -Werror

HELP2MAN := $(shell which help2man 2>/dev/null)

all: devtrigger man

devtrigger: devtrigger.c

man: devtrigger
ifdef HELP2MAN
	$(HELP2MAN) $(MANFLAGS) ./devtrigger | gzip - >devtrigger.1.gz
else
	$(error "help2man is not installed")
endif

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
