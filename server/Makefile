.PHONY: all
.PHONY: clean

TARGET = aesdsocket

all: $(TARGET)

$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) $< -o $@ $(LDFLAGS)

clean: 
	$(RM) $(TARGET)
