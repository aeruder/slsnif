
/*  slsnif.h
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

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_SYS_TIMEB_H
#include <sys/timeb.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#include "common.h"

#define OPTSTR      "bnhtuxp:l:s:i:o:" /* list of short options for getopt_long() */
#define PORT_OUT    "\nHost   --> "
#define PORT_IN     "\nDevice --> "
#define DEFPTRNAME  "/dev/pty??"
#define TMPPATH     "/tmp/slsnif_pty"
#define TOTALBYTES  "Total bytes transmitted:"
#define MEMFAIL     "Memory allocation failed"
#define DIFFFAIL    "First and second ports can not be the same device"
#define PTYFAIL     "Failed to open a pty"
#define PORTFAIL    "Failed to open port"
#define LOGFAIL     "Failed to open log file, defaulting to stdout"
#define TMPFAIL     "Failed to open temp. file to save opened pty name, continuing..."
#define SELFAIL     "`Select` failed"
#define RPORTFAIL   "Error reading from port"
#define RPTYFAIL    "Error reading from pty"
#define CLOSEFAIL   "Failed to close file - data loss may occur"
#define REMFAIL     "Failed to delete file"
#define RAWFAIL     "Failed to switch into raw mode"
#define SYNCFAIL    "Failed to synchronize ttys"
#define PIPEFAIL    "Failed to create pipe"
#define FORKFAIL    "Failed to create child process"
#define RPIPEFAIL   "Error reading from pipe"
#define TEEFAIL     "Failed to create tee file"
#define TEEWRTFAIL  "Failed to write to tee file"

#define max(x,y)	((x) > (y) ? (x) : (y))

#define BUFFSIZE    32
#define PRFXSIZE    12
#define DEFBAUDRATE B9600
#define WHITE       15
#define PORTNAMELEN 128

static tty_struct   tty_data;
static pid_t        pid = -1;
static int          reseek = FALSE;
static int          needsync = TRUE;

/* baudrates */
typedef struct _spd_struct {
    speed_t speed;
    char    *spdstr;
} spd_struct;

typedef struct _tee_entry {
     struct _tee_entry *next;
     char   *name;
     int    fd;
} tee_entry;

/*
 * tee_files[0] - linked list of raw input files
 * tee_files[1] - linked list of raw output files
 * entry, entryp, tmp_entry - index variables
 */
static tee_entry *tee_files[2], *tmp_entry, *entry, **entryp;

static spd_struct baudrates[] = {
    {B50,         "50"}, {B75,         "75"}, {B110,         "110"},
    {B134,       "134"}, {B150,       "150"}, {B200,         "200"},
    {B300,       "300"}, {B600,       "600"}, {B1200,       "1200"},
    {B1800,     "1800"}, {B2400,     "2400"}, {B4800,       "4800"},
    {B9600,     "9600"}, {B19200,   "19200"}, {B38400,     "38400"},
    {B57600,   "57600"}, {B115200, "115200"}, {0,             NULL}
};

static clr_struct colors[] = {
    {"black",         "\033[0;30m"},
    {"red",           "\033[0;31m"},
    {"green",         "\033[0;32m"},
    {"yellow",        "\033[0;33m"},
    {"blue",          "\033[0;34m"},
    {"magenta",       "\033[0;35m"},
    {"cyan",          "\033[0;36m"},
    {"white",         "\033[0;37m"},
    {"brightblack",   "\033[1;30m"},
    {"brightred",     "\033[1;31m"},
    {"brightgreen",   "\033[1;32m"},
    {"brightyellow",  "\033[1;33m"},
    {"brightblue",    "\033[1;34m"},
    {"brightmagenta", "\033[1;35m"},
    {"brightcyan",    "\033[1;36m"},
    {"brightwhite",   "\033[1;37m"},
    {NULL,            NULL        }
};

/* function prototypes */
pid_t dev_getlock(char *dev);
pid_t dev_setlock(char *dev);
int dev_unlock(char *dev);
void readRC(tty_struct *ptr);
int getpt(void);
int grantpt(int fd);
char *ptsname(int fd);
int unlockpt(int fd);
