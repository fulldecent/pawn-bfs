CFLAGS+= -Wall -O3 -g
#INCLUDE=-I/opt/local/include 
#LINK=-L/opt/local/lib

all: ub

%: %.c common.c
	gcc $(CFLAGS) -o $@ $+ $(INCLUDE) $(LINK) -lgmp

%: %.cxx common.c
	gcc $(CFLAGS) -o $@ $+ $(INCLUDE) $(LINK) -lgmp

clean:
	rm -rf *.o *.dSYM

#     foo : foo.c -lcurses
#:             cc $^ -o $@
