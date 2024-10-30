
RAND_PATH   = ../randombytes
HASH_PATH   = ../hash
NGEN_PATH   = ../ntru_gen

CC          = gcc

CFLAGS      = -Wall -Wextra -Wshadow -Wundef -O3 -I$(RAND_PATH) -I$(HASH_PATH) -I$(NGEN_PATH)

HEADERS     = $(wildcard *.h)
HEADERS    += $(wildcard $(RAND_PATH)/*.h)
HEADERS    += $(HASH_PATH)/blake.h
HEADERS    += $(wildcard $(NGEN_PATH)/*.h)

SOURCES     = $(filter-out test.c modgen257.c modgen769.c modgen64513.c, $(wildcard *.c))
SOURCES    += $(wildcard $(RAND_PATH)/*.c)
SOURCES    += $(HASH_PATH)/blake2b.c $(HASH_PATH)/blake2s.c
SOURCES    += $(wildcard $(NGEN_PATH)/*.c)

OBJS        = $(patsubst %.c, %.o, $(SOURCES))

LIB         = libbat.a
LIB_NAME    = bat

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






