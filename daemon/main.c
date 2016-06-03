/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2015 Leslie Zhai <xiang.zhai@i-soft.com.cn>
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
#include <stdarg.h>
#include <locale.h>
#include <libintl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-unix.h>
#include <gio/gio.h>

#include "daemon.h"
#include "utils.h"

#define NAME_TO_CLAIM "org.isoftlinux.Install"

static GMainLoop *loop;

static gboolean
ensure_directory(const char  *path,
                 GError     **error)
{
    if (g_mkdir_with_parents(path, 0775) < 0) {
        g_set_error(error,
                    G_FILE_ERROR,
                    g_file_error_from_errno(errno),
                    "Failed to create directory %s: %m",
                    path);
        return FALSE;
    }
    return TRUE;
}

static void
on_bus_acquired(GDBusConnection  *connection,
                const gchar      *name,
                gpointer          user_data)
{
    Daemon *daemon;
    GError *local_error = NULL;
    GError **error = &local_error;

    daemon = daemon_new();
    if (daemon == NULL) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "Failed to initialize daemon");
        goto out;
    }

    openlog("isoft-install-daemon", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "started daemon version %s", PROJECT_VERSION);
    closelog();
    openlog("isoft-install-daemon", 0, LOG_AUTHPRIV);

 out:
    if (local_error != NULL) {
        g_printerr ("%s\n", local_error->message);
        g_clear_error (&local_error);
        g_main_loop_quit (loop);
    }
}

static void
on_name_lost(GDBusConnection  *connection,
             const gchar      *name,
             gpointer          user_data)
{
    g_debug("got NameLost, exiting");
    g_main_loop_quit(loop);
}

static gboolean debug;

static void
on_log_debug(const gchar *log_domain,
             GLogLevelFlags log_level,
             const gchar *message,
             gpointer user_data)
{
    GString *string;
    const gchar *progname;
    int ret G_GNUC_UNUSED;

    string = g_string_new(NULL);

    progname = g_get_prgname();
#if 0
    g_string_append_printf(string, "(%s:%lu): %s%sDEBUG: %s\n",
                           progname ? progname : "process", (gulong)getpid (),
                           log_domain ? log_domain : "", log_domain ? "-" : "",
                           message ? message : "(NULL) message");
    ret = write(1, string->str, string->len);
#else
    g_string_append_printf(string, "%s",message ? message : "(NULL) message");
    write_log(LOG_DEBUG,string->str);
#endif
    if (string) {
        g_string_free(string,true);
    }
}

static void
log_handler(const gchar   *domain,
            GLogLevelFlags level,
            const gchar   *message,
            gpointer       data)
{
    /* filter out DEBUG messages if debug isn't set */
    if ((level & G_LOG_LEVEL_MASK) == G_LOG_LEVEL_DEBUG && !debug)
        return;

    g_log_default_handler(domain, level, message, data);
}

static gboolean
on_signal_quit(gpointer data)
{
    g_main_loop_quit(data);
    return FALSE;
}

int
main(int argc, char *argv[])
{
    GError *error;
    gint ret;
    GBusNameOwnerFlags flags;
    GOptionContext *context;
    static gboolean replace;
    static gboolean show_version;
    static GOptionEntry entries[] = {
        { "version", 0, 0, G_OPTION_ARG_NONE, &show_version, N_("Output version information and exit"), NULL },
        { "replace", 0, 0, G_OPTION_ARG_NONE, &replace, N_("Replace existing instance"), NULL },
        { "debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Enable debugging code"), NULL },

        { NULL }
    };

    ret = 1;
    error = NULL;

    write_log(LOG_NOTICE,"entering main function,pid[%d]",(int)getpid());

    bindtextdomain(GETTEXT_PACKAGE, PROJECT_LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

#if !GLIB_CHECK_VERSION(2, 35, 3)
    g_type_init();
#endif

    if (!g_setenv("GIO_USE_VFS", "local", TRUE)) {
        write_log(LOG_WARNING,"Couldn't set GIO_USE_GVFS");
        goto out;
    }

    setlocale(LC_ALL, "");
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    context = g_option_context_new("");
    g_option_context_set_translation_domain(context, GETTEXT_PACKAGE);
    g_option_context_set_summary(context, _("Provides D-Bus interfaces for querying and manipulating\niSOFT Install information."));
    g_option_context_add_main_entries(context, entries, NULL);
    error = NULL;
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        write_log(LOG_WARNING,error->message);
        g_error_free(error);
        goto out;
    }
    g_option_context_free(context);

    if (show_version) {
        g_print("isoft-install-daemon " PROJECT_VERSION "\n");
        ret = 0;
        goto out;
    }

    /* If --debug, then print debug messages even when no G_MESSAGES_DEBUG */
    if (debug && !g_getenv("G_MESSAGES_DEBUG")) {
        g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, on_log_debug, NULL);
        set_write_log_level(LOG_DEBUG);
    } else {
        set_write_log_level(LOG_DEBUG);
    }
    g_log_set_default_handler(log_handler, NULL);

    flags = G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT;
    if (replace)
        flags |= G_BUS_NAME_OWNER_FLAGS_REPLACE;
    g_bus_own_name(G_BUS_TYPE_SYSTEM,
                   NAME_TO_CLAIM,
                   flags,
                   on_bus_acquired,
                   NULL,
                   on_name_lost,
                   NULL,
                   NULL);

    loop = g_main_loop_new(NULL, FALSE);

    g_unix_signal_add(SIGINT, on_signal_quit, loop);
    g_unix_signal_add(SIGTERM, on_signal_quit, loop);
    write_log(LOG_DEBUG,"entering main loop.");
    g_main_loop_run(loop);

    write_log(LOG_DEBUG,"exiting.");
    g_main_loop_unref(loop);

    ret = 0;

 out:
    write_log(LOG_NOTICE,"exit main function,pid[%d].",(int)getpid());
    return ret;
}

