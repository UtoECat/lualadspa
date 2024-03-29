CCFLAGS = -Wall -Wextra -fno-math-errno
LDFLAGS = -lm

.PHONY: all clean

BUILD_CLI = 1

all : lualadspa

SHARED_HEADERS = ./src/lualadspa.hpp ./src/ladspa.h ./src/luau.hpp
SOURCES = ./src/buffer.cpp ./src/instance.cpp ./src/ladspa.cpp #./src/modules.cpp
CXXFLAGS = $(CCFLAGS) -std=c++17 -O2 -g -fPIC

$(SOURCES) : $(SHARED_HEADERS)

# we don't want to recompile this blob so often :/
./src/luau.cpp : ./src/luau.hpp ./src/strtod.h
luau.o : ./src/luau.cpp
	$(CXX) -c $^ -o $@ $(CXXFLAGS)

liblualadspa.so : luau.o $(SOURCES)
	$(CXX) $^ -o $@ -shared $(CXXFLAGS)

lualadspa : liblualadspa.so ./src/cmdline.cpp
	$(CXX) ./src/cmdline.cpp -o $@ $(CXXFLAGS) -ldl

./src/instance.cpp: ./src/internal.h

clean:
	rm -f *.o lualadspa liblualadspa.so

install: liblualadspa.so
	mkdir -p ~/.ladspa
	cp liblualadspa.so ~/.ladspa/
	mkdir -p ~/.lualadspa
	cp ./plugins/* ~/.lualadspa
