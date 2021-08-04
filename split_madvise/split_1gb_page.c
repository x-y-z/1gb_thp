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

#define KPF_THP      (1UL<<22)
#define KPF_PUD_THP      (1UL<<27)

#define MAP_HUGE_1GB (30UL << 26)

void print_paddr_and_flags(char *bigmem, int pagemap_file, int kpageflags_file)
{
        uint64_t paddr;
        uint64_t page_flags;

        if (pagemap_file) {
                pread(pagemap_file, &paddr, sizeof(paddr), ((long)bigmem>>12)*sizeof(paddr));


                if (kpageflags_file) {
                        pread(kpageflags_file, &page_flags, sizeof(page_flags),
                                (paddr & PFN_MASK)*sizeof(page_flags));

                        fprintf(stderr, "vpn: 0x%lx, pfn: 0x%lx is %s %s, %s, %s, %s\n",
                                ((long)bigmem)>>12,
                                (paddr & PFN_MASK),
                                paddr & PAGE_TYPE_MASK ? "file-page" : "anon",
                                paddr & PRESENT_MASK ? "there": "not there",
                                paddr & SWAPPED_MASK ? "swapped": "not swapped",
                                page_flags & KPF_THP ? "thp" : "not thp",
                                page_flags & KPF_PUD_THP ? "pud_thp" : "not pud_thp"
                                /*page_flags*/
                                );

                }
        }



}

int main(int argc, char** argv)
{
        char *one_page;
        size_t len = 2*PAGE_1GB;
        /* size_t len = 2*PAGE_2MB; */
        const char *pagemap_template = "/proc/%d/pagemap";
        const char *kpageflags_proc = "/proc/kpageflags";
        char pagemap_proc[128];
        int pagemap_fd = 0;
        int kpageflags_fd = 0;
        size_t i;
        size_t stride = PAGE_2MB;
        size_t stride_len = 3*stride;
        /* size_t stride = PAGE_4KB; */
        /* size_t stride_len = 3*stride; */
	printf("pid: %d\n", getpid());

        sprintf(pagemap_proc, pagemap_template, getpid());
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

        madvise(one_page, len, MADV_HUGEPAGE | MAP_HUGE_1GB);

        printf("%d\n", (int)(*one_page));

        memset(one_page, 1, len);

        for (i = 0; i < stride_len; i += stride)
                print_paddr_and_flags(one_page + i, pagemap_fd, kpageflags_fd);

	getchar();

	/* madvise(one_page + stride, len - stride, MADV_DONTNEED); */
	madvise(one_page + PAGE_2MB, len - PAGE_2MB, MADV_DONTNEED);

        printf("after MADV_DONTNEED\n");

        *one_page = 0;

        for (i = 0; i < stride_len; i += stride)
                print_paddr_and_flags(one_page + i, pagemap_fd, kpageflags_fd);

        getchar();

        for (i = 0; i < stride_len; i += stride)
                print_paddr_and_flags(one_page + i, pagemap_fd, kpageflags_fd);

        if (pagemap_fd)
                close(pagemap_fd);
        if (kpageflags_fd)
                close(kpageflags_fd);


        return 0;
}