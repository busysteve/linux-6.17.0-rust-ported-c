# SPDX-License-Identifier: GPL-2.0

# Nova Core GPU driver - C implementation (ported from Rust)
nova_core-y := \
	nova_core.o \
	driver.o \
	util.o \
	vbios.o \
	gpu.o \
	falcon.o \
	fb.o \
	firmware.o \
	gfw.o \
	dma.o \
	regs.o

# Subdirectory objects
nova_core-y += \
	falcon/gsp.o \
	falcon/hal.o \
	falcon/sec2.o \
	falcon/hal/ga102.o \
	fb/hal.o \
	fb/hal/ga100.o \
	fb/hal/ga102.o \
	fb/hal/tu102.o \
	firmware/fwsec.o \
	regs/macros.o

obj-$(CONFIG_NOVA_CORE) += nova_core.o
