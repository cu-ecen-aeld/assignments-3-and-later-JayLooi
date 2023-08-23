.PHONY: all
.PHONY: clean

TARGET = aesdsocket

$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) $< -o $@ $(LDFLAGS)

clean: 
	$(RM) $(TARGET)
