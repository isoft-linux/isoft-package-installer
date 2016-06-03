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
#include <unistd.h>

#include <glib/gi18n.h>

#include "debresolve.h"
#include "app-pkg.h"

static const char *m_dirtyfiles[] = { "conffiles", "control", "md5sums", 
                                      "postinst", "postrm", "preinst", "prerm", 
                                      NULL };

static void *m_arg = NULL;
static char *m_buildroot = NULL;
static AppPkg *m_apkg = NULL;

static void (*m_problem)(char *problem, void *arg);

static void traverse_directory(const gchar *path) 
{
    GDir *dir = NULL;
    GError *error = NULL;
    const gchar *file = NULL;
    gchar *newpath = NULL;
    gchar *syspath = NULL;

    dir = g_dir_open(path, 0, &error);
    if (!dir) {
        printf("ERROR: %s, line %d, %s\n", __func__, __LINE__, error->message);
        g_error_free(error);
        error = NULL;
        return;
    }

    while (file = g_dir_read_name(dir)) {
        newpath = g_strdup_printf("%s/%s", path, file);
        if (g_file_test(newpath, G_FILE_TEST_IS_DIR)) {
            traverse_directory(newpath);
        } else if (g_file_test(newpath, G_FILE_TEST_IS_REGULAR)) {
            syspath = g_utf8_substring(newpath, strlen(m_buildroot), strlen(newpath));
            app_pkg_add_filelist(m_apkg, syspath);

            g_free(syspath);
            syspath = NULL;
        }

        g_free(newpath);
        newpath = NULL;
    }

    g_dir_close(dir);
    dir = NULL;
}

void                                                                               
install_deb(const char *deb,
            void (*converted)(double *percent, void *arg, const char *filename),
            void (*problem)(char *prob, void *arg),
            void *arg)
{
    double percent = .0;
    char *meta = NULL;
	char *v1 = NULL, *v2 = NULL, *v3 = NULL;
	gchar *package = NULL;
    gchar *version = NULL;
    gchar *arch = NULL;
    char out[PATH_MAX] = { '\0' };

    m_arg = arg;
    m_problem = problem;

	m_buildroot = tmpnam(NULL);
	meta = tmpnam(NULL);
    if (!m_buildroot || !meta) {
        printf("ERROR: failed to create temporary directory\n");
        m_problem(_("ERROR: failed to create temporary directory"), arg);
        goto exit;
    }
#ifdef DEBUG
    printf("buildroot: %s\nmeta: %s\n", m_buildroot, meta);
#endif

	deb_extract(deb, m_buildroot);
	percent = 0.2;
    converted(&percent, m_arg, out);
    deb_control(deb, meta);
    percent = 0.4;
    converted(&percent, m_arg, out);

	v1 = malloc(256);
	v2 = malloc(256);
	v3 = malloc(256);
    if (!v1 || !v2 || !v3) {
        printf("ERROR: failed to allocate memory\n");
        m_problem(_("ERROR: failed to allocate memory"), arg);
        goto exit;
    }

	if (deb_info(deb, meta, "Package", v1) == -1)
        goto exit;
    package = g_utf8_substring(v1, 9, strlen(v1) - 1);
    if (deb_info(deb, meta, "Version", v2) == -1)
        goto exit;
    version = g_utf8_substring(v2, 9, strlen(v2) - 1);
	if (deb_info(deb, meta, "Architecture", v3) == -1)
        goto exit;
    arch = g_utf8_substring(v3, 14, strlen(v3) - 1);

    m_apkg = app_pkg_new();
    if (!m_apkg) {
        printf("ERROR: failed to instance AppPkg object\n");
        goto exit;
    }
    app_pkg_set_name(m_apkg, package);
    app_pkg_set_version(m_apkg, version);
    app_pkg_set_release(m_apkg, "1");
    if (strstr(v3, "amd64"))
        app_pkg_set_arch(m_apkg, "x86_64");
    else
        app_pkg_set_arch(m_apkg, arch);

	deb_info(deb, meta, "Homepage", v1);
	app_pkg_set_url(m_apkg, v1);

	deb_info(deb, meta, "Section", v1);
	app_pkg_set_group(m_apkg, v1);

	deb_info(deb, meta, "Description", v1);
	char *p = strchr(v1, '\n');
	*p = '\0';
	app_pkg_set_summary(m_apkg, v1);
	app_pkg_set_description(m_apkg, p+1);

	app_pkg_set_packager(m_apkg, "isoft-packager");
    percent = 0.6;
    converted(&percent, m_arg, out);

    for (int i = 0; m_dirtyfiles[i]; i++) {
        memset(out, 0, sizeof(out));
        snprintf(out, sizeof(out) - 1, "%s/%s", m_buildroot, m_dirtyfiles[i]);
        unlink(out);
    }
	traverse_directory(m_buildroot);
    memset(out, 0, sizeof(out));
    snprintf(out, sizeof(out) - 1, "%s.rpm", deb);
    app_pkg_write(m_apkg, m_buildroot, out);
    percent = 0.8;
    converted(&percent, m_arg, out);

    percent = 1.0;
    converted(&percent, m_arg, out);

exit:
    if (package) {
        g_free(package);
        package = NULL;
    }

    if (version) {
        g_free(version);
        version = NULL;
    }

    if (arch) {
        g_free(arch);
        arch = NULL;
    }

    if (m_apkg) {
        app_pkg_free(m_apkg);
        m_apkg = NULL;
    }
}
