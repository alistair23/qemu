/*
 * Netduino Plus 2 USART
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

/* #define DEBUG_NETUSART */

#ifdef DEBUG_NETUSART
#define DPRINTF(fmt, ...) \
do { printf("netduino_usart: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#endif

#define USART_SR   0x00
#define USART_DR   0x04
#define USART_BRR  0x08
#define USART_CR1  0x0C
#define USART_CR2  0x10
#define USART_CR3  0x14
#define USART_GTPR 0x18

#define USART_SR_TXE  (1 << 7)
#define USART_SR_TC   (1 << 6)
#define USART_SR_RXNE (1 << 5)

#define USART_CR1_UE  (1 << 13)
#define USART_CR1_RXNEIE  (1 << 5)
#define USART_CR1_TE  (1 << 3)

#define TYPE_NETDUINO_USART "netduino_usart"
#define NETDUINO_USART(obj) \
    OBJECT_CHECK(struct net_usart, (obj), TYPE_NETDUINO_USART)

struct net_usart {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    uint32_t usart_sr;
    uint32_t usart_dr;
    uint32_t usart_brr;
    uint32_t usart_cr1;
    uint32_t usart_cr2;
    uint32_t usart_cr3;
    uint32_t usart_gtpr;

    NICState *nic;
    CharDriverState *chr;
    qemu_irq irq;
    NICConf conf;
};

static int usart_can_receive(void *opaque)
{
    struct net_usart *s = opaque;

    if (s->usart_cr1 & USART_CR1_UE && s->usart_cr1 & USART_CR1_TE) {
        return 1;
    }

    return 0;
}

static void usart_receive(void *opaque, const uint8_t *buf, int size)
{
    struct net_usart *s = opaque;
    int i, num;

    num = size > 8 ? 8 : size;
    s->usart_dr = 0;

    for (i = 0; i < num; i++) {
        s->usart_dr |= (buf[i] << i);
    }

    s->usart_sr |= USART_SR_RXNE;

    if (s->usart_cr1 & USART_CR1_RXNEIE) {
        qemu_set_irq(s->irq, 1);
    }

    DPRINTF("Inputting: %c\n", s->usart_dr);
}

static void usart_reset(DeviceState *dev)
{
    struct net_usart *s = NETDUINO_USART(dev);

    s->usart_sr = 0x00C00000;
    s->usart_dr = 0x00000000;
    s->usart_brr = 0x00000000;
    s->usart_cr1 = 0x00000000;
    s->usart_cr2 = 0x00000000;
    s->usart_cr3 = 0x00000000;
    s->usart_gtpr = 0x00000000;
}

static uint64_t netduino_usart_read(void *opaque, hwaddr addr, unsigned int size)
{
    struct net_usart *s = opaque;
    uint64_t retvalue;

    DPRINTF("Read 0x%x\n", (uint) addr);

    switch (addr) {
        case USART_SR:
            retvalue = s->usart_sr;
            s->usart_sr &= (USART_SR_TC ^ 0xFFFF);
            return retvalue;
        case USART_DR:
            DPRINTF("Value: %x, %c\n", s->usart_dr, s->usart_dr);
            s->usart_sr |= USART_SR_TXE;
            return (s->usart_dr & 0x3FF);
        case USART_BRR:
            return s->usart_brr;
        case USART_CR1:
            return s->usart_cr1;
        case USART_CR2:
            return s->usart_cr2;
        case USART_CR3:
            return s->usart_cr3;
        case USART_GTPR:
            return s->usart_gtpr;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "net_usart_read: Bad offset %x\n", (int)addr);
            return 0;
    }

    return 0;
}

static void netduino_usart_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    struct net_usart *s = opaque;
    uint32_t value = (uint32_t) val64;
    unsigned char ch;

    DPRINTF("Write 0x%x, 0x%x\n", value, (uint) addr);

    switch (addr) {
        case USART_SR:
            if (value <= 0x3FF) {
                s->usart_sr = value;
            } else {
                s->usart_sr &= value;
            }
            return;
        case USART_DR:
            if (value < 0xF000) {
                ch = value;
                if (s->chr) {
                    qemu_chr_fe_write(s->chr, &ch, 1);
                }
                s->usart_sr |= USART_SR_TC;
            }
            return;
        case USART_BRR:
            s->usart_brr = value;
            return;
        case USART_CR1:
            s->usart_cr1 = value;
            return;
        case USART_CR2:
            s->usart_cr2 = value;
            return;
        case USART_CR3:
            s->usart_cr3 = value;
            return;
        case USART_GTPR:
            s->usart_gtpr = value;
            return;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "net_usart_write: Bad offset %x\n", (int)addr);
    }
}

static const MemoryRegionOps netduino_usart_ops = {
    .read = netduino_usart_read,
    .write = netduino_usart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int netduino_usart_init(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    struct net_usart *s = NETDUINO_USART(dev);

    sysbus_init_irq(sbd, &s->irq);

    memory_region_init_io(&s->mmio, OBJECT(s), &netduino_usart_ops, s,
                          TYPE_NETDUINO_USART, 0x2000);
    sysbus_init_mmio(sbd, &s->mmio);

    s->chr = qemu_char_get_next_serial();

    if (s->chr) {
        qemu_chr_add_handlers(s->chr, usart_can_receive, usart_receive,
                              NULL, s);
    }

    return 0;
}

static void netduino_usart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = netduino_usart_init;
    dc->props = NULL;
    dc->reset = usart_reset;
}

static const TypeInfo netduino_usart_info = {
    .name          = TYPE_NETDUINO_USART,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct net_usart),
    .class_init    = netduino_usart_class_init,
};

static void netduino_usart_register_types(void)
{
    type_register_static(&netduino_usart_info);
}

type_init(netduino_usart_register_types)
