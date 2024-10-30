
CC ?= gcc

CFLAGS = -O3 -Wall -mcpu=native -mtune=native

CYCL_PATH   = cycles

RAND_PATH   = randombytes
HASH_PATH   = hash
SYMM_PATH   = symmetric
NGEN_PATH   = ntru_gen
KEM_PATH    = BAT
RSIG_PATH   = Gandalf
DH_PATH     = dh

CFLAGS     += -I$(RAND_PATH) -I$(HASH_PATH) -I$(SYMM_PATH) -I$(NGEN_PATH) -I$(KEM_PATH) -I$(RSIG_PATH) -I$(DH_PATH)

CYCL_HEADER = $(wildcard $(CYCL_PATH)/*.h)
CYCL_SOURCE = $(wildcard $(CYCL_PATH)/*.c)

RAND_HEADER = $(wildcard $(RAND_PATH)/*.h)
RAND_SOURCE = $(wildcard $(RAND_PATH)/*.c)

HASH_HEADER = $(wildcard $(HASH_PATH)/*.h)
HASH_SOURCE = $(wildcard $(HASH_PATH)/*.c)

SYMM_HEADER = $(wildcard $(SYMM_PATH)/*.h)
SYMM_SOURCE = $(wildcard $(SYMM_PATH)/*.c)

NGEN_HEADER = $(wildcard $(NGEN_PATH)/*.h)
NGEN_SOURCE = $(wildcard $(NGEN_PATH)/*.c)

KEM_HEADER  = $(wildcard $(KEM_PATH)/*.h)
KEM_SOURCE  = $(filter-out $(KEM_PATH)/modgen.c $(KEM_PATH)/modgen257.c $(KEM_PATH)/modgen769.c $(KEM_PATH)/modgen64513.c $(KEM_PATH)/test.c, $(wildcard $(KEM_PATH)/*.c))

RSIG_HEADER = $(wildcard $(RSIG_PATH)/*.h)
RSIG_SOURCE = $(filter-out $(RSIG_PATH)/test.c, $(wildcard $(RSIG_PATH)/*.c))

DH_HEADER   = $(wildcard $(DH_PATH)/*.h)
DH_SOURCE   = $(filter-out $(DH_PATH)/test.c, $(wildcard $(DH_PATH)/*.c))

HEADERS     = $(RAND_HEADER) $(HASH_HEADER) $(SYMM_HEADER) $(NGEN_HEADER) $(KEM_HEADER) $(RSIG_HEADER) $(DH_HEADER)

# DH-AKEM

DH_AKEM_HEADERS    = dh_akem_api.h
DH_AKEM_HEADERS   += $(RAND_HEADER) $(HASH_HEADER) $(DH_HEADER)

DH_AKEM_SOURCES    = dh_akem.c
DH_AKEM_SOURCES   += $(RAND_SOURCE) $(HASH_SOURCE) $(DH_SOURCE)

DH_AKEM_CFLAGS     = $(CFLAGS)

DH_AKEM_OBJS       = $(patsubst %.c, %.o, $(DH_AKEM_SOURCES))

LIBDH              = libdh.a
LIBDH_NAME         = dh

# PQ-AKEM

PQ_AKEM_HEADERS    = pq_akem_api.h
PQ_AKEM_HEADERS   += $(RAND_HEADER) $(HASH_HEADER) $(SYMM_HEADER) $(NGEN_HEADER) $(KEM_HEADER) $(RSIG_HEADER)

PQ_AKEM_SOURCES    = pq_akem.c
PQ_AKEM_SOURCES   += $(RAND_SOURCE) $(HASH_SOURCE) $(SYMM_SOURCE) $(NGEN_SOURCE) $(KEM_SOURCE) $(RSIG_SOURCE)

PQ_AKEM_CFLAGS     = $(CFLAGS)

PQ_AKEM_OBJS       = $(patsubst %.c, %.o, $(PQ_AKEM_SOURCES))

LIBPQAKEM          = libpqakem.a
LIBPQAKEM_NAME     = pqakem

# Hybrid AKEM (Shadowfax)

H_AKEM_HEADERS     = h_akem_api.h
H_AKEM_HEADERS    += $(RAND_HEADER) $(HASH_HEADER) $(SYMM_HEADER) $(NGEN_HEADER) $(KEM_HEADER) $(RSIG_HEADER) $(DH_HEADER)

H_AKEM_SOURCES     = h_akem.c
H_AKEM_SOURCES    += $(RAND_SOURCE) $(HASH_SOURCE) $(SYMM_SOURCE) $(NGEN_SOURCE) $(KEM_SOURCE) $(RSIG_SOURCE) $(DH_SOURCE)

H_AKEM_CFLAGS     = $(CFLAGS)

H_AKEM_OBJS        = $(patsubst %.c, %.o, $(H_AKEM_SOURCES))

LIBHAKEM           = libhakem.a
LIBHAKEM_NAME      = hakem

all: test speed

test: test_dh_akem test_pq_akem test_h_akem

speed: speed_dh_akem speed_pq_akem speed_h_akem

%.o: %.c $(HEADERS)

.PRECIOUS: $(OBJS) test_dh_akem speed_dh_akem test_pq_akem speed_pq_akem test_h_akem speed_h_akem

$(LIBDH): $(DH_AKEM_OBJS)
	$(AR) -r $@ $(DH_AKEM_OBJS)

$(LIBPQAKEM): $(PQ_AKEM_OBJS)
	$(AR) -r $@ $(PQ_AKEM_OBJS)

$(LIBHAKEM): $(H_AKEM_OBJS)
	$(AR) -r $@ $(H_AKEM_OBJS)

test_dh_akem: test_dh_akem.c $(LIBDH)
	$(CC) $(DH_AKEM_CFLAGS) -L . -o $@ $< -l$(LIBDH_NAME)

speed_dh_akem: speed_dh_akem.c $(LIBDH) $(CYCL_HEADER) $(CYCL_SOURCE)
	$(CC) $(DH_AKEM_CFLAGS) -L . -I$(CYCL_PATH) $(CYCL_SOURCE) -o $@ $< -l$(LIBDH_NAME)

test_pq_akem: test_pq_akem.c $(LIBPQAKEM)
	$(CC) $(PQ_AKEM_CFLAGS) -L . -o $@ $< -l$(LIBPQAKEM_NAME) -lm

speed_pq_akem: speed_pq_akem.c $(LIBPQAKEM) $(CYCL_HEADER) $(CYCL_SOURCE)
	$(CC) $(PQ_AKEM_CFLAGS) -L . -I$(CYCL_PATH) $(CYCL_SOURCE) -o $@ $< -l$(LIBPQAKEM_NAME) -lm

test_h_akem: test_h_akem.c $(LIBHAKEM)
	$(CC) $(H_AKEM_CFLAGS) -L . -o $@ $< -l$(LIBHAKEM_NAME) -lm

speed_h_akem: speed_h_akem.c $(LIBHAKEM) $(CYCL_HEADER) $(CYCL_SOURCE)
	$(CC) $(H_AKEM_CFLAGS) -L . -I$(CYCL_PATH) $(CYCL_SOURCE) -o $@ $<  -l$(LIBHAKEM_NAME) -lm

.PHONY: clean

clean:
	rm -f test_dh_akem
	rm -f speed_dh_akem
	rm -f test_pq_akem
	rm -f speed_pq_akem
	rm -f test_h_akem
	rm -f speed_h_akem
	rm -f $(DH_AKEM_OBJS)
	rm -f $(PQ_AKEM_OBJS)
	rm -f $(H_AKEM_OBJS)
	rm -f $(LIBDH)
	rm -f $(LIBPQAKEM)
	rm -f $(LIBHAKEM)




