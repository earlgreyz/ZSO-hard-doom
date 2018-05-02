#ifndef DOOMPCI_COMMANDS_H
#define DOOMPCI_COMMANDS_H

#ifdef __KERNEL__
#include <linux/kernel.h>
#else
#include <stdint.h>
#endif

struct doompci_pt {
  uint32_t addr      : 26;
  uint32_t _type     :  6;
};

#define DOOMPCI_SURF_DST_PT 0x20
#define DOOMPCI_SURF_SRC_PT 0x21
#define DOOMPCI_TEXTURE_PT  0x22

struct doompci_flat_addr {
  uint32_t addr      : 20;
  uint32_t           :  6;
  uint32_t _type     :  6;
};

#define DOOMPCI_FLAT_ADDR 0x23

struct doompci_colormap_addr {
  uint32_t addr      : 20;
  uint32_t           :  6;
  uint32_t _type     :  6;
};

#define DOOMPCI_COLORMAP_ADDR    0x24
#define DOOMPCI_TRANSLATION_ADDR 0x25

struct doompci_surf_dims {
  uint32_t width     :  6;
  uint32_t           :  2;
  uint32_t height    : 12;
  uint32_t           :  6;
  uint32_t _type     :  6;
};

#define DOOMPCI_SURF_DIMS 0x26

struct doompci_texture_dims {
  uint32_t height    : 10;
  uint32_t           :  2;
  uint32_t size_m1   : 14;
  uint32_t _type     :  6;
};

#define DOOMPCI_TEXTURE_DIMS 0x27

struct doompci_fill_color {
  uint32_t color     :  8;
  uint32_t           : 18;
  uint32_t _type     :  6;
};

#define DOOMPCI_FILL_COLOR 0x28

struct doompci_draw_params {
  uint32_t fuzz      :  1;
  uint32_t translate :  1;
  uint32_t colormap  :  1;
  uint32_t           : 23;
  uint32_t _type     :  6;
};

#define DOOMPCI_DRAW_PARAMS 0x29

struct doompci_xy {
  uint32_t x         : 11;
  uint32_t           :  1;
  uint32_t y         : 11;
  uint32_t           :  3;
  uint32_t _type     :  6;
};

#define DOOMPCI_XY_A 0x2a
#define DOOMPCI_XY_B 0x2b

struct doompci_start {
  uint32_t coord     : 26;
  uint32_t _type     :  6;
};

#define DOOMPCI_USTART 0x2c
#define DOOMPCI_VSTART 0x2d

struct doompci_step {
  uint32_t delta     : 26;
  uint32_t _type     :  6;
};

#define DOOMPCI_USTEP 0x2c
#define DOOMPCI_VSTEP 0x2d

struct doompci_rect {
  uint32_t width     : 12;
  uint32_t height    : 12;
  uint32_t           :  2;
  uint32_t _type     :  6;
};

#define DOOMPCI_COPY_RECT 0x30
#define DOOMPCI_FILL_RECT 0x31

#define DOOMPCI_DRAW_LINE 0x32
#define DOOMPCI_DRAW_BACKGROUND 0x33

struct doompci_draw_column {
  uint32_t column    : 22;
  uint32_t           :  4;
  uint32_t _type     :  6;
};

#define DOOMPCI_DRAW_COLUMN 0x34

#define DOOMPCI_DRAW_SPAN 0x35
#define DOOMPCI_INTERLOCK 0x3f

struct doompci_fence {
  uint32_t value     : 26;
  uint32_t _type     :  6;
};

#define DOOMPCI_FENCE 0x3c

#define DOOMPCI_PING_SYNC  0x3d
#define DOOMPCI_PING_ASYNC 0x3e

struct doompci_jump {
  uint32_t ptr       : 30;
  uint32_t _type     :  2;
};

#define DOOMPCI_JUMP 0x00

#endif
