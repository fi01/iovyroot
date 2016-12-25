#ifndef OFFSETS_H
#define OFFSETS_H

//struct file_operations
#define FSYNC_OFFSET(i) ((i) + (14 * sizeof(void*)))
#define CHECK_FLAGS_OFFSET(i) ((i) + (20 * sizeof(void*)))

struct offsets {
	char* devname; //ro.product.model
	char* kernelver; // /proc/version
	union {
		void* fsync; //ptmx_fops -> fsync
		void* check_flags; //ptmx_fops -> check_flags
	};
#if (__LP64__)
	void* joploc; //gadget location, see getroot.c
	void* jopret; //return to setfl after check_flags() (fcntl.c), usually inlined in sys_fcntl
#endif
	void* sidtab; //optional, for selinux contenxt
	void* policydb; //optional, for selinux context
	void* selinux_enabled;
	void* selinux_enforcing;
};

struct offsets* get_offsets();
extern struct offsets offsets[];

struct offsets2 {
	char * devname;
	char * kernelver;
	void * check_flags;
	void * patch_addr_limit_joploc;
	int    sock_ops_offset;
	int    ops_setsockopt_offset;
	void * patch_addr_limit_jopret;
	void * get_threadinfo_joploc;
	int    get_threadinfo_jopret_offset;
	void * get_threadinfo_jopret;
};

struct offsets2 *get_offsets2();
extern struct offsets2 offsets2[];

#endif /* OFFSETS_H */
