/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2015 Leslie Zhai <xiang.zhai@i-soft.com.cn>
 * Copyright (C) 2015 fjiang <fujiang.zhu@i-soft.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>
#include <math.h>

#include "daemon.h"
#include "utils.h"
#include "rpm-helper.h"
#include "deb-helper.h"

enum {
    PROP_0,
    PROP_DAEMON_VERSION,
};

struct DaemonPrivate {
    GDBusConnection *bus_connection;
    GHashTable *extension_ifaces;
    GDBusMethodInvocation *context;

    gboolean task_locked;

    GList *rpm_list;

    gchar *deb;
    gchar *srcrpm;
};

static void daemon_install_install_iface_init(InstallInstallIface *iface);
static void *install_rpm_routine(void *arg);

G_DEFINE_TYPE_WITH_CODE(Daemon, daemon, INSTALL_TYPE_INSTALL_SKELETON, G_IMPLEMENT_INTERFACE(INSTALL_TYPE_INSTALL, daemon_install_install_iface_init));

#define DAEMON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), TYPE_DAEMON, DaemonPrivate))

static const GDBusErrorEntry install_error_entries[] =
{
    { ERROR_FAILED, "org.isoftlinux.Install.Error.Failed" },
    { ERROR_PERMISSION_DENIED, "org.isoftlinux.Install.Error.PermissionDenied" },
    { ERROR_NOT_SUPPORTED, "org.isoftlinux.Install.Error.NotSupported" }
};

GQuark
error_quark()
{
    static volatile gsize quark_volatile = 0;

    g_dbus_error_register_error_domain("install_error",
                                       &quark_volatile,
                                       install_error_entries,
                                       G_N_ELEMENTS(install_error_entries));

    return (GQuark)quark_volatile;
}
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
error_get_type()
{
    static GType etype = 0;

    if (etype == 0) {
        static const GEnumValue values[] =
        {
            ENUM_ENTRY(ERROR_FAILED, "Failed"),
            ENUM_ENTRY(ERROR_PERMISSION_DENIED, "PermissionDenied"),
            ENUM_ENTRY(ERROR_NOT_SUPPORTED, "NotSupported"),
            { 0, 0, 0 }
        };
        g_assert(NUM_ERRORS == G_N_ELEMENTS(values) - 1);
        etype = g_enum_register_static("Error", values);
    }
    return etype;
}

static void 
rpm_progress_handle(double *progress, void *arg_data, char *filename)
{
    Daemon *daemon = (Daemon *)arg_data;

    if (filename == NULL)
        return;

    install_install_emit_percent_changed(g_object_ref(INSTALL_INSTALL(daemon)),
                                         STATUS_INSTALL_INSTALL,
                                         filename,
                                         *progress);
}

static void 
rpm_problem_handler(char *problem, void *arg) 
{
    Daemon *daemon = (Daemon *)arg;
#ifdef DEBUG
    printf("DEBUG: %s, %s, line %d: %s\n", 
           __FILE__, __func__, __LINE__, problem);
#endif
    install_install_emit_error(g_object_ref(INSTALL_INSTALL(daemon)), 
                               g_strdup(problem));
}

static void 
deb_converted_handler(double *percent, void *arg_data, const char *filename)
{
    Daemon *daemon = (Daemon *)arg_data;
    GThread *thread = NULL;

	install_install_emit_percent_changed(g_object_ref(INSTALL_INSTALL(daemon)),
                                         STATUS_DEB2RPM,
                                         daemon->priv->deb,
                                         *percent);
    if (*percent != 1.0 || filename[0] == '\0')
        return;

#ifdef DEBUG
    printf("DEBUG: %s, %s, line %d: %s\n", 
           __FILE__, __func__, __LINE__, filename);
#endif
    daemon->priv->rpm_list = g_list_append(daemon->priv->rpm_list, 
                                           g_strdup(filename));
    g_thread_new(NULL, install_rpm_routine, daemon);
}

static void 
deb_problem_handler(char *problem, void *arg) 
{
    Daemon *daemon = (Daemon *)arg;
#ifdef DEBUG
    printf("DEBUG: %s, %s, line %d: %s\n", 
           __FILE__, __func__, __LINE__, problem);
#endif
    install_install_emit_error(g_object_ref(INSTALL_INSTALL(daemon)), 
                               g_strdup(problem));
}

static void
daemon_init(Daemon *daemon)
{
    GError *error = NULL;

    daemon->priv = DAEMON_GET_PRIVATE(daemon);

    daemon->priv->extension_ifaces = daemon_read_extension_ifaces();

    daemon->priv->context = NULL;

    daemon->priv->task_locked = FALSE;
}

static void
daemon_finalize(GObject *object)
{
    Daemon *daemon;

    g_return_if_fail(IS_DAEMON(object));

    daemon = DAEMON(object);

    if (daemon->priv->bus_connection) {
        g_object_unref(daemon->priv->bus_connection);
        daemon->priv->bus_connection = NULL;
    }

    G_OBJECT_CLASS(daemon_parent_class)->finalize(object);
}

static gboolean
register_install_daemon(Daemon *daemon)
{
    GError *error = NULL;

    daemon->priv->bus_connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (daemon->priv->bus_connection == NULL) {
        if (error != NULL) {
            write_log(LOG_CRIT,"error getting system bus: %s", error->message);
            g_error_free(error);
        }
        goto error;
    }

    if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(daemon),
                                          daemon->priv->bus_connection,
                                          "/org/isoftlinux/Install",
                                          &error)) {
        if (error != NULL) {
            write_log(LOG_CRIT,"error exporting interface: %s", error->message);
            g_error_free(error);
        }
        goto error;
    }

    return TRUE;

error:
    return FALSE;
}

Daemon *
daemon_new()
{
    Daemon *daemon;

    daemon = DAEMON(g_object_new(TYPE_DAEMON, NULL));

    if (!register_install_daemon(DAEMON(daemon))) {
        g_object_unref(daemon);
        goto error;
    }

    return daemon;

error:
    return NULL;
}

static void
throw_error(GDBusMethodInvocation *context,
            gint                   error_code,
            const gchar           *format,
            ...)
{
    va_list args;
    gchar *message;

    va_start(args, format);
    message = g_strdup_vprintf(format, args);
    va_end(args);

    g_dbus_method_invocation_return_error(context, ERROR, error_code, "%s", message);

    g_free(message);
}

static const gchar *
daemon_get_daemon_version(InstallInstall *object)
{
    return PROJECT_VERSION;
}

static void *
install_deb_routine(void *arg) 
{
    Daemon *daemon = (Daemon *)arg;
    
    install_deb(daemon->priv->deb, 
                deb_converted_handler, 
                deb_problem_handler, 
                daemon);

    daemon->priv->task_locked = FALSE;

    return NULL;
}

static gboolean 
daemon_add_install(InstallInstall *object, 
                   GDBusMethodInvocation *invocation, 
                   const gchar *file) 
{
    Daemon *daemon = (Daemon *)object;
    gchar *type = NULL;
    GThread *thread = NULL;
    
    type = g_content_type_guess(file, NULL, 0, NULL);
#ifdef DEBUG
    printf("DEBUG: %s, %s, line %d: %s %s\n", 
           __FILE__, __func__, __LINE__, file, type);
#endif
    if (strcmp(type, "application/x-rpm") == 0) {
        daemon->priv->rpm_list = g_list_append(daemon->priv->rpm_list, 
                                               g_strdup(file));
    } else if (strcmp(type, "application/vnd.debian.binary-package") == 0) {
        if (daemon->priv->task_locked) {
            printf("ERROR: %s, %s, line %d: task locked\n", 
                   __FILE__, __func__, __LINE__);
            rpm_problem_handler(_("task locked"), daemon);
            goto cleanup;
        }
        daemon->priv->deb = g_strdup(file);
        thread = g_thread_new(NULL, install_deb_routine, daemon);
    } else if (strcmp(type, "application/x-source-rpm") == 0) {
        rpm_problem_handler(_("NOTE: src.rpm will not be rebuilt to binary rpm"), 
                            daemon);
        daemon->priv->rpm_list = g_list_append(daemon->priv->rpm_list, 
                                               g_strdup(file));
    }

cleanup:
    if (type) {
        g_free(type);
        type = NULL;
    }

    return TRUE;
}

static void *
install_rpm_routine(void *arg) 
{
    Daemon *daemon = (Daemon *)arg;

    install_rpm(daemon->priv->rpm_list, 
                rpm_progress_handle, 
                rpm_problem_handler, 
                daemon);

    g_list_foreach(daemon->priv->rpm_list, (GFunc)g_free, NULL);
    g_list_free(daemon->priv->rpm_list);
    daemon->priv->rpm_list = NULL;

    install_install_emit_finished(g_object_ref(INSTALL_INSTALL(daemon)), 
                                  STATUS_INSTALL_INSTALL);

    daemon->priv->task_locked = FALSE;

    return NULL;
}

static gboolean 
daemon_install(InstallInstall *object, GDBusMethodInvocation *invocation) 
{
    Daemon *daemon = (Daemon *)object;
    GThread *thread = NULL;

    if (daemon->priv->task_locked) {
        printf("ERROR: %s, %s, line %d: task locked\n", __FILE__, __func__, __LINE__);
        rpm_problem_handler(_("task locked"), daemon);
        return TRUE;
    }
    daemon->priv->task_locked = TRUE;

    if (g_list_length(daemon->priv->rpm_list))
        thread = g_thread_new(NULL, install_rpm_routine, daemon);

    return TRUE;
}

GHashTable *
daemon_get_extension_ifaces(Daemon *daemon)
{
    return daemon->priv->extension_ifaces;
}

static void
daemon_get_property(GObject    *object,
                    guint       prop_id,
                    GValue     *value,
                    GParamSpec *pspec)
{
    Daemon *daemon = DAEMON(object);
    switch (prop_id) {
    case PROP_DAEMON_VERSION:
        g_value_set_string(value, PROJECT_VERSION);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
daemon_set_property(GObject      *object,
                    guint         prop_id,
                    const GValue *value,
                    GParamSpec   *pspec)
{
    switch (prop_id) {
    case PROP_DAEMON_VERSION:
        g_assert_not_reached();
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
daemon_class_init(DaemonClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = daemon_finalize;
    object_class->get_property = daemon_get_property;
    object_class->set_property = daemon_set_property;

    g_type_class_add_private(klass, sizeof(DaemonPrivate));

    g_object_class_override_property(object_class,
                                     PROP_DAEMON_VERSION,
                                     "daemon-version");
}

static void
daemon_install_install_iface_init(InstallInstallIface *iface)
{
    iface->get_daemon_version = daemon_get_daemon_version;
    iface->handle_add_install = daemon_add_install;
    iface->handle_install     = daemon_install;
}
