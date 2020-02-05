# Instructions
# 
# TARGETS and PLATFORMS
# 
# The default TARGET is set to DEVEL with all compiler warnings cranked up to 
# the absolute maximum. 
# This should be the default mode used during normal development / debug cycle.
#
# RELEASE is for the final binary release.
# It is also a good practice to strip
# the resulting binaries using: 'strip --strip-unneeded <binary>'
# 
# Profiled builds are a bit trickier, one has to compile twice:
# 1. first compile with 'TARGET := $(GENERATE_PROFILE)' and -lgcov linker flag
# 2. run your application to collect peformance samples (.gcda files in the 
# build directory)
# 3. run 'make clean' to delete old .o object files
# 4. now run 'make' again with 'TARGET := $(PROFILED_RELEASE)' to compile a new 
# binary using .gcda files for extra level of optimization
# Warning! The default flags -march=native and -march=tune will likely make the
# binary non backwards compatible CPU-wise, but this will squeeze out the best 
# performance for the particular machine we are building on.
# 
# PLATFORM flag can be used to group platform specific flags (Windows, Mac).
# Note. The easiest way to setup GNU-like programming environment on Windows is
# to install Git for Windows (which comes with bash terminal) and seperately the
# gcc compiler, either MinGW or tdm-gcc from sourceforge.
# 
# Static Code Analysis
# This target requires 'infer', 'flawfinder' and 'cppcheck' tools on your PATH.

#
# name of the resulting binary file
#
BUILD_ARTIFACT := inv

#
# build toolchain
#
SHELL := /usr/bin/env bash
CXX := g++

#
# TARGET can be only one of the below
#
DEVEL := 1
RELEASE := 2
GENERATE_PROFILE := 3
PROFILED_RELEASE := 4
#
# select one only at a time
#
TARGET := $(DEVEL)
# TARGET := $(RELEASE)
# TARGET := $(GENERATE_PROFILE)
# TARGET := $(PROFILED_RELEASE)

#
# PLATFORM can be only one of the below
#
WINDOWS := 1
MACOS := 2
LINUX := 3
#
# select one only at a time
#
# PLATFORM := WINDOWS
# PLATFORM := MACOS
PLATFORM := LINUX

#
# -I, -D preprocessor options
#
CPPFLAGS := -I.
ifneq ($(TARGET), $(DEVEL))
	CPPFLAGS += -DNDEBUG
endif

#
# debugging and optimization options for the C++ compiler
#
ifeq ($(TARGET), $(DEVEL))
	# language standard
	CXXFLAGS := -std=c++14 -pedantic -pipe
	# extra level of compile time checks
	CXXFLAGS += -Wall -Wextra -Weffc++
	CXXFLAGS += -Wduplicated-cond -Wduplicated-branches -Wrestrict
	CXXFLAGS += -Wnull-dereference -Wjump-misses-init
	CXXFLAGS += -Wmissing-declarations -Wodr -Wold-style-cast -Wuseless-cast
	CXXFLAGS += -Wlogical-op -Wdouble-promotion -Wshadow -Wformat=2
	CXXFLAGS += -Winvalid-pch -Wmissing-include-dirs
	CXXFLAGS += -Wredundant-decls -Wswitch-default -Wswitch-enum
	CXXFLAGS += -Wcast-align -Wconversion -Wcast-qual -Wmissing-prototypes
	# debugger experience
	CXXFLAGS += -Og -g3 -fno-omit-frame-pointer -fno-strict-aliasing
	CXXFLAGS += -fno-optimize-sibling-calls -fasynchronous-unwind-tables
	# code hardening
    CPPFLAGS += -D_FORTIFY_SOURCE=2 -D_GLIBCXX_ASSERTIONS
	CXXFLAGS += -fPIE -pie -Wl,-pie -Wl,-z,defs -Wl,-z,now -Wl,-z,relro
    CXXFLAGS += -fstack-protector-strong
	# sanitizers for dynamic checks
	CXXFLAGS += -fsanitize=address
	CXXFLAGS += -fsanitize=leak
	CPPFLAGS += -D_GLIBCXX_SANITIZE_VECTOR
	# CXXFLAGS += -fsanitize=pointer-compare -fsanitize=pointer-subtract
    # run with: ASAN_OPTIONS=detect_invalid_pointer_pairs=2 detect_stack_use_after_return=1: ./a.out
	CXXFLAGS += -fsanitize=undefined
	# CXXFLAGS += -fsanitize=thread
	# CXXFLAGS += -fsanitize=memory
	CXXFLAGS += -fno-sanitize-recover
else ifeq ($(TARGET), $(RELEASE))
	CXXLAGS := -O2
	# CXXLAGS += -fopt-info-vec-all
	# CXXLAGS += -Q --help=target --help=optimizers
else ifeq ($(TARGET), $(GENERATE_PROFILE))
    CXXFLAGS := -Ofast -march=native -mtune=native -flto -fprofile-generate -pipe
else ifeq ($(TARGET), $(PROFILED_RELEASE))
    CXXFLAGS := -Ofast -march=native -mtune=native -flto -fprofile-use -pipe
	# CXXFLAGS += -fprofile-correction
endif

#
# -L options for the linker
#
LDFLAGS := -L.

#
# -l options to pass to the linker
#
LDLIBS = -lSDL2
ifeq ($(TARGET), $(DEVEL))
	LDLIBS += -fsanitize=address -static-libasan
	LDLIBS += -fsanitize=undefined -static-libubsan
endif
ifeq ($(TARGET),$(filter $(TARGET),$(GENERATE_PROFILE) $(PROFILED_RELEASE)))
	LDLIBS += -lgcov
endif

#
# linking
#
OBJECTS := main.o
all: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(BUILD_ARTIFACT)

#
# compiling
#
main.o: main.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o $(BUILD_ARTIFACT)

very-clean:
	rm -f *.o *.gcda $(BUILD_ARTIFACT)

static-analysis:
	flawfinder *.c *.cpp *.h *.hpp
	cppcheck $(CPPFLAGS) *.c *.cpp *.h *.hpp
	infer run -- gcc -c *.cpp
