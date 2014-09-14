/*
 * STM32F205 SoC
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

#ifndef HW_ARM_STM32F205SOC_H
#define HW_ARM_STM32F205SOC_H

#include "hw/sysbus.h"
#include "hw/arm/arm.h"
#include "hw/ssi.h"
#include "hw/devices.h"
#include "qemu/timer.h"
#include "net/net.h"
#include "elf.h"
#include "hw/loader.h"
#include "hw/boards.h"
#include "exec/address-spaces.h"
#include "qemu/error-report.h"
#include "sysemu/qtest.h"
#include "hw/misc/stm32f205_syscfg.h"
#include "hw/timer/stm32f205_timer.h"
#include "hw/char/stm32f205_usart.h"

#define TYPE_STM32F205_SOC "stm32f205_soc"
#define STM32F205_SOC(obj) \
    OBJECT_CHECK(STM32F205State, (obj), TYPE_STM32F205_SOC)

#define STM_NUM_USARTS 5
#define STM_NUM_TIMERS 5

#define FLASH_BASE_ADDRESS 0x08000000
#define FLASH_SIZE (1024 * 1024)
#define SRAM_BASE_ADDRESS 0x20000000
#define SRAM_SIZE (128 * 1024)

typedef struct STM32F205State {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    char *kernel_filename;
    char *cpu_model;

    STM32f205SyscfgState syscfg;
    STM32f205UsartState usart[STM_NUM_USARTS];
    STM32f205TimerState timer[STM_NUM_TIMERS];
} STM32F205State;

#endif
