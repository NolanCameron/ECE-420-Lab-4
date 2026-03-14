CC = gcc
CFLAGS = -Wall -Werror -Wvla -ggdb3 -lm

DEPS = timer.h Lab3IO.h
OBJ = main_serial.o Lab4_IO.o


main_serial: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
all: datagen main_serial

.PHONY: datatrim
datatrim:
	gcc datatrim.c -o datatrim


.PHONY: memtest
memtest: main 
	valgrind -s --track-origins=yes --tool=memcheck --leak-check=yes --show-leak-kinds=all ./main 1
	
.PHONY: threadtest
threadtest: main
	valgrind --tool=helgrind ./main 4

.PHONY: clean
clean:
	rm -f *.o main datagen