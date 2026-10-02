CC      ?= cc
CFLAGS  ?= -Wall -Wextra -Werror -O2
SRC      = test_mode_logic.c
BIN      = test_mode_logic

all: run

$(BIN): $(SRC) ../mode_logic.h
	$(CC) $(CFLAGS) -o $@ $<

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)

.PHONY: all run clean
