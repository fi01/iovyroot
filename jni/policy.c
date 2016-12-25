#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "policy.h"

typedef struct _policy_t {
	void *policy_data;
	int policy_len;
} * policy_ptr;

policy_t alloc_policy(int len) {
	policy_ptr policy_desc = NULL;
	void *policy_buf = NULL;

	policy_desc = malloc(sizeof(*policy_desc));
	if(policy_desc == NULL) {
		return 0;
	}

	policy_buf = malloc(len);
	if(policy_buf == NULL) {
		free(policy_desc);
		return 0;
	}

	policy_desc->policy_data = policy_buf;
	policy_desc->policy_len = len;

	return policy_desc;
}

void free_policy(policy_t policy) {
	policy_ptr _policy = (policy_ptr)policy;
	free(_policy->policy_data);
	free(_policy);
}

int load_policy_into_kernel(const void *data, size_t len) {
	char *filename = "/sys/fs/selinux/load";
	int fd, ret;

	// based on libselinux security_load_policy()
	fd = open(filename, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Can't open '%s':  %s\n",
		        filename, strerror(errno));
		return -1;
	}
	ret = write(fd, data, len);
	close(fd);
	if (ret < 0) {
		fprintf(stderr, "Could not write policy to %s\n",
		        filename);
		return -1;
	}
	return 0;
}

policy_t prepare_policy() {
	int policy_fd = -1;
	struct stat policy_stat = { 0 };
	int ret = 0;
	policy_ptr patched_policy = NULL;
	ssize_t policy_len = 0;

	system("/data/local/tmp/patch_script.sh");

	policy_fd = open("/data/local/tmp/patched_sepolicy", O_RDONLY);
	if(policy_fd < 0) {
		return 0;
	}

	ret = fstat(policy_fd, &policy_stat);
	if(ret < 0) {
		close(policy_fd);
		return 0;
	}

	policy_len = policy_stat.st_size;
	patched_policy = alloc_policy(policy_len);
	if(patched_policy == NULL) {
		return 0;
	}

	ret = read(policy_fd, patched_policy->policy_data, patched_policy->policy_len);
	if(ret != policy_len) {
		free_policy(patched_policy);
		close(policy_fd);
		return 0;
	}

	return patched_policy;
}

int patch_policy(policy_t new_policy) {
	policy_ptr _new_policy = (policy_ptr)new_policy;
	return load_policy_into_kernel(_new_policy->policy_data, _new_policy->policy_len);
}

void destroy_policy(policy_t policy) {
	policy_ptr _policy = (policy_ptr)policy;
	free_policy(_policy);
}
