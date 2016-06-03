/** \ingroup rpmbuild
 * \file build/reqprov.c
 *  Add dependency tags to package header(s).
 */


#include <rpm/header.h>
#include <rpm/rpmstring.h>
#include <rpm/rpmlog.h>
#include "package.h"

int addReqProv(Package pkg, rpmTagVal tagN,
		const char * N, const char * EVR, rpmsenseFlags Flags,
		uint32_t index)
{
    rpmds newds, *dsp = NULL;

    dsp = packageDependencies(pkg, tagN);

    /* rpmlib() dependency sanity: only requires permitted, ensure sense bit */
    if (rstreqn(N, "rpmlib(", sizeof("rpmlib(")-1)) {
	if (tagN != RPMTAG_REQUIRENAME) return 1;
	Flags |= RPMSENSE_RPMLIB;
    }

    newds = rpmdsSinglePoolTix(pkg->pool, tagN, N, EVR,
			       rpmSanitizeDSFlags(tagN, Flags), index);

    rpmdsMerge(dsp, newds);
    rpmdsFree(newds);

    return 0;
}

int rpmlibNeedsFeature(Package pkg, const char * feature, const char * featureEVR)
{
    char *reqname = NULL;
    int res;

    rasprintf(&reqname, "rpmlib(%s)", feature);

    res = addReqProv(pkg, RPMTAG_REQUIRENAME, reqname, featureEVR,
		     RPMSENSE_RPMLIB|(RPMSENSE_LESS|RPMSENSE_EQUAL), 0);

    free(reqname);

    return res;
}
