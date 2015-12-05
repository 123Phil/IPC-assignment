all:
	gcc sender.c -o sender
	gcc recv.c -o recv

sender:
	gcc sender.c -o sender

recv:
	gcc recv.c -o recv

signal:
	gcc send_sig.c -o send_sig
	gcc recv_sig.c -o recv_sig

clean:
	rm -f recv sender recv_sig send_sig recvfile
