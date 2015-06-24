#
# Copyright (c) 2013-2015 Vladimir Alemasov
# All rights reserved
#
# This program and the accompanying materials are distributed under 
# the terms of GNU General Public License version 2 
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

TARGET = whsnbg
OBJDIR = obj
SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.c)
HEADERS = $(wildcard $(SRCDIR)/*.h)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS = $(HEADERS)

INCLPATH += -I.

# Installation directories by convention
# http://www.gnu.org/prep/standards/html_node/Directory-Variables.html
PREFIX = /usr/local
EXEC_PREFIX = $(PREFIX)
BINDIR = $(EXEC_PREFIX)/bin
SYSCONFDIR = $(PREFIX)/etc
LOCALSTATEDIR = $(PREFIX)/var

CFLAGS += -DSYSCONFDIR=\"$(SYSCONFDIR)\"
CFLAGS += -DLOCALSTATEDIR=\"$(LOCALSTATEDIR)\"

# Syntax of Conditionals
# https://www.gnu.org/software/make/manual/html_node/Conditional-Syntax.html
ifdef DAEMON_VERSION
  CFLAGS += -DLINUX_DAEMON_VERSION
endif
ifndef DETAILED_LOG
  CFLAGS += -DNDPRINTF
endif
ifdef RULES_ENGINE
  CFLAGS += -DRULES_ENGINE
endif
ifdef TLS_LIB_AXTLS
  CFLAGS += -DUSE_TLS_LIBRARY
  CFLAGS += -DAXTLS_LIBRARY
  LIBS += -laxtls
endif
ifdef TLS_LIB_OPENSSL
  CFLAGS += -DUSE_TLS_LIBRARY
  CFLAGS += -DOPENSSL_LIBRARY
  LIBS += -lssl -lcrypto
  ifdef STATIC_LINK
    LIBS += -lz
  endif
endif
ifdef SENSOR_DATA_DECODING
  CFLAGS += -DSENSOR_DATA
endif
ifdef SENSOR_DATA_STORING
  CFLAGS += -DSENSOR_DATA_MYSQL
endif
ifdef MQTT_DATA_STORING
  CFLAGS += -DMQTT_DATA_MYSQL
endif
ifneq ($(SENSOR_DATA_STORING),$(filter $(MQTT_DATA_STORING),))
  INCLPATH += -I/usr/include/mysql
  LIBS += -lmysqlclient
  ifdef STATIC_LINK
    LIBS += -lz
  endif
endif
LIBS += -lpthread -lgcc_eh -lm
ifdef STATIC_LINK
  LIBS += -ldl -static
endif
ifdef WITH_DEBUG
  CFLAGS += -g
else
  CFLAGS += -O2 -DNDEBUG
endif

# main goal
all: $(TARGET)

# target executable
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS) $(LIBPATH) $(LIBS)

# object files
$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c $(DEPS) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLPATH)

# create object files directory
$(OBJDIR):
	mkdir -p $(OBJDIR)

# clean
clean:
	rm -rf $(OBJDIR)

# distclean
distclean: clean
	rm -f $(TARGET)

# install
# http://unixhelp.ed.ac.uk/CGI/man-cgi?install
install: all
	install -d -m 755 "$(BINDIR)"
	install -m 755 $(TARGET) "$(BINDIR)/"
	install -d -m 755 "$(SYSCONFDIR)"
	install -m 644 ./res/whsnbg.conf "$(SYSCONFDIR)/"
	install -m 444 ./res/whsnbg.pem "$(SYSCONFDIR)/"

# uninstall
uninstall:
	rm -f $(BINDIR)/$(TARGET)
	rm -f $(SYSCONFDIR)/whsnbg.conf
	rm -f $(SYSCONFDIR)/whsnbg.pem
	rm -f $(LOCALSTATEDIR)/whsnbg.rules
	rm -f $(LOCALSTATEDIR)/whsnbg.log

.PHONY: all clean distclean install uninstall
