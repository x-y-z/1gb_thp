#include <stdio.h>
#include <stdlib.h>
#include "numa.h"
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <malloc.h>
#include <getopt.h>

#define PAGE_4KB (4096UL)
#define PAGE_2MB (512UL*PAGE_4KB)
#define PAGE_1GB (512UL*PAGE_2MB)

#define PRESENT_MASK (1UL<<63)
#define SWAPPED_MASK (1UL<<62)
#define PAGE_TYPE_MASK (1UL<<61)
#define PFN_MASK     ((1UL<<55)-1)

#define KPF_THP      (1UL<<22)

#define MADV_SPLITHUGEPAGE 24
#define MADV_PROMOTEHUGEPAGE 25

#define MADV_SPLITHUGEMAP 26
#define MADV_PROMOTEHUGEMAP 27

void print_paddr_and_flags(char *bigmem, int pagemap_file, int kpageflags_file)
{
	uint64_t paddr;
	uint64_t page_flags;

	if (pagemap_file) {
		pread(pagemap_file, &paddr, sizeof(paddr), ((long)bigmem>>12)*sizeof(paddr));


		if (kpageflags_file) {
			pread(kpageflags_file, &page_flags, sizeof(page_flags), 
				(paddr & PFN_MASK)*sizeof(page_flags));

			fprintf(stderr, "vpn: 0x%lx, pfn: 0x%lx is %s %s, %s, %s\n",
				((long)bigmem)>>12,
				(paddr & PFN_MASK),
				paddr & PAGE_TYPE_MASK ? "file-page" : "anon",
				paddr & PRESENT_MASK ? "there": "not there",
				paddr & SWAPPED_MASK ? "swapped": "not swapped",
				page_flags & KPF_THP ? "thp" : "not thp"
				/*page_flags*/
				);

		}
	}



}

int main(int argc, char** argv)
{
	char *one_page;
	size_t len = PAGE_2MB;
	const char *pagemap_proc = "/proc/self/pagemap";
	const char *kpageflags_proc = "/proc/kpageflags";
	int pagemap_fd = 0;
	int kpageflags_fd = 0;
	size_t i;
	static int split_page = 0;
	static int split_map = 0;
	int option_index = 0;
	int c;
	static struct option long_options [] =
	{
		{"split_page", no_argument, &split_page, 1},
		{"split_map", no_argument, &split_map, 1},
		{0,0,0,0}
	};

	while ((c = getopt_long(argc, argv, "",
							long_options, &option_index)) != -1) 
	{
		switch (c)
		{
			case 0:
				 /* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
				printf ("option %s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
					printf ("\n");
				break;
			case '?':
				return 1;
			default:
				abort();
		}
	}

	pagemap_fd = open(pagemap_proc, O_RDONLY);

	if (pagemap_fd == -1)
	{
		perror("read pagemap:");
		exit(-1);
	}

	kpageflags_fd = open(kpageflags_proc, O_RDONLY);

	if (kpageflags_fd == -1)
	{
		perror("read kpageflags:");
		exit(-1);
	}


	one_page = memalign(PAGE_1GB, len);

	if (madvise(one_page, len, MADV_HUGEPAGE)) {
		perror("madvise hugepage failed");
		exit(-1);
	}

	memset(one_page, 1, len);

	for (i = 0; i < len; i += PAGE_4KB)
		print_paddr_and_flags(one_page + i, pagemap_fd, kpageflags_fd);

	if (split_map) {
		if (madvise(one_page, len, MADV_SPLITHUGEMAP)) {
			perror("madvise split huge map failed");
			exit(-1);
		}
		printf("---------after split map---------\n");
	}

	if (split_page) {
		if (madvise(one_page, len, MADV_SPLITHUGEPAGE)) {
			perror("madvise split huge map failed");
			exit(-1);
		}
		printf("---------after split page---------\n");
	}

	for (i = 0; i < len; i += PAGE_4KB)
		print_paddr_and_flags(one_page + i, pagemap_fd, kpageflags_fd);

	if (pagemap_fd)
		close(pagemap_fd);
	if (kpageflags_fd)
		close(kpageflags_fd);


	return 0;
}