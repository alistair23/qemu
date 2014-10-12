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

#include "hw/sysbus.h"

/* TCP External Access to GPIO
 * This is based on the work by Biff Eros
 * https://sites.google.com/site/bifferboard/Home/howto/qemu
 */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
/* END TCP External Access to GPIO */

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

#define TYPE_STM32F405_GPIO "stm32f405-gpio"
#define STM32F405_GPIO(obj) OBJECT_CHECK(Stm32f405GpioState, (obj), \
                           TYPE_STM32F405_GPIO)

/* WARNING: Using the GPIO external access makes QEMU slow and unstable.
 * It is currently in alpha and constantly changing.
 * Use at your own risk!
 */

#define EXTERNAL_TCP_ACCESS 0

#if EXTERNAL_TCP_ACCESS
/* TCP External Access to GPIO
 * This is based on the work by Biff Eros
 * https://sites.google.com/site/bifferboard/Home/howto/qemu
 */
#define GPIO_PINS 16
#define PANEL_PORT 4321

typedef struct gpio_tcp_connection {
    int socket;
    fd_set fds;
} gpio_tcp_connection;
/* END TCP External Access to GPIO */
#endif

typedef struct Stm32f405GpioState {
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

    /* This is an internal QEMU Register, used by QEMU to do operations on
     * the GPIO direction as set by gpio_moder
     * 1: Input; 0: Output
     */
    uint16_t gpio_direction;
    /* The GPIO letter (a - k) from the datasheet */
    uint8_t gpio_letter;

    qemu_irq gpio_out[16];
    const unsigned char *id;

    #if EXTERNAL_TCP_ACCESS
    /* TCP External Access to GPIO
     * This is based on the work by Biff Eros
     * https://sites.google.com/site/bifferboard/Home/howto/qemu
     */
     gpio_tcp_connection tcp_info;

     QEMUTimer *timer;
     uint32_t tick_offset;
     uint32_t prev_out;
     /* END TCP External Access to GPIO */
     #endif
} Stm32f405GpioState;
