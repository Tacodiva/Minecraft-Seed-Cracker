
simd:
	gcc -march=native -lpthread -lcrypto -Ofast -o bin/ltrng main_simd.c

main:
	gcc -march=native -lpthread -lcrypto -Ofast -o bin/ltrng main.c
