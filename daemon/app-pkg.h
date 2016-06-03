/* vi: set sw=4 ts=4 wrap ai: */
/*
 * app-pkg.h: This file is part of ____
 *
 * Copyright (C) 2015 yetist <yetist@yetibook>
 *
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * */

#ifndef __APP_PKG_H__ 
#define __APP_PKG_H__  1

#include <rpm/rpmtypes.h>
#include <rpm/rpmds.h>

typedef struct _AppPkg AppPkg;

AppPkg* app_pkg_new                   (void);
void    app_pkg_set_name              (AppPkg* apkg, const char* name);
void    app_pkg_set_version           (AppPkg* apkg, const char* version);
void    app_pkg_set_release           (AppPkg* apkg, const char* release);
void    app_pkg_set_arch              (AppPkg* apkg, const char* arch);

void    app_pkg_set_url               (AppPkg* apkg, const char* url);
void    app_pkg_set_disttag           (AppPkg* apkg, const char* disttag);
void    app_pkg_set_bugurl            (AppPkg* apkg, const char* bugurl);
void    app_pkg_set_vcs               (AppPkg* apkg, const char* vcs);
void    app_pkg_set_disturl           (AppPkg* apkg, const char* disturl);
void    app_pkg_set_platform          (AppPkg* apkg, const char* platform);
void    app_pkg_set_optflags          (AppPkg* apkg, const char* optflags);

void    app_pkg_set_group             (AppPkg* apkg, const char* group);
void    app_pkg_set_summary           (AppPkg* apkg, const char* summary);
void    app_pkg_set_distribution      (AppPkg* apkg, const char* distribution);
void    app_pkg_set_vendor            (AppPkg* apkg, const char* vendor);
void    app_pkg_set_license           (AppPkg* apkg, const char* license);
void    app_pkg_set_packager          (AppPkg* apkg, const char* packager);
void    app_pkg_set_description       (AppPkg* apkg, const char* description);

void    app_pkg_add_lang_group        (AppPkg* apkg, const char* lang, const char* group);
void    app_pkg_add_lang_summary      (AppPkg* apkg, const char* lang, const char* summary);
void    app_pkg_add_lang_distribution (AppPkg* apkg, const char* lang, const char* distribution);
void    app_pkg_add_lang_vendor       (AppPkg* apkg, const char* lang, const char* vendor);
void    app_pkg_add_lang_license      (AppPkg* apkg, const char* lang, const char* license);
void    app_pkg_add_lang_packager     (AppPkg* apkg, const char* lang, const char* packager);
void    app_pkg_add_lang_description  (AppPkg* apkg, const char* lang, const char* description);

void    app_pkg_set_pre_scripts       (AppPkg* apkg, const char* scripts);
void    app_pkg_set_post_scripts      (AppPkg* apkg, const char* scripts);
void    app_pkg_set_preun_scripts     (AppPkg* apkg, const char* scripts);
void    app_pkg_set_postun_scripts    (AppPkg* apkg, const char* scripts);
void    app_pkg_set_pretrans_scripts  (AppPkg* apkg, const char* scripts);
void    app_pkg_set_posttrans_scripts (AppPkg* apkg, const char* scripts);

void    app_pkg_add_changelog         (AppPkg* apkg, rpm_time_t time, const char* name, const char* text);
void    app_pkg_add_filelist          (AppPkg* apkg, const char* flist);
void    app_pkg_add_require           (AppPkg* apkg, const char* name, rpmsenseFlags flags, const char* version);
void    app_pkg_add_provide           (AppPkg* apkg, const char* name, rpmsenseFlags flags, const char* version);

int     app_pkg_write                 (AppPkg* apkg, const char* root, const char* path);
int     app_pkg_free                  (AppPkg* apkg);

#endif /* __APP_PKG_H__ */
