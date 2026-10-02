CC      ?= cc
CFLAGS  ?= -Wall -Wextra -Werror -O2
SRC      = test_rgp_logic.c
BIN      = test_rgp_logic

all: run

$(BIN): $(SRC) ../rgp_logic.h
	$(CC) $(CFLAGS) -o $@ $<

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)

.PHONY: all run clean
