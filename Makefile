CC      = cc
SOURCES = sl.c sl_lang.c
OBJECTS = $(SOURCES:.c=.o)
TARGET  = sl

CFLAGS = -g
# CFLAGS := -O3 -DNDEBUG -flto -ffast-math -fno-math-errno -fno-trapping-math -fno-signed-zeros -ffp-contract=fast
LDFLAGS := -flto

.PHONY: all clean 

all: $(TARGET)

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./sl $(filter-out $@,$(MAKECMDGOALS))

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
