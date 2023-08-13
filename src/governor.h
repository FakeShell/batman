#ifndef GOVERNOR_H
#define GOVERNOR_H

#include <pthread.h>
#include <signal.h>
#include <sys/utsname.h>

extern volatile sig_atomic_t keep_going;
extern char cpu_usage[1024];
extern pthread_mutex_t cpu_usage_mutex;

extern const char *paths[];

void handle_sigint(int sig);
char *get_node_name(const char *path);
int is_arch_x86();
void *update_cpu_usage(void *arg);
void get_system_info(int x86);

#endif // GOVERNOR_H
