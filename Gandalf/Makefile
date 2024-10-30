
RAND_PATH   = ../randombytes
HASH_PATH   = ../hash
NGEN_PATH   = ../ntru_gen

CC          = gcc

CFLAGS      = -Wall -Wextra -march=native -O3 -I$(RAND_PATH) -I$(HASH_PATH) -I$(NGEN_PATH)

HEADERS     = $(wildcard *.h)
HEADERS    += $(wildcard $(RAND_PATH)/*.h)
HEADERS    += $(wildcard $(HASH_PATH)/*.h)
HEADERS    += $(wildcard $(NGEN_PATH)/*.h)

SOURCES     = $(filter-out test.c, $(wildcard *.c))
SOURCES    += $(wildcard $(RAND_PATH)/*.c)
SOURCES    += $(wildcard $(HASH_PATH)/*.c)
SOURCES    += $(wildcard $(NGEN_PATH)/*.c)

OBJS        = $(patsubst %.c, %.o, $(SOURCES))

LIB         = libgandalf.a
LIB_NAME    = gandalf

all: test

%.o: %.c $(HEADERS)

.PRECIOUS: $(OBJS) $(LIB) test

$(LIB): $(OBJS)
	$(AR) -r $@ $(OBJS)

test: test.c $(LIB)
	$(CC) $(CFLAGS) -L . -o $@ $< -l$(LIB_NAME) -lm

.PHONY: clean

clean:
	-rm -f $(OBJS)
	-rm -f $(LIB)
	-rm -f test

