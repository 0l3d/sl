CC      = cc
SOURCES = sl.c sl_lang.c
OBJECTS = $(SOURCES:.c=.o)
TARGET  = sl

#CFLAGS = -g -O0
CFLAGS := -O3 -DNDEBUG -flto -ffast-math -fno-math-errno -fno-trapping-math -fno-signed-zeros -ffp-contract=fast
LDFLAGS := -flto

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

