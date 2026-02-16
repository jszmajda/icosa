PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/man1

CFLAGS ?= -O2 -Wall
LDLIBS = -lm

icosa: icosa.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

install: icosa
	install -Dm755 icosa $(DESTDIR)$(BINDIR)/icosa
	install -Dm644 icosa.1 $(DESTDIR)$(MANDIR)/icosa.1

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/icosa
	rm -f $(DESTDIR)$(MANDIR)/icosa.1

clean:
	rm -f icosa

.PHONY: install uninstall clean
