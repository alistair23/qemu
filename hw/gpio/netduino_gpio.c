/*
 * Netduino Plus 2 GPIO
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

#define DEBUG_NETGPIO

#ifdef DEBUG_NETGPIO
#define DPRINTF(fmt, ...) \
do { printf("netduino_gpio: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#endif

static const uint8_t netduino_gpio_id[12] =
  { 0x00, 0x00, 0x00, 0x00, 0x61, 0x10, 0x04, 0x00, 0x0d, 0xf0, 0x05, 0xb1 };
static const uint8_t netduino_gpio_id_luminary[12] =
  { 0x00, 0x00, 0x00, 0x00, 0x61, 0x00, 0x18, 0x01, 0x0d, 0xf0, 0x05, 0xb1 };

#define TYPE_NETDUINO_GPIO "netduino_gpio"
#define NETDUINO_GPIO(obj) OBJECT_CHECK(NETDUINO_GPIOState, (obj), TYPE_NETDUINO_GPIO)

typedef struct NETDUINO_GPIOState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    uint32_t locked;
    uint32_t data;
    uint32_t old_data;
    uint32_t dir;
    uint32_t isense;
    uint32_t ibe;
    uint32_t iev;
    uint32_t im;
    uint32_t istate;
    uint32_t afsel;
    uint32_t dr2r;
    uint32_t dr4r;
    uint32_t dr8r;
    uint32_t odr;
    uint32_t pur;
    uint32_t pdr;
    uint32_t slr;
    uint32_t den;
    uint32_t cr;
    uint32_t float_high;
    uint32_t amsel;
    qemu_irq irq;
    qemu_irq out[8];
    const unsigned char *id;
} NETDUINO_GPIOState;

static const VMStateDescription vmstate_netduino_gpio = {
    .name = "netduino_gpio",
    .version_id = 2,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(locked, NETDUINO_GPIOState),
        VMSTATE_UINT32(data, NETDUINO_GPIOState),
        VMSTATE_UINT32(old_data, NETDUINO_GPIOState),
        VMSTATE_UINT32(dir, NETDUINO_GPIOState),
        VMSTATE_UINT32(isense, NETDUINO_GPIOState),
        VMSTATE_UINT32(ibe, NETDUINO_GPIOState),
        VMSTATE_UINT32(iev, NETDUINO_GPIOState),
        VMSTATE_UINT32(im, NETDUINO_GPIOState),
        VMSTATE_UINT32(istate, NETDUINO_GPIOState),
        VMSTATE_UINT32(afsel, NETDUINO_GPIOState),
        VMSTATE_UINT32(dr2r, NETDUINO_GPIOState),
        VMSTATE_UINT32(dr4r, NETDUINO_GPIOState),
        VMSTATE_UINT32(dr8r, NETDUINO_GPIOState),
        VMSTATE_UINT32(odr, NETDUINO_GPIOState),
        VMSTATE_UINT32(pur, NETDUINO_GPIOState),
        VMSTATE_UINT32(pdr, NETDUINO_GPIOState),
        VMSTATE_UINT32(slr, NETDUINO_GPIOState),
        VMSTATE_UINT32(den, NETDUINO_GPIOState),
        VMSTATE_UINT32(cr, NETDUINO_GPIOState),
        VMSTATE_UINT32(float_high, NETDUINO_GPIOState),
        VMSTATE_UINT32_V(amsel, NETDUINO_GPIOState, 2),
        VMSTATE_END_OF_LIST()
    }
};

static void netduino_gpio_update(NETDUINO_GPIOState *s)
{
    uint8_t changed;
    uint8_t mask;
    uint8_t out;
    int i;

    out = (s->data & s->dir) | ~s->dir;
    changed = s->old_data ^ out;
    if (!changed)
        return;

    DPRINTF("Update\n");

    s->old_data = out;
    for (i = 0; i < 8; i++) {
        mask = 1 << i;
        if (changed & mask) {
            DPRINTF("Set output %d = %d\n", i, (out & mask) != 0);
            DPRINTF("Set output %d = %d\n", i, (out & mask) != 0);
            qemu_set_irq(s->out[i], (out & mask) != 0);
        }
    }
}

static uint64_t netduino_gpio_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    NETDUINO_GPIOState *s = (NETDUINO_GPIOState *)opaque;

    DPRINTF("Read 0x%x, 0x%x\n", s->data, (uint) offset);

    if (offset < 0x400) {
        return s->data;
    }
    return 0;
}

static void netduino_gpio_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
    NETDUINO_GPIOState *s = (NETDUINO_GPIOState *)opaque;

    DPRINTF("Write 0x%x, 0x%x\n", (uint) value, (uint) offset);

    if (offset < 0x40) {
        s->data = value;
        netduino_gpio_update(s);
        return;
    }
    return;
}

static void netduino_gpio_reset(NETDUINO_GPIOState *s)
{
    s->locked = 1;
    s->cr = 0xff;
}

static void netduino_gpio_set_irq(void * opaque, int irq, int level)
{
    NETDUINO_GPIOState *s = (NETDUINO_GPIOState *)opaque;
    uint8_t mask;

    mask = 1 << irq;
    if ((s->dir & mask) == 0) {
        s->data &= ~mask;
        if (level)
            s->data |= mask;
        netduino_gpio_update(s);
    }
}

static const MemoryRegionOps netduino_gpio_ops = {
    .read = netduino_gpio_read,
    .write = netduino_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int netduino_gpio_initfn(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    NETDUINO_GPIOState *s = NETDUINO_GPIO(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &netduino_gpio_ops, s, "netduino_gpio", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
    qdev_init_gpio_in(dev, netduino_gpio_set_irq, 8);
    qdev_init_gpio_out(dev, s->out, 8);
    netduino_gpio_reset(s);
    return 0;
}

static void netduino_gpio_init(Object *obj)
{
    NETDUINO_GPIOState *s = NETDUINO_GPIO(obj);

    s->id = netduino_gpio_id;
}

static void netduino_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = netduino_gpio_initfn;
    dc->vmsd = &vmstate_netduino_gpio;
}

static const TypeInfo netduino_gpio_info = {
    .name          = TYPE_NETDUINO_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(NETDUINO_GPIOState),
    .instance_init = netduino_gpio_init,
    .class_init    = netduino_gpio_class_init,
};

static void netduino_gpio_register_types(void)
{
    type_register_static(&netduino_gpio_info);
}

type_init(netduino_gpio_register_types)
