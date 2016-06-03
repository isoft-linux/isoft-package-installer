/* vi: set sw=4 ts=4 wrap ai: */
/*
 * package.c: This file is part of ____
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

#include <rpm/header.h>
#include <rpm/rpmds.h>
#include <rpm/rpmfi.h>
#include <rpm/rpmts.h>
#include <rpm/rpmlog.h>
#include <rpm/rpmfileutil.h>

#include "package.h"
#include "reqprov.h"
#include "files.h"

/**
 * @param p		trigger entry chain
 * @return		NULL always
 */
static inline struct TriggerFileEntry * freeTriggerFiles(struct TriggerFileEntry * p)
{
    struct TriggerFileEntry *o, *q = p;
    
    while (q != NULL) {
	o = q;
	q = q->next;
	o->fileName = rfree(o->fileName);
	o->script = rfree(o->script);
	o->prog = rfree(o->prog);
	free(o);
    }
    return NULL;
}

/**
 * Destroy source component chain.
 * @param s		source component chain
 * @return		NULL always
 */
static inline struct Source * freeSources(struct Source * s)
{
    struct Source *r, *t = s;

    while (t != NULL) {
	r = t;
	t = t->next;
	r->fullSource = rfree(r->fullSource);
	free(r);
    }
    return NULL;
}


Package newPackage(const char *name)
{
    Package p = rcalloc(1, sizeof(*p));
    p->header = headerNew();
    p->autoProv = 1;
    p->autoReq = 1;
    p->fileList = NULL;
    p->fileFile = NULL;
    p->policyList = NULL;
	p->pool = rpmstrPoolCreate();
    p->dpaths = NULL;

    if (name)
		p->name = rpmstrPoolId(p->pool, name, 1);

    return p;
}

Package freePackage(Package pkg)
{
    if (pkg == NULL) return NULL;
    
    pkg->preInFile = rfree(pkg->preInFile);
    pkg->postInFile = rfree(pkg->postInFile);
    pkg->preUnFile = rfree(pkg->preUnFile);
    pkg->postUnFile = rfree(pkg->postUnFile);
    pkg->verifyFile = rfree(pkg->verifyFile);

    pkg->header = headerFree(pkg->header);
    pkg->ds = rpmdsFree(pkg->ds);

    for (int i=0; i<PACKAGE_NUM_DEPS; i++) {
	pkg->dependencies[i] = rpmdsFree(pkg->dependencies[i]);
    }

    pkg->fileList = argvFree(pkg->fileList);
    pkg->fileFile = argvFree(pkg->fileFile);
    pkg->policyList = argvFree(pkg->policyList);
    pkg->removePostfixes = argvFree(pkg->removePostfixes);
    pkg->cpioList = rpmfilesFree(pkg->cpioList);
    pkg->dpaths = argvFree(pkg->dpaths);

    pkg->icon = freeSources(pkg->icon);
    pkg->triggerFiles = freeTriggerFiles(pkg->triggerFiles);
    pkg->fileTriggerFiles = freeTriggerFiles(pkg->fileTriggerFiles);
    pkg->transFileTriggerFiles = freeTriggerFiles(pkg->transFileTriggerFiles);
    pkg->pool = rpmstrPoolFree(pkg->pool);

    free(pkg);
    return NULL;
}

rpmds * packageDependencies(Package pkg, rpmTagVal tag)
{
    for (int i=0; i<PACKAGE_NUM_DEPS; i++) {
	if (pkg->dependencies[i] == NULL) {
	    return &pkg->dependencies[i];
	}
	rpmTagVal tagN = rpmdsTagN(pkg->dependencies[i]);
	if (tagN == tag || tagN == 0) {
	    return &pkg->dependencies[i];
	}
    }
    return NULL;
}

void addPackageProvides(Package pkg)
{
    const char *arch, *name;
    char *evr, *isaprov;
    rpmsenseFlags pflags = RPMSENSE_EQUAL;

    /* <name> = <evr> provide */
    name = headerGetString(pkg->header, RPMTAG_NAME);
    arch = headerGetString(pkg->header, RPMTAG_ARCH);
    evr = headerGetAsString(pkg->header, RPMTAG_EVR);
    addReqProv(pkg, RPMTAG_PROVIDENAME, name, evr, pflags, 0);

    /*
     * <name>(<isa>) = <evr> provide
     * FIXME: noarch needs special casing for now as BuildArch: noarch doesn't
     * cause reading in the noarch macros :-/ 
     */
    isaprov = rpmExpand(name, "%{?_isa}", NULL);
    if (!rstreq(arch, "noarch") && !rstreq(name, isaprov)) {
		addReqProv(pkg, RPMTAG_PROVIDENAME, isaprov, evr, pflags, 0);
    }
    free(isaprov);
    free(evr);
}
