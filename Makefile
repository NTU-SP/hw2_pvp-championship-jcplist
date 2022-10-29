all:
	gcc -std=gnu11 battle.c -o battle
	gcc -std=gnu11 player.c -o player
clean:
	rm battle player log* *fifo
