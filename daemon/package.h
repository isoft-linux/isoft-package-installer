/* vi: set sw=4 ts=4 wrap ai: */
/*
 * package.h: This file is part of ____
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

#ifndef __PACKAGE_H__ 
#define __PACKAGE_H__  1

#include <rpm/rpmbuild.h>
#include <rpm/rpmutil.h>
#include <rpm/rpmstrpool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define xmalloc(_size) rmalloc((_size))
#define xcalloc(_nmemb, _size) rcalloc((_nmemb), (_size))
#define xrealloc(_ptr, _size) rrealloc((_ptr), (_size))
#define xstrdup(_str) rstrdup((_str))
#define _free(_ptr) rfree((_ptr))
#define _(Text) Text

#define PACKAGE_NUM_DEPS 12

typedef struct _Package              *Package;

struct TriggerFileEntry {
    int index;
    char * fileName;
    char * script;
    char * prog;
    uint32_t flags;
    struct TriggerFileEntry * next;
    uint32_t priority;
};

struct Source {
    char * fullSource;
    const char * source;     /* Pointer into fullSource */
    int flags;
    uint32_t num;
struct Source * next;
};

struct _Package {
    rpmsid name;
    rpmstrPool pool;
    Header header;
    rpmds ds;			/*!< Requires: N = EVR */
    rpmds dependencies[PACKAGE_NUM_DEPS];
    rpmfiles cpioList;
    rpm_loff_t  cpioArchiveSize;
    ARGV_t dpaths;

    struct Source * icon;

    int autoReq;
    int autoProv;

    char * preInFile;	/*!< %pre scriptlet. */
    char * postInFile;	/*!< %post scriptlet. */
    char * preUnFile;	/*!< %preun scriptlet. */
    char * postUnFile;	/*!< %postun scriptlet. */
    char * preTransFile;	/*!< %pretrans scriptlet. */
    char * postTransFile;	/*!< %posttrans scriptlet. */
    char * verifyFile;	/*!< %verifyscript scriptlet. */

    struct TriggerFileEntry * triggerFiles;
    struct TriggerFileEntry * fileTriggerFiles;
    struct TriggerFileEntry * transFileTriggerFiles;

    ARGV_t fileFile;
    ARGV_t fileList;		/* If NULL, package will not be written */
    ARGV_t removePostfixes;
    ARGV_t policyList;

};

Package newPackage(const char *name);
Package freePackage(Package pkg);
rpmds * packageDependencies(Package pkg, rpmTagVal tag);
void addPackageProvides(Package pkg);

#ifdef __cplusplus
}
#endif

#endif /* __PACKAGE_H__ */
