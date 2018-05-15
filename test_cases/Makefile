
CC=gcc

split_1gb_page: split_1gb_page.c
	$(CC) -o $@ $^
	sudo setcap "all=ep" $@

mprotect_4kb_from_1gb: ./mprotect_4kb_from_1gb.c
	$(CC) -o $@ $^
	sudo setcap "all=ep" $@

mprotect_multi_map: ./mprotect_multi_map.c
	$(CC) -o $@ $^
	sudo setcap "all=ep" $@

clean:
	rm split_1gb_page