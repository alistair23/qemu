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

/* #define DEBUG_NETGPIO */

#ifdef DEBUG_NETGPIO
#define DPRINTF(fmt, ...) \
do { printf("netduino_gpio: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while (0)
#endif

#define GPIO_MODER     0x00
#define GPIO_OTYPER    0x04
#define GPIO_OSPEEDR   0x08
#define GPIO_PUPDR     0x0C
#define GPIO_IDR       0x10
#define GPIO_ODR       0x14
#define GPIO_BSRR      0x18
#define GPIO_BSRR_HIGH 0x1A
#define GPIO_LCKR      0x1C
#define GPIO_AFRL      0x20
#define GPIO_AFRH      0x24

#define GPIO_MODER_INPUT       0
#define GPIO_MODER_GENERAL_OUT 1
#define GPIO_MODER_ALT         2
#define GPIO_MODER_ANALOG      3

#define TYPE_NETDUINO_GPIO "netduino_gpio"
#define NETDUINO_GPIO(obj) OBJECT_CHECK(NETDUINO_GPIOState, (obj), \
                           TYPE_NETDUINO_GPIO)

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

    /* This is an internal QEMU Register, used to determine the
     * GPIO direction as set by gpio_moder
     * 1: Input; 0: Output
     */
    uint16_t gpio_direction;
    /* The GPIO letter (a - k) from the datasheet */
    uint8_t gpio_letter;

    qemu_irq irq;
    const unsigned char *id;
} NETDUINO_GPIOState;

static const VMStateDescription vmstate_netduino_gpio = {
    .name = "netduino_gpio",
    .version_id = 1,
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

static void gpio_reset(DeviceState *dev)
{
    NETDUINO_GPIOState *s = NETDUINO_GPIO(dev);

    if (s->gpio_letter == 'a') {
        s->gpio_moder = 0xA8000000;
        s->gpio_pupdr = 0x64000000;
        s->gpio_ospeedr = 0x00000000;
    } else if (s->gpio_letter == 'b') {
        s->gpio_moder = 0x00000280;
        s->gpio_pupdr = 0x00000100;
        s->gpio_ospeedr = 0x000000C0;
    } else {
        s->gpio_moder = 0x00000000;
        s->gpio_pupdr = 0x00000000;
        s->gpio_ospeedr = 0x00000000;
    }

    s->gpio_otyper = 0x00000000;
    s->gpio_idr = 0x00000000;
    s->gpio_odr = 0x00000000;
    s->gpio_bsrr = 0x00000000;
    s->gpio_lckr = 0x00000000;
    s->gpio_afrl = 0x00000000;
    s->gpio_afrh = 0x00000000;
    s->gpio_direction = 0x0000;
}

static uint64_t netduino_gpio_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    NETDUINO_GPIOState *s = (NETDUINO_GPIOState *)opaque;

    DPRINTF("Read 0x%x\n", (uint) offset);

    switch (offset) {
    case GPIO_MODER:
        return s->gpio_moder;
    case GPIO_OTYPER:
        return s->gpio_otyper;
    case GPIO_OSPEEDR:
        return s->gpio_ospeedr;
    case GPIO_PUPDR:
        return s->gpio_pupdr;
    case GPIO_IDR:
        /* This register changes based on the external GPIO pins */
        return s->gpio_idr & s->gpio_direction;
    case GPIO_ODR:
        return s->gpio_odr;
    case GPIO_BSRR_HIGH:
        /* gpio_bsrr reads as zero */
        return 0xFFFF;
    case GPIO_BSRR:
        /* gpio_bsrr reads as zero */
        return 0x00000000;
    case GPIO_LCKR:
        return s->gpio_lckr;
    case GPIO_AFRL:
        return s->gpio_afrl;
    case GPIO_AFRH:
        return s->gpio_afrh;
    }
    return 0;
}

static void netduino_gpio_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
    NETDUINO_GPIOState *s = (NETDUINO_GPIOState *)opaque;
    int i, mask;

    DPRINTF("Write 0x%x, 0x%x\n", (uint) value, (uint) offset);

    switch (offset) {
    case GPIO_MODER:
        s->gpio_moder = (uint32_t) value;
        for (i = 0; i < 32; i = i + 2) {
            /* Two bits determine the I/O direction/mode */
            mask = (1 << i) + (1 << (i + 1));

            if ((s->gpio_moder & mask) == GPIO_MODER_INPUT) {
                s->gpio_direction |= (1 << (i/2));
            } else if ((s->gpio_moder & mask) == GPIO_MODER_GENERAL_OUT) {
                s->gpio_direction &= (0xFFFF ^ (1 << (i/2)));
            } else {
                /* Not supported at the moment */
            }
        }
        return;
    case GPIO_OTYPER:
        s->gpio_otyper = (uint32_t) value;
        return;
    case GPIO_OSPEEDR:
        s->gpio_ospeedr = (uint32_t) value;
        return;
    case GPIO_PUPDR:
        s->gpio_pupdr = (uint32_t) value;
        return;
    case GPIO_IDR:
        /* Read Only Register */
        qemu_log_mask(LOG_GUEST_ERROR,
                      "net_gpio%c_write: Read Only Register 0x%x\n",
                      s->gpio_letter, (int)offset);
        return;
    case GPIO_ODR:
        s->gpio_odr = ((uint32_t) value & (s->gpio_direction ^ 0xFFFF));
        return;
    case GPIO_BSRR_HIGH:
        /* Reset the output value */
        s->gpio_odr &= (uint32_t) (value ^ 0xFFFF);
        s->gpio_bsrr = (uint32_t) (value << 16);
        DPRINTF("Output: 0x%x\n", s->gpio_odr);
        return;
    case GPIO_BSRR:
        /* Reset the output value */
        s->gpio_odr &= (uint32_t) ((value >> 16) ^ 0xFFFF);
        /* Sets the output value */
        s->gpio_odr |= (uint32_t) (value & 0xFFFF);
        s->gpio_bsrr = (uint32_t) value;
        DPRINTF("Output: 0x%x\n", s->gpio_odr);
        return;
    case GPIO_LCKR:
        s->gpio_lckr = (uint32_t) value;
        return;
    case GPIO_AFRL:
        s->gpio_afrl = (uint32_t) value;
        return;
    case GPIO_AFRH:
        s->gpio_afrh = (uint32_t) value;
        return;
    }
}

static const MemoryRegionOps netduino_gpio_ops = {
    .read = netduino_gpio_read,
    .write = netduino_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static Property net_gpio_properties[] = {
    DEFINE_PROP_UINT8("gpio-letter", NETDUINO_GPIOState, gpio_letter,
                      (uint) 'a'),
    DEFINE_PROP_END_OF_LIST(),
};


static int netduino_gpio_initfn(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    NETDUINO_GPIOState *s = NETDUINO_GPIO(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &netduino_gpio_ops, s,
                          "netduino_gpio", 0x2000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
    return 0;
}

static void netduino_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = netduino_gpio_initfn;
    dc->vmsd = &vmstate_netduino_gpio;
    dc->props = net_gpio_properties;
    dc->reset = gpio_reset;
}

static const TypeInfo netduino_gpio_info = {
    .name          = TYPE_NETDUINO_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(NETDUINO_GPIOState),
    .class_init    = netduino_gpio_class_init,
};

static void netduino_gpio_register_types(void)
{
    type_register_static(&netduino_gpio_info);
}

type_init(netduino_gpio_register_types)
