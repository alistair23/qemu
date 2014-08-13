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

//#define DEBUG_NETGPIO

#ifdef DEBUG_NETGPIO
#define DPRINTF(fmt, ...) \
do { printf("netduino_gpio: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#endif

static const uint8_t netduino_gpio_id[12] =
  { 0x00, 0x00, 0x00, 0x00, 0x61, 0x10, 0x04, 0x00, 0x0d, 0xf0, 0x05, 0xb1 };

#define TYPE_NETDUINO_GPIO "netduino_gpio"
#define NETDUINO_GPIO(obj) OBJECT_CHECK(NETDUINO_GPIOState, (obj), TYPE_NETDUINO_GPIO)

typedef struct NETDUINO_GPIOState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;

    uint32_t gpio_moder;
    uint32_t gpio_otyper;
    uint32_t gpio_ospeedr;
    uint32_t gpio_pupdr;
    uint32_t gpio_idr;
    uint32_t gpio_odr;
    uint32_t gpio_bsrr;
    uint32_t gpio_lckr;
    uint32_t gpio_afrl;
    uint32_t gpio_afrh;

    qemu_irq irq;
    //qemu_irq out[8];
    const unsigned char *id;
} NETDUINO_GPIOState;

static const VMStateDescription vmstate_netduino_gpio = {
    .name = "netduino_gpio",
    .version_id = 2,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(gpio_moder, NETDUINO_GPIOState),
        VMSTATE_UINT32(gpio_otyper, NETDUINO_GPIOState),
        VMSTATE_UINT32(gpio_ospeedr, NETDUINO_GPIOState),
        VMSTATE_UINT32(gpio_pupdr, NETDUINO_GPIOState),
        VMSTATE_UINT32(gpio_idr, NETDUINO_GPIOState),
        VMSTATE_UINT32(gpio_odr, NETDUINO_GPIOState),
        VMSTATE_UINT32(gpio_bsrr, NETDUINO_GPIOState),
        VMSTATE_UINT32(gpio_lckr, NETDUINO_GPIOState),
        VMSTATE_UINT32(gpio_afrl, NETDUINO_GPIOState),
        VMSTATE_UINT32(gpio_afrh, NETDUINO_GPIOState),
        VMSTATE_END_OF_LIST()
    }
};

//static void netduino_gpio_update(NETDUINO_GPIOState *s)
//{
    /*uint8_t changed;
    uint8_t mask;
    uint8_t out;
    int i; */

    //out = (s->data & s->dir) | ~s->dir;
    //changed = s->old_data ^ out;
    //if (!changed)
    //    return;

    //DPRINTF("Update\n");

    //s->old_data = out;
    /*for (i = 0; i < 8; i++) {
        mask = 1 << i;
        if (changed & mask) {
            DPRINTF("Set output %d = %d\n", i, (out & mask) != 0);
            DPRINTF("Set output %d = %d\n", i, (out & mask) != 0);
            qemu_set_irq(s->out[i], (out & mask) != 0);
        }
    }*/
//}

static void gpio_reset(DeviceState *dev)
{
    NETDUINO_GPIOState *s = NETDUINO_GPIO(dev);

    s->gpio_moder = 0xA8000000;
    s->gpio_otyper = 0x00000000;
    s->gpio_ospeedr = 0x00000000;
    s->gpio_pupdr = 0x64000000;
    s->gpio_idr = 0x00000000;
    s->gpio_odr = 0x00000000;
    s->gpio_bsrr = 0x00000000;
    s->gpio_lckr = 0x00000000;
    s->gpio_afrl = 0x00000000;
    s->gpio_afrh = 0x00000000;
}

static uint64_t netduino_gpio_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    NETDUINO_GPIOState *s = (NETDUINO_GPIOState *)opaque;

    DPRINTF("Read 0x%x, 0x%x\n", s->gpio_odr, (uint) offset);

    switch (offset) {
        case 0x00:
            return s->gpio_moder;
        case 0x04:
            return s->gpio_otyper;
        case 0x08:
            return s->gpio_ospeedr;
        case 0x0C:
            return s->gpio_pupdr;
        case 0x10:
            /* HACK: This is a hack to trigger interrupts */
            qemu_set_irq(s->irq, 0);
            qemu_set_irq(s->irq, 1);
            qemu_set_irq(s->irq, 0);
            return s->gpio_idr;
        case 0x14:
            return s->gpio_odr;
        case 0x18:
            return s->gpio_bsrr;
        case 0x1C:
            return s->gpio_lckr;
        case 0x20:
            return s->gpio_afrl;
        case 0x24:
            return s->gpio_afrh;

        /*int i;
        for (i = 0; i < 8; i++) {
            qemu_set_irq(s->out[i], 0);
            qemu_set_irq(s->out[i], 1);
        }*/

        /*case 0x10:
            qemu_set_irq(s->irq, 0);
            qemu_set_irq(s->irq, 1);
            qemu_set_irq(s->irq, 0);
            return 0;*/
    }
    return 0x0000FFFF;
}

static void netduino_gpio_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
    NETDUINO_GPIOState *s = (NETDUINO_GPIOState *)opaque;

    DPRINTF("Write 0x%x, 0x%x, 0x%x\n", (uint) value, (uint) offset, (uint) s->gpio_odr);

    switch (offset) {
        case 0x00:
            s->gpio_moder = (uint32_t) value;
        case 0x04:
            s->gpio_otyper = (uint32_t) value;
        case 0x08:
            s->gpio_ospeedr = (uint32_t) value;
        case 0x0C:
            s->gpio_pupdr = (uint32_t) value;
        case 0x10:
            s->gpio_idr = (uint32_t) value;
        case 0x14:
            s->gpio_odr = (uint32_t) value;
        case 0x18:
            s->gpio_bsrr = (uint32_t) value;
        case 0x1C:
            s->gpio_lckr = (uint32_t) value;
        case 0x20:
            s->gpio_afrl = (uint32_t) value;
        case 0x24:
            s->gpio_afrh = (uint32_t) value;
        }

    if (offset < 0x40) {
        s->gpio_odr = value;
        //netduino_gpio_update(s);
        return;
    }
    return;
}

static void netduino_gpio_set_irq(void * opaque, int irq, int level)
{
    //NETDUINO_GPIOState *s = (NETDUINO_GPIOState *)opaque;
    //uint8_t mask;

    //mask = 1 << irq;
    /*if ((s->dir & mask) == 0) {
        s->gpio_odr &= ~mask;
        if (level)
            s->gpio_odr |= mask;
        netduino_gpio_update(s);
    }*/
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

    memory_region_init_io(&s->iomem, OBJECT(s), &netduino_gpio_ops, s, "netduino_gpio", 0x2000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
    //qdev_init_gpio_in(dev, netduino_gpio_set_irq, 8);
    //qdev_init_gpio_out(dev, s->out, 8);
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
    dc->reset = gpio_reset;
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
