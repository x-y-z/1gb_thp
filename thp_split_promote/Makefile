
CC=gcc

split_thp: split_thp.c
	$(CC) -o $@ $^
	sudo setcap "all=ep" $@

split_1gb_thp: split_1gb_thp.c
	$(CC) -o $@ $^
	sudo setcap "all=ep" $@

clean:
	rm split_thp split_1gb_thp