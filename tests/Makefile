CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic

TEST_BIN := test_walk_mode

.PHONY: all run clean

all: $(TEST_BIN)

$(TEST_BIN): test_walk_mode.c ../src/cycleiq_protocol.c \
		../include/cycleiq_protocol.h
	$(CC) $(CFLAGS) -I../include test_walk_mode.c \
		../src/cycleiq_protocol.c -o $(TEST_BIN)

run: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(TEST_BIN)
