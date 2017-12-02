bins = sender1 receiver1 sender2 receiver2 sender3 receiver3

all: sender1 receiver1 sender2 receiver2 sender3 receiver3
sender1: sender1.c
	gcc sender1.c -o sender1
receiver1: receiver1.c
	gcc receiver1.c -o receiver1
sender2: sender2.c
	gcc sender2.c -o sender2
receiver2: receiver2.c
	gcc receiver2.c -o receiver2
sender3: sender3.c
	gcc sender3.c -o sender3
receiver3: receiver3.c
	gcc receiver3.c -o receiver3

clean:
	rm -f $(bins)
