/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2025 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef BATMAN_WRAPPER_H
#define BATMAN_WRAPPER_H

#include <upower.h>
#include <gio/gio.h>
#include <glib.h>

/**
 * Battery state enumeration representing different possible states of the battery
 */
typedef enum {
    BATMAN_NO_BATTERY = 0,      /** No battery present in the system */
    BATMAN_CHARGING = 1,        /** Battery is currently charging */
    BATMAN_DISCHARGING = 2,     /** Battery is currently discharging */
    BATMAN_FULLY_CHARGED = 3,   /** Battery is fully charged */
    BATMAN_UNKNOWN = 4          /** Battery state cannot be determined */
} batman_state_t;

/**
 * Structure containing detailed memory information from /proc/meminfo
 */
struct meminfo {
    long long int memtotal;      /** Total usable RAM */
    long long int memfree;       /** Unused memory */
    long long int buffers;       /** Temporary storage for raw disk blocks */
    long long int cached;        /** Cached files in memory */
    long long int sreclaimable;  /** Reclaimable kernel memory */
};


/**
 * Structure containing CPU time information
 */
typedef struct {
    long long user;     /** Time spent in user mode */
    long long nice;     /** Time spent in user mode with low priority */
    long long system;   /** Time spent in system mode */
    long long idle;     /** Time spent in idle task */
    long long iowait;   /** Time waiting for I/O to complete */
    long long irq;      /** Time servicing interrupts */
    long long softirq;  /** Time servicing softirqs */
    long long steal;    /** Stolen time */
} cpu_time_t;

/**
 * Get complete battery information including state and percentage
 * @param upower The UPower client instance
 * @param percentage Pointer to store battery percentage (0-100)
 * @return String representation of battery state ("charging", "discharging", "fully-charged") or NULL if no battery
 */
const gchar *
get_battery_all(UpClient *upower, gdouble *percentage);

/**
 * Get only the battery percentage
 * @param upower The UPower client instance
 * @return Current battery percentage (0-100) or 0 if no battery
 */
gdouble
get_battery_percentage(UpClient *upower);

/**
 * Get battery state as an enum value
 * @param upower The UPower client instance
 * @return Current battery state as batman_state_t enum
 */
batman_state_t
get_battery_state(UpClient *upower);

/**
 * Read memory information from /proc/meminfo
 * @param mem Pointer to meminfo structure to fill
 * @return 1 on success, 0 on failure with errno set
 */
int
read_mem_info(struct meminfo *mem);

/**
 * Calculate current memory usage as a percentage
 * @return Memory usage percentage (0-100) or -1.0 on error
 */
long double
mem_usage(void);

/**
 * Read CPU statistics from /proc/stat
 * @param cpu_time Pointer to cpu_time_t structure to fill
 * @return Number of fields successfully read or -1 on error
 */
int
read_cpu_stats(cpu_time_t *cpu_time);

/**
 * Calculate total CPU time across all states
 * @param cpu_time Pointer to cpu_time_t structure containing CPU stats
 * @return Total CPU time as sum of all states
 */
long long
get_total_time(const cpu_time_t *cpu_time);

/**
 * Calculate idle CPU time (idle + iowait)
 * @param cpu_time Pointer to cpu_time_t structure containing CPU stats
 * @return Sum of idle and iowait times
 */
long long
get_idle_time(const cpu_time_t *cpu_time);

/**
 * Calculate current CPU usage as percentage based on multiple samples
 * @return CPU usage percentage (0-100) or -1.0 on error
 */
double get_cpu_usage(void);

/* Backwards compatibility functions */

/**
 * Legacy function for get_battery_all()
 * @see get_battery_all
 */
const gchar *
findBattery(UpClient *upower, gdouble *percentage);

/**
 * Legacy function for get_battery_all()
 * @see get_battery_all
 */
const gchar *
find_battery(UpClient *upower, gdouble *percentage);

/**
 * Legacy function for read_mem_info()
 * @see read_mem_info
 */
int
readMemInfo(struct meminfo *mem);

/**
 * Legacy function for cpu_usage()
 * @see cpu_usage
 */
double
cpuUsage(void);

/**
 * Legacy function for mem_usage()
 * @see mem_usage
 */
long double
memUsage(void);

/**
 * Block until device display configuration has been modified
 */
void
block_display_changed(void);

#endif // BATMAN_WRAPPER_H
