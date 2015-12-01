/*  devlck.h
 *  Copyright (C) 2001 Yan Gurtovoy (ymg@dakotacom.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef HAVE_DEVLCK_H

#define HAVE_DEVLCK_H

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <paths.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>

#ifdef HAVE_LINUX_KDEV_T_H
#include <linux/kdev_t.h>
#else
/* define MAJOR() and MINOR() here */
#define MAJOR(dev)      ((dev)>>8)
#define MINOR(dev)      ((dev) & 0xff)
#endif

#ifdef HAVE_LINUX_MAJOR_H
#include <linux/major.h>
#else
/* define TTY_MAJOR and TTYAUX_MAJOR here*/
#define TTY_MAJOR 4
#define TTYAUX_MAJOR 5
#endif

#ifndef _PATH_LOCK
#define LOCK_PATH "/var/lock"
#else
#define LOCK_PATH _PATH_LOCK
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 255
#endif

#define DINVNAME "Invalid device name"
#define DACCFAIL "Failed to access device"
#define RACECOND "Unable to obtain lock. Detected race condition with process "
#define REMFAIL  "Failed to remove lock file "

typedef struct {
    struct stat strec;       /* stat structure */
    char        fsstnd[256]; /* FSSTND-1.2 lock file name */
    char        svr4[256];   /* SVr4 lock file name */
} devrec;

#endif
