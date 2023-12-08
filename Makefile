CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lraylib -lGL -lm -lpthread -ldl -lrt
SRC_DIR=src
BUILD_DIR=build
TARGET=raylon

all: $(TARGET)

$(TARGET): $(BUILD_DIR)/raylon.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/raylon.o: $(SRC_DIR)/raylon.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

run: all
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
