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

#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmcli.h>
#include <rpm/rpmts.h>
#include <glib/gi18n.h>

#include "rpm-helper.h"

enum modes {
    MODE_QUERY      = (1 <<  0),
    MODE_VERIFY     = (1 <<  3),
#define MODES_QV (MODE_QUERY | MODE_VERIFY)

    MODE_INSTALL    = (1 <<  1),
    MODE_ERASE      = (1 <<  2),
#define MODES_IE (MODE_INSTALL | MODE_ERASE)

    MODE_UNKNOWN    = 0
};

#define MODES_FOR_NODEPS    (MODES_IE | MODE_VERIFY)
#define MODES_FOR_TEST      (MODES_IE)

static int quiet;
static void *m_arg_data = NULL;

static struct poptOption optionsTable[] = {
    {NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmQVSourcePoptTable, 0,
     N_("Query/Verify package selection options:"),
     NULL},
    {NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmQueryPoptTable, 0,
     N_("Query options (with -q or --query):"),
     NULL},
    {NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmVerifyPoptTable, 0,
     N_("Verify options (with -V or --verify):"),
     NULL},
    {NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmInstallPoptTable, 0,
     N_("Install/Upgrade/Erase options:"),
     NULL},
    {"quiet", '\0', POPT_ARGFLAG_DOC_HIDDEN, &quiet, 0, NULL, NULL},
    {NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliAllPoptTable, 0,
     N_("Common options for all rpm modes and executables:"),
     NULL},
    POPT_AUTOALIAS
    POPT_AUTOHELP
    POPT_TABLEEND
};

static void (*m_progress_handler)(double *progress, void *arg_data, char *filename);
static void (*m_problem_handler)(char *problem, void *arg_data);

static int rpmcliProgressState = 0;
static int rpmcliPackagesTotal = 0;
static int rpmcliHashesCurrent = 0;
static int rpmcliHashesTotal = 0;
static int rpmcliProgressCurrent = 0;
static int rpmcliProgressTotal = 0;

void *
rpmShowProgress(const void *arg,
                const rpmCallbackType what,
                const rpm_loff_t amount,
                const rpm_loff_t total,
                fnpyKey key,
                void *data)
{
    Header h = (Header) arg;
    int flags = (int) ((long)data);
    void *rc = NULL;
    const char *filename = (const char *)key;
    static FD_t fd = NULL;

    switch (what) {
    case RPMCALLBACK_INST_OPEN_FILE:
        if (filename == NULL || filename[0] == '\0')
            return NULL;
        fd = Fopen(filename, "r.ufdio");
        if(fd == NULL)
            return NULL;
        if (fd == NULL || Ferror(fd)) {
            if (fd != NULL) {
                Fclose(fd);

                fd = NULL;
            }
        } else
            fd = fdLink(fd);
        return (void *)fd;
        break;

    case RPMCALLBACK_INST_CLOSE_FILE:
        fd = fdFree(fd);
        if (fd != NULL) {
            Fclose(fd);
            fd = NULL;
        }
        break;

    case RPMCALLBACK_INST_START:
    case RPMCALLBACK_UNINST_START:
        if (rpmcliProgressState != what) {
            rpmcliProgressState = what;
            if (flags & INSTALL_HASH) {
                if (what == RPMCALLBACK_INST_START) {
                    fprintf(stdout, _("Updating / installing...\n"));
                } else {
                    fprintf(stdout, _("Cleaning up / removing...\n"));
                }
                fflush(stdout);
            }
        }

        rpmcliHashesCurrent = 0;
        if (h == NULL || !(flags & INSTALL_LABEL))
            break;
        if (flags & INSTALL_HASH) {
            char *s = headerGetAsString(h, RPMTAG_NEVR);
            if (isatty (STDOUT_FILENO))
                fprintf(stdout, "%4d:%-33.33s", rpmcliProgressCurrent + 1, s);
            else
                fprintf(stdout, "%-38.38s", s);
            (void) fflush(stdout);
            free(s);
        } else {
            char *s = headerGetAsString(h, RPMTAG_NEVRA);
            fprintf(stdout, "%s\n", s);
            (void) fflush(stdout);
            free(s);
        }
        break;

    case RPMCALLBACK_INST_STOP:
        break;

    case RPMCALLBACK_TRANS_PROGRESS:
    case RPMCALLBACK_INST_PROGRESS:
    case RPMCALLBACK_UNINST_PROGRESS:
        if (flags & INSTALL_PERCENT) {
            double progress = (double) (total ? (((float) amount) / total): 100.0);
            m_progress_handler(&progress, m_arg_data, (char *)filename);
        }

        (void) fflush(stdout);

        break;

    case RPMCALLBACK_TRANS_START:
        rpmcliHashesCurrent = 0;
        rpmcliProgressTotal = 1;
        rpmcliProgressCurrent = 0;
        rpmcliPackagesTotal = total;
        rpmcliProgressState = what;
        if (!(flags & INSTALL_LABEL))
            break;
        if (flags & INSTALL_HASH)
            fprintf(stdout, "%-38s", _("Preparing..."));
        else
            fprintf(stdout, "%s\n", _("Preparing packages..."));
        (void) fflush(stdout);
        break;

    case RPMCALLBACK_TRANS_STOP:
        rpmcliProgressTotal = rpmcliPackagesTotal;
        rpmcliProgressCurrent = 0;
        break;

    case RPMCALLBACK_UNINST_STOP:
        break;
    case RPMCALLBACK_UNPACK_ERROR:
        break;

    case RPMCALLBACK_CPIO_ERROR:
        break;
    case RPMCALLBACK_SCRIPT_ERROR:
        break;
    case RPMCALLBACK_SCRIPT_START:
        break;
    case RPMCALLBACK_SCRIPT_STOP:
        break;
    case RPMCALLBACK_UNKNOWN:
    default:
        break;
    }

    return rc;
}

void 
install_rpm(GList *rpm_list, 
            void (*report_progress)(double *progress, void *arg_data, char *filename),
            void (*problem_handler)(char *prob, void *arg_data),
            void *arg_data)
{
    rpmts ts = NULL;
    struct rpmInstallArguments_s *ia = &rpmIArgs;
    poptContext optCon = NULL;
    chunk_t prob;
    int argc = -1;
    char **argv = NULL;
    int i = 0;
    GList *l = NULL;

    if (rpm_list == NULL        || 
        report_progress == NULL || 
        problem_handler == NULL || 
        arg_data == NULL) {
        printf("ERROR: %s, %s, line %d: invalid parameter\n", 
               __FILE__, __func__, __LINE__);
        return;
    }

    m_arg_data = arg_data;
    m_progress_handler = report_progress;
    m_problem_handler = problem_handler;

    argc = g_list_length(rpm_list) + 1;
    argv = malloc(sizeof(char *) * argc);
    if (argv == NULL) {
        m_problem_handler(
            g_strdup_printf(_("ERROR: %s, %s, line %d: failed to allocate memory"), 
                __FILE__, __func__, __LINE__), 
            m_arg_data);
        goto cleanup;
    }

    memset(argv, 0, sizeof(char *) * argc);
    for (i = 0; i < argc; i++) {
        argv[i] = malloc(sizeof(char) * PATH_MAX);
        if (argv[i] == NULL) {
            m_problem_handler(
                g_strdup_printf(_("ERROR: %s, %s, line %d: failed to allocate memory"), 
                    __FILE__, __func__, __LINE__), 
                m_arg_data);
            goto cleanup;
        }
        memset(argv[i], 0, sizeof(char) * PATH_MAX);
    }
    snprintf(argv[0], strlen(argv[0]) - 1, __func__);
    for (l = rpm_list, i = 1; l; l = g_list_next(l), i++)
        snprintf(argv[i], strlen(argv[i]) - 1, l->data);

    optCon = rpmcliInit(argc, argv, optionsTable);

    ia->installInterfaceFlags = INSTALL_UPGRADE | INSTALL_INSTALL | INSTALL_PERCENT;

    ts = rpmtsCreate();
    if (ts == NULL) {
        m_problem_handler(
            g_strdup_printf(_("ERROR: %s, %s, line %d: failed to create rpm ts"), 
                __FILE__, __func__, __LINE__), 
            m_arg_data);
        goto cleanup;
    }
    rpmtsSetRootDir(ts, rpmcliRootDir);

    if (!ia->incldocs) {
        if (rpmExpandNumeric("%{_excludedocs}"))
            ia->transFlags |= RPMTRANS_FLAG_NODOCS;
    }

    if (ia->noDeps)
        ia->installInterfaceFlags |= INSTALL_NODEPS;

    if (ia->prefix) {
        ia->relocations = malloc(2 * sizeof(*ia->relocations));
        ia->relocations[0].oldPath = NULL;   /* special case magic */
        ia->relocations[0].newPath = ia->prefix;
        ia->relocations[1].oldPath = NULL;
        ia->relocations[1].newPath = NULL;
    } else if (ia->relocations) {
        ia->relocations = realloc(ia->relocations,
            sizeof(*ia->relocations) * (ia->numRelocations + 1));
        ia->relocations[ia->numRelocations].oldPath = NULL;
        ia->relocations[ia->numRelocations].newPath = NULL;
    }

    prob.memory = malloc(1);
    if (prob.memory == NULL) {
        m_problem_handler(
            g_strdup_printf(_("ERROR: %s, %s, line %d: failed to allocate memory"), 
                __FILE__, __func__, __LINE__), 
            m_arg_data);
        goto cleanup;
    }
    prob.size = 0;
    /* iSOFT App interface */
    rpmInstallISoftApp(ts, ia, (ARGV_t) poptGetArgs(optCon), &prob);
    if (prob.size)
        m_problem_handler(prob.memory, m_arg_data);

cleanup:
    if (ts) {
        rpmtsFree(ts);
        ts = NULL;
    }

    if (optCon) {
        rpmcliFini(optCon);
        optCon = NULL;
    }

    if (argv) {
        for (i = 0; i < argc; i++) {
            if (argv[i]) {
                free(argv[i]);
                argv[i] = NULL;
            }
        }
        free(argv);
        argv = NULL;
    }
}
