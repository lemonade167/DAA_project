CC      = gcc
CFLAGS  = -std=c17 -O2 -Wall -Wextra -pedantic
LIB_SRC = core.c algo_dc.c algo_greedy.c algo_dp.c algo_bt.c algo_flow.c algo_str.c domain.c
LIB_OBJ = $(LIB_SRC:.c=.o)

all: parking tests

parking: main.o $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

tests: tests.o $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c parking.h
	$(CC) $(CFLAGS) -c $< -o $@

test: tests
	./tests

bench: parking
	./parking --bench

clean:
	rm -f *.o parking tests parking_lot.csv
