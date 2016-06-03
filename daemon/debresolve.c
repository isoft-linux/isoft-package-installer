#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <ar.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <dpkg/path.h>
#include <dpkg/string.h>

#include <dpkg/dpkg.h>
#include <dpkg/fdio.h>
#include <dpkg/buffer.h>
#include <dpkg/subproc.h>
#include <dpkg/command.h>
#include <dpkg/compress.h>
#include <dpkg/ar.h>
#include <dpkg/deb-version.h>
#include <dpkg/options.h>

#include <dpkg/parsedump.h>

#include "debresolve.h"

#define TAR "/usr/bin/tar"
#define DEBMAGIC		"debian-binary"
#define ADMINMEMBER		"control.tar"
#define DATAMEMBER		"data.tar"
#define OLDOLDDEBDIR		".DEBIAN"
#define OLDDEBDIR		"DEBIAN"
#define BUILDCONTROLDIR		"DEBIAN"
#define EXTRACTCONTROLDIR	BUILDCONTROLDIR

enum dpkg_tar_options {
        /** Output the tar file directly, without any processing. */
        DPKG_TAR_PASSTHROUGH = 0,
        /** List tar files. */
        DPKG_TAR_LIST = DPKG_BIT(0),
        /** Extract tar files. */
        DPKG_TAR_EXTRACT = DPKG_BIT(1),
        /** Preserve tar permissions on extract. */
        DPKG_TAR_PERMS = DPKG_BIT(2),
        /** Do not set tar mtime on extract. */
        DPKG_TAR_NOMTIME = DPKG_BIT(3),
};


void *deb_malloc(size_t amount) {
#ifdef MDEBUG
  unsigned short *ptr_canary, canary;
#endif
  void *ptr;

  ptr = malloc(amount);

#ifdef MDEBUG
  ptr_canary = ptr;
  canary = (unsigned short)amount ^ 0xf000;
  while (amount >= 2) {
    *ptr_canary++ = canary;
    amount -= 2;
  }
#endif
  return ptr;
}
int
deb_output(FILE *f, const char *name)
{
  fflush(f);
  if (ferror(f))
    return -1;
}
pid_t
deb_subproc_fork(void)
{
        pid_t pid;

        pid = fork();
        if (pid == -1) {
            return -1;
        }
        if (pid > 0)
                return pid;

        /* Push a new error context, so that we don't do the other cleanups,
         * because they'll be done by/in the parent process. */
        //push_error_context_func(catch_fatal_error, print_subproc_error, NULL);

        return pid;
}
static int
deb_subproc_check(int status, const char *desc, enum subproc_flags flags)
{
        void (*out)(const char *fmt, ...) DPKG_ATTR_PRINTF(1);
        int n;

        if (flags & SUBPROC_WARN)
                out = warning;
        else
                out = ohshit;

        if (WIFEXITED(status)) {
                n = WEXITSTATUS(status);
                if (!n)
                        return 0;
                if (flags & SUBPROC_RETERROR)
                        return n;

                return -1;
        } else if (WIFSIGNALED(status)) {
                n = WTERMSIG(status);
                if (!n)
                        return 0;
                if ((flags & SUBPROC_NOPIPE) && n == SIGPIPE)
                        return 0;
                if (flags & SUBPROC_RETSIGNO)
                        return n;

                if (n == SIGINT)
                        return -1;
                else
                        return -1;
        } else {
                if (flags & SUBPROC_RETERROR)
                        return -1;

                return -1;
        }

        return -1;
}


static int
deb_subproc_wait(pid_t pid, const char *desc)
{
        pid_t dead_pid;
        int status;

        while ((dead_pid = waitpid(pid, &status, 0)) == -1 && errno == EINTR) ;

        if (dead_pid != pid) {
                return -1;
        }

        return status;
}

int
deb_subproc_reap(pid_t pid, const char *desc, enum subproc_flags flags)
{
        int status, rc;

        status = deb_subproc_wait(pid, desc);

        if (flags & SUBPROC_NOCHECK)
                rc = status;
        else
                rc = deb_subproc_check(status, desc, flags);

        return rc;
}

int deb_dup2(int oldfd, int newfd) {
  const char *const stdstrings[]= { "in", "out", "err" };

  if (dup2(oldfd,newfd) == newfd) return 0;

  if (newfd < 3) return -1;
}

void
deb_command_shell(const char *cmd, const char *name)
{
        const char *shell;
        const char *mode;

        if (cmd == NULL) {
                mode = "-i";
                shell = getenv("SHELL");
        } else {
                mode = "-c";
                shell = NULL;
        }

        if (str_is_unset(shell))
                shell = DEFAULTSHELL;

        execlp(shell, shell, mode, cmd, NULL);
}
void
deb_command_init(struct command *cmd, const char *filename, const char *name)
{
        cmd->filename = filename;
        if (name == NULL)
                cmd->name = path_basename(filename);
        else
                cmd->name = name;
        cmd->argc = 0;
        cmd->argv_size = 10;
        cmd->argv = deb_malloc(cmd->argv_size * sizeof(const char *));
        cmd->argv[0] = NULL;
}


static void movecontrolfiles(const char *thing) {
	char buf[200];
	pid_t pid;

	sprintf(buf, "mv %s/* . && rmdir %s", thing, thing);
    pid = deb_subproc_fork();
	if (pid == 0) {
        deb_command_shell(buf, "shell command to move files");
	}
    deb_subproc_reap(pid, "shell command to move files", 0);
}

static void DPKG_ATTR_NORET read_fail(int rc, const char *filename, const char *what)
{
	if (rc >= 0)
		ohshit("unexpected end of file in %s in %.255s",what,filename);
	else
		ohshite("error reading %s from file %.255s", what, filename);
}


static ssize_t read_line(int fd, char *buf, size_t min_size, size_t max_size)//bufsize -1
{
	ssize_t line_size = 0;
	size_t n = min_size;

	while (line_size < (ssize_t)max_size) {
		ssize_t r;
		char *nl;

		r = fd_read(fd, buf + line_size, n);
		if (r <= 0)
			return r;

		nl = (char *)memchr(buf + line_size, '\n', r);
		line_size += r;

		if (nl != NULL) {
			nl[1] = '\0';
			return line_size;
		}

		n = 1;
	}

	buf[line_size] = '\0';
	return line_size;
}

/*
 * FIXME:
 * 注意处理所有的返回值，返回0表示成功，返回其它值表示失败。
 * 尤其要注意的是里面调用的一些函数会调用exit()直接退出，这样会导致调用本函数如果出错，整个程序直接退出。
 * 这类函数有exit(), ohshit(), ohshite()，m_xxx()之类的函数。
 * 类似的还有m_xxx()。
 * subproc_*(), command_*() 之类的函数暂不确定，需要检查。
 */
static int extracthalf(const char *debar, const char *dir, enum dpkg_tar_options taroption, int admininfo)
{
	struct dpkg_error err;
	const char *errstr;
	char versionbuf[40];
	struct deb_version version;
	off_t ctrllennum, memberlen = 0;
	ssize_t r;
	int dummy;
	pid_t c1=0,c2,c3;
	int p1[2], p2[2];
	int p2_out;
	int arfd;
	struct stat stab;
	char nlc;
	int adminmember = -1;
	bool header_done;
	enum compressor_type decompressor = COMPRESSOR_TYPE_GZIP;

	arfd = open(debar, O_RDONLY);
	if (arfd < 0)
        return -1;
	if (fstat(arfd, &stab))
        return -1;

	r = read_line(arfd, versionbuf, strlen(DPKG_AR_MAGIC), sizeof(versionbuf));
	if (r < 0)
        return -1;

	if (strcmp(versionbuf, DPKG_AR_MAGIC) == 0) {
		ctrllennum= 0;
		header_done = false;
		for (;;) {
			struct ar_hdr arh;

			r = fd_read(arfd, &arh, sizeof(arh));
			if (r != sizeof(arh))
                return -1;

			dpkg_ar_normalize_name(&arh);

			if (dpkg_ar_member_is_illegal(&arh))
                return -1;
			memberlen = dpkg_ar_member_get_size(debar, &arh);
			if (!header_done) {
				char *infobuf;

				if (strncmp(arh.ar_name, DEBMAGIC, sizeof(arh.ar_name)) != 0)
                    return -1;
                infobuf= deb_malloc(memberlen+1);
				r = fd_read(arfd, infobuf, memberlen + (memberlen & 1));
				if (r != (memberlen + (memberlen & 1)))
                    return -1;
				infobuf[memberlen] = '\0';

				if (strchr(infobuf, '\n') == NULL)
                    return -1;
				errstr = deb_version_parse(&version, infobuf);
				if (errstr)
                    return -1;
				if (version.major != 2)
                    return -1;

				free(infobuf);

				header_done = true;
			} else if (arh.ar_name[0] == '_') {
				/* Members with ‘_’ are noncritical, and if we don't understand
				 * them we skip them. */
				if (fd_skip(arfd, memberlen + (memberlen & 1), &err) < 0)
                    return -1;
			} else {
				if (strncmp(arh.ar_name, ADMINMEMBER, strlen(ADMINMEMBER)) == 0) {
					const char *extension = arh.ar_name + strlen(ADMINMEMBER);

					adminmember = 1;
					decompressor = compressor_find_by_extension(extension);
					if (decompressor != COMPRESSOR_TYPE_NONE &&
							decompressor != COMPRESSOR_TYPE_GZIP &&
							decompressor != COMPRESSOR_TYPE_XZ)
                        return -1;
				} else {
					if (adminmember != 1)
                        return -1;
					if (strncmp(arh.ar_name, DATAMEMBER, strlen(DATAMEMBER)) == 0) {
						const char *extension = arh.ar_name + strlen(DATAMEMBER);

						adminmember= 0;
						decompressor = compressor_find_by_extension(extension);
						if (decompressor == COMPRESSOR_TYPE_UNKNOWN)
                            return -1;
					} else {
                        return -1;
					}
				}
				if (adminmember == 1) {
					if (ctrllennum != 0)
                        return -1;
					ctrllennum= memberlen;
				}
				if (!adminmember != !admininfo) {
					if (fd_skip(arfd, memberlen + (memberlen & 1), &err) < 0)
                        return -1;
				} else {
					/* Yes! - found it. */
					break;
				}
			}
		}

		if (admininfo >= 2) {
			printf(" new debian package, version %d.%d.\n"
					" size %jd bytes: control archive=%jd bytes.\n",
					version.major, version.minor,
					(intmax_t)stab.st_size, (intmax_t)ctrllennum);
            deb_output(stdout, "<standard output>");
		}
	} else if (strncmp(versionbuf, "0.93", 4) == 0) {
		char ctrllenbuf[40];
		int l;

		l = strlen(versionbuf);

		if (strchr(versionbuf, '\n') == NULL)
            return -1;
		errstr = deb_version_parse(&version, versionbuf);
		if (errstr)
            return -1;

		r = read_line(arfd, ctrllenbuf, 1, sizeof(ctrllenbuf));
		if (r < 0)
            return -1;
		if (sscanf(ctrllenbuf, "%jd%c%d", &ctrllennum, &nlc, &dummy) != 2 ||
				nlc != '\n')
            return -1;

		if (admininfo) {
			memberlen = ctrllennum;
		} else {
			memberlen = stab.st_size - ctrllennum - strlen(ctrllenbuf) - l;
			if (fd_skip(arfd, ctrllennum, &err) < 0)
                return -1;
		}

		if (admininfo >= 2) {
			printf(" old debian package, version %d.%d.\n"
					" size %jd bytes: control archive=%jd, main archive=%jd.\n",
					version.major, version.minor,
					(intmax_t)stab.st_size, (intmax_t)ctrllennum,
					(intmax_t)(stab.st_size - ctrllennum - strlen(ctrllenbuf) - l));
            deb_output(stdout, "<standard output>");
		}
	} else {
		if (strncmp(versionbuf, "!<arch>", 7) == 0) {
			notice("file looks like it might be an archive which has been\n"
					" corrupted by being downloaded in ASCII mode");
		}

        return -1;
	}

    pipe(p1);
    c1 = deb_subproc_fork();
	if (!c1) {
		close(p1[0]);
		if (fd_fd_copy(arfd, p1[1], memberlen, &err) < 0)
            return -1;
		if (close(p1[1]))
            return -1;
        exit(0);
	}
	close(p1[1]);

	if (taroption) {
        pipe(p2);
		p2_out = p2[1];
	} else {
		p2_out = 1;
	}

    c2 = deb_subproc_fork();
	if (!c2) {
		if (taroption)
			close(p2[0]);
		decompress_filter(decompressor, p1[0], p2_out,
				"decompressing archive member");
        exit(0);
	}
	close(p1[0]);
	close(arfd);
	if (taroption) close(p2[1]);

	if (taroption) {
        c3 = deb_subproc_fork();
		if (!c3) {
			struct command cmd;

            deb_command_init(&cmd, TAR, "tar");
			command_add_arg(&cmd, "tar");

			if ((taroption & DPKG_TAR_LIST) && (taroption & DPKG_TAR_EXTRACT))
				command_add_arg(&cmd, "-xv");
			else if (taroption & DPKG_TAR_EXTRACT)
				command_add_arg(&cmd, "-x");
			else if (taroption & DPKG_TAR_LIST)
				command_add_arg(&cmd, "-tv");
			else
				internerr("unknown or missing tar action '%d'", taroption);

			if (taroption & DPKG_TAR_PERMS)
				command_add_arg(&cmd, "-p");
			if (taroption & DPKG_TAR_NOMTIME)
				command_add_arg(&cmd, "-m");

			command_add_arg(&cmd, "-f");
			command_add_arg(&cmd, "-");
			command_add_arg(&cmd, "--warning=no-timestamp");

            deb_dup2(p2[0],0);
			close(p2[0]);

			unsetenv("TAR_OPTIONS");

			if (dir) {
				if (chdir(dir)) {
					if (errno != ENOENT)
                        return -1;

					if (mkdir(dir, 0777))
                        return -1;
					if (chdir(dir))
                        return -1;
				}
			}

			command_exec(&cmd);
		}
		close(p2[0]);
        deb_subproc_reap(c3, "tar", 0);
	}

    deb_subproc_reap(c2, "<decompress>", SUBPROC_NOPIPE);
	if (c1 != -1)
        deb_subproc_reap(c1, "paste", 0);
	if (version.major == 0 && admininfo) {
		/* Handle the version as a float to preserve the behaviour of old code,
		 * because even if the format is defined to be padded by 0's that might
		 * not have been always true for really ancient versions... */
		while (version.minor && (version.minor % 10) == 0)
			version.minor /= 10;

		if (version.minor ==  931)
			movecontrolfiles(OLDOLDDEBDIR);
		else if (version.minor == 932 || version.minor == 933)
			movecontrolfiles(OLDDEBDIR);
	}
	return 0;
}

int deb_extract(const char *debpath, const char *dir)
{
	g_return_val_if_fail(debpath != NULL, -1);
	g_return_val_if_fail(dir != NULL, -1);

	enum dpkg_tar_options options = DPKG_TAR_EXTRACT | DPKG_TAR_PERMS;
	options |= DPKG_TAR_LIST;
	return extracthalf(debpath, dir, options, 0);
}

int deb_control(const char *debpath, const char *dir)
{
	g_return_val_if_fail(debpath != NULL, -1);
	g_return_val_if_fail(dir != NULL, -1);

	if (dir == NULL)
		dir = EXTRACTCONTROLDIR;
	return extracthalf(debpath, dir, DPKG_TAR_EXTRACT| DPKG_TAR_NOMTIME, 1);
}

int do_contents(const char *filename)
{
	return deb_extract(filename, "debian");
}

int deb_info(const char *debpath, const char *dir, const char *name, char *info)
{
	char *controlfile;
	enum fwriteflags fieldflags;
	struct varbuf str = VARBUF_INIT;
	struct pkginfo *pkg;
	struct parsedb_state *ps;
	const struct fieldinfo *field;
	const struct arbitraryfield *arbfield;

	int i;
	int fd;

    fieldflags = fw_printheader;
	controlfile = g_build_filename(dir, CONTROLFILE, NULL);
	if (! g_file_test(controlfile, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS)) {
		deb_control(debpath, dir);
	}

	fd = open(controlfile, O_RDONLY);
	if (fd == -1) {
		printf("failed to open package info file `%.255s' for reading\n", controlfile);
		return -1;
	}
	ps = parsedb_new(controlfile, fd, pdb_parse_binary | pdb_ignorefiles);
	parsedb_load(ps);
	parsedb_parse(ps, &pkg);
	parsedb_close(ps);
	g_free(controlfile);

	varbuf_reset(&str);
	field = find_field_info(fieldinfos, name);
	if (field) {
		field->wcall(&str, pkg, &pkg->available, fieldflags, field);
	} else {
		arbfield = find_arbfield_info(pkg->available.arbs, name);
		if (arbfield)
			varbuf_add_arbfield(&str, arbfield, fieldflags);
	}
	varbuf_end_str(&str);

	if (fieldflags & fw_printheader)
		printf("%s", str.buf);
	else
		printf("%s\n", str.buf);
	strcpy(info, str.buf);
	m_output(stdout, "<standard output>");
	varbuf_destroy(&str);
	return 0;
}
