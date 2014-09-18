/*
 * STM32F405 GPIO
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

#include "hw/char/stm32f405_gpio.h"

#ifndef ST_GPIO_ERR_DEBUG
#define ST_GPIO_ERR_DEBUG 0
#endif

#ifndef DB_PRINT_L
#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (ST_GPIO_ERR_DEBUG >= lvl) { \
        fprintf(stderr, "stm32f405_gpio: %s:" fmt, __func__, ## args); \
    } \
} while (0);

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)
#endif

#if (EXTERNAL_TCP_ACCESS == 1)
/* TCP External Access to GPIO
 * This is based on the work by Biff Eros
 * https://sites.google.com/site/bifferboard/Home/howto/qemu
 */
static int tcp_connection_open(gpio_tcp_connection* c)
{
    struct sockaddr_in remote;

    if ((c->socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "%s: Socket creation failed\n", __func__);
        return -1;
    }

    remote.sin_family = AF_INET;
    remote.sin_port = htons(PANEL_PORT);
    remote.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(c->socket, (struct sockaddr *)&remote, sizeof(remote)) == -1) {
        fprintf(stderr, "%s: Connection creation failed\n", __func__);
        close(c->socket);
        c->socket = -1;
        return -1;
    }

    FD_ZERO(&c->fds);

    /* Set our connected socket */
    FD_SET(c->socket, &c->fds);

    DB_PRINT("Connection successful\n");
    return 0;
}

static void tcp_connection_command(gpio_tcp_connection* c, const char* command)
{
    if (send(c->socket, command, strlen(command), 0) == -1) {
        fprintf(stderr, "%s: Sending failed\n", __func__);
        exit(1);
    }
}


static int tcp_connection_getpins(gpio_tcp_connection* c,
                                  const char* command, uint32_t* reg)
{
    char str[100];
    fd_set rfds, efds;
    int t, i;
    /* Wait for a response from peripheral, assume it arrives all together
     * (likely since it's only a few bytes long)
     */

    rfds = c->fds;
    efds = c->fds;

    if (select(c->socket + 1, &rfds, NULL, &efds, NULL) == -1) {
        fprintf(stderr, "%s: Select failed\n", __func__);
        return 1;
    }

    if (FD_ISSET(c->socket, &rfds)) {
        /* Receive Data */
        if ((t = recv(c->socket, str, sizeof(str)-1, 0)) > 0) {
            str[t] = '\0';
            if (strncmp(str, command, strlen(command)) == 0) {
                for (i = 0; i < strlen(str); i++) {
                    if (str[i] == '1') {
                        *reg |= (1 << i);
                    }
                }
            } else {
                DB_PRINT("Invalid data recieved\n");
            }
        } else {
            if (t < 0) {
                perror("recv");
            }
            else {
                DB_PRINT("Connection closed\n");
            }
            exit(1);
        }
    }

    if (FD_ISSET(c->socket, &efds)) {
        DB_PRINT("Connection closed\n");
        exit(1);
    }
    return 0;
}


static void gpio_pin_write(gpio_tcp_connection* c, char gpio_letter,
                           hwaddr addr, uint32_t reg)
{
    char command[100];

    sprintf(command, "GPIO W %c 0x%" HWADDR_PRIx " %x \r\n", gpio_letter,
            addr, reg);
    tcp_connection_command(c, command);
}


static uint32_t gpio_pin_read(gpio_tcp_connection* c,
                              char gpio_letter, hwaddr addr)
{
    char command[100];
    int len = 1;
    /* Assume all values are low by default */
    uint32_t out = 0x00000000;

    sprintf(command, "GPIO R %c 0x%" HWADDR_PRIx "\r\n", gpio_letter, addr);
    tcp_connection_command(c, command);

    sprintf(command, "GPIO R %c ", gpio_letter);
    while (len) {
        len = tcp_connection_getpins(c, command, &out);
    }

    return out;
}
/* END TCP External Access to GPIO */
#endif

static const VMStateDescription vmstate_stm32f405_gpio = {
    .name = "stm32f405_gpio",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(gpio_moder, Stm32f405GpioState),
        VMSTATE_UINT32(gpio_otyper, Stm32f405GpioState),
        VMSTATE_UINT32(gpio_ospeedr, Stm32f405GpioState),
        VMSTATE_UINT32(gpio_pupdr, Stm32f405GpioState),
        VMSTATE_UINT32(gpio_idr, Stm32f405GpioState),
        VMSTATE_UINT32(gpio_odr, Stm32f405GpioState),
        VMSTATE_UINT32(gpio_bsrr, Stm32f405GpioState),
        VMSTATE_UINT32(gpio_lckr, Stm32f405GpioState),
        VMSTATE_UINT32(gpio_afrl, Stm32f405GpioState),
        VMSTATE_UINT32(gpio_afrh, Stm32f405GpioState),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32f405_gpio_reset(DeviceState *dev)
{
    Stm32f405GpioState *s = STM32F405_GPIO(dev);

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

static uint64_t stm32f405_gpio_read(void *opaque, hwaddr offset,
                                    unsigned size)
{
    Stm32f405GpioState *s = (Stm32f405GpioState *)opaque;

    DB_PRINT("Read 0x%x\n", (uint) offset);

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
        #if (EXTERNAL_TCP_ACCESS == 1)
        s->gpio_idr = gpio_pin_read(&s->tcp_info, s->gpio_letter, offset);
        #endif
        return s->gpio_idr & s->gpio_direction;
    case GPIO_ODR:
        return s->gpio_odr;
    case GPIO_BSRR_HIGH:
        return 0x0000;
    case GPIO_BSRR:
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

static void stm32f405_gpio_set_irq(void * opaque, int irq, int level)
{
    Stm32f405GpioState *s = (Stm32f405GpioState *)opaque;

    DB_PRINT("Line: %d Level: %d\n", irq, level);

    s->gpio_odr |= level << irq;

    qemu_set_irq(s->gpio_out[irq], !!((level << irq) & (0xFFFF ^ s->gpio_direction)));
}

static void stm32f405_gpio_write(void *opaque, hwaddr offset,
                                   uint64_t value, unsigned size)
{
    Stm32f405GpioState *s = (Stm32f405GpioState *)opaque;
    int i, mask;

    DB_PRINT("Write 0x%x, 0x%x\n", (uint) value, (uint) offset);

    switch (offset) {
    case GPIO_MODER:
        s->gpio_moder = (uint32_t) value;
        for (i = 0; i < 15; i++) {
            /* Two bits determine the I/O direction/mode */
            mask = 3U << (i * 2);

            if ((s->gpio_moder & mask) == GPIO_MODER_INPUT) {
                s->gpio_direction |= (1 << i);
            } else if ((s->gpio_moder & mask) == GPIO_MODER_GENERAL_OUT) {
                s->gpio_direction &= (0xFFFF ^ (1 << i));
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
                      "STM32F405_gpio%c_write: Read Only Register 0x%x\n",
                      s->gpio_letter, (int)offset);
        return;
    case GPIO_ODR:
        #if (EXTERNAL_TCP_ACCESS == 1)
        gpio_pin_write(&s->tcp_info, s->gpio_letter, offset, value);
        #endif
        s->gpio_odr = ((uint32_t) value & (~s->gpio_direction));
        return;
    case GPIO_BSRR_HIGH:
        /* Reset the output value */
        s->gpio_odr &= (uint32_t) (value ^ 0xFFFF);
        s->gpio_bsrr = (uint32_t) (value << 16);
        DB_PRINT("Output: 0x%x\n", s->gpio_odr);
        return;
    case GPIO_BSRR:
        /* Top 16 bits are "write one to clear output" */
        s->gpio_odr &= (uint32_t) ((value >> 16) ^ 0xFFFF);
        /* Bottom 16 bits are "write one to set output" */
        s->gpio_odr |= (uint32_t) (value & 0xFFFF);
        s->gpio_bsrr = (uint32_t) value;
        DB_PRINT("Output: 0x%x\n", s->gpio_odr);
        return;
    case GPIO_LCKR:
        s->gpio_lckr = (uint32_t) value;
        /* Unimplemented */
        return;
    case GPIO_AFRL:
        s->gpio_afrl = (uint32_t) value;
        return;
    case GPIO_AFRH:
        s->gpio_afrh = (uint32_t) value;
        return;
    }
}

static const MemoryRegionOps stm32f405_gpio_ops = {
    .read = stm32f405_gpio_read,
    .write = stm32f405_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static Property stm32f405_gpio_properties[] = {
    DEFINE_PROP_UINT8("gpio-letter", Stm32f405GpioState, gpio_letter,
                      (uint) 'a'),
    DEFINE_PROP_END_OF_LIST(),
};


static void stm32f405_gpio_initfn(Object *obj)
{
    Stm32f405GpioState *s = STM32F405_GPIO(obj);

    memory_region_init_io(&s->iomem, obj, &stm32f405_gpio_ops, s,
                          "stm32f405_gpio", 0x2000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);

    qdev_init_gpio_in(DEVICE(obj), stm32f405_gpio_set_irq, 15);
    qdev_init_gpio_out(DEVICE(obj), s->gpio_out, 15);

    #if (EXTERNAL_TCP_ACCESS == 1)
    /* TCP External Access to GPIO
     * This is based on the work by Biff Eros
     * https://sites.google.com/site/bifferboard/Home/howto/qemu
     */
    tcp_connection_open(&s->tcp_info);
    /* END TCP External Access to GPIO */
    #endif
}

static void stm32f405_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_stm32f405_gpio;
    dc->props = stm32f405_gpio_properties;
    dc->reset = stm32f405_gpio_reset;
}

static const TypeInfo stm32f405_gpio_info = {
    .name          = TYPE_STM32F405_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Stm32f405GpioState),
    .instance_init = stm32f405_gpio_initfn,
    .class_init    = stm32f405_gpio_class_init,
};

static void stm32f405_gpio_register_types(void)
{
    type_register_static(&stm32f405_gpio_info);
}

type_init(stm32f405_gpio_register_types)
