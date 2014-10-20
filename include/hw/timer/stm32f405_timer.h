/*
 * STM32F405 Timer
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

#ifndef HW_STM_TIMER_H
#define HW_STM_TIMER_H

#include "hw/sysbus.h"
#include "qemu/timer.h"
#include "sysemu/sysemu.h"

#define TIM_CR1      0x00
#define TIM_CR2      0x04
#define TIM_SMCR     0x08
#define TIM_DIER     0x0C
#define TIM_SR       0x10
#define TIM_EGR      0x14
#define TIM_CCMR1    0x18
#define TIM_CCMR2    0x1C
#define TIM_CCER     0x20
#define TIM_CNT      0x24
#define TIM_PSC      0x28
#define TIM_ARR      0x2C
#define TIM_CCR1     0x34
#define TIM_CCR2     0x38
#define TIM_CCR3     0x3C
#define TIM_CCR4     0x40
#define TIM_DCR      0x48
#define TIM_DMAR     0x4C
#define TIM_OR       0x50

#define TIM_CR1_CEN   1

#define TIM_EGR_UG 1

#define TIM_CCER_CC2E   (1 << 4)
#define TIM_CCER_CC3E   (1 << 8)
#define TIM_CCER_CC4E   (1 << 12)

#define TIM_CCMR1_OC2M2 (1 << 14)
#define TIM_CCMR1_OC2M1 (1 << 13)
#define TIM_CCMR1_OC2M0 (1 << 12)
#define TIM_CCMR1_OC2PE (1 << 11)

#define TIM_CCMR2_OC3M2 (1 << 6)
#define TIM_CCMR2_OC3M1 (1 << 5)
#define TIM_CCMR2_OC3M0 (1 << 4)
#define TIM_CCMR2_OC3PE (1 << 3)

#define TIM_CCMR2_OC4M2 (1 << 14)
#define TIM_CCMR2_OC4M1 (1 << 13)
#define TIM_CCMR2_OC4M0 (1 << 12)
#define TIM_CCMR2_OC4PE (1 << 11)

#define TIM_DIER_UIE  1

#define TYPE_STM32F405_TIMER "stm32f405-timer"
#define STM32F405TIMER(obj) OBJECT_CHECK(STM32f405TimerState, \
                            (obj), TYPE_STM32F405_TIMER)

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
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define GPIO_PINS 16
#define PANEL_PORT 4321

typedef struct pwm_tcp_connection {
    int socket;
    fd_set fds;
} pwm_tcp_connection;
/* END TCP External Access to PWM */
#endif

typedef struct STM32f405TimerState {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion iomem;
    QEMUTimer *timer;
    qemu_irq irq;

    uint32_t tick_offset;
    uint64_t freq_hz;

    uint32_t tim_cr1;
    uint32_t tim_cr2;
    uint32_t tim_smcr;
    uint32_t tim_dier;
    uint32_t tim_sr;
    uint32_t tim_egr;
    uint32_t tim_ccmr1;
    uint32_t tim_ccmr2;
    uint32_t tim_ccer;
    uint32_t tim_cnt;
    uint32_t tim_psc;
    uint32_t tim_arr;
    uint32_t tim_ccr1;
    uint32_t tim_ccr2;
    uint32_t tim_ccr3;
    uint32_t tim_ccr4;
    uint32_t tim_dcr;
    uint32_t tim_dmar;
    uint32_t tim_or;

    #if EXTERNAL_TCP_ACCESS
    /* TCP External Access to GPIO
     * This is based on the work by Biff Eros
     * https://sites.google.com/site/bifferboard/Home/howto/qemu
     */
     pwm_tcp_connection tcp_info;

     int pwm_angle;
     /* END TCP External Access to GPIO */
     #endif
} STM32f405TimerState;

#endif
