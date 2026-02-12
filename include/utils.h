/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef UTILS_H
#define UTILS_H

#include <gio/gio.h>

/**
 * Calculate current CPU usage as percentage based on multiple samples.
 *
 * @return CPU usage percentage (0-100) or -1.0 on error
 */
double
get_cpu_usage(void);

/**
 * Check whether a filesystem path exists.
 *
 * @param path Path to check
 * @return true if the path exists, false otherwise
 */
gboolean
exists(const char *path);

/**
 * Check whether a filesystem path exists and is a directory.
 *
 * @param path Path to check
 * @return true if the path exists and is a directory, false otherwise
 */
gboolean
dir_exists(const char *path);

/**
 * Ensure a directory exists.
 *
 * @param path Directory path
 * @param mode Permissions to use
 */
void
ensure_dir(const char *path, unsigned int mode);

/**
 * Write a string to a file.
 *
 * @param path File to write
 * @param value String value to write
 */
void
write_str(const char *path, const char *value);

/**
 * Write an integer to a file.
 *
 * @param path File to write
 * @param value Integer value to write
 */
void
write_int(const char *path, long long value);

/**
 * Read a file into a buffer.
 *
 * @param path File to read
 * @param out Output buffer
 * @param out_len Output buffer length
 * @return true on success, false on failure
 */
gboolean
read_str(const char *path, char *out, size_t out_len);

/**
 * Check if a string contains a token.
 *
 * @param haystack Input string
 * @param needle Token to search for
 * @return true if token exists, false otherwise
 */
gboolean
string_contains_token(const char *haystack, const char *needle);

/**
 * Check if a file contains a token.
 *
 * @param path File to read
 * @param needle Token to search for
 * @return true if token exists, false otherwise
 */
gboolean
file_contains_token(const char *path, const char *needle);

/**
 * Recursively change ownership of a directory tree.
 *
 * @param path Root path to operate on
 * @param uid  User ID to assign
 * @param gid  Group ID to assign
 */
void
chown_recursive(const char *path, uid_t uid, gid_t gid);

/**
 * Asynchronously start a systemd unit.
 *
 * @param unit Unit name (like "irqbalance" or "irqbalance.service")
 */
void
systemd_service_start_async(const char *unit);

/**
 * Asynchronously stop a systemd unit.
 *
 * @param unit Unit name (like "irqbalance" or "irqbalance.service")
 */
void
systemd_service_stop_async(const char *unit);

/**
 * Check if systemd knows about a unit.
 *
 * Accepts names like "irqbalance" or "irqbalance.service".
 *
 * @param unit Unit name
 * @return true if systemd can resolve it, false otherwise
 */
gboolean
systemd_service_exists(const char *unit);

/**
 * Ensure a string is valid UTF-8.
 *
 * @param s Input string (may be NULL)
 * @return Newly allocated valid UTF-8 string (never NULL)
 */
gchar *
make_valid_utf8(const gchar *s);

#endif // UTILS_H
