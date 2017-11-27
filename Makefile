bins = sender1 receiver1

all: sender1 receiver1
sender1: sender1.c
	gcc sender1.c -o sender1
receiver1: receiver1.c
	gcc receiver1.c -o receiver1

clean:
	rm -f $(bins)
