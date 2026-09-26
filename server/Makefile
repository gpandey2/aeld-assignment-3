CC      := $(CROSS_COMPILE)gcc
CFLAGS  := -Wall 

TARGET  := aesdsocket
SRCS    := aesdsocket.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): 
	$(CC) $(CFLAGS) $(SRCS) -o $@

clean:
	rm -f $(TARGET)
