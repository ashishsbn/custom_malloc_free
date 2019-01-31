CC = gcc                                # C compiler
CFLAGS = -fPIC -Wall -Wextra -g         # C flags
LDFLAGS = -shared                       # linking flags
RM = rm -f                              # rm command
TARGET_LIB = libs/libmymalloc.so                 # target lib

SRCS = src/malloc.c               # source file
DEPS = include/malloc.h                         # header file
OBJS = $(SRCS:.c=.o)                    # object file

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^       # -o $@ says, put the output of the compilation in the file named on the left side of the :

$(SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@      # the $< is the first item in the dependencies list, and the CFLAGS macro is defined as above
include $(SRCS:.c=.d)

.PHONY: clean
clean:
	-${RM} ${TARGET_LIB} ${OBJS} $(SRCS:.c=.d)
