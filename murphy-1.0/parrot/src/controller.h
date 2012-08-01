
#ifndef _MALOS_CONTROLLER_H_
#define _MALOS_CONTROLLER_H_

#include <stdint.h>
#include <sys/types.h>

struct controller_context_t {
	int64_t syscall_count;
	pid_t pid;
	unsigned int childnum;
	const char *ioctl_meta;
	const char *syscall_name;
	int64_t syscall_number;

};

bool
controller_doevil(const char *gremlin_name,
	const controller_context_t *context,
	const char *classad_context);

bool
controller_setconfig(const char* config_line);

bool
controller_init();

#endif
