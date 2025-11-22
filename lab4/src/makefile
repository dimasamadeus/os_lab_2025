CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread

all: task1_makefile task3_makefile parallel_sum

# Сборка process_memory
task1_makefile: process_memory.c
	$(CC) $(CFLAGS) -o task1_makefile process_memory.c

# Сборка parallel_min_max
task3_makefile: ../../lab3/src/parallel_min_max_time.c ../../lab3/src/find_min_max.c ../../lab3/src/utils.c
	$(CC) $(CFLAGS) -o task3_makefile ../../lab3/src/parallel_min_max_time.c ../../lab3/src/find_min_max.c ../../lab3/src/utils.c

# Сборка parallel_sum
parallel_sum: parallel_sum.c sum_lib.c utils.c
	$(CC) $(CFLAGS) -o parallel_sum parallel_sum.c sum_lib.c utils.c

# Очистка всех бинарников
clean:
	rm -f task1_makefile task3_makefile parallel_sum

.PHONY: all clean