/*
 * selinux.h
 *
 *  Created on: 2016-12-21
 *      Author: lizhongyuan
 */

#ifndef SELINUX_H_
#define SELINUX_H_

int setprocattrcon(const char * context,
			  pid_t pid, const char *attr);

int getprocattrcon(char * * context,
			  pid_t pid, const char *attr);

#endif /* SELINUX_H_ */
