CC = mpicc
CFLAGS = -Wall -Werror -Wvla -ggdb3 -lm -g
DEPS = timer.h Lab3IO.h
OBJ = main.o Lab4_IO.o

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
all: main

main_serial: Lab4_IO.o main_serial.o
	gcc -o $@ $^ $(CFLAGS)

.PHONY: datatrim
datatrim:
	gcc datatrim.c -o datatrim


.PHONY: memtest
memtest: main 
	valgrind -s --track-origins=yes --tool=memcheck --leak-check=yes --show-leak-kinds=all ./main
	
.PHONY: threadtest
threadtest: main
	valgrind --tool=helgrind ./main

.PHONY: clean
clean:
	rm -f *.o main datatrim main_serial