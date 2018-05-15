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
#include <sys/wait.h>
#include <malloc.h>

#define PAGE_4KB (4096UL)
#define PAGE_2MB (512UL*PAGE_4KB)
#define PAGE_1GB (512UL*PAGE_2MB)

#define PRESENT_MASK (1UL<<63)
#define SWAPPED_MASK (1UL<<62)
#define PAGE_TYPE_MASK (1UL<<61)
#define PFN_MASK     ((1UL<<55)-1)

#define KPF_ANON     (1UL<<12)
#define KPF_THP      (1UL<<22)
#define KPF_PUD_THP      (1UL<<26)

void print_paddr_and_flags(char *bigmem, int pagemap_file, int kpageflags_file, char *msg)
{
	uint64_t paddr;
	uint64_t page_flags;

	if (pagemap_file) {
		pread(pagemap_file, &paddr, sizeof(paddr), ((long)bigmem>>12)*sizeof(paddr));


		if (kpageflags_file) {
			pread(kpageflags_file, &page_flags, sizeof(page_flags), 
				(paddr & PFN_MASK)*sizeof(page_flags));

			fprintf(stderr, "%s: vpn: 0x%lx, pfn: 0x%lx is %s %s, %s, %s\n",
				msg,
				((long)bigmem)>>12,
				(paddr & PFN_MASK),
				paddr & PAGE_TYPE_MASK ? "file-page" : "anon",
				/*page_flags & KPF_ANON ? "anon":"file-page",*/
				paddr & PRESENT_MASK ? "there": "not there",
				paddr & SWAPPED_MASK ? "swapped": "not swapped",
				page_flags & KPF_PUD_THP? "1gb-thp":(page_flags & KPF_THP ? "thp" : "not thp")
				/*page_flags*/
				);

		}
	}



}

int main(int argc, char** argv)
{
	char *one_page;
	size_t len = 2*PAGE_1GB;
	const char *pagemap_proc = "/proc/self/pagemap";
	const char *kpageflags_proc = "/proc/kpageflags";
	int pagemap_fd = 0;
	int kpageflags_fd = 0;
	size_t i;
	pid_t child_pid;

	pagemap_fd = open(pagemap_proc, O_RDONLY);

	if (pagemap_fd == -1)
	{
		perror("parent read pagemap:");
		exit(-1);
	}

	kpageflags_fd = open(kpageflags_proc, O_RDONLY);

	if (kpageflags_fd == -1)
	{
		perror("read kpageflags:");
		exit(-1);
	}


	one_page = mmap(NULL, len, PROT_READ|PROT_WRITE,
		MAP_SHARED|MAP_ANONYMOUS, -1, PAGE_1GB);

	madvise(one_page, len, MADV_HUGEPAGE);

	memset(one_page, 1, len);

	for (i = 0; i < len; i += PAGE_2MB)
		print_paddr_and_flags(one_page + i, pagemap_fd, kpageflags_fd, "parent");
	getchar();

	printf("---------------before mprotect-------------\n");

	child_pid = fork();

	if (child_pid == 0) {
		const char *pagemap_child = "/proc/self/pagemap";

		pagemap_fd = open(pagemap_child, O_RDONLY);

		if (pagemap_fd == -1)
		{
			perror("child read pagemap:");
			exit(-1);
		}
		memset(one_page, 1, len);
		for (i = 0; i < 2*PAGE_2MB; i += PAGE_4KB)
			print_paddr_and_flags(one_page + i, pagemap_fd, kpageflags_fd, "child-pre");
		if (mprotect(one_page, PAGE_4KB, PROT_READ))
			perror("mprotect failed");
		else {
			int one_page_out;
			for (i = 0; i < 2*PAGE_2MB; i += PAGE_4KB)
				one_page_out = *(one_page + i);

			for (i = 0; i < 2*PAGE_2MB; i += PAGE_4KB)
				print_paddr_and_flags(one_page + i, pagemap_fd, kpageflags_fd, "child");
		}
	} else {
		wait(NULL);
		/*for (i = 0; i < 2*PAGE_2MB; i += PAGE_4KB)*/
			/*print_paddr_and_flags(one_page + i, pagemap_fd, kpageflags_fd, "parent");*/
	}

	if (pagemap_fd)
		close(pagemap_fd);
	if (kpageflags_fd)
		close(kpageflags_fd);

	return 0;
}