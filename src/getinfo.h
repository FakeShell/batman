#ifndef BATMAN_H
#define BATMAN_H

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <glib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/sysinfo.h>

#define FILE_PATH_CPU "/var/lib/batman/default_cpu_governor"
#define FILE_PATH_GPU "/var/lib/batman/default_gpu_governor"
#define CONFIG_FILE "/var/lib/batman/config"

void append_to_gstring(GString *string, char *format, ...);
GString* display_info();

#endif /* BATMAN_H */
