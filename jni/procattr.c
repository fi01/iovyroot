/*
 * procattr.c
 *
 *  Created on: 2016-12-21
 *      Author: lizhongyuan
 */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int setprocattrcon(const char * context,
			  pid_t pid, const char *attr)
{
	char *path;
	int fd, rc;
	pid_t tid;
	ssize_t ret;
	int errno_hold;

	if (pid > 0)
		rc = asprintf(&path, "/proc/%d/attr/%s", pid, attr);
	else {
		tid = gettid();
		rc = asprintf(&path, "/proc/self/task/%d/attr/%s", tid, attr);
	}
	if (rc < 0)
		return -1;

	fd = open(path, O_RDWR);
	free(path);
	if (fd < 0)
		return -1;
	if (context)
		do {
			ret = write(fd, context, strlen(context) + 1);
		} while (ret < 0 && errno == EINTR);
	else
		do {
			ret = write(fd, NULL, 0);	/* clear */
		} while (ret < 0 && errno == EINTR);
	errno_hold = errno;
	close(fd);
	errno = errno_hold;
	if (ret < 0)
		return -1;
	else
		return 0;
}

int getprocattrcon(char * * context,
			  pid_t pid, const char *attr)
{
	char *path, *buf;
	size_t size;
	int fd, rc;
	ssize_t ret;
	pid_t tid;
	int errno_hold;

	if (pid > 0)
		rc = asprintf(&path, "/proc/%d/attr/%s", pid, attr);
	else {
		tid = gettid();
		rc = asprintf(&path, "/proc/self/task/%d/attr/%s", tid, attr);
	}
	if (rc < 0)
		return -1;

	fd = open(path, O_RDONLY);
	free(path);
	if (fd < 0)
		return -1;

	size = 2048;
	buf = malloc(size);
	if (!buf) {
		ret = -1;
		goto out;
	}
	memset(buf, 0, size);

	do {
		ret = read(fd, buf, size - 1);
	} while (ret < 0 && errno == EINTR);
	if (ret < 0)
		goto out2;

	if (ret == 0) {
		*context = NULL;
		goto out2;
	}

	*context = strdup(buf);
	if (!(*context)) {
		ret = -1;
		goto out2;
	}
	ret = 0;
      out2:
	free(buf);
      out:
	errno_hold = errno;
	close(fd);
	errno = errno_hold;
	return ret;
}
