/** \ingroup rpmbuild
 * \file build/pack.c
 *  Assemble components of an RPM package.
 */

#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <sys/wait.h>
#include <iconv.h>

#include <rpm/rpmlib.h>			/* RPMSIGTAG*, rpmReadPackageFile */
#include <rpm/rpmfileutil.h>
#include <rpm/rpmlog.h>

#include "package.h"
#include "misc.h"
#include "reqprov.h"

rpmRC rpmGenerateSignature(char *SHA1, uint8_t *MD5, rpm_loff_t size, rpm_loff_t payloadSize, FD_t fd);
void fdInitDigest(FD_t fd, int hashalgo, rpmDigestFlags flags);

typedef struct rpmlead_s * rpmlead;
rpmlead rpmLeadFromHeader(Header h);
rpmRC rpmLeadWrite(FD_t fd, rpmlead lead);
rpmlead rpmLeadFree(rpmlead lead);


void fdFiniDigest(FD_t fd, int hashalgo, void ** datap, size_t * lenp, int asAscii);

static int rpmPackageFilesArchive(rpmfiles fi, int isSrc,
				  FD_t cfd, ARGV_t dpaths,
				  rpm_loff_t * archiveSize, char ** failedFile)
{
    int rc = 0;
    rpmfi archive = rpmfiNewArchiveWriter(cfd, fi);

    while (!rc && (rc = rpmfiNext(archive)) >= 0) {
        /* Copy file into archive. */
	FD_t rfd = NULL;
	const char *path = dpaths[rpmfiFX(archive)];

	rfd = Fopen(path, "r.ufdio");
	if (Ferror(rfd)) {
	    rc = RPMERR_OPEN_FAILED;
	} else {
	    rc = rpmfiArchiveWriteFile(archive, rfd);
	}

	if (rc && failedFile)
	    *failedFile = xstrdup(path);
	if (rfd) {
	    /* preserve any prior errno across close */
	    int myerrno = errno;
	    Fclose(rfd);
	    errno = myerrno;
	}
    }

    if (rc == RPMERR_ITER_END)
	rc = 0;

    /* Finish the payload stream */
    if (!rc)
	rc = rpmfiArchiveClose(archive);

    if (archiveSize)
	*archiveSize = (rc == 0) ? rpmfiArchiveTell(archive) : 0;

    rpmfiFree(archive);

    return rc;
}
/**
 * @todo Create transaction set *much* earlier.
 */
static rpmRC cpio_doio(FD_t fdo, Package pkg, const char * fmodeMacro)
{
    char *failedFile = NULL;
    FD_t cfd;
    int fsmrc;

    (void) Fflush(fdo);
    cfd = Fdopen(fdDup(Fileno(fdo)), fmodeMacro);
    if (cfd == NULL)
	return RPMRC_FAIL;

    fsmrc = rpmPackageFilesArchive(pkg->cpioList, headerIsSource(pkg->header),
				   cfd, pkg->dpaths,
				   &pkg->cpioArchiveSize, &failedFile);

    if (fsmrc) {
	char *emsg = rpmfileStrerror(fsmrc);
	if (failedFile)
	    rpmlog(RPMLOG_ERR, _("create archive failed on file %s: %s\n"),
		   failedFile, emsg);
	else
	    rpmlog(RPMLOG_ERR, _("create archive failed: %s\n"), emsg);
	free(emsg);
    }

    free(failedFile);
    Fclose(cfd);

    return (fsmrc == 0) ? RPMRC_OK : RPMRC_FAIL;
}

static int haveTildeDep(Package pkg)
{
    for (int i = 0; i < PACKAGE_NUM_DEPS; i++) {
	rpmds ds = rpmdsInit(pkg->dependencies[i]);
	while (rpmdsNext(ds) >= 0) {
	    if (strchr(rpmdsEVR(ds), '~'))
		return 1;
	}
    }
    return 0;
}

static int haveRichDep(Package pkg)
{
    for (int i = 0; i < PACKAGE_NUM_DEPS; i++) {
	rpmds ds = rpmdsInit(pkg->dependencies[i]);
	rpmTagVal tagN = rpmdsTagN(ds);
	if (tagN != RPMTAG_REQUIRENAME && tagN != RPMTAG_CONFLICTNAME)
	    continue;
	while (rpmdsNext(ds) >= 0) {
	    if (rpmdsIsRich(ds))
		return 1;
	}
    }
    return 0;
}

static rpm_loff_t estimateCpioSize(Package pkg)
{
    rpmfi fi;
    rpm_loff_t size = 0;

    fi = rpmfilesIter(pkg->cpioList, RPMFI_ITER_FWD);
    while (rpmfiNext(fi) >= 0) {
	size += strlen(rpmfiDN(fi)) + strlen(rpmfiBN(fi));
	size += rpmfiFSize(fi);
	size += 300;
    }
    return size;
}

rpmRC checkForEncoding(Header h, int addtag)
{
    rpmRC rc = RPMRC_OK;
    const char *encoding = "utf-8";
    rpmTagVal tag;
    iconv_t ic = (iconv_t) -1;
    char *dest = NULL;
    size_t destlen = 0;
    int strict = rpmExpandNumeric("%{_invalid_encoding_terminates_build}");
    HeaderIterator hi = headerInitIterator(h);

    ic = iconv_open(encoding, encoding);
    if (ic == (iconv_t) -1) {
	rpmlog(RPMLOG_WARNING,
		_("encoding %s not supported by system\n"), encoding);
	goto exit;
    }

    while ((tag = headerNextTag(hi)) != RPMTAG_NOT_FOUND) {
	struct rpmtd_s td;
	const char *src = NULL;

	if (rpmTagGetClass(tag) != RPM_STRING_CLASS)
	    continue;

	headerGet(h, tag, &td, (HEADERGET_RAW|HEADERGET_MINMEM));
	while ((src = rpmtdNextString(&td)) != NULL) {
	    size_t srclen = strlen(src);
	    size_t outlen, inlen = srclen;
	    char *out, *in = (char *) src;

	    if (destlen < srclen) {
		destlen = srclen * 2;
		dest = xrealloc(dest, destlen);
	    }
	    out = dest;
	    outlen = destlen;

	    /* reset conversion state */
	    iconv(ic, NULL, &inlen, &out, &outlen);

	    if (iconv(ic, &in, &inlen, &out, &outlen) == (size_t) -1) {
		rpmlog(strict ? RPMLOG_ERR : RPMLOG_WARNING,
			_("Package %s: invalid %s encoding in %s: %s - %s\n"),
			headerGetString(h, RPMTAG_NAME),
			encoding, rpmTagGetName(tag), src, strerror(errno));
		rc = RPMRC_FAIL;
	    }

	}
	rpmtdFreeData(&td);
    }

    /* Stomp "known good utf" mark in header if requested */
    if (rc == RPMRC_OK && addtag)
	headerPutString(h, RPMTAG_ENCODING, encoding);
    if (!strict)
	rc = RPMRC_OK;

exit:
    if (ic != (iconv_t) -1)
	iconv_close(ic);
    headerFreeIterator(hi);
    free(dest);
    return rc;
}

rpmRC writeRPM(Package pkg, const char *fileName)
{

    FD_t fd = NULL;
    char * rpmio_flags = NULL;
    char * SHA1 = NULL;
    uint8_t * MD5 = NULL;
    const char *s;
    rpmRC rc = RPMRC_OK;
    rpm_loff_t estimatedCpioSize;
    unsigned char buf[32*BUFSIZ];
    uint8_t zeros[] =  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    char *zerosS = "0000000000000000000000000000000000000000";
    off_t sigStart;
    off_t sigTargetStart;
    off_t sigTargetSize;

    /* Save payload information */
    if (headerIsSource(pkg->header))
	rpmio_flags = rpmExpand("%{?_source_payload}", NULL);
    else 
	rpmio_flags = rpmExpand("%{?_binary_payload}", NULL);

    /* If not configured or bogus, fall back to gz */
    if (rpmio_flags[0] != 'w') {
	free(rpmio_flags);
	rpmio_flags = xstrdup("w9.gzdio");
    }
    s = strchr(rpmio_flags, '.');
    if (s) {
	char *buf = NULL;
	const char *compr = NULL;
	headerPutString(pkg->header, RPMTAG_PAYLOADFORMAT, "cpio");

	if (rstreq(s+1, "ufdio")) {
	    compr = NULL;
	} else if (rstreq(s+1, "gzdio")) {
	    compr = "gzip";
	} else if (rstreq(s+1, "bzdio")) {
	    compr = "bzip2";
	    /* Add prereq on rpm version that understands bzip2 payloads */
	    (void) rpmlibNeedsFeature(pkg, "PayloadIsBzip2", "3.0.5-1");
	} else if (rstreq(s+1, "xzdio")) {
	    compr = "xz";
	    (void) rpmlibNeedsFeature(pkg, "PayloadIsXz", "5.2-1");
	} else if (rstreq(s+1, "lzdio")) {
	    compr = "lzma";
	    (void) rpmlibNeedsFeature(pkg, "PayloadIsLzma", "4.4.6-1");
	} else {
	    rpmlog(RPMLOG_ERR, _("Unknown payload compression: %s\n"),
		   rpmio_flags);
	    rc = RPMRC_FAIL;
	    goto exit;
	}

	if (compr)
	    headerPutString(pkg->header, RPMTAG_PAYLOADCOMPRESSOR, compr);
	buf = xstrdup(rpmio_flags);
	buf[s - rpmio_flags] = '\0';
	headerPutString(pkg->header, RPMTAG_PAYLOADFLAGS, buf+1);
	free(buf);
    }

    /* check if the package has a dependency with a '~' */
    if (haveTildeDep(pkg))
	(void) rpmlibNeedsFeature(pkg, "TildeInVersions", "4.10.0-1");

    /* check if the package has a rich dependency */
    if (haveRichDep(pkg))
	(void) rpmlibNeedsFeature(pkg, "RichDependencies", "4.12.0-1");

    /* All dependencies added finally, write them into the header */
    for (int i = 0; i < PACKAGE_NUM_DEPS; i++) {
	/* Nuke any previously added dependencies from the header */
	headerDel(pkg->header, rpmdsTagN(pkg->dependencies[i]));
	headerDel(pkg->header, rpmdsTagEVR(pkg->dependencies[i]));
	headerDel(pkg->header, rpmdsTagF(pkg->dependencies[i]));
	headerDel(pkg->header, rpmdsTagTi(pkg->dependencies[i]));
	/* ...and add again, now with automatic dependencies included */
	rpmdsPutToHeader(pkg->dependencies[i], pkg->header);
    }

    /* Check for UTF-8 encoding of string tags, add encoding tag if all good */
    if (checkForEncoding(pkg->header, 1)) {
	rc = RPMRC_FAIL;
	goto exit;
    }

    /* Reallocate the header into one contiguous region. */
    pkg->header = headerReload(pkg->header, RPMTAG_HEADERIMMUTABLE);
    if (pkg->header == NULL) {
	rc = RPMRC_FAIL;
	rpmlog(RPMLOG_ERR, _("Unable to create immutable header region.\n"));
	goto exit;
    }

    /* Open the output file */
    fd = Fopen(fileName, "w+.ufdio");
    if (fd == NULL || Ferror(fd)) {
	rc = RPMRC_FAIL;
	rpmlog(RPMLOG_ERR, _("Could not open %s: %s\n"),
		fileName, Fstrerror(fd));
	goto exit;
    }

    /* Write the lead section into the package. */
    {	
	rpmlead lead = rpmLeadFromHeader(pkg->header);
	rc = rpmLeadWrite(fd, lead);
	rpmLeadFree(lead);
	if (rc != RPMRC_OK) {
	    rc = RPMRC_FAIL;
	    rpmlog(RPMLOG_ERR, _("Unable to write package: %s\n"),
		 Fstrerror(fd));
	    goto exit;
	}
    }

    /* Estimate cpio archive size to decide if use 32 bits or 64 bit tags. */
    estimatedCpioSize = estimateCpioSize(pkg);

    /* Save the position of signature section */
    sigStart = Ftell(fd);

    /* Generate and write a placeholder signature header */
    rc = rpmGenerateSignature(zerosS, zeros, 0, estimatedCpioSize, fd);
    if (rc != RPMRC_OK) {
	rc = RPMRC_FAIL;
	goto exit;
    }

    /* Write the header and archive section. Calculate SHA1 from them. */
    sigTargetStart = Ftell(fd);
    fdInitDigest(fd, PGPHASHALGO_SHA1, 0);
    if (headerWrite(fd, pkg->header, HEADER_MAGIC_YES)) {
	rc = RPMRC_FAIL;
	rpmlog(RPMLOG_ERR, _("Unable to write temp header\n"));
	goto exit;
    }
    (void) Fflush(fd);
    fdFiniDigest(fd, PGPHASHALGO_SHA1, (void **)&SHA1, NULL, 1);

    /* Write payload section (cpio archive) */
    if (pkg->cpioList == NULL) {
	rc = RPMRC_FAIL;
	rpmlog(RPMLOG_ERR, _("Bad CSA data\n"));
	goto exit;
    }
    rc = cpio_doio(fd, pkg, rpmio_flags);
    if (rc != RPMRC_OK)
	goto exit;

    sigTargetSize = Ftell(fd) - sigTargetStart;

    if(Fseek(fd, sigTargetStart, SEEK_SET) < 0) {
	rc = RPMRC_FAIL;
	rpmlog(RPMLOG_ERR, _("Could not seek in file %s: %s\n"),
		fileName, Fstrerror(fd));
	goto exit;
    }

    /* Calculate MD5 checksum from header and archive section. */
    fdInitDigest(fd, PGPHASHALGO_MD5, 0);
    while (Fread(buf, sizeof(buf[0]), sizeof(buf), fd) > 0)
	;
    if (Ferror(fd)) {
	rc = RPMRC_FAIL;
	rpmlog(RPMLOG_ERR, _("Fread failed in file %s: %s\n"),
		fileName, Fstrerror(fd));
	goto exit;
    }
    fdFiniDigest(fd, PGPHASHALGO_MD5, (void **)&MD5, NULL, 0);

    if(Fseek(fd, sigStart, SEEK_SET) < 0) {
	rc = RPMRC_FAIL;
	rpmlog(RPMLOG_ERR, _("Could not seek in file %s: %s\n"),
		fileName, Fstrerror(fd));
	goto exit;
    }

    /* Generate the signature. Now with right values */
    rc = rpmGenerateSignature(SHA1, MD5, sigTargetSize, pkg->cpioArchiveSize, fd);
    if (rc != RPMRC_OK) {
	rc = RPMRC_FAIL;
	goto exit;
    }

exit:
    free(rpmio_flags);
    free(SHA1);

    Fclose(fd);

    if (rc == RPMRC_OK)
	rpmlog(RPMLOG_NOTICE, _("Wrote: %s\n"), fileName);
    else
	(void) unlink(fileName);

    return rc;
}
