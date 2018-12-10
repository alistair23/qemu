# Default configuration for riscv32-softmmu

# Uncomment the following lines to disable these optional devices:
#
#CONFIG_PCI_DEVICES=n

# Boards:
#
CONFIG_SPIKE=y
CONFIG_SIFIVE_E=y
CONFIG_SIFIVE_U=y
CONFIG_RISCV_VIRT=y

CONFIG_SSI=y

CONFIG_SD=y
CONFIG_SSI_SD=y
CONFIG_SDHCI=y
CONFIG_SSI_M25P80=y
