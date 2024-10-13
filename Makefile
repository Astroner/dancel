CC=gcc-14
SOURCES=$(wildcard src/*.c)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=run

.PHONY: all
all: $(EXECUTABLE)
	./$(EXECUTABLE) $(EFLAGS)

$(EXECUTABLE): $(OBJECTS)
	$(CC) -o $(EXECUTABLE) $(CFLAGS) $^

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -Wall -Wextra -std=c99 -pedantic -c -o $@ $<


Dancel.h: src/dancel.h src/dancel.c
	cat src/dancel.h > Dancel.h
	echo "#if defined(DANCEL_IMPLEMENTATION)" >> Dancel.h
	tail -n +2 src/dancel.c >> Dancel.h
	echo "#endif // DANCEL_IMPLEMENTATION" >> Dancel.h

lib: Dancel.h

.PHONY: tests
tests: lib
	$(CC) -Wall -Wextra -std=c99 -pedantic -o run-tests tests/main.spec.c
	./run-tests
	rm run-tests

.PHONY: clean
clean:
	rm -f $(EXECUTABLE) $(OBJECTS) test
