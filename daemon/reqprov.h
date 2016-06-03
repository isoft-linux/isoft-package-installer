#ifndef __REQPROV_H__ 
#define __REQPROV_H__  1

#include <rpm/header.h>
#include <rpm/rpmstring.h>
#include <rpm/rpmlog.h>
#include "package.h"

int addReqProv(Package pkg, rpmTagVal tagN, const char * N, const char * EVR, rpmsenseFlags Flags, uint32_t index);
int rpmlibNeedsFeature(Package pkg, const char * feature, const char * featureEVR);

#endif /* __REQPROV_H__ */
