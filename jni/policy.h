/*
 * policy.h
 *
 *  Created on: 2016-11-30
 *      Author: lizhongyuan
 */

#ifndef POLICY_H_
#define POLICY_H_

typedef void * policy_t;

policy_t prepare_policy();

int patch_policy(policy_t new_policy);

void destroy_policy(policy_t policy);

#endif /* POLICY_H_ */
