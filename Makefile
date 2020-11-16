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

clean:
	rm -rf *.o *.a
	rm -f test
	rm -f my_vm

something:
	$(CC) -g -m32 -o my_vm my_vm.c -lm