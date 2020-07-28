/* 
 * errno.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Error numbers and access to error reporting.
 *
 */

#ifndef _ERRNO_H_
#define	_ERRNO_H_

#include <crtdefs.h>

/*
 * Error numbers.
 * TODO: Can't be sure of some of these assignments, I guessed from the
 * names given by strerror and the defines in the Cygnus errno.h. A lot
 * of the names from the Cygnus errno.h are not represented, and a few
 * of the descriptions returned by strerror do not obviously match
 * their error naming.
 *
 * 错误号。
 * TODO 从strerror给出的名称和天鹅座errno.h中的定义我猜得出来，不能确定其中的某些分配。 Cygnus errno.h中的许多名称未得到表示，strerror返回的一些描述显然与它们的错误命名不匹配。
 */
#define EPERM		1	/* Operation not permitted  不允许操作 */
#define	ENOFILE		2	/* No such file or directory  无此文件或目录 */
#define	ENOENT		2
#define	ESRCH		3	/* No such process  没有这样的程序 */
#define	EINTR		4	/* Interrupted function call  函数调用中断 */
#define	EIO		5	/* Input/output error  输入/输出错误 */
#define	ENXIO		6	/* No such device or address  没有这样的设备或地址 */
#define	E2BIG		7	/* Arg list too long  Arg列表过长 */
#define	ENOEXEC		8	/* Exec format error  执行格式错误 */
#define	EBADF		9	/* Bad file descriptor  错误的文件描述符 */
#define	ECHILD		10	/* No child processes  没有子进程 */
#define	EAGAIN		11	/* Resource temporarily unavailable  资源暂时不可用 */
#define	ENOMEM		12	/* Not enough space  没有足够的空位 */
#define	EACCES		13	/* Permission denied  没有权限 */
#define	EFAULT		14	/* Bad address  地址错误 */
/* 15 - Unknown Error */
#define	EBUSY		16	/* strerror reports "Resource device"  strerror报告“资源设备” */
#define	EEXIST		17	/* File exists  文件已存在 */
#define	EXDEV		18	/* Improper link (cross-device link?)  链接不正确（跨设备链接？） */
#define	ENODEV		19	/* No such device  无此设备 */
#define	ENOTDIR		20	/* Not a directory  不是目录 */
#define	EISDIR		21	/* Is a directory  是目录 */
#define	EINVAL		22	/* Invalid argument  无效的参数 */
#define	ENFILE		23	/* Too many open files in system  系统中打开的文件过多 */
#define	EMFILE		24	/* Too many open files  打开的文件太多 */
#define	ENOTTY		25	/* Inappropriate I/O control operation  I/O控制操作不当 */
/* 26 - Unknown Error */
#define	EFBIG		27	/* File too large  文件过大 */
#define	ENOSPC		28	/* No space left on device  设备上没有剩余空间 */
#define	ESPIPE		29	/* Invalid seek (seek on a pipe?)  无效搜索（在管道上搜索？） */
#define	EROFS		30	/* Read-only file system  只读文件系统 */
#define	EMLINK		31	/* Too many links  链接太多 */
#define	EPIPE		32	/* Broken pipe  断管 */
#define	EDOM		33	/* Domain error (math functions)  域错误（数学函数） */
#define	ERANGE		34	/* Result too large (possibly too small)  结果太大（可能太小） */
/* 35 - Unknown Error */
#define	EDEADLOCK	36	/* Resource deadlock avoided (non-Cyg)  避免资源死锁（非Cyg） */
#define	EDEADLK		36
#if 0
/* 37 - Unknown Error */
#define	ENAMETOOLONG	38	/* Filename too long (91 in Cyg?)  文件名太长（Cyg中为91？） */
#define	ENOLCK		39	/* No locks available (46 in Cyg?)  没有可用的锁（Cyg中为46？） */
#define	ENOSYS		40	/* Function not implemented (88 in Cyg?)  未实现功能（Cyg中为88？） */
#define	ENOTEMPTY	41	/* Directory not empty (90 in Cyg?)  目录不为空（Cyg中为90？） */
#define	EILSEQ		42	/* Illegal byte sequence  非法字节序列 */
#endif

/*
 * NOTE: ENAMETOOLONG and ENOTEMPTY conflict with definitions in the
 *       sockets.h header provided with windows32api-0.1.2.
 *       You should go and put an #if 0 ... #endif around the whole block
 *       of errors (look at the comment above them).
 *
 * 注意：ENAMETOOLONG 和 ENOTEMPTY 与 windows32 api-1.0.2 随附的 sockets.h 标头中的定义冲突。
 *      您应该将 #if 0 ... #endif 放在整个错误块中（请查看它们上方的注释）。
 */

#ifndef	RC_INVOKED

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Definitions of errno. For _doserrno, sys_nerr and * sys_errlist, see stdlib.h.
 * errno的定义。 对于_doserrno，sys_nerr和 * sys_errlist，请参见stdlib.h。
 */
#if defined(_UWIN) || defined(_WIN32_WCE)
#undef errno
extern int errno;
#else
_CRTIMP int* __cdecl _errno(void);
#define	errno		(*_errno())
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _ERRNO_H_ */