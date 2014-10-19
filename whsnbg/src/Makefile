# make - ubuntu debug
# make release - ubuntu release
# make openwrt - openwrt release or debug (see below openwrt: FLAGS)

TARGET = whsnbg
SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.c=.o)
DEPS = $(HEADERS)

# ubuntu
INCLPATH = -I. -I/usr/include/mysql
LIBPATH = -L. -L/usr/lib
LIBS = -laxtls -lpthread `mysql_config --cflags --libs`
FLAGS = -g -O2
DEF = -D __UBUNTU__

all: $(TARGET)

$(TARGET): $(OBJECTS) $(DEPS)
	$(CC) $(LDFLAGS) $(STATIC) -o $(TARGET) $(OBJECTS) $(LIBPATH) $(LIBS)

$(OBJECTS): $(SOURCES)
	$(CC) $(FLAGS) $(CFLAGS) $(DEF) -c $(SOURCES) $(INCLPATH)


.PHONY: openwrt

# debug version
openwrt: FLAGS = -O2
# release version
#openwrt: FLAGS = -O2 -D NDEBUG
openwrt: INCLPATH = $(TARGET_INCLPATH)
openwrt: LIBPATH = $(TARGET_LIBPATH)
openwrt: LIBS = $(TARGET_LIBS)
#openwrt: STATIC = -static
openwrt: DEF = -D __OPENWRT__
openwrt: $(TARGET)

.PHONY: release

release: FLAGS = -O2 -D NDEBUG
release: $(TARGET)

.PHONY: clean

clean:
	rm -f $(OBJECTS) $(TARGET)

