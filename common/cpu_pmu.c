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
#include <linux/perf_event.h>
#include "met_drv.h"
#include "met_kernel_symbol.h"
#include "interface.h"
#include "trace.h"
#include "cpu_pmu.h"

struct cpu_pmu_hw *cpu_pmu;
static int counter_cnt[MXNR_CPU];
static int nr_arg[MXNR_CPU];

int met_perf_cpupmu_status;

static DEFINE_PER_CPU(unsigned long long[MXNR_PMU_EVENTS], perfCurr);
static DEFINE_PER_CPU(unsigned long long[MXNR_PMU_EVENTS], perfPrev);
static DEFINE_PER_CPU(int[MXNR_PMU_EVENTS], perfCntFirst);
static DEFINE_PER_CPU(struct perf_event * [MXNR_PMU_EVENTS], pevent);
static DEFINE_PER_CPU(struct perf_event_attr [MXNR_PMU_EVENTS], pevent_attr);
static DEFINE_PER_CPU(int, perfSet);
static DEFINE_PER_CPU(unsigned int, perf_task_init_done);
static DEFINE_PER_CPU(unsigned int, perf_cpuid);
static DEFINE_PER_CPU(struct delayed_work, cpu_pmu_dwork);
static DEFINE_PER_CPU(struct delayed_work *, perf_delayed_work_setup);

noinline void mp_cpu(unsigned char cnt, unsigned int *value)
{
	MET_GENERAL_PRINT(MET_TRACE, cnt, value);
}

static void dummy_handler(struct perf_event *event, struct perf_sample_data *data,
			  struct pt_regs *regs)
{
	/*
	 * Required as perf_event_create_kernel_counter() requires an overflow handler,
	 * even though all we do is poll.
	 */
}

static void perf_cpupmu_polling(unsigned long long stamp, int cpu)
{
	int			event_count = cpu_pmu->event_count[cpu];
	struct met_pmu		*pmu = cpu_pmu->pmu[cpu];
	int			i, count;
	unsigned long long	delta;
	struct perf_event	*ev;
	unsigned int		pmu_value[MXNR_PMU_EVENTS];

	if (per_cpu(perfSet, cpu) == 0)
		return;

	count = 0;
	for (i = 0; i < event_count; i++) {
		if (pmu[i].mode == 0)
			continue;

		ev = per_cpu(pevent, cpu)[i];
		if ((ev != NULL) && (ev->state == PERF_EVENT_STATE_ACTIVE)) {
			per_cpu(perfCurr, cpu)[i] = met_perf_event_read_local_symbol(ev);
			delta = (per_cpu(perfCurr, cpu)[i] - per_cpu(perfPrev, cpu)[i]);
			per_cpu(perfPrev, cpu)[i] = per_cpu(perfCurr, cpu)[i];
			if (per_cpu(perfCntFirst, cpu)[i] == 1) {
				/* we shall omit delta counter when we get first counter */
				per_cpu(perfCntFirst, cpu)[i] = 0;
				continue;
			}
			pmu_value[count] = (unsigned int)delta;
			count++;
		}
	}

	if (count == counter_cnt[cpu])
		mp_cpu(count, pmu_value);
}

static int perf_thread_set_perf_events(unsigned int cpu)
{
	int			i, size;
	struct perf_event	*ev;
	struct perf_event_attr	*ev_attr;

	size = sizeof(struct perf_event_attr);
	if (per_cpu(perfSet, cpu) == 0) {
		int event_count = cpu_pmu->event_count[cpu];
		struct met_pmu *pmu = cpu_pmu->pmu[cpu];
		for (i = 0; i < event_count; i++) {
			per_cpu(pevent, cpu)[i] = NULL;
			if (!pmu[i].mode)
				continue;	/* Skip disabled counters */
			per_cpu(perfPrev, cpu)[i] = 0;
			per_cpu(perfCurr, cpu)[i] = 0;
			ev_attr = per_cpu(pevent_attr, cpu)+i;
			memset(ev_attr, 0, size);
			ev_attr->config = pmu[i].event;
			ev_attr->type = PERF_TYPE_RAW;
			ev_attr->size = size;
			ev_attr->sample_period = 0;
			ev_attr->pinned = 1;
			if (pmu[i].event == 0xff) {
				ev_attr->type = PERF_TYPE_HARDWARE;
				ev_attr->config = PERF_COUNT_HW_CPU_CYCLES;
			}

			ev = perf_event_create_kernel_counter(ev_attr, cpu, NULL, dummy_handler, NULL);
			if (IS_ERR(ev))
				continue;
			if (ev->state != PERF_EVENT_STATE_ACTIVE) {
				perf_event_release_kernel(ev);
				continue;
			}
			per_cpu(pevent, cpu)[i] = ev;

			perf_event_enable(ev);
			per_cpu(perfCntFirst, cpu)[i] = 1;
		}	/* for all PMU counter */
		per_cpu(perfSet, cpu) = 1;
	}	/* for perfSet */

	return 0;
}

static void perf_thread_setup(struct work_struct *work)
{
	int			cpu;
	struct delayed_work	*dwork = to_delayed_work(work);

	cpu = dwork->cpu;
	if (per_cpu(perf_task_init_done, cpu) == 0) {
		per_cpu(perf_task_init_done, cpu) = 1;
		perf_thread_set_perf_events(cpu);
	}
}

void met_perf_cpupmu_online(unsigned int cpu)
{
	if (met_cpupmu.mode == 0)
		return;

	per_cpu(perf_cpuid, cpu) = cpu;
	if (per_cpu(perf_delayed_work_setup, cpu) == NULL) {
		struct delayed_work *dwork;

		dwork = &per_cpu(cpu_pmu_dwork, cpu);
		dwork->cpu = cpu;
		INIT_DELAYED_WORK(dwork, perf_thread_setup);
		schedule_delayed_work(dwork, 0);
	}
}

void met_perf_cpupmu_down(void *perf_cpuid)
{
	int			cpu, i;
	struct perf_event	*ev;
	int			event_count;
	struct met_pmu		*pmu;

	cpu = *(int*)perf_cpuid;
	if (met_cpupmu.mode == 0)
		return;
	if (per_cpu(perfSet, cpu) == 0)
		return;

	per_cpu(perfSet, cpu) = 0;
	event_count = cpu_pmu->event_count[cpu];
	pmu = cpu_pmu->pmu[cpu];
	for (i = 0; i < event_count; i++) {
		if (!pmu[i].mode)
			continue;
		ev = per_cpu(pevent, cpu)[i];
		if ((ev != NULL) && (ev->state == PERF_EVENT_STATE_ACTIVE)) {
			perf_event_disable(ev);
			perf_event_release_kernel(ev);
		}
		per_cpu(pevent, cpu)[i] = NULL;
	}
	per_cpu(perf_task_init_done, cpu) = 0;
	per_cpu(perf_delayed_work_setup, cpu) = NULL;
}

inline static void met_perf_cpupmu_start(int cpu)
{
	met_perf_cpupmu_online(cpu);
}

inline static void met_perf_cpupmu_stop(int cpu)
{
	per_cpu(perf_cpuid, cpu) = cpu;
	met_smp_call_function_single_symbol(cpu, met_perf_cpupmu_down, &per_cpu(perf_cpuid, cpu), 1);
}

static int cpupmu_create_subfs(struct kobject *parent)
{
	cpu_pmu = cpu_pmu_hw_init();
	if (cpu_pmu == NULL) {
		PR_BOOTMSG("Failed to init CPU PMU HW!!\n");
		return -ENODEV;
	}

	return 0;
}

static void cpupmu_delete_subfs(void)
{
}

void met_perf_cpupmu_polling(unsigned long long stamp, int cpu)
{
	int count;
	unsigned int pmu_value[MXNR_PMU_EVENTS];

	if (met_cpu_pmu_method) {
		perf_cpupmu_polling(stamp, cpu);
	} else {
		count = cpu_pmu->polling(cpu_pmu->pmu[cpu], cpu_pmu->event_count[cpu], pmu_value);
		mp_cpu(count, pmu_value);
	}
}

static void cpupmu_start(void)
{
	int	cpu = raw_smp_processor_id();

	if (met_cpu_pmu_method)
		met_perf_cpupmu_start(cpu);
	else {
		nr_arg[cpu] = 0;
		cpu_pmu->start(cpu_pmu->pmu[cpu], cpu_pmu->event_count[cpu]);
	}
	met_perf_cpupmu_status = 1;
}

static void cpupmu_stop(void)
{
	int	cpu = raw_smp_processor_id();

	met_perf_cpupmu_status = 0;
	if (met_cpu_pmu_method)
		met_perf_cpupmu_stop(cpu);
	else
		cpu_pmu->stop(cpu_pmu->event_count[cpu]);
}

static const char cache_line_header[] =
	"met-info [000] 0.0: met_cpu_cache_line_size: %d\n";
static const char header[] =
	"met-info [000] 0.0: met_cpu_header_v2: %d";

static const char help[] =
	"  --pmu-cpu-evt=[cpu_list:]event_list   select CPU-PMU events in %s\n"
	"                                        cpu_list: specify the cpu_id list or apply to all the cores\n"
	"                                            example: 0,1,2\n"
	"                                        event_list: specify the event number\n"
	"                                            example: 0x8,0xff\n";

static int cpupmu_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help, cpu_pmu->cpu_name);
}

static int reset_driver_stat(void)
{
	int		cpu, i;
	int		event_count;
	struct met_pmu	*pmu;

	met_cpupmu.mode = 0;
	for_each_possible_cpu(cpu) {
		event_count = cpu_pmu->event_count[cpu];
		pmu = cpu_pmu->pmu[cpu];
		counter_cnt[cpu] = 0;
		nr_arg[cpu] = 0;
		for (i = 0; i < event_count; i++) {
			pmu[i].mode = MODE_DISABLED;
			pmu[i].event = 0;
			pmu[i].freq = 0;
		}
	}

	return 0;
}

static int cpupmu_print_header(char *buf, int len)
{
	int		cpu, i, ret, first;
	int		event_count;
	struct met_pmu	*pmu;

	ret = 0;

	/*append CPU PMU access method*/
	if (met_cpu_pmu_method)
		ret += snprintf(buf + ret, PAGE_SIZE,
			"met-info [000] 0.0: CPU_PMU_method: perf APIs\n");
	else
		ret += snprintf(buf + ret, PAGE_SIZE,
			"met-info [000] 0.0: CPU_PMU_method: MET pmu driver\n");

	/*append cache line size*/
	ret += snprintf(buf + ret, PAGE_SIZE - ret, cache_line_header, cache_line_size());
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "# mp_cpu: pmu_value1, ...\n");

	for_each_possible_cpu(cpu) {
		event_count = cpu_pmu->event_count[cpu];
		pmu = cpu_pmu->pmu[cpu];
		first = 1;
		for (i = 0; i < event_count; i++) {
			if (pmu[i].mode == 0)
				continue;
			if (first) {
				ret += snprintf(buf + ret, PAGE_SIZE - ret, header, cpu);
				first = 0;
			}
			ret += snprintf(buf + ret, PAGE_SIZE - ret, ",0x%x", pmu[i].event);
			pmu[i].mode = 0;
		}
		if (!first)
			ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	}

	reset_driver_stat();

	return ret;
}

static int met_parse_num_list(char *arg, int len, int *list, int list_cnt)
{
	int	nr_num = 0;
	char	*num;
	int	num_len;

	/* search ',' as the splitter */
	while (len) {
		num = arg;
		num_len = 0;
		if (list_cnt <= 0)
			return -1;
		while (len) {
			len--;
			if (*arg == ',') {
				*(arg++) = '\0';
				break;
			}
			arg++;
			num_len++;
		}
		if (met_parse_num(num, list, num_len) < 0)
			return -1;
		list++;
		list_cnt--;
		nr_num++;
	}

	return nr_num;
}

static int cpupmu_process_argument(const char *arg, int len)
{
	char		*arg1 = (char*)arg;
	int		len1 = len;
	int		cpu, cpu_list[MXNR_CPU];
	int		nr_events, event_list[MXNR_PMU_EVENTS];
	int		i;
	int		nr_counters;
	struct met_pmu	*pmu;
	int		arg_nr;
	int		counters;
	int		event_no;

	/*
	 * split cpu_list and event_list by ':'
	 *   arg, len: cpu_list when found (i < len)
	 *   arg1, len1: event_list
	 */
	for (i = 0; i < len; i++) {
		if (arg[i] == ':') {
			arg1[i] = '\0';
			arg1 += i+1;
			len1 = len - i - 1;
			len = i;
			break;
		}
	}

	/*
	 * setup cpu_list array
	 *   1: selected
	 *   0: unselected
	 */
	if (arg1 != arg) {	/* is cpu_id list specified? */
		int list[MXNR_CPU], cnt;
		int cpu_id;
		if ((cnt = met_parse_num_list((char*)arg, len, list, ARRAY_SIZE(list))) <= 0)
			goto arg_out;
		memset(cpu_list, 0, sizeof(cpu_list));
		for (i = 0; i < cnt; i++) {
			cpu_id = list[i];
			if (cpu_id < 0 || cpu_id >= ARRAY_SIZE(cpu_list))
				goto arg_out;
			cpu_list[cpu_id] = 1;
		}
	}
	else
		memset(cpu_list, 1, sizeof(cpu_list));

	/* get event_list */
	if ((nr_events = met_parse_num_list(arg1, len1, event_list, ARRAY_SIZE(event_list))) <= 0)
		goto arg_out;

	/* for each cpu in cpu_list, add all the events in event_list */
	for_each_possible_cpu(cpu) {
		nr_counters = cpu_pmu->event_count[cpu];
		pmu = cpu_pmu->pmu[cpu];
		arg_nr = nr_arg[cpu];

		if (cpu_list[cpu] == 0)
			continue;

		if (met_cpu_pmu_method) {
			/*
			 * setup nr_counters for linux native perf mode.
			 * because the selected events are stored in pmu,
			 * so nr_counters can't large then event count in pmu.
			 */
			counters = perf_num_counters();
			if (counters < nr_counters)
				nr_counters = counters;
		}
		if (nr_counters == 0)
			goto arg_out;

		for (i = 0; i < nr_events; i++) {
			event_no = event_list[i];
			/*
			 * check if event is duplicate,
			 * but may not include 0xff when met_cpu_pmu_method == 0.
			 */
			if (cpu_pmu->check_event(pmu, arg_nr, event_no) < 0)
				goto arg_out;

			if (event_no == 0xff && met_cpu_pmu_method == 0) {
				if (pmu[nr_counters-1].mode == MODE_POLLING)
					goto arg_out;
				pmu[nr_counters-1].mode = MODE_POLLING;
				pmu[nr_counters-1].event = 0xff;
				pmu[nr_counters-1].freq = 0;
			} else {
				if (arg_nr >= (nr_counters - 1))
					goto arg_out;
				pmu[arg_nr].mode = MODE_POLLING;
				pmu[arg_nr].event = event_no;
				pmu[arg_nr].freq = 0;
				arg_nr++;
			}
			counter_cnt[cpu]++;
		}
		nr_arg[cpu] = arg_nr;
	}

	met_cpupmu.mode = 1;
	return 0;

arg_out:
	reset_driver_stat();
	return -EINVAL;
}

struct metdevice met_cpupmu = {
	.name = "cpu",
	.type = MET_TYPE_PMU,
	.cpu_related = 1,
	.create_subfs = cpupmu_create_subfs,
	.delete_subfs = cpupmu_delete_subfs,
	.start = cpupmu_start,
	.stop = cpupmu_stop,
	.polling_interval = 1,
	.timed_polling = met_perf_cpupmu_polling,
	.print_help = cpupmu_print_help,
	.print_header = cpupmu_print_header,
	.process_argument = cpupmu_process_argument
};
