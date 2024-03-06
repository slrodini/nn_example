WAR_FLAGS="-Wall -Wextra -Wno-unused-result -Wno-unused-function"
clang-16 -o main.elf main.c adam.c annealing.c minimize.c multilayer.c read_config.c utils.c ran2.c chi2.c -I./ -lm -fopenmp -march=native -ftree-vectorize -O3 ${WAR_FLAGS}
#-pg
