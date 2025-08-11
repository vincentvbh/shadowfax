
RAND_PATH   = ../randombytes
HASH_PATH   = ../hash

CC          = gcc

CFLAGS      = -Wall -Wextra -Wshadow -Wundef -O3 -I.
CFLAGS     += -I$(RAND_PATH) -I$(HASH_PATH)

HEADERS     = $(wildcard *.h)
HEADERS    += $(wildcard $(RAND_PATH)/*.h)
HEADERS    += $(wildcard $(HASH_PATH)/*.h)

SOURCES     = $(filter-out test.c, $(wildcard *.c))
SOURCES    += $(wildcard $(RAND_PATH)/*.c)
SOURCES    += $(wildcard $(HASH_PATH)/*.c)

OBJS        = $(patsubst %.c, %.o, $(SOURCES))

LIB         = libmlkem.a
LIB_NAME    = mlkem

all: test

%.o: %.c $(HEADERS)

.PRECIOUS: $(OBJS) $(LIB) test

$(LIB): $(OBJS)
	$(AR) -r $@ $(OBJS)


test: test.c $(LIB)
	$(CC) $(CFLAGS) -L . -o $@ $< -l$(LIB_NAME)

.PHONY: clean

clean:
	-rm -f $(OBJS)
	-rm -f $(LIB)
	-rm -f test






