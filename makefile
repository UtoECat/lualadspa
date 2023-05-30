CCFLAGS = -Wall -Wextra -fno-math-errno
LDFLAGS = -lm

.PHONY: all clean

BUILD_CLI = 1

all : lualadspa

SHARED_HEADERS = ./src/lualadspa.hpp ./src/ladspa.h ./src/luau.hpp
SOURCES = ./src/buffer.cpp ./src/instance.cpp ./src/ladspa.cpp
CXXFLAGS = $(CCFLAGS) -std=c++17 -O1 -g -fPIC

$(SOURCES) : $(SHARED_HEADERS)

# we don't want to recompile this blob so often :/
./src/luau.cpp : ./src/luau.hpp ./src/strtod.h
luau.o : ./src/luau.cpp
	$(CXX) -c $^ -o $@ $(CXXFLAGS)

liblualadspa.so : luau.o $(SOURCES)
	$(CXX) $^ -o $@ -shared $(CXXFLAGS)
	cp liblualadspa.so ~/.ladspa/ || true

lualadspa : liblualadspa.so ./src/cmdline.cpp
	$(CXX) ./src/cmdline.cpp -o $@ $(CXXFLAGS) -ldl

clean:
	rm -f *.o lualadspa liblualadspa.so

install: liblualadspa.so
	mkdir -p ~/.ladspa
	cp liblualadspa.so ~/.ladspa/
	mkdir -p ~/.lualadspa
	cp -r ./plugins ~/.lualadspa
