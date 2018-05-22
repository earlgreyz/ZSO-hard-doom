#ifndef HARDDOOM_PT_H
#define HARDDOOM_PT_H

#include <linux/kernel.h>

#define DOOM_PAGE_SIZE  (4 << 10)
#define PT_ALIGNMENT     (1 << 6)

/// Represents an entry in a page table.
struct pt_entry {
  int32_t valid :  1;
  int32_t       : 11;
  int32_t addr  : 20;
};

/**
 * Calculates the number of pages required to fit a buffer of a given size.
 * @success returns a positive number of pages.
 * @failure may return -EOVERFLOW if the number of pages exceeds the limit.
 **/
int pt_length(uint32_t buffer_size);

/**
 * Fills a page table with consecutive DMA addresses of buffer pages.
 **/
void pt_fill(dma_addr_t buffer, struct pt_entry *pt, int n);

#endif
