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

/* The SPI device the nRF240L01+ device is connected to
 * Set to zero to remove it
 */
#define NRF24L01PLUS_SPI_NUMBER 2

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
    static const uint32_t timer_addr[] = { 0x40000000, 0x40000400,
        0x40000800, 0x40000C00 };
    static const uint32_t usart_addr[] = { 0x40011000, 0x40004400,
        0x40004800, 0x40004C00, 0x40005000, 0x40011400, 0x40007800,
        0x40007C00 };
    static const uint32_t spi_addr[] = { 0x40013000, 0x40003800, 0x40003C00,
                                         0x40013400, 0x40015000, 0x40015400 };
    const char *kernel_filename = machine->kernel_filename;

    static const int spi_irq[] = {35, 36, 51, 0, 0, 0};
    static const int timer_irq[] = {28, 29, 30, 50};
    static const int usart_irq[] = {37, 38, 39, 52, 53, 71, 82, 83};
    static const int exti_irq[] = {6, 7, 8, 9, 10, 23, 23, 23, 23, 23, 40,
                                   40, 40, 40, 40, 40};
    const char *cpu_model = machine->cpu_model;

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
    DeviceState *syscfg;
    DeviceState *dev;
    SysBusDevice *busdev;
    void *bus;
    qemu_irq gpio_in[11][16];

    int image_size;
    uint64_t entry;
    uint64_t lowaddr;
    int i, j;
    int big_endian = 0;

    /* The Netduinio Plus 2 uses a Cortex-M4, while QEMU currently supports
     * the Cortex-M3, so that is being used instead
     */
    if (!cpu_model) {
        cpu_model = "cortex-m3";
    }
    cpu = cpu_arm_init(cpu_model);
    env = &cpu->env;

    memory_region_init_ram(flash, NULL, "netduino.flash", FLASH_SIZE,
                           &error_abort);
    memory_region_init_alias(flash_alias, NULL, "netduino.flash.alias",
                             flash, 0, FLASH_SIZE);

    vmstate_register_ram_global(flash);

    memory_region_set_readonly(flash, true);
    memory_region_set_readonly(flash_alias, true);

    memory_region_add_subregion(system_memory, FLASH_BASE_ADDRESS, flash);
    memory_region_add_subregion(system_memory, 0, flash_alias);

    memory_region_init_ram(sram, NULL, "netduino.sram", SRAM_SIZE,
                           &error_abort);
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

    /* Attach UART (uses USART registers) and USART controllers */
    for (i = 0; i < 7; i++) {
        sysbus_create_simple("stm32f405-usart", usart_addr[i],
                             pic[usart_irq[i]]);
    }

    /* Timer 2 to 5 */
    for (i = 0; i < 4; i++) {
        sysbus_create_simple("stm32f405-timer", timer_addr[i],
                             pic[timer_irq[i]]);
    }

    /* Attach GPIO devices */
    for (i = 0; i < 11; i++) {
        gpio[i] = qdev_create(NULL, "stm32f405-gpio");
        qdev_prop_set_uint8(gpio[i], "gpio-letter", gpio_letters[i]);
        qdev_init_nofail(gpio[i]);
        busdev = SYS_BUS_DEVICE(gpio[i]);
        sysbus_mmio_map(busdev, 0, gpio_addr[i]);
        for (j = 0; j < 16; j++) {
            gpio_in[i][j] = qdev_get_gpio_in(gpio[i], j);
            qemu_set_irq(gpio_in[i][j], 0);
        }
    }

    /* System configuration controller */
    syscfg = qdev_create(NULL, "stm32f405-syscfg");
    qdev_init_nofail(syscfg);
    busdev = SYS_BUS_DEVICE(syscfg);
    sysbus_mmio_map(busdev, 0, 0x40013800);
    sysbus_connect_irq(busdev, 0, pic[71]);
    for (i = 0; i < 9; ++i) {
        for (j = 0; j < 16; j++) {
            qdev_connect_gpio_out(gpio[i], j, qdev_get_gpio_in(syscfg,
                                  (i * 16) + j));
        }
    }

    /* Attach the ADC */
    for (i = 0; i < 3; i++) {
        sysbus_create_simple("stm32f405-adc", 0x40012000 + (i * 0x100),
                             pic[18]);
    }

    /* Attach the SPI Controller */
    for (i = 0; i < 6; i++) {
        dev = sysbus_create_simple("stm32f405-spi", spi_addr[i],
                                   pic[spi_irq[i]]);
        if (i == (NRF24L01PLUS_SPI_NUMBER - 1)) {
            /* Connect the 2.4GHz wireless chip via SPI
             * NOTE: This is not part of the Netduino Plus 2, but is
             * an external device attached via the boards pins
             */
            bus = qdev_get_child_bus(dev, "ssi");
            ssi_create_slave(bus, "nrf24l01plus");
        }
    }

    /* Attach the External Interrupt device */
    dev = qdev_create(NULL, "stm32f405-exti");
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x40013C00);
    for (i = 0; i < 16; i++) {
        sysbus_connect_irq(busdev, i, pic[exti_irq[i]]);
    }
    for (j = 0; j < 16; j++) {
        qdev_connect_gpio_out(syscfg, j, qdev_get_gpio_in(dev, j));
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
    memory_region_init_ram(hack, NULL, "netduino.hack", 0x1000, &error_abort);
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
