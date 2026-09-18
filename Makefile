CC      = musl-gcc
SOURCES = sl.c sl_lang.c
OBJECTS = $(SOURCES:.c=.o)
TARGET  = sl

# CFLAGS = -g -O0 # debug 
CFLAGS  := -O3 -DNDEBUG -flto -fno-strict-aliasing -fvisibility=hidden # standart release 
# CFLAGS  := -O3 -DNDEBUG -fno-strict-aliasing # MUSL Release
# LDFLAGS := -O3 -flto -lm -static # MUSL
LDFLAGS := -O3 -flto -lm # STANDART 

ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
endif

.PHONY: all clean valgrind

all: $(TARGET)

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./sl $(filter-out $@,$(MAKECMDGOALS))

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

