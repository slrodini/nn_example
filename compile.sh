WAR_FLAGS="-Wall -Wextra -Wno-unused-result -Wno-unused-function"
gcc-13 -o main.elf main.c adam.c annealing.c minimize.c multilayer.c read_config.c utils.c ran2.c chi2.c -I./ -lm -O3 ${WAR_FLAGS}
