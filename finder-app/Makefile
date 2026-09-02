CC      := $(CROSS_COMPILE)gcc
CFLAGS  := -Wall 

TARGET  := writer
SRCS    := writer.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): 
	$(CC) $(CFLAGS) $(SRCS) -o $@

clean:
	rm -f $(TARGET)
