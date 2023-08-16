.PHONY: all

TARGET = writer

all: $(TARGET)

$(TARGET): writer.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) $< -o $@ $(LDFLAGS)

clean:
	$(RM) $(TARGET)

