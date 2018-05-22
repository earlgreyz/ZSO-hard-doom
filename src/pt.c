#include "pt.h"

#include <linux/errno.h>

#define PT_MAX_ENTRIES      1024
#define PT_MK_ENTRY(addr) (((addr) & 0xfffff000) | 0x00000001)

int pt_length(uint32_t buffer_size) {
  int len = buffer_size / DOOM_PAGE_SIZE;
  if (buffer_size % DOOM_PAGE_SIZE > 0) {
    ++len;
  }

  if (len > PT_MAX_ENTRIES) {
    return -EOVERFLOW;
  }

  return len;
}

void pt_fill(dma_addr_t buffer, struct pt_entry *pt, int n) {
  size_t i;
  uint32_t *pt_val = (uint32_t *)pt;
  for (i = 0; i < n; ++i) {
    pt_val[i] = PT_MK_ENTRY(buffer + i * DOOM_PAGE_SIZE);
  }
}
