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
#include <linux/delay.h>

#include "ondiemet_sspm.h"
#define MET_USER_EVENT_SUPPORT
#include "met_drv.h"
#include "interface.h"

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#ifdef CONFIG_MET_ARM_32BIT
#include <linux/module.h> /* symbol_get */
#include <asm/dma-mapping.h> /* arm_coherent_dma_ops */
#else
#include <linux/dma-mapping.h>
#endif

dma_addr_t ondiemet_sspm_log_phy_addr;
void *ondiemet_sspm_log_virt_addr;
uint32_t ondiemet_sspm_log_size = 0x400000;

/* SSPM_LOG_FILE 0 */
/* SSPM_LOG_SRAM 1 */
/* SSPM_LOG_DRAM 2 */
int sspm_log_mode;
/* SSPM_RUN_NORMAL mode 0 */
/* SSPM_RUN_CONTINUOUS mode 1 */
int sspm_run_mode;
int met_sspm_log_discard = -1;
int sspm_log_size = 100;

int sspm_buffer_size;
int sspm_buf_available;
EXPORT_SYMBOL(sspm_buf_available);
int sspm_buf_mapped = -1; /* get buffer by MET itself */

static ssize_t sspm_buffer_size_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(sspm_buffer_size, 0444, sspm_buffer_size_show, NULL);

static ssize_t sspm_available_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(sspm_available, 0444, sspm_available_show, NULL);

static ssize_t sspm_log_discard_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(sspm_log_discard, 0444, sspm_log_discard_show, NULL);

static ssize_t sspm_log_mode_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_log_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(sspm_log_mode, 0664, sspm_log_mode_show, sspm_log_mode_store);

static ssize_t sspm_log_size_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_log_size_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(sspm_log_size, 0664, sspm_log_size_show, sspm_log_size_store);


static ssize_t sspm_run_mode_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_run_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(sspm_run_mode, 0664, sspm_run_mode_show, sspm_run_mode_store);

static ssize_t sspm_modules_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_modules_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(sspm_modules, 0664, sspm_modules_show, sspm_modules_store);

static ssize_t sspm_op_ctrl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(sspm_op_ctrl, 0220, NULL, sspm_op_ctrl_store);

static ssize_t sspm_buffer_size_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", sspm_buffer_size);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_available_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", 1);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_log_discard_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", met_sspm_log_discard);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_log_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", sspm_log_mode);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_log_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;
	mutex_lock(&dev->mutex);
	sspm_log_mode = value;
	mutex_unlock(&dev->mutex);
	return count;
}


static ssize_t sspm_log_size_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", sspm_log_size);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_log_size_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;
	mutex_lock(&dev->mutex);
	sspm_log_size = value;
	mutex_unlock(&dev->mutex);
	return count;
}


static ssize_t sspm_run_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", sspm_run_mode);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_run_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;
	mutex_lock(&dev->mutex);
	sspm_run_mode = value;
	mutex_unlock(&dev->mutex);
	return count;
}

static ssize_t sspm_op_ctrl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;
	mutex_lock(&dev->mutex);
	if (value == 1)
		sspm_start();
	else if (value == 2)
		sspm_stop();
	else if (value == 3)
		sspm_extract();
	else if (value == 4)
		sspm_flush();
	mutex_unlock(&dev->mutex);
	return count;
}

static ssize_t sspm_modules_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%x\n", ondiemet_module[ONDIEMET_SSPM]);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_modules_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	uint32_t value;

	if (kstrtouint(buf, 0, &value) != 0)
		return -EINVAL;
	mutex_lock(&dev->mutex);
	ondiemet_module[ONDIEMET_SSPM] = value;
	mutex_unlock(&dev->mutex);
	return count;
}

int sspm_attr_init(struct device *dev)
{
	int ret;

#ifdef CONFIG_MET_ARM_32BIT
	struct dma_map_ops *ops = (struct dma_map_ops *)symbol_get(arm_coherent_dma_ops);

	if (ops && ops->alloc) {
		dev->coherent_dma_mask = DMA_BIT_MASK(32);
		ondiemet_sspm_log_virt_addr = ops->alloc(dev,
						ondiemet_sspm_log_size,
						&ondiemet_sspm_log_phy_addr,
						GFP_KERNEL,
						0);
	}
#else
	/* dma_alloc */
	ondiemet_sspm_log_virt_addr = dma_alloc_coherent(dev,
			ondiemet_sspm_log_size,
			&ondiemet_sspm_log_phy_addr,
			GFP_KERNEL);
#endif

	ret = device_create_file(dev, &dev_attr_sspm_buffer_size);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_buffer_size\n");
		return ret;
	}

	ret = device_create_file(dev, &dev_attr_sspm_available);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_available\n");
		return ret;
	}

	ret = device_create_file(dev, &dev_attr_sspm_log_discard);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_log_discard\n");
		return ret;
	}
	ret = device_create_file(dev, &dev_attr_sspm_log_mode);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_log_mode\n");
		return ret;
	}
	ret = device_create_file(dev, &dev_attr_sspm_log_size);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_log_size\n");
		return ret;
	}
	ret = device_create_file(dev, &dev_attr_sspm_run_mode);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_run_mode\n");
		return ret;
	}
	ret = device_create_file(dev, &dev_attr_sspm_op_ctrl);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_op_ctrl\n");
		return ret;
	}
	ret = device_create_file(dev, &dev_attr_sspm_modules);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_modules\n");
		return ret;
	}

	if (ondiemet_sspm_log_virt_addr != NULL) {
		start_sspm_ipi_recv_thread();
		sspm_buf_available = 1;
		sspm_buffer_size = ondiemet_sspm_log_size;
	} else {
		sspm_buf_available = 0;
		sspm_buffer_size = -1;
	}

	return 0;
}

int sspm_attr_uninit(struct device *dev)
{
	/* dma_free */
	if (ondiemet_sspm_log_virt_addr != NULL) {
#ifdef CONFIG_MET_ARM_32BIT
		struct dma_map_ops *ops = (struct dma_map_ops *)symbol_get(arm_coherent_dma_ops);

		if (ops && ops->free) {
			ops->free(dev,
				ondiemet_sspm_log_size,
				ondiemet_sspm_log_virt_addr,
				ondiemet_sspm_log_phy_addr,
				0);
		}
#else
		dma_free_coherent(dev,
			ondiemet_sspm_log_size,
			ondiemet_sspm_log_virt_addr,
			ondiemet_sspm_log_phy_addr);
#endif
		ondiemet_sspm_log_virt_addr = NULL;
		stop_sspm_ipi_recv_thread();
	}

	device_remove_file(dev, &dev_attr_sspm_buffer_size);
	device_remove_file(dev, &dev_attr_sspm_available);
	device_remove_file(dev, &dev_attr_sspm_log_discard);
	device_remove_file(dev, &dev_attr_sspm_log_mode);
	device_remove_file(dev, &dev_attr_sspm_log_size);
	device_remove_file(dev, &dev_attr_sspm_run_mode);
	device_remove_file(dev, &dev_attr_sspm_op_ctrl);
	device_remove_file(dev, &dev_attr_sspm_modules);

	return 0;
}

#if 0 /* move to sspm_attr_init() */
void sspm_get_buffer_info(void)
{
	if (ondiemet_sspm_log_virt_addr != NULL) {
		sspm_buf_available = 1;
		sspm_buffer_size = ondiemet_sspm_log_size;
	} else {
		sspm_buf_available = 0;
		sspm_buffer_size = -1;
	}
}
#endif

extern const char *met_get_platform_name(void);
void sspm_start(void)
{
	int32_t ret = 0;
	uint32_t rdata;
	uint32_t ipi_buf[4];
	const char* platform_name = NULL;
	unsigned int platform_id = 0;
	met_sspm_log_discard = -1;

	/* clear DRAM buffer */
	if (ondiemet_sspm_log_virt_addr != NULL)
		memset_io((void *)ondiemet_sspm_log_virt_addr, 0, ondiemet_sspm_log_size);
	else
		return;

	platform_name = met_get_platform_name();
	if (platform_name)
		ret = kstrtouint(&platform_name[2], 10, &platform_id);

	/* send DRAM physical address */
	ipi_buf[0] = MET_MAIN_ID | MET_BUFFER_INFO;
	ipi_buf[1] = (unsigned int)ondiemet_sspm_log_phy_addr; /* address */
	if (ret == 0)
		ipi_buf[2] = platform_id;
	else
		ipi_buf[2] = 0;
	ipi_buf[3] = 0;
	ret = sspm_ipi_send_sync(IPI_ID_MET, IPI_OPT_WAIT, (void *)ipi_buf, 0, &rdata, 1);

	/* start ondiemet now */
	ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_START;
	ipi_buf[1] = ondiemet_module[ONDIEMET_SSPM];
	ipi_buf[2] = sspm_log_mode;
	ipi_buf[3] = sspm_run_mode;
	ret = sspm_ipi_send_sync(IPI_ID_MET, IPI_OPT_WAIT, (void *)ipi_buf, 0, &rdata, 1);
}

void sspm_stop(void)
{
	int32_t ret;
	uint32_t rdata;
	uint32_t ipi_buf[4];

	if (sspm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID|MET_OP|MET_OP_STOP;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = sspm_ipi_send_sync(IPI_ID_MET, IPI_OPT_WAIT, (void *)ipi_buf, 0, &rdata, 1);
	}
}

void sspm_extract(void)
{
	int32_t ret;
	uint32_t rdata;
	uint32_t ipi_buf[4];
	int32_t count;

	count = 20;
	if (sspm_buf_available == 1) {
		while ((sspm_buffer_dumping == 1) && (count != 0)) {
			msleep(50);
			count--;
		}
		ipi_buf[0] = MET_MAIN_ID|MET_OP|MET_OP_EXTRACT;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = sspm_ipi_send_sync(IPI_ID_MET, IPI_OPT_WAIT, (void *)ipi_buf, 0, &rdata, 1);
	}

	if (sspm_run_mode == SSPM_RUN_NORMAL)
		ondiemet_module[ONDIEMET_SSPM] = 0;
}

void sspm_flush(void)
{
	int32_t ret;
	uint32_t rdata;
	uint32_t ipi_buf[4];

	if (sspm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID|MET_OP|MET_OP_FLUSH;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = sspm_ipi_send_sync(IPI_ID_MET, IPI_OPT_WAIT, (void *)ipi_buf, 0, &rdata, 1);
	}

	if (sspm_run_mode == SSPM_RUN_NORMAL)
		ondiemet_module[ONDIEMET_SSPM] = 0;
}
#else /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
int sspm_buffer_size = -1;

int sspm_attr_init(struct device *dev)
{
	return 0;
}

int sspm_attr_uninit(struct device *dev)
{
	return 0;
}

void sspm_start(void)
{
}

void sspm_stop(void)
{
}

void sspm_extract(void)
{
}

void sspm_flush(void)
{
}

#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
