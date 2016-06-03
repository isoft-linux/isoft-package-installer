#ifndef __DEBRESOLVE_H__
#define __DEBRESOLVE_H__
#include <glib.h>

int deb_extract(const char *debpath, const char *dir);
int deb_control(const char *debpath, const char *dir);
int deb_info(const char *debpath, const char *dir, const char *name, char *buf);

G_DEPRECATED_FOR(deb_extract)
int do_contents(const char *filename);

#endif /* __DEBRESOLVE_H__ */
