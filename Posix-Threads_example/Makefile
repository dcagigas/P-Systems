CC=gcc
CXXFLAGS= -fopenmp -lpthread -static -O3
CXXFLAGS2= -c -fopenmp -lpthread -O3

EXE=main

all: $(EXE)

p-systems_lib.o: p-systems_lib.c
	$(CC) $(CXXFLAGS2) p-systems_lib.c 

main: p-systems_lib.o
	$(CC) -o p-system_example  p-system_example.c p-systems_lib.o $(CXXFLAGS)

clean:
	rm -rf p-system_example   *.o

