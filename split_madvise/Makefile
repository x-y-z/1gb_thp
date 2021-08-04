CC=gcc

split_1gb_page: split_1gb_page.c
	$(CC) -o $@ $^
	sudo setcap "all=ep" $@

%: %.c
	$(CC) -o $@ $^
	sudo setcap "all=ep" $@

clean:
	rm split_1gb_page