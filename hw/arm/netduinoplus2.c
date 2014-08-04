/*
 * Netduino Plus 2 Machine Model
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
#include "hw/ssi.h"
#include "hw/arm/arm.h"
#include "hw/devices.h"
#include "qemu/timer.h"
#include "net/net.h"
#include "elf.h"
#include "hw/loader.h"
#include "hw/boards.h"
#include "exec/address-spaces.h"
#include "qemu/error-report.h"
#include "sysemu/qtest.h"

static void armv7m_reset(void *opaque)
{
    ARMCPU *cpu = opaque;

    cpu_reset(CPU(cpu));
}

static void netduinoplus2_init(MachineState *machine)
{
    static const uint32_t gpio_addr[9] =
      { 0x40020000, 0x40020400, 0x40020800, 0x40020C00, 0x40021000,
        0x40021400, 0x40021800, 0x40021C00, 0x40022000};
    const char *kernel_filename = machine->kernel_filename;

    MemoryRegion *address_space_mem = get_system_memory();
    MemoryRegion *sram = g_new(MemoryRegion, 1);
    MemoryRegion *flash = g_new(MemoryRegion, 1);
    DeviceState *gpio_dev[9];

    qemu_irq gpio_in[7][8];
    qemu_irq gpio_out[7][8];
    qemu_irq pic[64];
    ARMCPU *cpu;
    CPUARMState *env;
    DeviceState *nvic;

    int image_size;
    uint64_t entry;
    uint64_t lowaddr;
    int i, j;
    int big_endian = 0;

    /* The Netduinio Plus 2 uses a Cortex-M4, while QEMU currently supports
     * the Cortex-M3, so that is being used instead */
    cpu = cpu_arm_init("cortex-m3");
    env = &cpu->env;

    memory_region_init_ram(flash, NULL, "netduino.flash", 1024 * 1024);
    vmstate_register_ram_global(flash);
    memory_region_set_readonly(flash, true);
    memory_region_add_subregion(address_space_mem, 0x00000000, flash);

    memory_region_init_ram(sram, NULL, "netduino.sram", 192 * 1024);
    vmstate_register_ram_global(sram);
    memory_region_add_subregion(address_space_mem, 0x20000000, sram);

    nvic = qdev_create(NULL, "armv7m_nvic");
    env->nvic = nvic;
    qdev_init_nofail(nvic);
    sysbus_connect_irq(SYS_BUS_DEVICE(nvic), 0,
                       qdev_get_gpio_in(DEVICE(cpu), ARM_CPU_IRQ));
    for (i = 0; i < 64; i++) {
        pic[i] = qdev_get_gpio_in(nvic, i);
    }

    /* Load the kernel */
    if (!kernel_filename && !qtest_enabled()) {
        fprintf(stderr, "Guest image must be specified (using -kernel)\n");
        exit(1);
    }
    if (kernel_filename) {
        image_size = load_elf(kernel_filename, NULL, NULL, &entry, &lowaddr,
                              NULL, big_endian, ELF_MACHINE, 1);
        if (image_size < 0) {
            image_size = load_image_targphys(kernel_filename, 0, 1024 * 1024);
            lowaddr = 0;
        }
        if (image_size < 0) {
            error_report("Could not load kernel '%s'", kernel_filename);
            exit(1);
        }
    }

    qemu_register_reset(armv7m_reset, cpu);

    /* Attach a sample UART controller */
    sysbus_create_simple("netduino_uart", 0x40004C00,
                         pic[52]);

    /* Attach GPIO devices */
    for (i = 0; i < 9; i++) {
        gpio_dev[i] = sysbus_create_simple("netduino_gpio", gpio_addr[i],
                                           pic[i]);
        for (j = 0; j < 8; j++) {
            gpio_in[i][j] = qdev_get_gpio_in(gpio_dev[i], j);
            gpio_out[i][j] = NULL;
        }
    }
}

static QEMUMachine netduinoplus2_machine = {
    .name = "netduinoplus2",
    .desc = "Netduino Plus 2 Machine (Except with a Cortex-M3)",
    .init = netduinoplus2_init,
};

static void netduinoplus2_machine_init(void)
{
    qemu_register_machine(&netduinoplus2_machine);
}

machine_init(netduinoplus2_machine_init);
