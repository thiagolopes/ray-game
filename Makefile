CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lGL -lm -lpthread -ldl -lrt raylib/lib/libraylib.a
SRC_DIR=src
BUILD_DIR=build
TARGET=raylon

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
HDRS := $(wildcard $(SRC_DIR)/*.h)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

run: all
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
