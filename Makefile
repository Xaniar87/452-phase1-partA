# This is a sample Makefile for Phase 1. It provides the following targets 
# (assumes test source files are named testN.c)
# You may change this, e.g. to add new test cases, but keep in mind we will compile
# your phase1.c using our own copy of this file.
#
#	make		(makes libphase1.a and all tests)
#        make phase1     ditto
#
#	make testN 	(makes testN)
#	make testN.out	(runs testN and puts output in testN.out)
#	make tests	(makes all testN.out files, i.e. runs all tests)
#	make tests_bg	(runs all tests in the background)
#
#	make testN.v	(runs valgrind on testN and puts output in testN.v)
#	make valgrind	(makes all testN.v files, i.e. runs valgrind on all tests)
#
#	make clean	(removes all files created by this Makefile)

ifndef CS452_STUDENTS
	452-STUDENTS = $(HOME)/Dropbox/452-students
else
	452-STUDENTS = $(CS452_STUDENTS)
endif

# Set to version of USLOSS you want to use.
USLOSS_VERSION = 2.11

SRCS = phase1.c

# Add any tests here. If the test is named test0 then the source file is assumed to be test0.c.
TESTS = test0 test1 test2 test3 test4 test5 test6 test7 test8 test9 test10 test11 test12

# Change this if you want to change the arguments to valgrind.
VGFLAGS = --track-origins=yes --leak-check=full --max-stackframe=100000

# Change this if you need to link against additional libraries (probably not).
LIBS = -lusloss$(USLOSS_VERSION) -lphase1

CFLAGS += -Wall -g
CFLAGS += -DDEBUG

# You shouldn't need to change anything below here. 

TARGET = libphase1.a

INCLUDES = -I$(452-STUDENTS)/include -I.

ifeq ($(shell uname),Darwin)
	DEFINES += -D_XOPEN_SOURCE
	OS = macosx
	CFLAGS += -Wno-int-to-void-pointer-cast -Wno-extra-tokens -Wno-unused-label -Wno-unused-function
else
	OS = linux
	CFLAGS += -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-unused-but-set-variable
endif

CC=gcc
LD=gcc
AR=ar    
CFLAGS += $(INCLUDES) $(DEFINES)
LDFLAGS = -L$(452-STUDENTS)/lib/$(OS) -L.
COBJS = ${SRCS:.c=.o}
DEPS = ${COBJS:.o=.d}
TSRCS = {$TESTS:=.c}
TOBJS = ${TESTS:=.o}
TOUTS = ${TESTS:=.out}
TVS = ${TESTS:=.v}

# The following is to deal with circular dependencies between the USLOSS and phase1
# libraries. Unfortunately the linkers handle this differently on the two OSes.

ifeq ($(OS), macosx)
	LIBFLAGS = -Wl,-all_load $(LIBS)
else
	LIBFLAGS = -Wl,--start-group $(LIBS) -Wl,--end-group
endif

%.d: %.c
	$(CC) -c $(CFLAGS) -MM -MF $@ $<

all: phase1

phase1: $(TARGET) $(TESTS)


$(TARGET):  $(COBJS)
	$(AR) -r $@ $^

tests: $(TOUTS)

# Remove implicit rules so that "make phase1" doesn't try to build it from phase1.c or phase1.o
% : %.c

% : %.o

%.out: %
	./$< 1> $@ 2>&1

$(TESTS):   %: $(TARGET) %.o
	$(LD) $(LDFLAGS) -o $@ $@.o $(LIBFLAGS)

clean:
	rm -f $(COBJS) $(TARGET) $(TOBJS) $(TESTS) $(DEPS) $(TVS)

%.d: %.c
	$(CC) -c $(CFLAGS) -MM -MF $@ $<

valgrind: $(TVS)

%.v: %
	valgrind $(VGFLAGS) ./$< 1> $@ 2>&1

-include $(DEPS)
