.PHONY: all
.PHONY: clean

TARGET = aesdsocket
BUILD_DIR = ./build
INCLUDES = -I./include
SOURCES = $(wildcard src/*.c)
OBJS = $(SOURCES:%=$(BUILD_DIR)/%.o)
LDFLAGS += -lpthread -lrt

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean: 
	$(RM) $(TARGET)
	$(RM) -r $(BUILD_DIR)
