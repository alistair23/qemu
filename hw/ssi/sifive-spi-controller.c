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

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/ssi/sifive-spi-controller.h"

#ifndef SIFIVE_SPI_ERR_DEBUG
#define SIFIVE_SPI_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (SIFIVE_SPI_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

static void sifive_spi_reset(DeviceState *dev)
{
    SiFiveSPIState *s = SIFIVE_SPI(dev);

    s->regs[SIFIVE_SPI_SCKDIV] = 0x03;
    s->regs[SIFIVE_SPI_CSDEF]  = 0x01;
    s->regs[SIFIVE_SPI_DELAY0] = 0x1 | 0x1 << 16;
    s->regs[SIFIVE_SPI_DELAY1] = 0x1;
    s->regs[SIFIVE_SPI_FMT] = 0x1 << 3 | 0x8 << 16;
    s->regs[SIFIVE_SPI_TXMARK] = 0x1;
    s->regs[SIFIVE_SPI_FCTRL] = 0x1;
    s->regs[SIFIVE_SPI_FFMT] = 0x1 | 0x3 << 1 | 0x3 << 16;

    fifo8_reset(&s->rx_fifo);
    fifo8_reset(&s->tx_fifo);
}

static void sifive_spi_irq(SiFiveSPIState *s)
{
    bool irq = false;
    bool cs_line;
    int i;

    if (fifo8_num_used(&s->rx_fifo) > s->regs[SIFIVE_SPI_RXMARK]) {
        DB_PRINT("Setting RXMARK IP bit\n");
        s->regs[SIFIVE_SPI_IP] |= 1 << 1;

        if (s->regs[SIFIVE_SPI_IP] & s->regs[SIFIVE_SPI_IE] & (1 << 1)) {
            irq = true;
        }
    }

    if (fifo8_num_used(&s->tx_fifo) < s->regs[SIFIVE_SPI_TXMARK]) {
        DB_PRINT("Setting TXMARK IP bit\n");
        s->regs[SIFIVE_SPI_IP] |= 1 << 0;

        if (s->regs[SIFIVE_SPI_IP] & s->regs[SIFIVE_SPI_IE] & (1 << 0)) {
            irq = true;
        }
    }

    DB_PRINT("Setting irq to: %d\n", irq);
    qemu_set_irq(s->irq, irq);

    for (i = 0; i < s->cs_width; i++) {
        if (s->regs[SIFIVE_SPI_CSMODE] == 2) {
            /* HOLD the CS Line */
            cs_line = true;
        } else if  (s->regs[SIFIVE_SPI_CSMODE] == 0) {
            /* AUTO the CS Line */
            if (s->regs[SIFIVE_SPI_CSID] & (1 << i)) {
                cs_line = s->regs[SIFIVE_SPI_CSDEF] & (1 << i);
            } else {
                cs_line = ~s->regs[SIFIVE_SPI_CSDEF] & (1 << i);
            }
        } else {
            /* Hardware control disabled */
            cs_line = s->regs[SIFIVE_SPI_CSDEF] & (1 << i);
        }

        DB_PRINT("Setting cs_line[%d] to: %d\n", i, cs_line);
        qemu_set_irq(s->cs_lines[i], cs_line);
    }
}

static void sifive_spi_transfer(SiFiveSPIState *s)
{
    uint8_t rx, tx;
    uint8_t bits_per_frame = (s->regs[SIFIVE_SPI_FMT] >> 16) & 0xF;
    bool lsb = s->regs[SIFIVE_SPI_FMT] & (1 << 2);

    tx = fifo8_pop(&s->tx_fifo);
    tx &= MAKE_64BIT_MASK(0, bits_per_frame);
    if (!lsb) {
        /* Transmit MSB first */
        tx = (tx & (1 << 0)) << 7 |
             (tx & (1 << 1)) << 5 |
             (tx & (1 << 2)) << 3 |
             (tx & (1 << 3)) << 1 |
             (tx & (1 << 4)) >> 1 |
             (tx & (1 << 5)) >> 3 |
             (tx & (1 << 6)) >> 5 |
             (tx & (1 << 7)) >> 7;
    }
    DB_PRINT("Data to send: 0x%x\n", tx);

    rx = ssi_transfer(s->ssi, tx);

    rx &= MAKE_64BIT_MASK(0, bits_per_frame);
    if (!lsb) {
        /* Transmit MSB first */
        rx = (rx & (1 << 0)) << 7 |
             (rx & (1 << 1)) << 5 |
             (rx & (1 << 2)) << 3 |
             (rx & (1 << 3)) << 1 |
             (rx & (1 << 4)) >> 1 |
             (rx & (1 << 5)) >> 3 |
             (rx & (1 << 6)) >> 5 |
             (rx & (1 << 7)) >> 7;
    }
    DB_PRINT("Data received: 0x%x - rx pos: %d/%d\n",
             rx, fifo8_num_used(&s->rx_fifo),
             s->regs[SIFIVE_SPI_RXMARK]);
    fifo8_push(&s->rx_fifo, rx);

    sifive_spi_irq(s);
}

static uint64_t sifive_spi_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    SiFiveSPIState *s = opaque;
    uint64_t ret = 0;

    addr = addr >> 2;

    switch (addr) {
    case SIFIVE_SPI_SCKDIV...SIFIVE_SPI_SCKMODE:
    case SIFIVE_SPI_CSID...SIFIVE_SPI_CSMODE:
    case SIFIVE_SPI_DELAY0...SIFIVE_SPI_DELAY1:
    case SIFIVE_SPI_FMT:
    case SIFIVE_SPI_TXDATA:
        ret = s->regs[addr];
        break;
    case SIFIVE_SPI_RXDATA:
        ret = fifo8_pop(&s->rx_fifo);
        break;
    case SIFIVE_SPI_FCTRL...SIFIVE_SPI_FFMT:
        qemu_log_mask(LOG_UNIMP, "Direct memory mapped mode is "
                      "unimplemented.\n");
        /* Fall through */
    case SIFIVE_SPI_TXMARK...SIFIVE_SPI_RXMARK:
    case SIFIVE_SPI_IE:
    case SIFIVE_SPI_IP:
        ret = s->regs[addr];
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "Bad offset 0x%" HWADDR_PRIx "\n",
                      addr << 2);
    }

    DB_PRINT("Address: 0x%" HWADDR_PRIx ", value: 0x%lx\n", addr << 2, ret);

    return ret;
}

static void sifive_spi_write(void *opaque, hwaddr addr,
                                uint64_t val64, unsigned int size)
{
    SiFiveSPIState *s = opaque;
    uint32_t value = val64;

    DB_PRINT("Address: 0x%" HWADDR_PRIx ", Value: 0x%x\n", addr, value);

    addr = addr >> 2;

    switch (addr) {
    case SIFIVE_SPI_SCKDIV...SIFIVE_SPI_SCKMODE:
    case SIFIVE_SPI_CSID...SIFIVE_SPI_CSMODE:
    case SIFIVE_SPI_DELAY0...SIFIVE_SPI_DELAY1:
    case SIFIVE_SPI_FMT:
        s->regs[addr] = value;
        break;
    case SIFIVE_SPI_TXDATA:
        fifo8_push(&s->tx_fifo, value);
        sifive_spi_transfer(s);
        break;
    case SIFIVE_SPI_FCTRL...SIFIVE_SPI_FFMT:
        qemu_log_mask(LOG_UNIMP, "Direct memory mapped mode is "
                      "unimplemented.\n");
        /* Fall through */
    case SIFIVE_SPI_TXMARK...SIFIVE_SPI_RXMARK:
    case SIFIVE_SPI_IE:
    case SIFIVE_SPI_IP:
        s->regs[addr] = value;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "Bad offset 0x%" HWADDR_PRIx "\n", addr << 2);
    }

    sifive_spi_irq(s);
}

static const MemoryRegionOps sifive_spi_ops = {
    .read = sifive_spi_read,
    .write = sifive_spi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static Property sifive_spi_properties[] = {
    DEFINE_PROP_UINT8("cs-width", SiFiveSPIState, cs_width, 1),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_sifive = {
    .name = TYPE_SIFIVE_SPI,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        /* DO */
        VMSTATE_END_OF_LIST()
    }
};

static void sifive_spi_realize(DeviceState *dev, Error **errp)
{
    SiFiveSPIState *s = SIFIVE_SPI(dev);
    int i;

    s->ssi = ssi_create_bus(dev, "ssi");

    s->cs_lines = g_new0(qemu_irq, s->cs_width);
    ssi_auto_connect_slaves(dev, s->cs_lines, s->ssi);
    for (i = 0; i < s->cs_width; ++i) {
        sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->cs_lines[i]);
    }

    fifo8_create(&s->tx_fifo, 64);
    fifo8_create(&s->rx_fifo, 64);
}

static void sifive_spi_init(Object *obj)
{
    SiFiveSPIState *s = SIFIVE_SPI(obj);

    memory_region_init_io(&s->mmio, obj, &sifive_spi_ops, s,
                          TYPE_SIFIVE_SPI, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);
}

static void sifive_spi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = sifive_spi_realize;
    dc->reset = sifive_spi_reset;
    dc->vmsd = &vmstate_sifive;
    dc->props = sifive_spi_properties;
}

static const TypeInfo sifive_spi_info = {
    .name          = TYPE_SIFIVE_SPI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(SiFiveSPIState),
    .instance_init = sifive_spi_init,
    .class_init    = sifive_spi_class_init,
};

static void sifive_spi_register_types(void)
{
    type_register_static(&sifive_spi_info);
}

type_init(sifive_spi_register_types)
