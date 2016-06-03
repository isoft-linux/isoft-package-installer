/* vi: set sw=4 ts=4 wrap ai: */
/*
 * files.h: This file is part of ____
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

#ifndef __FILES_H__ 
#define __FILES_H__  1

#include <rpm/rpmbuild.h>
#include <rpm/rpmutil.h>

#ifdef __cplusplus
extern "C" {
#endif

rpmRC processPackageFiles(const char* buildroot, Package pkg, rpmBuildPkgFlags pkgFlags, int installSpecialDoc, int test);

#ifdef __cplusplus
}
#endif

#endif /* __FILES_H__ */
