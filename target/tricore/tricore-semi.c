#include "qemu/osdep.h"
#include "cpu.h"
#include "disas/disas.h"
#include "exec/exec-all.h"
#include "tcg/tcg-op.h"
#include "exec/cpu_ldst.h"
#include "qemu/qemu-print.h"
#include "exec/gdbstub.h"
#include "semihosting/semihost.h"

#include "exec/helper-proto.h"
#include "exec/helper-gen.h"

#include "tricore-opcodes.h"
#include "exec/translator.h"
#include "exec/log.h"


#ifdef __VIRTUAL_IO__
/* The stuff below implements "virtual I/O" that lets TriCore programs
   running on some target hardware (or a simulator) use the host OS to
   do file I/O.  It is assumed that programs are linked against the
   "newlib" standard C library and the "libos" support library.  */


/* 1 if at least one file was opened.  */

static int tricore_vio_in_use = 0;

/* The following system calls are supported.  */

#define SYS__OPEN        0x01
#define SYS__CLOSE       0x02
#define SYS__LSEEK       0x03
#define SYS__READ        0x04
#define SYS__WRITE       0x05
#define SYS__CREAT       0x06
#define SYS__UNLINK      0x07
#define SYS__STAT        0x08
#define SYS__FSTAT       0x09
#define SYS__GETTIME     0x0a

#define NL_O_RDONLY     0x00001
#define NL_O_WRONLY     0x00002
#define NL_O_RDWR       0x00003
#define NL_O_APPEND     0x00008
#define NL_O_CREAT      0x00200
#define NL_O_TRUNC      0x00400
#define NL_O_EXCL       0x00800
#define NL_O_NDELAY     0x01000
#define NL_O_SYNC       0x02000
#define NL_O_NONBLOCK   0x04000
#define NL_O_NOCTTY     0x08000
#define NL_O_BINARY     0x10000

#define NL_S_IRWXU      000700
#define NL_S_IRUSR      000400
#define NL_S_IWUSR      000200
#define NL_S_IXUSR      000100
#define NL_S_IRWXG      000070
#define NL_S_IRGRP      000040
#define NL_S_IWGRP      000020
#define NL_S_IXGRP      000010
#define NL_S_IRWXO      000007
#define NL_S_IROTH      000004
#define NL_S_IWOTH      000002
#define NL_S_IXOTH      000001

#define NL_SEEK_SET     0
#define NL_SEEK_CUR     1
#define NL_SEEK_END     2

#define NL_EPERM        1
#define NL_ENOENT       2
#define NL_ESRCH        3
#define NL_EINTR        4
#define NL_EIO          5
#define NL_ENXIO        6
#define NL_E2BIG        7
#define NL_ENOEXEC      8
#define NL_EBADF        9
#define NL_ECHILD       10
#define NL_EAGAIN       11
#define NL_ENOMEM       12
#define NL_EACCES       13
#define NL_EFAULT       14
#define NL_ENOTBLK      15
#define NL_EBUSY        16
#define NL_EEXIST       17
#define NL_EXDEV        18
#define NL_ENODEV       19
#define NL_ENOTDIR      20
#define NL_EISDIR       21
#define NL_EINVAL       22
#define NL_ENFILE       23
#define NL_EMFILE       24
#define NL_ENOTTY       25
#define NL_ETXTBSY      26
#define NL_EFBIG        27
#define NL_ENOSPC       28
#define NL_ESPIPE       29
#define NL_EROFS        30
#define NL_EMLINK       31
#define NL_EPIPE        32
#define NL_ENOSYS       88
#define NL_ENOTEMPTY    90
#define NL_ENAMETOOLONG 91

/* Max number of files a program may have open simultaneously.  */

#define MAX_VIO_FILES   10

typedef struct _vio_file
{
	char *pathname;       /* Name of the file.  */
	int desc;             /* File descriptor (the slot number, actually).  */
	int mode;             /* Mode that was given in the open/creat call.  */
	int orig_flags;       /* Flags that were given in the open/creat call.  */
	int new_flags;        /* Flags to use when re-opening the file.  */
	off_t offset;         /* Current file offset.  */
	int is_valid;         /* 1 if descriptor is valid.  */
} vio_file;

vio_file tricore_vio_files[MAX_VIO_FILES];

/* Initialize virtual I/O.  This is called after a "target" command
   and whenever an inferior is created (usually after a "run" command).  */

static int32_t target_read_memory (CPUState *cs,uint32_t addr, char *buf, size_t len)
{
	TriCoreCPU *cpu = TRICORE_CPU(cs);
	CPUTriCoreState *env = &cpu->env;
	int32_t i;
	for (i=0; i<len; i+=1) { buf[i]=cpu_ldub_code(env, addr+i); }
	return 0;

}

static int32_t target_write_memory (CPUState *cs,uint32_t addr, char *buf, size_t len)
{

	TriCoreCPU *cpu = TRICORE_CPU(cs);
	CPUTriCoreState *env = &cpu->env;
	int32_t i;
	for (i=0; i<len; i+=1) { cpu_stb_data(env, addr+i,buf[i]); }
	return 0;
}

static int32_t target_read_string (CPUState *cs,uint32_t addr, char **buf, uint32_t maxsize, int32_t *ret)
{
	TriCoreCPU *cpu = TRICORE_CPU(cs);
	CPUTriCoreState *env = &cpu->env;	int32_t i;
	char *temp;
	*ret=0; //ret 0 is success, else fail
	*buf=(char *) malloc(maxsize);
	temp=*buf;
	if (temp==NULL) { *ret=1; return 0; }
	i=0;
	while (i<maxsize)
	{
		if (i==maxsize) {*ret=1; return maxsize; }
		temp[i]=cpu_ldub_code(env, addr+i);
		if (temp[i]==0) {*ret=0; return (i+1); }
		i+=1;
	}
	*ret=1;
	return maxsize;
}

void tricore_vio_init (void)
{
	int i;
	if (tricore_vio_in_use)
		for (i = 3; i < MAX_VIO_FILES; ++i)
			if (tricore_vio_files[i].pathname != NULL)
				free (tricore_vio_files[i].pathname);
	memset (tricore_vio_files, 0, sizeof (tricore_vio_files));
	tricore_vio_files[0].is_valid = 1;  /* stdin  */
	tricore_vio_files[1].is_valid = 1;  /* stdout  */
	tricore_vio_files[2].is_valid = 1;  /* stderr  */
	tricore_vio_in_use = 0;
}

/* Map host's errno value to newlib's equivalent.  */

static int
tricore_vio_map_errno (CPUState *cs,int host_errno)
{
	switch (host_errno)
	{
#ifdef EPERM
	case EPERM: return NL_EPERM;
#endif
#ifdef ENOENT
	case ENOENT: return NL_ENOENT;
#endif
#ifdef ESRCH
	case ESRCH: return NL_ESRCH;
#endif
#ifdef EINTR
	case EINTR: return NL_EINTR;
#endif
#ifdef EIO
	case EIO: return NL_EIO;
#endif
#ifdef ENXIO
	case ENXIO: return NL_ENXIO;
#endif
#ifdef E2BIG
	case E2BIG: return NL_E2BIG;
#endif
#ifdef ENOEXEC
	case ENOEXEC: return NL_ENOEXEC;
#endif
#ifdef EBADF
	case EBADF: return NL_EBADF;
#endif
#ifdef ECHILD
	case ECHILD: return NL_ECHILD;
#endif
#ifdef EAGAIN
	case EAGAIN: return NL_EAGAIN;
#endif
#ifdef ENOMEM
	case ENOMEM: return NL_ENOMEM;
#endif
#ifdef EACCES
	case EACCES: return NL_EACCES;
#endif
#ifdef EFAULT
	case EFAULT: return NL_EFAULT;
#endif
#ifdef ENOTBLK
	case ENOTBLK: return NL_ENOTBLK;
#endif
#ifdef EBUSY
	case EBUSY: return NL_EBUSY;
#endif
#ifdef EEXIST
	case EEXIST: return NL_EEXIST;
#endif
#ifdef EXDEV
	case EXDEV: return NL_EXDEV;
#endif
#ifdef ENODEV
	case ENODEV: return NL_ENODEV;
#endif
#ifdef ENOTDIR
	case ENOTDIR: return NL_ENOTDIR;
#endif
#ifdef EISDIR
	case EISDIR: return NL_EISDIR;
#endif
#ifdef EINVAL
	case EINVAL: return NL_EINVAL;
#endif
#ifdef ENFILE
	case ENFILE: return NL_ENFILE;
#endif
#ifdef EMFILE
	case EMFILE: return NL_EMFILE;
#endif
#ifdef ENOTTY
	case ENOTTY: return NL_ENOTTY;
#endif
#ifdef ETXTBSY
	case ETXTBSY: return NL_ETXTBSY;
#endif
#ifdef EFBIG
	case EFBIG: return NL_EFBIG;
#endif
#ifdef ENOSPC
	case ENOSPC: return NL_ENOSPC;
#endif
#ifdef ESPIPE
	case ESPIPE: return NL_ESPIPE;
#endif
#ifdef EROFS
	case EROFS: return NL_EROFS;
#endif
#ifdef EMLINK
	case EMLINK: return NL_EMLINK;
#endif
#ifdef EPIPE
	case EPIPE: return NL_EPIPE;
#endif
#ifdef ENOSYS
	case ENOSYS: return NL_ENOSYS;
#endif
#ifdef ENOTEMPTY
	case ENOTEMPTY: return NL_ENOTEMPTY;
#endif
#ifdef ENAMETOOLONG
	case ENAMETOOLONG: return NL_ENAMETOOLONG;
#endif
	default: return (host_errno);
	}
}

/* Set return and errno values;  the ___virtio function takes care
   that the target's errno variable gets updated from %d12, and
   eventually moves %d11 to the return register (%d2).  */

static void tricore_vio_set_result (CPUState *cs,int retval, int host_errno)
{
	TriCoreCPU *cpu = TRICORE_CPU(cs);
	CPUTriCoreState *env = &cpu->env;
	env->gpr_d[11]=retval;
	env->gpr_d[12]=tricore_vio_map_errno (cs,host_errno);
}

/* Perform an open (is_open_call = 1) or creat system call.  */

static void tricore_vio_open_creat (CPUState *cs,int is_open_call)
{
	TriCoreCPU *cpu = TRICORE_CPU(cs);
	CPUTriCoreState *env = &cpu->env;
	char *filename;
	uint32_t nameptr;
	int namelen;
	int rem_flags, flags, new_flags;
	int retval, tmp_errno;
	//int reterrno;
	int filenr;
	int rem_mode, mode = 0, parse_mode = 0;
	flags=0;
	uint32_t uvalue;
	int32_t value;
	for (filenr = 4; filenr < MAX_VIO_FILES; ++filenr)
		if (!tricore_vio_files[filenr].is_valid)
			break;
	if (filenr == MAX_VIO_FILES)
	{
		tricore_vio_set_result (cs,-1, EMFILE);
		return;
	}


	uvalue=env->gpr_a[4];
	nameptr = (uint32_t) uvalue;
	namelen = target_read_string (cs,nameptr, &filename, FILENAME_MAX, &retval);
	if (retval != 0)
	{
		if (namelen > 0)
			free (filename);
		tricore_vio_set_result (cs,-1, EIO);
		return;
	}

	if (!is_open_call)
	{
		flags = O_CREAT | O_WRONLY | O_TRUNC;
		new_flags = O_WRONLY;
		value=env->gpr_d[4];
		rem_mode=(int) value;
		parse_mode = 1;
	}
	else
	{
		value=env->gpr_d[4];
		rem_flags =(int) value;
		if (rem_flags & NL_O_CREAT)
		{
			parse_mode = 1;
			value=env->gpr_d[5];
			rem_mode=(int) value;
		}
		switch ((rem_flags + 1) & 0x3)
		{
		case 1: flags = O_RDONLY; break;
		case 2: flags = O_WRONLY; break;
		case 3: flags = O_RDWR; break;
		}
		if (rem_flags & NL_O_APPEND)
			flags |= O_APPEND;
		if (rem_flags & NL_O_CREAT)
			flags |= O_CREAT;
		if (rem_flags & NL_O_TRUNC)
			flags |= O_TRUNC;
		if (rem_flags & NL_O_EXCL)
			flags |= O_EXCL;
		if (rem_flags & NL_O_NDELAY)
			flags |= O_NDELAY;
		if (rem_flags & NL_O_SYNC)
			flags |= O_SYNC;
		if (rem_flags & NL_O_NONBLOCK)
			flags |= O_NONBLOCK;
		if (rem_flags & NL_O_NOCTTY)
			flags |= O_NOCTTY;
#ifdef O_BINARY
if (rem_flags & NL_O_BINARY)
	flags |= O_BINARY;
#endif
new_flags = flags & ~(O_CREAT | O_TRUNC | O_APPEND);
	}

	if (parse_mode)
	{
		if (rem_mode & NL_S_IRUSR)
			mode |= S_IRUSR;
		if (rem_mode & NL_S_IWUSR)
			mode |= S_IWUSR;
		if (rem_mode & NL_S_IXUSR)
			mode |= S_IXUSR;
		if (rem_mode & NL_S_IRGRP)
			mode |= S_IRGRP;
		if (rem_mode & NL_S_IWGRP)
			mode |= S_IWGRP;
		if (rem_mode & NL_S_IXGRP)
			mode |= S_IXGRP;
		if (rem_mode & NL_S_IROTH)
			mode |= S_IROTH;
		if (rem_mode & NL_S_IWOTH)
			mode |= S_IWOTH;
		if (rem_mode & NL_S_IXOTH)
			mode |= S_IXOTH;
	}

	if (is_open_call)
	{
		if (parse_mode)
		{
			errno=0;
			retval = open (filename, flags, mode);
			tmp_errno = errno;
		}
		else
		{
			retval = open (filename, flags);
			tmp_errno = errno;
		}
	}
	else
	{
		retval = creat (filename, mode);
		tmp_errno = errno;
	}


	if (retval >= 0)
	{
		tricore_vio_files[filenr].pathname = filename;
		tricore_vio_files[filenr].desc = filenr;
		tricore_vio_files[filenr].mode = mode;
		tricore_vio_files[filenr].orig_flags = flags;
		tricore_vio_files[filenr].new_flags = new_flags;
		tricore_vio_files[filenr].is_valid = 1;
		tricore_vio_files[filenr].offset = lseek (retval, 0, SEEK_CUR);
		close (retval);
		tricore_vio_in_use = 1;
	}
	else
	{
		filenr = -1;
		free (filename);
	}
	tricore_vio_set_result (cs,filenr, tmp_errno);
}

/* Perform a close system call.  */

static void tricore_vio_close (CPUState *cs)
{
	TriCoreCPU *cpu = TRICORE_CPU(cs);
	CPUTriCoreState *env = &cpu->env;
	int filenr;
	int32_t value;
	value=env->gpr_d[4];
	filenr=(int) value;
	if ((filenr <= 2) || (filenr >= MAX_VIO_FILES)
			|| !tricore_vio_files[filenr].is_valid)
	{
		tricore_vio_set_result (cs,-1, EACCES);  /* Well, sort of... ;-)  */
		return;
	}

	if (tricore_vio_files[filenr].pathname != NULL)
	{
		free (tricore_vio_files[filenr].pathname);
		tricore_vio_files[filenr].pathname = NULL;
	}
	tricore_vio_files[filenr].is_valid = 0;
	tricore_vio_set_result (cs,0, 0);
}

/* Perform a read system call.  */

static void tricore_vio_read (CPUState *cs)
{
	TriCoreCPU *cpu = TRICORE_CPU(cs);
	CPUTriCoreState *env = &cpu->env;
	int filenr, desc, flags;
	size_t len=0, rlen=0;
	off_t offset;
	uint32_t rem_buf;
	char *filename;
	char *buf;
	int32_t value;
	uint32_t uvalue;

	value=env->gpr_d[4];
	filenr=(int) value;
	if ((filenr < 0) || (filenr >= MAX_VIO_FILES)
			|| !tricore_vio_files[filenr].is_valid)
	{
		tricore_vio_set_result (cs,-1, EACCES);  /* Well, sort of... ;-)  */
		return;
	}

	flags = tricore_vio_files[filenr].new_flags;
	filename = tricore_vio_files[filenr].pathname;
	offset = tricore_vio_files[filenr].offset;
	uvalue=env->gpr_a[4];
	rem_buf=uvalue;
	value=env->gpr_d[5];
	len=(size_t) value;

	if ((buf = (char *) malloc (len)) == NULL)
	{
		tricore_vio_set_result (cs,-1, ENOMEM);
		return;
	}

	if (filenr <= 2)
		desc = filenr;
	else
	{
		if ((desc = open (filename, flags)) == -1)
		{
			tricore_vio_set_result (cs,-1, errno);
			free (buf);
			return;
		}
		if (lseek (desc, offset, SEEK_SET) == -1)
		{
			tricore_vio_set_result (cs,-1, errno);
			goto done;
		}
	}
	if (desc<=2)
	{
		if (desc==0) {
			if ((rlen = read (desc, buf, len)) == -1)
			{
				tricore_vio_set_result (cs,-1, errno);
				goto done;
			}
		}
		if (desc==1)  {
			if ((rlen = read (desc, buf, len)) == -1)
			{
				tricore_vio_set_result (cs,-1, errno);
				goto done;
			}
		}
		if (desc==2)  {
			if ((rlen = read (desc, buf, len)) == -1)
			{
				tricore_vio_set_result (cs,-1, errno);
				goto done;
			}
		}


		if (rlen==-1)
		{
			tricore_vio_set_result (cs,-1, errno);
			goto done;
		}
	}
	else
		if ((rlen = read (desc, buf, len)) == -1)
		{
			tricore_vio_set_result (cs,-1, errno);
			goto done;
		}

    if (target_write_memory (cs,rem_buf, buf, rlen))
	{
		tricore_vio_set_result (cs,-1, EIO);
		goto done;
	}
	tricore_vio_set_result (cs,rlen, 0);

	done:
	free (buf);
	if (filenr > 2)
	{
		tricore_vio_files[filenr].offset = lseek (desc, 0, SEEK_CUR);
		close (desc);
	}
}

/* Perform a write system call.  */

static void tricore_vio_write (CPUState *cs)
{
	TriCoreCPU *cpu = TRICORE_CPU(cs);
	CPUTriCoreState *env = &cpu->env;
	int filenr, desc, flags;
	size_t len=0, wlen=0;
	off_t offset;
	uint32_t rem_buf;
	char *filename;
	char *buf;
	int32_t value;
	uint32_t uvalue;
	value=env->gpr_d[4];
	filenr=(int) value;
	if ((filenr < 0) || (filenr >= MAX_VIO_FILES)
			|| !tricore_vio_files[filenr].is_valid)
	{
		tricore_vio_set_result (cs,-1, EACCES);  /* Well, sort of... ;-)  */
		return;
	}

	flags = tricore_vio_files[filenr].new_flags;
	filename = tricore_vio_files[filenr].pathname;
	offset = tricore_vio_files[filenr].offset;
	uvalue=env->gpr_a[4];
	rem_buf=(uint32_t) uvalue;
	value=env->gpr_d[5];
	len=(size_t) value;
	if ((buf = (char *) malloc (len + 1)) == NULL)
	{
		tricore_vio_set_result (cs,-1, ENOMEM);
		return;
	}

	if (target_read_memory (cs,rem_buf, buf, len))
	{
		tricore_vio_set_result (cs,-1, EIO);
		free (buf);
		return;
	}

	if (filenr <= 2)
		desc = filenr;
	else
	{
		if ((desc = open (filename, flags)) == -1)
		{
			tricore_vio_set_result (cs,-1, errno);
			free (buf);
			return;
		}
		if (lseek (desc, offset, SEEK_SET) == -1)
		{
			tricore_vio_set_result (cs,-1, errno);
			goto done;
		}
	}

	if (desc <= 2)
	{
		if (desc==0) {
			if ((wlen = write (desc, buf, len)) == -1)
			{
				tricore_vio_set_result (cs,-1, errno);
				goto done;
			}

		}
		if (desc==1) {
			if ((wlen = write (desc, buf, len)) == -1)
			{
				tricore_vio_set_result (cs,-1, errno);
				goto done;
			}
		}
		if (desc==2) {
			if ((wlen = write (desc, buf, len)) == -1)
			{
				tricore_vio_set_result (cs,-1, errno);
				goto done;
			}
		}
		tricore_vio_set_result (cs,len, 0);
	}
	else
	{
		if ((wlen = write (desc, buf, len)) == -1)
		{
			tricore_vio_set_result (cs,-1, errno);
			goto done;
		}
		tricore_vio_set_result (cs,wlen, 0);
	}

	done:
	free (buf);
	if (filenr > 2)
	{
		tricore_vio_files[filenr].offset = lseek (desc, 0, SEEK_CUR);
		close (desc);
	}
}

/* Perform a lseek system call.  */

static void tricore_vio_lseek (CPUState *cs)
{
	TriCoreCPU *cpu = TRICORE_CPU(cs);
	CPUTriCoreState *env = &cpu->env;
	int filenr, desc, whence, flags;
	off_t offset;
	char *filename;
	int32_t value;
	value=env->gpr_d[4];
	filenr=(int) value;
	if ((filenr <= 2) || (filenr >= MAX_VIO_FILES)
			|| !tricore_vio_files[filenr].is_valid)
	{
		tricore_vio_set_result (cs,-1, EACCES);  /* Well, sort of... ;-)  */
		return;
	}

	flags = tricore_vio_files[filenr].new_flags;
	filename = tricore_vio_files[filenr].pathname;
	if ((desc = open (filename, flags)) < 0)
	{
		tricore_vio_set_result (cs,-1, errno);
		return;
	}

	value=env->gpr_d[5];
	offset=(off_t) value;
	value=env->gpr_d[6];
	whence=(int) value;
	if (whence == NL_SEEK_CUR)
		if (lseek (desc, tricore_vio_files[filenr].offset, SEEK_CUR) == -1)
		{
			close (desc);
			tricore_vio_set_result (cs,-1, errno);
			return;
		}

	switch (whence)
	{
	case NL_SEEK_SET: whence = SEEK_SET; break;
	case NL_SEEK_CUR: whence = SEEK_CUR; break;
	case NL_SEEK_END: whence = SEEK_END; break;
	default:
		tricore_vio_set_result (cs,-1, EINVAL);
		close (desc);
		return;
	}

	tricore_vio_files[filenr].offset = lseek (desc, offset, whence);
	tricore_vio_set_result (cs,tricore_vio_files[filenr].offset, errno);
	close (desc);
}

/* Perform an unlink system call.  */

static void tricore_vio_unlink (CPUState *cs)
{
	TriCoreCPU *cpu = TRICORE_CPU(cs);
	CPUTriCoreState *env = &cpu->env;
	char *filename;
	uint32_t nameptr;
	int namelen, retval;
	uint32_t uvalue;
	uvalue=env->gpr_a[4];
	nameptr =(uint32_t) uvalue;
	namelen = target_read_string (cs,nameptr, &filename, FILENAME_MAX, &retval);
	if (retval != 0)
		tricore_vio_set_result (cs,-1, EIO);
	else
	{
		retval = unlink (filename);
		tricore_vio_set_result (cs,retval, errno);
	}

	if (namelen > 0)
		free (filename);
}

/* See if PC points to the beginning of the ___virtio function.  If not,
   return 0, otherwise register %d12 contains the number of the system
   call to be performed.  Dispatch and execute it, then return 1.  */

int do_tricore_semihosting (CPUState *cs)
{
	TriCoreCPU *cpu = TRICORE_CPU(cs);
	CPUTriCoreState *env = &cpu->env;
	int syscall;
	int32_t value;
	value=env->gpr_d[12];
	syscall=(int) value;
	switch (syscall)
	{
	case SYS__OPEN: tricore_vio_open_creat (cs,1); break;
	case SYS__CLOSE: tricore_vio_close (cs); break;
	case SYS__LSEEK: tricore_vio_lseek (cs); break;
	case SYS__READ: tricore_vio_read (cs); break;
	case SYS__WRITE: tricore_vio_write (cs); break;
	case SYS__CREAT: tricore_vio_open_creat (cs,0); break;
	case SYS__UNLINK: tricore_vio_unlink (cs); break;

	case SYS__STAT:
	case SYS__FSTAT:
	case SYS__GETTIME:
	default: tricore_vio_set_result (cs,-1, ENOSYS);
	}
	return 1;
}

#endif


