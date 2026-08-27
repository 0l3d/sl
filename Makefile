CC      = cc
SOURCES = sl.c sl_lang.c
OBJECTS = $(SOURCES:.c=.o)
TARGET  = sl

CFLAGS =
LDFLAGS = 

.PHONY: all clean 

all: $(TARGET)

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./sl

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
