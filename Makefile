CC      = cc
SOURCES = sl.c sl_lang.c
OBJECTS = $(SOURCES:.c=.o)
TARGET  = sl

CFLAGS  = -g
LDFLAGS = 

.PHONY: all clean 

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
