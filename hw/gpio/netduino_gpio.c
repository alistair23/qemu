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

    /* Outputs float high.  */
    /* FIXME: This is board dependent.  */
    out = (s->data & s->dir) | ~s->dir;
    changed = s->old_data ^ out;
    if (!changed)
        return;

fprintf(stderr, "Update\n");

    s->old_data = out;
    for (i = 0; i < 8; i++) {
        mask = 1 << i;
        if (changed & mask) {
            fprintf(stderr, "Set output %d = %d\n", i, (out & mask) != 0);
            DPRINTF("Set output %d = %d\n", i, (out & mask) != 0);
            qemu_set_irq(s->out[i], (out & mask) != 0);
        }
    }

    /* FIXME: Implement input interrupts.  */
}

static uint64_t netduino_gpio_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    NETDUINO_GPIOState *s = (NETDUINO_GPIOState *)opaque;

    fprintf(stderr, "Read %d\n", s->data);

    return s->data;

    if (offset >= 0xfd0 && offset < 0x1000) {
        return s->id[(offset - 0xfd0) >> 2];
    }
    if (offset < 0x400) {
        return s->data & (offset >> 2);
    }
    switch (offset) {
    case 0x400: /* Direction */
        return s->dir;
    case 0x404: /* Interrupt sense */
        return s->isense;
    case 0x408: /* Interrupt both edges */
        return s->ibe;
    case 0x40c: /* Interrupt event */
        return s->iev;
    case 0x410: /* Interrupt mask */
        return s->im;
    case 0x414: /* Raw interrupt status */
        return s->istate;
    case 0x418: /* Masked interrupt status */
        return s->istate | s->im;
    case 0x420: /* Alternate function select */
        return s->afsel;
    case 0x500: /* 2mA drive */
        return s->dr2r;
    case 0x504: /* 4mA drive */
        return s->dr4r;
    case 0x508: /* 8mA drive */
        return s->dr8r;
    case 0x50c: /* Open drain */
        return s->odr;
    case 0x510: /* Pull-up */
        return s->pur;
    case 0x514: /* Pull-down */
        return s->pdr;
    case 0x518: /* Slew rate control */
        return s->slr;
    case 0x51c: /* Digital enable */
        return s->den;
    case 0x520: /* Lock */
        return s->locked;
    case 0x524: /* Commit */
        return s->cr;
    case 0x528: /* Analog mode select */
        return s->amsel;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "netduino_gpio_read: Bad offset %x\n", (int)offset);
        return 0;
    }
}

static void netduino_gpio_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
    NETDUINO_GPIOState *s = (NETDUINO_GPIOState *)opaque;
    uint8_t mask;

    fprintf(stderr, "Data: %d\n", s->data);
    s->data = value;

    if (offset < 0x400) {
        mask = (offset >> 2) & s->dir;
        //fprintf(stderr, "Write %x, %x\n", value, offset);
        s->data = (s->data & ~mask) | (value & mask);
        netduino_gpio_update(s);
        return;
    }
    switch (offset) {
    case 0x400: /* Direction */
        s->dir = value & 0xff;
        break;
    case 0x404: /* Interrupt sense */
        s->isense = value & 0xff;
        break;
    case 0x408: /* Interrupt both edges */
        s->ibe = value & 0xff;
        break;
    case 0x40c: /* Interrupt event */
        s->iev = value & 0xff;
        break;
    case 0x410: /* Interrupt mask */
        s->im = value & 0xff;
        break;
    case 0x41c: /* Interrupt clear */
        s->istate &= ~value;
        break;
    case 0x420: /* Alternate function select */
        mask = s->cr;
        s->afsel = (s->afsel & ~mask) | (value & mask);
        break;
    case 0x500: /* 2mA drive */
        s->dr2r = value & 0xff;
        break;
    case 0x504: /* 4mA drive */
        s->dr4r = value & 0xff;
        break;
    case 0x508: /* 8mA drive */
        s->dr8r = value & 0xff;
        break;
    case 0x50c: /* Open drain */
        s->odr = value & 0xff;
        break;
    case 0x510: /* Pull-up */
        s->pur = value & 0xff;
        break;
    case 0x514: /* Pull-down */
        s->pdr = value & 0xff;
        break;
    case 0x518: /* Slew rate control */
        s->slr = value & 0xff;
        break;
    case 0x51c: /* Digital enable */
        s->den = value & 0xff;
        break;
    case 0x520: /* Lock */
        s->locked = (value != 0xacce551);
        break;
    case 0x524: /* Commit */
        if (!s->locked)
            s->cr = value & 0xff;
        break;
    case 0x528:
        s->amsel = value & 0xff;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "netduino_gpio_write: Bad offset %x\n", (int)offset);
    }
    netduino_gpio_update(s);
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
