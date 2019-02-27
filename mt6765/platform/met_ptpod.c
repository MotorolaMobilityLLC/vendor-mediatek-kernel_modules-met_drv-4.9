/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/fs.h>

#include "met_drv.h"
#include "trace.h"
#include "core_plf_init.h"

static int ptpod_started;
static unsigned int g_u4CPUVolt_LL;
static unsigned int g_u4CPUVolt_L;
static unsigned int g_u4CPUVolt_CCI;
static unsigned int g_u4GPUVolt;

noinline void ptpod(void)
{
	MET_PRINTK("%u,%u,%u,%u\n", g_u4CPUVolt_LL, g_u4CPUVolt_L, g_u4CPUVolt_CCI, g_u4GPUVolt);
}

#if 0
static void ptpod_cpu_voltSampler(enum mt_cpu_dvfs_id id, unsigned int volt)
{
	switch (id) {
	case MT_CPU_DVFS_LL:
		g_u4CPUVolt_LL = volt;
		break;
	case MT_CPU_DVFS_L:
		g_u4CPUVolt_L = volt;
		break;
	case MT_CPU_DVFS_CCI:
		g_u4CPUVolt_CCI = volt;
		break;
	default:
		return;
	}
	ptpod();
}
#endif

#if 0
static void ptpod_gpu_voltSampler(unsigned int a_u4Volt)
{
	g_u4GPUVolt = (a_u4Volt+50)/100;

	if (ptpod_started)
		ptpod();
}
#endif

static int ptpod_create(struct kobject *parent)
{
	return 0;
}

static void ptpod_delete(void)
{
}

static void met_ptpod_polling(unsigned long long stamp, int cpu)
{
	if (mt_cpufreq_get_cur_volt_symbol) {
		g_u4CPUVolt_LL = mt_cpufreq_get_cur_volt_symbol(MT_CPU_DVFS_LL)/100;
		g_u4CPUVolt_L = mt_cpufreq_get_cur_volt_symbol(MT_CPU_DVFS_L)/100;
		g_u4CPUVolt_CCI = mt_cpufreq_get_cur_volt_symbol(MT_CPU_DVFS_CCI)/100;
	}

	ptpod();
}

/*
 * Called from "met-cmd --start"
 */
static void ptpod_start(void)
{
#if 0
	met_gpufreq_setvolt_registerCB(ptpod_gpu_voltSampler, kFOR_MET_PTPOD_USE);
#endif

	if (mt_cpufreq_get_cur_volt_symbol) {
		g_u4CPUVolt_LL = mt_cpufreq_get_cur_volt_symbol(MT_CPU_DVFS_LL)/100;
		g_u4CPUVolt_L = mt_cpufreq_get_cur_volt_symbol(MT_CPU_DVFS_L)/100;
		g_u4CPUVolt_CCI = mt_cpufreq_get_cur_volt_symbol(MT_CPU_DVFS_CCI)/100;
	}

	if (mt_gpufreq_get_cur_volt_symbol)
		g_u4GPUVolt = ((mt_gpufreq_get_cur_volt_symbol()+50)/100);

	ptpod();

	/* register callback */
#if 0
	if (mt_cpufreq_setvolt_registerCB_symbol)
		mt_cpufreq_setvolt_registerCB_symbol(ptpod_cpu_voltSampler);
#endif

	ptpod_started = 1;
}

/*
 * Called from "met-cmd --stop"
 */
static void ptpod_stop(void)
{
	ptpod_started = 0;

	/* unregister callback */
#if 0
	if (mt_cpufreq_setvolt_registerCB_symbol)
		mt_cpufreq_setvolt_registerCB_symbol(NULL);
#endif

	if (mt_cpufreq_get_cur_volt_symbol) {
		g_u4CPUVolt_LL = mt_cpufreq_get_cur_volt_symbol(MT_CPU_DVFS_LL)/100;
		g_u4CPUVolt_L = mt_cpufreq_get_cur_volt_symbol(MT_CPU_DVFS_L)/100;
		g_u4CPUVolt_CCI = mt_cpufreq_get_cur_volt_symbol(MT_CPU_DVFS_CCI)/100;
	}

	if (mt_gpufreq_get_cur_volt_symbol)
		g_u4GPUVolt = ((mt_gpufreq_get_cur_volt_symbol()+50)/100);

	ptpod();

#if 0
	met_gpufreq_setvolt_registerCB(NULL, kFOR_MET_PTPOD_USE);
#endif
}

static char help[] =
	"  --ptpod                               Measure CPU/GPU voltage\n";
static int ptpod_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

/*
 * It will be called back when run "met-cmd --extract" and mode is 1
 */
static char header[] =
	"met-info [000] 0.0: ms_ud_sys_header: ptpod,CPUVolt_LL,CPUVolt_L,CPUVolt_CCI,GPUVolt,d,d,d,d\n";
static int ptpod_print_header(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, header);
}

struct metdevice met_ptpod = {
	.name = "ptpod",
	.owner = THIS_MODULE,
	.type = MET_TYPE_PMU,
	.cpu_related = 0,
	.create_subfs = ptpod_create,
	.delete_subfs = ptpod_delete,
	.start = ptpod_start,
	.stop = ptpod_stop,
	.timed_polling = met_ptpod_polling,
	.print_help = ptpod_print_help,
	.print_header = ptpod_print_header,
};
EXPORT_SYMBOL(met_ptpod);
