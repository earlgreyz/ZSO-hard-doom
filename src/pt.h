#ifndef HARDDOOM_PT_H
#define HARDDOOM_PT_H

#include <linux/kernel.h>

#define DOOM_PAGE_SIZE  (4 << 10)
#define PT_MAX_ENTRIES      1024
#define PT_ALIGNMENT     (1 << 6)

struct pt_entry {
  int32_t valid :  1;
  int32_t       : 11;
  int32_t addr  : 20;
};

// Number of pages required to fit a buffer of a given size.
long pt_length(uint32_t buffer_size);

// Fills a page table for a given buffer.
void pt_fill(dma_addr_t buffer, struct pt_entry *pt, int n);

#endif
