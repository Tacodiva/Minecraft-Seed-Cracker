# Minecraft 1.20 Loot Table Seed Finder

The code I'm using to find seeds with specific drop from loot table, thanks to the changes made in 1.20. This is not user friendly at all, but thought I'd share.

Seeds like 3078515690265 can be found with this. In that seed on 1.20-pre4, the first 19 melons broken will yield the maximum number of melon slices.

This project has three different entrypoint files, `main.c` which is a very standard multithreaded implimentation of the search, and `main_simd.c` which is a far more impressive SIMD implimentation, which works 2-3x faster than the "simple" version and `main_vk.c` which is a GPU Vulkan computer-shader implimentation. 

Thanks to Matthew Bolan for the math behind the algorithm, and to [this sample implimentation of said algorithm](https://gist.github.com/mjtb49/f3e01e3355178d2bb6c814606971c374).
