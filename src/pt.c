#include "pt.h"

#include <linux/errno.h>

#define MK_PT_ENTRY(addr) (((uint32_t)(uint64_t)(addr)) & 0xfffff001)

long pt_length(uint32_t buffer_size) {
  int len = buffer_size / DOOM_PAGE_SIZE;
  if (buffer_size % DOOM_PAGE_SIZE > 0) {
    ++len;
  }

  if (len > PT_MAX_ENTRIES) {
    return -EOVERFLOW;
  }

  return len;
}

void pt_fill(void *buffer, struct pt_entry *pt, int n) {
  size_t i;
  for (i = 0; i < n; ++i) {
    *(uint32_t *)(pt + i) = MK_PT_ENTRY(buffer + i * DOOM_PAGE_SIZE);
  }
}
