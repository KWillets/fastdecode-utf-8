test:  test.c utf8_code.h utils.h shuffle_table.h
	gcc -o test test.c

benchmark: benchmarks/benchmark.c utf8_code.h shuffle_table.h
	gcc -std=c99 -march=native -O3 -Wall -o benchmark benchmarks/benchmark.c -I. -Ibenchmarks -D_GNU_SOURCE

