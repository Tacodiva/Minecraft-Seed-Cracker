
simd:
	gcc -march=native -lpthread -lcrypto -Ofast -o bin/ltrng main_simd.c

main:
	gcc -march=native -lpthread -lcrypto -Ofast -o bin/ltrng main.c

vk:
	gcc -march=native -Ofast -lcrypto -lvulkan -o bin/ltrng main_vk.c

shader:
	glslc -I shaders shaders/shader.comp -o shaders/shader.spv