
TARGET   := lpeek
VERSION  := 0.1
DESC     := Regular File Viewer
WWW      := "https://github.com/LeonCSt/lpeek"
LICENSE  := GNU

CXX      := g++
CPPFLAGS := -I/usr/include/freetype2
LDLIBS   := -lX11 -lXext -lfreetype

PREFIX   := /usr/local
sysconfdir ?= $(PREFIX)/etc
sharedir   ?= $(PREFIX)/share

CONFDIR    := $(sysconfdir)/lpeek
SHAREDIR := $(sharedir)/lpeek

CXXFLAGS := -std=c++17 -Wall -DCONF_DIR='"$(CONFDIR)"' -DSHARE_DIR='"$(SHAREDIR)"'

SRCS := lpeek011.cpp lpeek012.cpp
HDRS := lpeek010.h

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $(SRCS) $(LDLIBS)

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/lpeek
	install -d $(DESTDIR)$(CONFDIR)
	install -m 644 lpeek.conf $(DESTDIR)$(CONFDIR)/lpeek.conf
	install -d $(DESTDIR)$(SHAREDIR)
	install -m 644 lpeek_example.bin $(DESTDIR)$(SHAREDIR)/lpeek_example.bin
	install -m 644 FreeMono.otf $(DESTDIR)$(SHAREDIR)/FreeMono.otf

clean:
	rm -f $(TARGET) *.o

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/lpeek
	rm -rf $(DESTDIR)$(CONFDIR) $(DESTDIR)$(SHAREDIR)

