# SPDX-License-Identifier: GPL-2.0

ccflags-y += -include $(srctree)/drivers/gpu/drm/panfrost/backport_shim.h
panfrost-y := \
	panfrost_drv.o \
	panfrost_device.o \
	panfrost_devfreq.o \
	panfrost_sprd_glue.o \
	panfrost_gem.o \
	panfrost_gem_shrinker.o \
	panfrost_gpu.o \
	panfrost_job.o \
	panfrost_mmu.o \
	panfrost_perfcnt.o

obj-$(CONFIG_DRM_PANFROST) += panfrost.o
