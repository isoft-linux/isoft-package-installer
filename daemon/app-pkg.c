/* vi: set sw=4 ts=4 wrap ai: */
/*
 * app-pkg.c: This file is part of ____
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

#include <netdb.h>
#include "package.h"
#include "files.h"
#include "reqprov.h"
#include "app-pkg.h"

rpmRC writeRPM(Package pkg, const char *fileName);
rpmRC rpmfcGenerateDepends(const char* buildRoot, Package pkg);

struct _AppPkg {
	Package pkg;
};

AppPkg* app_pkg_new(void)
{
	AppPkg *apkg;
	Package pkg;

	rpmcliConfigured();
	pkg = newPackage(NULL);
	pkg->fileList = argvNew();

	apkg = xmalloc(sizeof(AppPkg));
	apkg->pkg = pkg;
	return apkg;
}

void app_pkg_set_nvra(AppPkg* apkg, const char* name, const char* version, const char *release, const char* arch)
{
	Package pkg;
	pkg = apkg->pkg;

	if (!pkg->name) {
		pkg->name = rpmstrPoolId(pkg->pool, name, 1);
	}
	headerPutString(pkg->header, RPMTAG_NAME, name);
	headerPutString(pkg->header, RPMTAG_VERSION, version);
	headerPutString(pkg->header, RPMTAG_RELEASE, release);
	headerPutString(pkg->header, RPMTAG_ARCH, arch);
}

void app_pkg_set_name(AppPkg* apkg, const char* name)
{
	Package pkg;
	pkg = apkg->pkg;

	pkg->name = rpmstrPoolId(pkg->pool, name, 1);
	headerPutString(pkg->header, RPMTAG_NAME, name);
}

void app_pkg_set_version(AppPkg* apkg, const char* version)
{
	Package pkg;
	pkg = apkg->pkg;

	headerPutString(pkg->header, RPMTAG_VERSION, version);
}

void app_pkg_set_release(AppPkg* apkg, const char* release)
{
	Package pkg;
	pkg = apkg->pkg;

	headerPutString(pkg->header, RPMTAG_RELEASE, release);
}

void app_pkg_set_arch(AppPkg* apkg, const char* arch)
{
	Package pkg;
	pkg = apkg->pkg;

	headerPutString(pkg->header, RPMTAG_ARCH, arch);
}

void app_pkg_set_url(AppPkg* apkg, const char* url)
{
	Package pkg;
	pkg = apkg->pkg;

	headerPutString(pkg->header, RPMTAG_URL, url);
}

void app_pkg_set_disttag(AppPkg* apkg, const char* disttag)
{
	Package pkg;
	pkg = apkg->pkg;

	headerPutString(pkg->header, RPMTAG_DISTTAG, disttag);
}

void app_pkg_set_bugurl(AppPkg* apkg, const char* bugurl)
{
	Package pkg;
	pkg = apkg->pkg;

	headerPutString(pkg->header, RPMTAG_BUGURL, bugurl);
}

void app_pkg_set_vcs(AppPkg* apkg, const char* vcs)
{
	Package pkg;
	pkg = apkg->pkg;

	headerPutString(pkg->header, RPMTAG_VCS, vcs);
}

void app_pkg_set_disturl(AppPkg* apkg, const char* disturl)
{
	Package pkg;
	pkg = apkg->pkg;

	headerPutString(pkg->header, RPMTAG_DISTURL, disturl);
}

void app_pkg_set_platform(AppPkg* apkg, const char* platform)
{
	Package pkg;
	pkg = apkg->pkg;

	headerPutString(pkg->header, RPMTAG_PLATFORM, platform);
}

void app_pkg_set_optflags(AppPkg* apkg, const char* optflags)
{
	Package pkg;
	pkg = apkg->pkg;

	headerPutString(pkg->header, RPMTAG_OPTFLAGS, optflags);
}

void app_pkg_set_group(AppPkg* apkg, const char* group)
{
	Package pkg;
	pkg = apkg->pkg;
	headerPutString(pkg->header, RPMTAG_GROUP, group);
}

void app_pkg_set_summary(AppPkg* apkg, const char* summary)
{
	Package pkg;
	pkg = apkg->pkg;
	headerPutString(pkg->header, RPMTAG_SUMMARY, summary);
}

void app_pkg_set_distribution(AppPkg* apkg, const char* distribution)
{
	Package pkg;
	pkg = apkg->pkg;
	headerPutString(pkg->header, RPMTAG_DISTRIBUTION, distribution);
}

void app_pkg_set_vendor(AppPkg* apkg, const char* vendor)
{
	Package pkg;
	pkg = apkg->pkg;
	headerPutString(pkg->header, RPMTAG_VENDOR, vendor);
}

void app_pkg_set_license(AppPkg* apkg, const char* license)
{
	Package pkg;
	pkg = apkg->pkg;
	headerPutString(pkg->header, RPMTAG_LICENSE, license);
}

void app_pkg_set_packager(AppPkg* apkg, const char* packager)
{
	Package pkg;
	pkg = apkg->pkg;
	headerPutString(pkg->header, RPMTAG_PACKAGER, packager);
}

void app_pkg_set_description(AppPkg* apkg, const char* description)
{
	Package pkg;
	pkg = apkg->pkg;
	headerPutString(pkg->header, RPMTAG_DESCRIPTION, description);
}

void app_pkg_add_lang_group(AppPkg* apkg, const char* lang, const char* group)
{
	Package pkg;
	pkg = apkg->pkg;

	headerAddI18NString(pkg->header, RPMTAG_GROUP, group, lang);
}

void app_pkg_add_lang_summary(AppPkg* apkg, const char* lang, const char* summary)
{
	Package pkg;
	pkg = apkg->pkg;

	headerAddI18NString(pkg->header, RPMTAG_SUMMARY, summary, lang);
}

void app_pkg_add_lang_distribution(AppPkg* apkg, const char* lang, const char* distribution)
{
	Package pkg;
	pkg = apkg->pkg;

	headerAddI18NString(pkg->header, RPMTAG_DISTRIBUTION, distribution, lang);
}

void app_pkg_add_lang_vendor(AppPkg* apkg, const char* lang, const char* vendor)
{
	Package pkg;
	pkg = apkg->pkg;

	headerAddI18NString(pkg->header, RPMTAG_VENDOR, vendor, lang);
}

void app_pkg_add_lang_license(AppPkg* apkg, const char* lang, const char* license)
{
	Package pkg;
	pkg = apkg->pkg;

	headerAddI18NString(pkg->header, RPMTAG_LICENSE, license, lang);
}

void app_pkg_add_lang_packager(AppPkg* apkg, const char* lang, const char* packager)
{
	Package pkg;
	pkg = apkg->pkg;

	headerAddI18NString(pkg->header, RPMTAG_PACKAGER, packager, lang);
}

void app_pkg_add_lang_description(AppPkg* apkg, const char* lang, const char* description)
{
	Package pkg;
	pkg = apkg->pkg;

	headerAddI18NString(pkg->header, RPMTAG_DESCRIPTION, description, lang);
}

void app_pkg_add_filelist(AppPkg* apkg, const char* flist)
{
	Package pkg;
	pkg = apkg->pkg;

	argvAdd(&(pkg->fileList), flist);
}

/* add changelog */
void app_pkg_add_changelog(AppPkg* apkg, rpm_time_t time, const char* name, const char* text)
{
	Package pkg;
	pkg = apkg->pkg;

	headerPutUint32(pkg->header, RPMTAG_CHANGELOGTIME, &time, 1);
	headerPutString(pkg->header, RPMTAG_CHANGELOGNAME, name);
	headerPutString(pkg->header, RPMTAG_CHANGELOGTEXT, text);
}

void app_pkg_set_pre_scripts(AppPkg* apkg, const char* scripts)
{
	Package pkg;
	pkg = apkg->pkg;

	/* add scripts */
	headerPutString(pkg->header, RPMTAG_PREINPROG, "/bin/sh");
	headerPutString(pkg->header, RPMTAG_PREIN, scripts);
}

void app_pkg_set_post_scripts(AppPkg* apkg, const char* scripts)
{
	Package pkg;
	pkg = apkg->pkg;

	/* add scripts */
	headerPutString(pkg->header, RPMTAG_POSTINPROG, "/bin/sh");
	headerPutString(pkg->header, RPMTAG_POSTIN, scripts);
}

void app_pkg_set_preun_scripts(AppPkg* apkg, const char* scripts)
{
	Package pkg;
	pkg = apkg->pkg;

	/* add scripts */
	headerPutString(pkg->header, RPMTAG_PREUNPROG, "/bin/sh");
	headerPutString(pkg->header, RPMTAG_PREUN, scripts);
}

void app_pkg_set_postun_scripts(AppPkg* apkg, const char* scripts)
{
	Package pkg;
	pkg = apkg->pkg;

	/* add scripts */
	headerPutString(pkg->header, RPMTAG_POSTUNPROG, "/bin/sh");
	headerPutString(pkg->header, RPMTAG_POSTUN, scripts);
}

void app_pkg_set_pretrans_scripts(AppPkg* apkg, const char* scripts)
{
	Package pkg;
	pkg = apkg->pkg;

	/* add scripts */
	headerPutString(pkg->header, RPMTAG_PRETRANSPROG, "/bin/sh");
	headerPutString(pkg->header, RPMTAG_PRETRANS, scripts);
}

void app_pkg_set_posttrans_scripts(AppPkg* apkg, const char* scripts)
{
	Package pkg;
	pkg = apkg->pkg;

	/* add scripts */
	headerPutString(pkg->header, RPMTAG_POSTTRANSPROG, "/bin/sh");
	headerPutString(pkg->header, RPMTAG_POSTTRANS, scripts);
}


void app_pkg_add_require(AppPkg* apkg, const char* name, rpmsenseFlags flags, const char* version)
{
	Package pkg;
	pkg = apkg->pkg;

	addReqProv(pkg, RPMTAG_REQUIRENAME, name, version, flags, 0);
}

void app_pkg_add_provide(AppPkg* apkg, const char* name, rpmsenseFlags flags, const char* version)
{
	Package pkg;
	pkg = apkg->pkg;

	addReqProv(pkg, RPMTAG_PROVIDENAME, name, version, flags, 0);
}

static const char * buildHost(void)
{
    static char hostname[1024];
    static int oneshot = 0;
    struct hostent *hbn;

    if (! oneshot) {
        (void) gethostname(hostname, sizeof(hostname));
	hbn = gethostbyname(hostname);
	if (hbn)
	    strcpy(hostname, hbn->h_name);
	else
	    rpmlog(RPMLOG_WARNING, "Could not canonicalize hostname: %s\n", hostname);
	oneshot = 1;
    }
    return(hostname);
}

static rpm_time_t * getBuildTime(void)
{
    static rpm_time_t buildTime[1];

    if (buildTime[0] == 0)
	buildTime[0] = (int32_t) time(NULL);
    return buildTime;
}

int app_pkg_write(AppPkg* apkg, const char* root, const char* path)
{
	rpmRC rc = RPMRC_OK;
	char *nvr;
	char cookie[256];
	char srpm[256];

	Package pkg;
	pkg = apkg->pkg;

	addPackageProvides(pkg);
	pkg->ds = rpmdsThis(pkg->header, RPMTAG_REQUIRENAME, RPMSENSE_EQUAL);

	/* setup src.rpm name */
	nvr = headerGetAsString(pkg->header, RPMTAG_NVR);
	snprintf(srpm, sizeof(srpm), "%s.src.rpm", nvr);
	free(nvr);
	headerPutString(pkg->header, RPMTAG_SOURCERPM, srpm);

	if ((rc = processPackageFiles(root, pkg, 0, 4, 0)) != RPMRC_OK)
		return RPMRC_FAIL;

	if ((rc = rpmfcGenerateDepends(root, pkg)) != RPMRC_OK)
		return RPMRC_FAIL;

	/* Add os header, required */
	headerPutString(pkg->header, RPMTAG_OS, "linux");
    /* Add rpmversion, buildhost and buildtime */
    headerPutString(pkg->header, RPMTAG_RPMVERSION, RPMVERSION);
    headerPutString(pkg->header, RPMTAG_BUILDHOST, buildHost());
    headerPutUint32(pkg->header, RPMTAG_BUILDTIME, getBuildTime(), 1);

    /* Create and add the cookie */
	sprintf(cookie, "%s %d", buildHost(), (int) (*getBuildTime()));
	headerPutString(pkg->header, RPMTAG_COOKIE, cookie);

	writeRPM(apkg->pkg, path);
	return RPMRC_OK;
}

int app_pkg_free(AppPkg* apkg)
{
	freePackage(apkg->pkg);
	free(apkg);
	return 0;
}
