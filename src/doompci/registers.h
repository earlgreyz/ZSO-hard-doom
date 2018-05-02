#ifndef DOOMPCI_REGISTERS_H
#define DOOMPCI_REGISTERS_H

#define DOOMPCI_FENCE_LAST             0x10
#define DOOMPCI_FENCE_WAIT             0x14

#define DOOMPCI_CMD_READ_PTR           0x18
#define DOOMPCI_CMD_WRITE_PTR          0x1c

#define DOOMPCI_FE_CODE_ADDR           0x20
#define DOOMPCI_FE_CODE_WINDOW         0x24

#define DOOMPCI_FIFO_FREE              0x30
#define DOOMPCI_FIFO_SEND              0x30

#define DOOMPCI_ENABLE                 0x00

#define ENABLE_ALL               (0x00003ff)
#define ENABLE_FETCH             (1 << 0x00)
#define ENABLE_FIFO              (1 << 0x01)
#define ENABLE_FE                (1 << 0x02)
#define ENABLE_XY                (1 << 0x03)
#define ENABLE_TEX               (1 << 0x04)
#define ENABLE_FLAT              (1 << 0x05)
#define ENABLE_FUZZ              (1 << 0x06)
#define ENABLE_SR                (1 << 0x07)
#define ENABLE_OG                (1 << 0x08)
#define ENABLE_SW                (1 << 0x09)

#define DOOMPCI_RESET                  0x04

#define RESET_ALL                (0xffffffe)
#define RESET_FIFO               (1 << 0x01)
#define RESET_FE                 (1 << 0x02)
#define RESET_XY                 (1 << 0x03)
#define RESET_TEX                (1 << 0x04)
#define RESET_FLAT               (1 << 0x05)
#define RESET_FUZZ               (1 << 0x06)
#define RESET_SR                 (1 << 0x07)
#define RESET_OG                 (1 << 0x08)
#define RESET_SW                 (1 << 0x09)
#define RESET_FE2XY              (1 << 0x0a)
#define RESET_FE2TEX             (1 << 0x0b)
#define RESET_FE2FLAT            (1 << 0x0c)
#define RESET_FE2FUZZ            (1 << 0x0d)
#define RESET_FE2OG              (1 << 0x0e)
#define RESET_XY2SW              (1 << 0x0f)
#define RESET_XY2SR              (1 << 0x10)
#define RESET_SR2OG              (1 << 0x11)
#define RESET_TEX2OG             (1 << 0x12)
#define RESET_FLAT2OG            (1 << 0x13)
#define RESET_FUZZ2OG            (1 << 0x14)
#define RESET_OG2SW              (1 << 0x15)
#define RESET_OG2SW_C            (1 << 0x16)
#define RESET_SW2XY              (1 << 0x17)
#define RESET_STATS              (1 << 0x18)
#define RESET_TLB                (1 << 0x19)
#define RESET_TEX_CACHE          (1 << 0x1a)
#define RESET_FLAT_CACHE         (1 << 0x1b)

#define DOOMPCI_INTR                   0x08
#define DOOMPCI_INTR_ENABLE            0x0c

#define INTR_ALL                 (0x00003ff)
#define INTR_FENCE               (1 << 0x00)
#define INTR_PONG_SYNC           (1 << 0x01)
#define INTR_PONG_ASYNC          (1 << 0x02)
#define INTR_FE_ERROR            (1 << 0x03)
#define INTR_FIFO_OVERFLOW       (1 << 0x04)
#define INTR_SURF_DST_OVERFLOW   (1 << 0x05)
#define INTR_SURF_SRC_OVERFLOW   (1 << 0x06)
#define INTR_PAGE_FAULT_SURF_DST (1 << 0x07)
#define INTR_PAGE_FAULT_SURF_SRC (1 << 0x08)
#define INTR_PAGE_FAULT_TEXTURE  (1 << 0x09)

#define DOOMPCI_STATUS                 0x04

#define STATUS_FETCH             (1 << 0x00)
#define STATUS_FIFO              (1 << 0x01)
#define STATUS_FE                (1 << 0x02)
#define STATUS_XY                (1 << 0x03)
#define STATUS_TEX               (1 << 0x04)
#define STATUS_FLAT              (1 << 0x05)
#define STATUS_FUZZ              (1 << 0x06)
#define STATUS_SR                (1 << 0x07)
#define STATUS_OG                (1 << 0x08)
#define STATUS_SW                (1 << 0x09)
#define STATUS_FE2XY             (1 << 0x0a)
#define STATUS_FE2TEX            (1 << 0x0b)
#define STATUS_FE2FLAT           (1 << 0x0c)
#define STATUS_FE2FUZZ           (1 << 0x0d)
#define STATUS_FE2OG             (1 << 0x0e)
#define STATUS_XY2SW             (1 << 0x0f)
#define STATUS_XY2SR             (1 << 0x10)
#define STATUS_SR2OG             (1 << 0x11)
#define STATUS_TEX2OG            (1 << 0x12)
#define STATUS_FLAT2OG           (1 << 0x13)
#define STATUS_FUZZ2OG           (1 << 0x14)
#define STATUS_OG2SW             (1 << 0x15)
#define STATUS_OG2SW_C           (1 << 0x16)
#define STATUS_SW2XY             (1 << 0x17)

#define DOOMPCI_FE_ERROR_CODE          0x28
#define DOOMPCI_FE_ERROR_DATA          0x2c

#define FE_RESERVED_TYPE               0x00
#define FE_RESERVED_BIT                0x01
#define FE_SURF_WIDTH_ZERO             0x02
#define FE_SURF_HEIGHT_ZERO            0x03
#define FE_SURF_WIDTH_OVF              0x04
#define FE_SURF_HEIGHT_OVF             0x05
#define FE_RECT_DST_X_OVF              0x06
#define FE_RECT_DST_Y_OVF              0x07
#define FE_RECT_SRC_X_OVF              0x08
#define FE_RECT_SRC_Y_OVF              0x09
#define FE_DRAW_COLUMN_REV             0x0a
#define FE_DRAW_SPAN_REV               0x0b

#define DOOMPCI_XY_SURF_DIMS          0x200
#define DOOMPCI_XY_DST_CMD            0x208
#define DOOMPCI_XY_SRC_CMD            0x20c

#define DOOMPCI_TLB_PT_SURF_DST       0x300
#define DOOMPCI_TLB_PT_SURF_SRC       0x304
#define DOOMPCI_TLB_PT_TEXTURE        0x308

#define DOOMPCI_TLB_VADDR_SURF_DST    0x320
#define DOOMPCI_TLB_VADDR_SURF_SRC    0x324
#define DOOMPCI_TLB_VADDR_TEXTURE     0x328

#endif
