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

#define FLASH_BASE_ADDRESS 0x08000000
#define FLASH_SIZE (1024 * 1024)
#define SRAM_BASE_ADDRESS 0x20000000
#define SRAM_SIZE (192 * 1024)

typedef struct ARMV7MResetArgs {
    ARMCPU *cpu;
    uint32_t reset_sp;
    uint32_t reset_pc;
} ARMV7MResetArgs;

static void armv7m_reset(void *opaque)
{
    ARMV7MResetArgs *args = opaque;

    cpu_reset(CPU(args->cpu));

    args->cpu->env.regs[13] = args->reset_sp & 0xFFFFFFFC;
    args->cpu->env.thumb = args->reset_pc & 1;
    args->cpu->env.regs[15] = args->reset_pc & ~1;
}

static void netduinoplus2_init(MachineState *machine)
{
    static const uint32_t gpio_addr[] = { 0x40020000, 0x40020400, 0x40020800,
        0x40020C00, 0x40021000, 0x40021400, 0x40021800, 0x40021C00,
        0x40022000, 0x40022400, 0x40022800 };
    static const uint8_t gpio_letters[] = { 'a', 'b', 'c',
        'd', 'e', 'f', 'g', 'h',
        'i', 'j', 'k' };
    static const uint32_t tim2_5_addr[] = { 0x40000000, 0x40000400,
        0x40000800, 0x40000C00 };
    static const uint32_t usart_addr[] = { 0x40011000, 0x40004400,
        0x40004800, 0x40004C00, 0x40005000, 0x40011400, 0x40007800,
        0x40007C00 };
    const char *kernel_filename = machine->kernel_filename;

    static const int tim2_5_irq[] = {28, 29, 30, 50};
    static const int usart_irq[] = {37, 38, 39, 52, 53, 71, 82, 83};
    static const int exti_irq[] = {6, 7, 8, 9, 10, 23, 23, 23, 23, 23, 40, 40, 40, 40, 40, 40};

    MemoryRegion *system_memory = get_system_memory();
    MemoryRegion *sram = g_new(MemoryRegion, 1);
    MemoryRegion *flash = g_new(MemoryRegion, 1);
    MemoryRegion *flash_alias = g_new(MemoryRegion, 1);
    MemoryRegion *hack = g_new(MemoryRegion, 1);
    ARMV7MResetArgs reset_args;

    qemu_irq pic[96];
    ARMCPU *cpu;
    CPUARMState *env;
    DeviceState *nvic;
    DeviceState *gpio[11];
    DeviceState *dev;
    SysBusDevice *busdev;
    //qemu_irq gpio_in[11][15];
    //qemu_irq gpio_out[11][15];

    int image_size;
    uint64_t entry;
    uint64_t lowaddr;
    int i, j;
    int big_endian = 0;

    /* The Netduinio Plus 2 uses a Cortex-M4, while QEMU currently supports
     * the Cortex-M3, so that is being used instead
     */
    cpu = cpu_arm_init("cortex-m3");
    env = &cpu->env;

    memory_region_init_ram(flash, NULL, "netduino.flash", FLASH_SIZE);
    memory_region_init_alias(flash_alias, NULL, "netduino.flash.alias",
                             flash, 0, FLASH_SIZE);

    vmstate_register_ram_global(flash);

    memory_region_set_readonly(flash, true);
    memory_region_set_readonly(flash_alias, true);

    memory_region_add_subregion(system_memory, FLASH_BASE_ADDRESS, flash);
    memory_region_add_subregion(system_memory, 0, flash_alias);

    memory_region_init_ram(sram, NULL, "netduino.sram", SRAM_SIZE);
    vmstate_register_ram_global(sram);
    memory_region_add_subregion(system_memory, SRAM_BASE_ADDRESS, sram);

    nvic = qdev_create(NULL, "armv7m_nvic");
    qdev_prop_set_uint32(nvic, "num-irq", 96);
    env->nvic = nvic;
    qdev_init_nofail(nvic);
    sysbus_connect_irq(SYS_BUS_DEVICE(nvic), 0,
                       qdev_get_gpio_in(DEVICE(cpu), ARM_CPU_IRQ));
    for (i = 0; i < 96; i++) {
        pic[i] = qdev_get_gpio_in(nvic, i);
    }

    /* System configuration controller */
    sysbus_create_simple("stn32f405xx-syscfg", 0x40013800,
                         pic[71]);

    /* Attach a UART (uses USART registers) and USART controllers */
    for (i = 0; i < 7; i++) {
        sysbus_create_simple("stn32f405xx-usart", usart_addr[i],
                             pic[usart_irq[i]]);
    }

    /* Timer 2 to 5 */
    for (i = 0; i < 4; i++) {
        sysbus_create_simple("stn32f405xx-timer", tim2_5_addr[i],
                             pic[tim2_5_irq[i]]);
    }

    /* Attach GPIO devices */
    for (i = 0; i < 11; i++) {
        gpio[i] = qdev_create(NULL, "stn32f405xx-gpio");
        qdev_prop_set_uint8(gpio[i], "gpio-letter", gpio_letters[i]);
        qdev_init_nofail(gpio[i]);
        busdev = SYS_BUS_DEVICE(gpio[i]);
        sysbus_mmio_map(busdev, 0, gpio_addr[i]);
    }

    /* Attach the External Interrupt */
    dev = qdev_create(NULL, "stn32f405xx-exti");
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x40013C00);
    for (i = 0; i < 15; ++i) {
        sysbus_connect_irq(busdev, i, pic[exti_irq[i]]);
    }
    for (i = 0; i < 9; ++i) {
        for (j = 0; j < 15; j++) {
            qdev_connect_gpio_out(gpio[i], j, qdev_get_gpio_in(dev, (i * 15) + j));
        }
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
            image_size = load_image_targphys(kernel_filename, 0, FLASH_SIZE);
            lowaddr = 0;
        }
        if (image_size < 0) {
            error_report("Could not load kernel '%s'", kernel_filename);
            exit(1);
        }
    }

    /* Hack to map an additional page of ram at the top of the address
     * space.  This stops qemu complaining about executing code outside RAM
     * when returning from an exception.
     */
    memory_region_init_ram(hack, NULL, "netduino.hack", 0x1000);
    vmstate_register_ram_global(hack);
    memory_region_set_readonly(hack, true);
    memory_region_add_subregion(system_memory, 0xfffff000, hack);

    reset_args = (ARMV7MResetArgs) {
        .cpu = cpu,
        .reset_pc = entry,
        .reset_sp = (SRAM_BASE_ADDRESS + (SRAM_SIZE * 2)/3),
    };
    qemu_register_reset(armv7m_reset,
                        g_memdup(&reset_args, sizeof(reset_args)));
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
