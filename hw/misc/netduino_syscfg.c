/*
 * Netduino Plus 2 SYSCFG
 *
 * Copyright (c) 2014 Alistair Francis <alistair@alistair23.me>
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

#include "hw/sysbus.h"
#include "sysemu/char.h"
#include "hw/hw.h"
#include "net/net.h"

/* #define DEBUG_NETSYSCFG */

#ifdef DEBUG_NETSYSCFG
#define DPRINTF(fmt, ...) \
do { printf("netduino_syscfg: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#endif

#define SYSCFG_MEMRMP  0x00
#define SYSCFG_PMC     0x04
#define SYSCFG_EXTICR1 0x08
#define SYSCFG_EXTICR2 0x0C
#define SYSCFG_EXTICR3 0x10
#define SYSCFG_EXTICR4 0x14
#define SYSCFG_CMPCR   0x20

#define TYPE_NETDUINO_SYSCFG "netduino_syscfg"
#define NETDUINO_SYSCFG(obj) \
    OBJECT_CHECK(struct net_syscfg, (obj), TYPE_NETDUINO_SYSCFG)

struct net_syscfg {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    uint32_t syscfg_memrmp;
    uint32_t syscfg_pmc;
    uint32_t syscfg_exticr1;
    uint32_t syscfg_exticr2;
    uint32_t syscfg_exticr3;
    uint32_t syscfg_exticr4;
    uint32_t syscfg_cmpcr;

    qemu_irq irq;
};

static void syscfg_reset(DeviceState *dev)
{
    struct net_syscfg *s = NETDUINO_SYSCFG(dev);

    s->syscfg_memrmp = 0x00000000;
    s->syscfg_pmc = 0x00000000;
    s->syscfg_exticr1 = 0x00000000;
    s->syscfg_exticr2 = 0x00000000;
    s->syscfg_exticr3 = 0x00000000;
    s->syscfg_exticr4 = 0x00000000;
    s->syscfg_cmpcr = 0x00000000;
}

static uint64_t netduino_syscfg_read(void *opaque, hwaddr addr, unsigned int size)
{
    struct net_syscfg *s = opaque;

    if (addr >= 0x400) {
        addr = addr >> 7;
    }

    DPRINTF("Read 0x%x\n", (uint) addr);

    switch (addr) {
        case SYSCFG_MEMRMP:
            return s->syscfg_memrmp;
        case SYSCFG_PMC:
            return s->syscfg_pmc;
        case SYSCFG_EXTICR1:
            return s->syscfg_exticr1;
        case SYSCFG_EXTICR2:
            return s->syscfg_exticr2;
        case SYSCFG_EXTICR3:
            return s->syscfg_exticr3;
        case SYSCFG_EXTICR4:
            return s->syscfg_exticr4;
        case SYSCFG_CMPCR:
            return s->syscfg_cmpcr;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "net_syscfg_read: Bad offset %x\n", (int)addr);
            return 0;
    }

    return 0;
}

static void netduino_syscfg_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    struct net_syscfg *s = opaque;
    uint32_t value = (uint32_t) val64;

    if (addr >= 0x400) {
        addr = addr >> 7;
    }

    DPRINTF("Write 0x%x, 0x%x\n", value, (uint) addr);

    switch (addr) {
        case SYSCFG_MEMRMP:
            /* This isn't supported, so don't allow the guest to write to it
             * s->syscfg_memrmp = value;
             */
            return;
        case SYSCFG_PMC:
            /* This isn't supported, so don't allow the guest to write to it
             * s->syscfg_pmc = value;
             */
            return;
        case SYSCFG_EXTICR1:
            s->syscfg_exticr1 = (value & 0xFF);
            return;
        case SYSCFG_EXTICR2:
            s->syscfg_exticr2 = (value & 0xFF);
            return;
        case SYSCFG_EXTICR3:
            s->syscfg_exticr3 = (value & 0xFF);
            return;
        case SYSCFG_EXTICR4:
            s->syscfg_exticr4 = (value & 0xFF);
            return;
        case SYSCFG_CMPCR:
            s->syscfg_cmpcr = value;
            return;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "net_syscfg_write: Bad offset %x\n", (int)addr);
    }
}

static const MemoryRegionOps netduino_syscfg_ops = {
    .read = netduino_syscfg_read,
    .write = netduino_syscfg_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int netduino_syscfg_init(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    struct net_syscfg *s = NETDUINO_SYSCFG(dev);

    sysbus_init_irq(sbd, &s->irq);

    memory_region_init_io(&s->mmio, OBJECT(s), &netduino_syscfg_ops, s,
                          TYPE_NETDUINO_SYSCFG, 0x2000);
    sysbus_init_mmio(sbd, &s->mmio);

    return 0;
}

static void netduino_syscfg_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = netduino_syscfg_init;
    dc->props = NULL;
    dc->reset = syscfg_reset;
}

static const TypeInfo netduino_syscfg_info = {
    .name          = TYPE_NETDUINO_SYSCFG,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct net_syscfg),
    .class_init    = netduino_syscfg_class_init,
};

static void netduino_syscfg_register_types(void)
{
    type_register_static(&netduino_syscfg_info);
}

type_init(netduino_syscfg_register_types)
