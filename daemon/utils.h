/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>


/*
* to write logs
*/
#define LOG_MAX_LINE_SIZE 1024
#define LOG_MAX_FILE_SIZE (1024*1024*10)
#define LOG_FILE_PATH     "/var/log/isoft-install"
#define UPT_RECORD_LOG_FILE_PATH     "/etc/update/updates"
#define INSTALLED_LOG_FILE     UPT_RECORD_LOG_FILE_PATH"/updated.log"
#define NOT_INSTALLED_LOG_FILE     UPT_RECORD_LOG_FILE_PATH"/updating.log"
#define LOG_FILE_FILTER     "|||"


#define error_exit(_errmsg_)	error(EXIT_FAILURE, errno, \
		"%s:%d -> %s\n", __FILE__, __LINE__, _errmsg_)	

extern char *utils_split_string(char *str, const char *needle);
extern bool utils_find_file(const char *path, const char *name);
extern bool utils_check_number(const char *str);
extern int utils_copy_file(const char *src, const char *dest);
extern int utils_remove_dir(const char *dir);

/*
* write_log usage:
* 1.int _write_log(const char *fmt,...); default:LOG_DEBUG
* 2.int write_log(int log_level,const char *fmt,...);
* 3.int write_error_log(const char *fmt,...); only for ERROR
* 4.int write_debug_log(const char *fmt,...); only for DEBUG
* 5.int set_write_log_level(int log_level); set default level to @log_level
*/
extern int set_write_log_level(int log_level);
extern int _write_log(const char *fmt,...);
extern int write_log(int log_level,const char *fmt,...);
extern int write_error_log(const char *fmt,...);
extern int write_debug_log(const char *fmt,...);

/*
* to write upt_name state  type  date to the following 2 files:
* /var/log/isoft-update/updated.log: installed
* /var/log/isoft-update/updating.log: no installed
*
* @upt_name:update-2015-09-10-5-x86_64.upt
* @type:bugfix/update/security
* @action:'u' -- not installed; 'i'--installed
* @state:1--installed successfully;0--failed;-1--not installed
*/
extern int write_upt_reocord_file(const char *upt_name,const char *type,char action,int state);

#endif /* __UTILS_H__ */
