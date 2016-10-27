/*
 * QEMU model of the ARM Generic Timer
 *
 * Copyright (c) 2016 Xilinx Inc.
 * Written by Alistair Francis <alistair.francis@xilinx.com>
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

#include "qemu/osdep.h"
#include "hw/timer/arm_generic_timer.h"
#include "qemu/timer.h"
#include "qemu/log.h"

#ifndef ARM_GEN_TIMER_ERR_DEBUG
#define ARM_GEN_TIMER_ERR_DEBUG 0
#endif

static void counter_control_postw(RegisterInfo *reg, uint64_t val64)
{
    ARMGenTimer *s = ARM_GEN_TIMER(reg->opaque);
    bool new_status = extract32(s->regs[R_CNTCR],
                                R_CNTCR_EN_SHIFT,
                                R_CNTCR_EN_LENGTH);
    uint64_t current_ticks;

    current_ticks = muldiv64(qemu_clock_get_us(QEMU_CLOCK_VIRTUAL),
                             NANOSECONDS_PER_SECOND, 1000000);

    if (s->enabled != new_status) {
        /* The timer is being disabled or enabled */
        s->tick_offset = current_ticks - s->tick_offset;
    }

    s->enabled = new_status;
}

static uint64_t counter_value_postr(RegisterInfo *reg)
{
    ARMGenTimer *s = ARM_GEN_TIMER(reg->opaque);
    uint64_t current_ticks, total_ticks;

    if (s->enabled) {
        current_ticks = muldiv64(qemu_clock_get_us(QEMU_CLOCK_VIRTUAL),
                                 NANOSECONDS_PER_SECOND, 1000000);
        total_ticks = current_ticks - s->tick_offset;
    } else {
        /* Timer is disabled, return the time when it was disabled */
        total_ticks = s->tick_offset;
    }

    return total_ticks;
}

static uint64_t counter_low_value_postr(RegisterInfo *reg, uint64_t val64)
{
    /* This could be a 64-bit or a 32-bit operation, let the register API
     * masking handle that.
     */
    return counter_value_postr(reg);
}

static uint64_t counter_high_value_postr(RegisterInfo *reg, uint64_t val64)
{
    /* This is always only a 32-bit value */
    return (uint32_t) (counter_value_postr(reg) >> 32);
}

static RegisterAccessInfo arm_gen_timer_regs_info[] = {
    {   .name = "CNTCR",
        .addr = A_CNTCR,
        .rsvd = 0xFC,
        .post_write = counter_control_postw,
    },{ .name = "CNTSR",
        .addr = A_CNTSR,
        .rsvd = 0xFD, .ro = ~0xFD,
    },{ .name = "CNTCV_LOWER",
        .addr = A_CNTCV_LOWER,
        .post_read = counter_low_value_postr,
    },{ .name = "CNTCV_UPPER",
        .addr = A_CNTCV_UPPER,
        .post_read = counter_high_value_postr,
    },{ .name = "CNTFID0",
        .addr = A_CNTFID0,
    }
    /* We don't model the CNTFIDn registers */
    ,{  .name = "CNTZERO",
        .addr = A_CNTZERO, .rsvd = 0xFFFFFFFF,
    }
    /* We don't model the CounterID registers either */
};

static void arm_gen_timer_reset(DeviceState *dev)
{
    ARMGenTimer *s = ARM_GEN_TIMER(dev);
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(s->regs_info); ++i) {
        register_reset(&s->regs_info[i]);
    }

    s->tick_offset = 0;
    s->enabled = false;
}

static const MemoryRegionOps arm_gen_timer_ops = {
    .read = register_read_memory,
    .write = register_write_memory,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 8,
    },
};

static const VMStateDescription vmstate_arm_gen_timer = {
    .name = TYPE_ARM_GEN_TIMER,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, ARMGenTimer, R_ARM_GEN_TIMER_MAX),
        VMSTATE_END_OF_LIST(),
    }
};

static void arm_gen_timer_init(Object *obj)
{
    ARMGenTimer *s = ARM_GEN_TIMER(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    RegisterInfoArray *reg_array;

    memory_region_init_io(&s->iomem, obj, &arm_gen_timer_ops, s,
                          TYPE_ARM_GEN_TIMER, R_ARM_GEN_TIMER_MAX * 4);
    reg_array =
        register_init_block32(DEVICE(obj), arm_gen_timer_regs_info,
                              ARRAY_SIZE(arm_gen_timer_regs_info),
                              s->regs_info, s->regs,
                              &arm_gen_timer_ops,
                              ARM_GEN_TIMER_ERR_DEBUG,
                              R_ARM_GEN_TIMER_MAX * 4);
    memory_region_add_subregion(&s->iomem,
                                A_CNTCR,
                                &reg_array->mem);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void arm_gen_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = arm_gen_timer_reset;
    dc->vmsd = &vmstate_arm_gen_timer;
}

static const TypeInfo arm_gen_timer_info = {
    .name          = TYPE_ARM_GEN_TIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(ARMGenTimer),
    .class_init    = arm_gen_timer_class_init,
    .instance_init = arm_gen_timer_init,
};

static void arm_gen_timer_register_types(void)
{
    type_register_static(&arm_gen_timer_info);
}

type_init(arm_gen_timer_register_types)
