/*
 * QEMU model of the SiFive SPI Controller
 *
 * Copyright (C) 2018 Western Digital
 * Copyright (C) 2018 Alistair Francis <alistair.francis@wdc.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef HW_SIFIVE_SPI_H
#define HW_SIFIVE_SPI_H

#include "hw/sysbus.h"
#include "hw/hw.h"
#include "hw/ssi/ssi.h"
#include "qemu/fifo8.h"

#define TYPE_SIFIVE_SPI "sifive-spi"
#define SIFIVE_SPI(obj) \
    OBJECT_CHECK(SiFiveSPIState, (obj), TYPE_SIFIVE_SPI)

#define SIFIVE_SPI_SCKDIV        (0x00 / 4)
#define SIFIVE_SPI_SCKMODE       (0x04 / 4)
#define SIFIVE_SPI_CSID          (0x10 / 4)
#define SIFIVE_SPI_CSDEF         (0x14 / 4)
#define SIFIVE_SPI_CSMODE        (0x18 / 4)
#define SIFIVE_SPI_DELAY0        (0x28 / 4)
#define SIFIVE_SPI_DELAY1        (0x2C / 4)
#define SIFIVE_SPI_FMT           (0x40 / 4)
#define SIFIVE_SPI_TXDATA        (0x48 / 4)
#define SIFIVE_SPI_RXDATA        (0x4C / 4)
#define SIFIVE_SPI_TXMARK        (0x50 / 4)
#define SIFIVE_SPI_RXMARK        (0x54 / 4)
#define SIFIVE_SPI_FCTRL         (0x60 / 4)
#define SIFIVE_SPI_FFMT          (0x64 / 4)
#define SIFIVE_SPI_IE            (0x70 / 4)
#define SIFIVE_SPI_IP            (0x74 / 4)

#define SIFIVE_SPI_MAX_REGS      (SIFIVE_SPI_IP + 1)

#define SIFIVE_SPI_SCKDIV_DIV_MASK    ((1 << 12) - 1)

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;
    uint32_t regs[SIFIVE_SPI_MAX_REGS];
    Fifo8 rx_fifo;
    Fifo8 tx_fifo;

    qemu_irq irq;
    uint8_t cs_width;
    qemu_irq *cs_lines;
    SSIBus *ssi;
} SiFiveSPIState;

#endif
