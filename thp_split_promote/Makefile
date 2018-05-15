
CC=gcc

split_thp: split_thp.c
	$(CC) -o $@ $^
	sudo setcap "all=ep" $@

clean:
	rm split_thp