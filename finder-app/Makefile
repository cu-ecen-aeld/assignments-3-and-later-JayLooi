.PHONY: all

TARGET = writer

cc = $(CROSS_COMPILE)gcc

all: $(TARGET)

$(TARGET): writer.c
	$(cc) $< -o $@

clean:
	rm $(TARGET)

