CC = gcc
CFLAGS = -g -c -m32 -lm
AR = ar -rc
RANLIB = ranlib

all: my_vm.a test

my_vm.a: my_vm.o
	$(AR) libmy_vm.a my_vm.o
	$(RANLIB) libmy_vm.a

my_vm.o: my_vm.h

	$(CC) $(CFLAGS)  my_vm.c -lm

test: 
	$(CC) -g -m32 test.c -o test -L. -lmy_vm -lm

runbenchmark: clean my_vm.a
	gcc benchmark/test.c -L./ -lmy_vm -m32 -lm -o benchmark/test
	./benchmark/test

clean:
	rm -rf *.o *.a
	rm -f test
	rm -f my_vm
	rm -f benchmark/test

something:
	$(CC) -g -m32 -o my_vm my_vm.c -lm