/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2025 Bardia Moshiri <bardia@furilabs.com>
 */

#include <glib.h>
#include <gio/gio.h>
#include <fcntl.h>

#define TYPEC_PORT_PATH "/sys/class/typec/port0"

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='io.FuriOS.BatmanPowerConfig'>"
  "    <method name='SetPowerRole'>"
  "      <arg type='s' name='mode' direction='in'/>"
  "    </method>"
  "    <method name='SetDataRole'>"
  "      <arg type='s' name='mode' direction='in'/>"
  "    </method>"
  "    <method name='SetPreferredRole'>"
  "      <arg type='s' name='mode' direction='in'/>"
  "    </method>"
  "    <method name='SetVCONNSource'>"
  "      <arg type='s' name='mode' direction='in'/>"
  "    </method>"
  "    <property name='PowerRole' type='s' access='read'/>"
  "    <property name='DataRole' type='s' access='read'/>"
  "    <property name='PreferredRole' type='s' access='read'/>"
  "    <property name='VCONNSource' type='s' access='read'/>"
  "  </interface>"
  "</node>";

static void
write_to_file(const char *path,
              const char *value)
{
  g_debug("Attempting to write to %s: %s", path, value);
  int fd = open(path, O_WRONLY);
  if (fd == -1) {
    g_debug("open failed for %s: %s", path, g_strerror(errno));
    return;
  }

  if (write(fd, value, strlen(value)) == -1)
    g_debug("write failed for %s: %s", path, g_strerror(errno));
  close (fd);
}

static gchar *
read_file(const gchar *filename,
          GError     **error)
{
  gchar *content = NULL;
  gsize length;

  if (g_file_get_contents(filename, &content, &length, error)) {
    if (length > 0 && content[length - 1] == '\n')
      content[length - 1] = '\0';
    return content;
  }

  return NULL;
}

static gchar *
find_text_between_brackets(const gchar *text)
{
  const gchar *start, *end;

  start = g_strstr_len(text, -1, "[");
  if (start != NULL) {
    start++;
    end = g_strstr_len(start, -1, "]");
    if (end != NULL)
      return g_strndup(start, end - start);
  }

  return g_strdup(text);
}

static void
handle_method_call(GDBusConnection       *connection,
                   const gchar           *sender,
                   const gchar           *object_path,
                   const gchar           *interface_name,
                   const gchar           *method_name,
                   GVariant              *parameters,
                   GDBusMethodInvocation *invocation,
                   gpointer               user_data)
{
  const gchar *mode = NULL;
  g_autofree gchar *file_path = NULL;
  gboolean valid_input = FALSE;

  g_variant_get(parameters, "(&s)", &mode);

  if (g_strcmp0(method_name, "SetPowerRole") == 0) {
    file_path = g_build_filename(TYPEC_PORT_PATH, "power_role", NULL);
    valid_input = (g_strcmp0(mode, "source") == 0 || g_strcmp0(mode, "sink") == 0);
  } else if (g_strcmp0(method_name, "SetDataRole") == 0) {
    file_path = g_build_filename(TYPEC_PORT_PATH, "data_role", NULL);
    valid_input = (g_strcmp0(mode, "host") == 0 || g_strcmp0(mode, "device") == 0);
  } else if (g_strcmp0(method_name, "SetPreferredRole") == 0) {
    file_path = g_build_filename(TYPEC_PORT_PATH, "preferred_role", NULL);
    valid_input = (g_strcmp0(mode, "source") == 0 || g_strcmp0(mode, "sink") == 0 || g_strcmp0(mode, "none") == 0);
  } else if (g_strcmp0(method_name, "SetVCONNSource") == 0) {
    file_path = g_build_filename(TYPEC_PORT_PATH, "vconn_source", NULL);
    valid_input = (g_strcmp0(mode, "yes") == 0 || g_strcmp0(mode, "no") == 0);
  } else {
    g_dbus_method_invocation_return_error(invocation,
                                          G_DBUS_ERROR,
                                          G_DBUS_ERROR_UNKNOWN_METHOD,
                                          "Unknown method %s",
                                          method_name);
    return;
  }

  if (!valid_input) {
    g_dbus_method_invocation_return_error(invocation,
                                          G_DBUS_ERROR,
                                          G_DBUS_ERROR_INVALID_ARGS,
                                          "Invalid mode '%s' for method %s",
                                          mode,
                                          method_name);
    return;
  }

  write_to_file(file_path, mode);

  g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
}

static GVariant *
handle_get_property(GDBusConnection  *connection,
                    const gchar      *sender,
                    const gchar      *object_path,
                    const gchar      *interface_name,
                    const gchar      *property_name,
                    GError          **error,
                    gpointer          user_data)
{
  g_autofree gchar *filepath = NULL;
  g_autofree gchar *state = NULL;
  g_autofree gchar *bracketed_text = NULL;
  GVariant *result = NULL;

  if (g_strcmp0(property_name, "PowerRole") == 0)
    filepath = g_build_filename(TYPEC_PORT_PATH, "power_role", NULL);
  else if (g_strcmp0(property_name, "DataRole") == 0)
    filepath = g_build_filename(TYPEC_PORT_PATH, "data_role", NULL);
  else if (g_strcmp0(property_name, "PreferredRole") == 0)
    filepath = g_build_filename(TYPEC_PORT_PATH, "preferred_role", NULL);
  else if (g_strcmp0(property_name, "VCONNSource") == 0)
    filepath = g_build_filename(TYPEC_PORT_PATH, "vconn_source", NULL);
  else {
    g_set_error(error,
                G_IO_ERROR,
                G_IO_ERROR_INVALID_ARGUMENT,
                "Property %s is not supported",
                property_name);
    return NULL;
  }

  state = read_file(filepath, error);
  if (state == NULL)
    return NULL;

  if (g_strcmp0(property_name, "PowerRole") == 0 ||
      g_strcmp0(property_name, "DataRole") == 0) {
    bracketed_text = find_text_between_brackets(state);
    result = g_variant_new_string(bracketed_text);
  } else
    result = g_variant_new_string(state);

  return result;
}

static const
GDBusInterfaceVTable interface_vtable = {
  .method_call = handle_method_call,
  .get_property = handle_get_property,
  .set_property = NULL
};

static void
on_bus_acquired(GDBusConnection *connection,
                const gchar *name, gpointer user_data)
{
  GDBusNodeInfo *introspection_data = (GDBusNodeInfo *) user_data;

  GError *error = NULL;

  g_dbus_connection_register_object(
    connection,
    "/io/FuriOS/BatmanPowerConfig",
    introspection_data->interfaces[0],
    &interface_vtable,
    NULL,
    NULL,
    &error);

  if (error) {
    g_printerr("Error registering object: %s\n", error->message);
    g_error_free(error);
  }
}

static void
on_name_acquired(GDBusConnection *connection,
                 const gchar *name, gpointer user_data)
{
  g_debug("Name acquired: %s", name);
}

static void
on_name_lost(GDBusConnection *connection,
             const gchar *name, gpointer user_data)
{
  g_debug("Name lost: %s", name);
}

int
main()
{
  GMainLoop *loop;
  guint owner_id;
  GError *error = NULL;

  GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, &error);
  if (error) {
    g_printerr("Error parsing introspection XML: %s\n", error->message);
    g_error_free(error);
    return 1;
  }

  owner_id = g_bus_own_name(
    G_BUS_TYPE_SYSTEM,
    "io.FuriOS.BatmanPowerConfig",
    G_BUS_NAME_OWNER_FLAGS_NONE,
    on_bus_acquired,
    on_name_acquired,
    on_name_lost,
    introspection_data,
    NULL);

  loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);

  g_bus_unown_name(owner_id);
  g_dbus_node_info_unref(introspection_data);
  g_main_loop_unref(loop);

  return 0;
}
