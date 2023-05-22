# Minecraft 1.20 Loot Table Seed Cracker

The code I'm using to crack seeds with specific loot, thanks to the new 1.20 loot table system. This is not user friendly at all, but thought I'd share.

This project has two different entrypoint files, `main.c` which is a very standard multithreaded implimentation of the search, and `main_simd.c` which is a far more impressive SIMD implimentation, which works 2-3x faster than the "simple" version. 

I plan on looking into a GPU computer-shader driven implimentation of this algorithm too, not sure how much faster it will end up being, though.

Thanks to Matthew Bolan for the math behind the algorithm, and thank you to the sample code [here](https://gist.github.com/mjtb49/f3e01e3355178d2bb6c814606971c374).