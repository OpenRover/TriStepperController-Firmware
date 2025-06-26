CXX?=g++
CFLAGS:=-x c++ -std=c++20 -Wall -Wextra -pedantic-errors -g -Og
CFLAGS+=-Wno-gnu-anonymous-struct
CFLAGS+=-Wno-nested-anon-types

SRCS:=$(wildcard ./*.cpp)
INCS:=$(wildcard ./*.h)

LIBS:=$(wildcard ../lib/*)
LIB_SRCS:=$(wildcard ../lib/**/*.cpp)

COMPILE_COMMAND:=$(CXX) -o ../emulator.o $(CFLAGS) -I../include $(foreach LIB,$(LIBS),-I$(LIB)) \
		$(SRCS) $(LIB_SRCS)

emulator:
	$(COMPILE_COMMAND)

compile_commands:
	@echo $(COMPILE_COMMAND)

clean:
	rm -f emulator *.o *.dSYM

.PHONY: clean emulator compile_commands
