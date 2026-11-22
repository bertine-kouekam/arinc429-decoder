CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Werror -pedantic -Wconversion -Wshadow -O1 -Isrc
BUILD   := build

.PHONY: all test clean

all: $(BUILD)/test_arinc429

$(BUILD)/test_arinc429: src/arinc429.c src/arinc429.h tests/test_arinc429.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) src/arinc429.c tests/test_arinc429.c -o $@

test: $(BUILD)/test_arinc429
	./$(BUILD)/test_arinc429

clean:
	rm -rf $(BUILD)
