
/*  slsnif.c
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

#include "ascii.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "slsnif.h"

void copyright() {
    printf("\n\nSerial Line Sniffer. Version %s\n", VERSION);
    printf("\tCopyright (C) 2001 Yan Gurtovoy (ymg@dakotacom.net)\n\n");
}

void usage() {
    copyright();
    printf("Usage: slsnif [options] <port>\n\n");
    printf("REQUIRED PARAMETERS:\n");
    printf("  <port>     - serial port to use (i.e /dev/ttyS0, /dev/ttyS1, etc.)\n\n");
    printf("OPTIONS:\n");
    printf("  -h (--help)             - displays this help.\n");
    printf("  -b (--bytes)            - print number of bytes transmitted on every read.\n");
    printf("  -n (--nolock)           - don't try to lock the port.\n");
    printf("  -t (--timestamp)        - print timestamp for every transmission.\n");
    printf("  -l (--log) <logfile>    - file to store output in, defaults to stdout.\n");
    printf("  -i (--in-tee)  <file>   - write raw data from device to this file(s).\n");
    printf("  -o (--out-tee) <file>   - write raw data from host to this file(s).\n");
    printf("  -s (--speed) <speed>    - baudrate to use, defaults to 9600 baud.\n");
    printf("  -p (--port2) <port2>    - serial port to use instead of pty.\n");
    printf("  -x (--hex)              - display hexadecimal ascii values.\n");
    printf("  -u (--unix98)           - use SYSV (unix98) ptys instead of BSD ones\n");
    printf("  --color      <color>    - color to use for normal output.\n");
    printf("  --timecolor  <color>    - color to use for timestamp.\n");
    printf("  --bytescolor <color>    - color to use for number of bytes transmitted.\n\n");
    printf("Following names are valid colors:\n");
    printf(" \tblack, red, green, yellow, blue, magenta, cyan, white,\n");
    printf("\tbrightblack,brightred, brightgreen, brightyellow,\n");
    printf("\tbrightblue, brightmagenta, brightcyan, brightwhite\n\n");
    printf("Example: slsnif -l log.txt -s 2400 /dev/ttyS1\n\n");
}

void fatalError(char *msg) {
    perror(msg);
    _exit(-1);
}

int setRaw(int fd, struct termios *ttystate_orig) {
/* set tty into raw mode */

    struct termios    tty_state;

    if (tcgetattr(fd, &tty_state) < 0) return 0;
    /* save off original settings */
    *ttystate_orig = tty_state;
    /* set raw mode */
    tty_state.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO);
    tty_state.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT);
    tty_state.c_oflag &= ~OPOST;
    tty_state.c_cflag |= CS8;	
    tty_state.c_cc[VMIN]  = 1;
    tty_state.c_cc[VTIME] = 0;
    cfsetispeed(&tty_state, tty_data.baudrate);
    cfsetospeed(&tty_state, tty_data.baudrate);
    if (tcsetattr(fd, TCSAFLUSH, &tty_state) < 0) return 0;
    return 1;
}

void fmtData(unsigned char *in, char *out, int in_size) {
/* format data */

    char    charbuf[15];
    int     i;

    /* flush output buffer */
    out[0] = 0;
    for (i = 0; i < in_size; i++) {
        if (in[i] == 127) {
            /* it's a DEL character */
            sprintf(charbuf, "%s (%03i) ", DEL, 127);
        } else {
            if (in[i] < 33)
                /* it's a control character or space */
                sprintf(charbuf, "%s (%03i) ", ascii_chars[(int) in[i]], in[i]);
            else
                /* it's a printable character */
                sprintf(charbuf, "%c (%03i) ", in[i], in[i]);
        }
        /* put formatted data into output buffer */
        strcat(out, charbuf);
    }
}

void fmtDataHex(unsigned char *in, char *out, int in_size) {
/* format data */

    char    charbuf[15];
    int     i;

    /* flush output buffer */
    out[0] = 0;
    for (i = 0; i < in_size; i++) {
        if (in[i] == 127) {
            /* it's a DEL character */
            sprintf(charbuf, "%s (%02x) ", DEL, 127);
        } else {
            if (in[i] < 33)
                /* it's a control character or space */
                sprintf(charbuf, "%s (%02x) ", ascii_chars[(int) in[i]], in[i]);
            else
                /* it's a printable character */
                sprintf(charbuf, "%c (%02x) ", in[i], in[i]);
        }
        /* put formatted data into output buffer */
        strcat(out, charbuf);
    }
}

void setColor(int out, char *color) {
/* changes color on the terminal */

    if (color[0]) {
        write(out, color, 7);
    } else {
        write(out, colors[WHITE].color, 7);
    }
}

void writeData(int in, int out, int aux, int mode) {
/* reads data from `in`, formats it, writes it to `out` and `aux`.
 * mode 0 - read from pipe
 * mode 1 - read from port
 * mode 2 - read from pty
 */

    unsigned char   buffer[BUFFSIZE];
    char            outbuf[BUFFSIZE * 16 + 1];
    int             n;
#ifdef HAVE_SYS_TIMEB_H
    struct timeb    tstamp;
    char            tbuf[29];
    char            tmp[25];
    char            tmp1[4];
#else
#ifdef HAVE_TIME_H
    time_t          tstamp;
#endif
#endif
    if ((n = read(in, buffer, BUFFSIZE)) < 0) {
        if (mode)
            if (errno == EIO)
                sleep(1);
            else
                perror(mode == 1 ? RPORTFAIL : RPTYFAIL);
        else
            perror(RPIPEFAIL);
    } else {
        if (n > 0) {
            if (mode) {
                write(out, buffer, n);
                write(aux, buffer, n);
            } else {
                /* print timestamp if necessary */
                if (tty_data.tstamp) {
                    if (out == STDOUT_FILENO) setColor(out, tty_data.tclr);
                    write(out, "\n\n", 2);
#ifdef HAVE_SYS_TIMEB_H
                    ftime(&tstamp);
                    tmp[0] = tmp1[0] = tbuf[0] = 0;
                    strncat(tmp, ctime(&(tstamp.time)), 24);
                    strncat(tbuf, tmp, 19);
                    sprintf(tmp1, ".%2ui", tstamp.millitm);
                    strncat(tbuf, tmp1, 3);
                    strcat(tbuf, tmp + 19);
                    write(out, tbuf, 28);
#else
#ifdef HAVE_TIME_H
                    time(&tstamp);
                    write(out, ctime(&tstamp), 24);
#endif
#endif
                } else {
                    write(out, "\n", 1);
                }
                if (out == STDOUT_FILENO) setColor(out, tty_data.clr);
                /* print prefix */
                write(out, aux ? PORT_IN : PORT_OUT, PRFXSIZE);
                /* format data */
                if (tty_data.dsphex) {
                    fmtDataHex(buffer, outbuf, n);
                } else {
                    fmtData(buffer, outbuf, n);
                }
                if (aux && reseek) {
                    /* rotate log file */
                    lseek(tty_data.logfd, 0, SEEK_SET);
		    for (entry= tee_files[0]; entry; entry = entry->next) lseek(entry->fd, 0, SEEK_SET);
                    /* clear the flag */
                    reseek = FALSE;
                }
                /* print data */
                write(out, outbuf, strlen(outbuf));
                /* print total number of bytes if necessary */
                if (tty_data.dspbytes) {
                    buffer[0] = 0;
                    sprintf(buffer, "\n%s %i", TOTALBYTES, n);
                    if (out == STDOUT_FILENO) setColor(out, tty_data.bclr);
                    write (out, buffer, strlen(buffer));
                }
                for (entry = (aux ? tee_files[0] : tee_files[1]); entry; entry = entry->next) {
                    if (n != write(entry->fd, buffer, n)) fatalError(TEEWRTFAIL);
                }
            }
        }
    }
}

void pipeReader() {
/* get data drom pipes */

    int             maxfd;
    fd_set          read_set;

    maxfd = max(tty_data.ptypipefd[0], tty_data.portpipefd[0]);
    while (TRUE) {
        FD_ZERO(&read_set);
        FD_SET(tty_data.ptypipefd[0], &read_set);
        FD_SET(tty_data.portpipefd[0], &read_set);
        if (select(maxfd + 1, &read_set, NULL, NULL, NULL) < 0) {
	    /* don't bail out if error was caused by interrupt */
	    if (errno != EINTR) {
                perror(SELFAIL);
                return;
            } else {
	        continue;
            }
        }
        if (FD_ISSET(tty_data.ptypipefd[0], &read_set))
            writeData(tty_data.ptypipefd[0], tty_data.logfd, 0, 0);
        else
            if (FD_ISSET(tty_data.portpipefd[0], &read_set))
                writeData(tty_data.portpipefd[0], tty_data.logfd, 1, 0);
    }
}

void closeAll() {
    int i;
    FILE *fp;
    
    /* close all opened file descriptors */
    /* unlock the port(s) if necessary */
    if (!tty_data.nolock) {
        if (tty_data.portName && tty_data.portName[0])
            dev_unlock(tty_data.portName);
        /* this pointer should be NULL if pty is used */
        if (tty_data.ptyName) dev_unlock(tty_data.ptyName);
    }
    /* restore color */
    if (tty_data.logfd == STDOUT_FILENO)
	   setColor(tty_data.logfd, colors[WHITE].color);
    /* restore settings on pty */
    if (tty_data.ptyraw)
        tcsetattr(tty_data.ptyfd, TCSAFLUSH, &tty_data.ptystate_orig);
    /* close pty */
    if (tty_data.ptyfd >= 0) close(tty_data.ptyfd);
    /* restore settings on port */
    if (tty_data.portraw)
        tcsetattr(tty_data.portfd, TCSAFLUSH, &tty_data.portstate_orig);
    /* close port */
    if (tty_data.portfd >= 0) close(tty_data.portfd);
    /* close log file */
    write(tty_data.logfd, "\n", 1);
    if (tty_data.logfd != STDOUT_FILENO && tty_data.logfd >= 0)
        if ((close(tty_data.logfd)) < 0) perror(CLOSEFAIL);
    /* delete TMPPATH file if it exists */
    if ((fp = fopen(TMPPATH, "r"))) {
        fclose(fp);
        if (unlink(TMPPATH) < 0) perror (REMFAIL);
    }
    /* close write pipes */
    if (tty_data.ptypipefd[1] >= 0) close(tty_data.ptypipefd[1]);
    if (tty_data.portpipefd[1] >= 0) close(tty_data.portpipefd[1]);
    /* close tee files and free allocated memory for in/out tees */
    for (i = 0; i < 2; i++) {
        entry = tee_files[i];
        while (entry) {
            close(entry->fd);
            free(entry->name);
            tmp_entry = entry;
            entry = entry->next;
            free(tmp_entry);
        }
    }
    /* free allocated memory for portName */
    if (tty_data.portName) free(tty_data.portName);
    if (pid >= 0) kill(pid, SIGINT);
}

RETSIGTYPE sighupP(int sig) {
/*  parent signal handler for SIGHUP */
    if (pid >= 0) kill(pid, SIGHUP);
    return;
}

RETSIGTYPE sighupC(int sig) {
/*  child signal handler for SIGHUP */
    reseek = TRUE;
    return;
}

RETSIGTYPE sigintP(int sig) {
/*parent signal handler for SIGINT */
    closeAll();
    _exit(1);
}

RETSIGTYPE sigintC(int sig) {
/* child signal handler for SIGINT */
    /* close read pipes */
    if (tty_data.ptypipefd[0] >= 0) close(tty_data.ptypipefd[0]);
    if (tty_data.portpipefd[0] >= 0) close(tty_data.portpipefd[0]);
    _exit(1);
}

RETSIGTYPE sigchldP(int sig) {
/* signal handler for SIGCHLD */
    int status;

    wait(&status);
}

RETSIGTYPE sigusr1P(int sig) {
/* signal handler for SIGUSR1 */
    needsync = TRUE;
    return;
}

char *getColor(char *name) {
/* returns escape sequence that corresponds to a given color name */

    int i = 0;

    while (colors[i].name && strcmp(name, colors[i].name)) i++;
    if (colors[i].name) return colors[i].color;
    return NULL;
}

int main(int argc, char *argv[]) {

    int             i, j, maxfd, tmpfd, optret;
    char            *logName = NULL, baudstr[7], *ptr1, *ptr2;
    struct termios  tty_state;
    fd_set          rset;

#ifdef HAVE_GETOPT_LONG
    struct option longopts[] = {
        {"help",       0, NULL, 'h'},
        {"log",        1, NULL, 'l'},
        {"nolock",     0, NULL, 'n'},
        {"port2",      1, NULL, 'p'},
        {"speed",      1, NULL, 's'},
        {"bytes",      0, NULL, 'b'},
        {"timestamp",  0, NULL, 't'},
        {"hex",        0, NULL, 'x'},
        {"unix98",     0, NULL, 'u'},
        {"color",      1, NULL, 'w'},
        {"timecolor",  1, NULL, 'y'},
        {"bytescolor", 1, NULL, 'z'},
        {"in-tee",     1, NULL, 'i'},
        {"out-tee",    1, NULL, 'o'},
        {NULL,         0, NULL,   0}
    };
#endif
    /* don't lose last chunk of data when output is non-interactive */
    setvbuf(stdout,NULL,_IONBF,0);
    setvbuf(stderr,NULL,_IONBF,0);

    /* initialize variables */
    baudstr[0] = 0;
    tee_files[0] = tee_files[1] = NULL;
    tty_data.portName = tty_data.ptyName = NULL;
    tty_data.ptyfd = tty_data.portfd = tty_data.logfd = -1;
    tty_data.ptyraw = tty_data.portraw = tty_data.nolock = FALSE;
    tty_data.dspbytes = tty_data.tstamp = tty_data.dsphex = FALSE;
    tty_data.sysvpty = FALSE;
    tty_data.ptypipefd[0] = tty_data.ptypipefd[1] = -1;
    tty_data.portpipefd[0] = tty_data.portpipefd[1] = -1;
    tty_data.baudrate = DEFBAUDRATE;
    tty_data.clr[0] = tty_data.bclr[0] = tty_data.tclr[0] = 0;
    /* parse rc-file */
    readRC(&tty_data);
    /* activate signal handlers */
    signal(SIGINT, sigintP);
    signal(SIGCHLD, sigchldP);
    signal(SIGHUP, sighupP);
    signal(SIGUSR1, sigusr1P);
    /* register closeAll() function to be called on normal termination */
    atexit(closeAll);
    /* process command line arguments */
#ifdef HAVE_GETOPT_LONG
    while ((optret = getopt_long(argc, argv, OPTSTR, longopts, NULL)) != -1) {
#else
    while ((optret = getopt(argc, argv, OPTSTR)) != -1) {
#endif
        switch (optret) {
            case 'b':
                tty_data.dspbytes = TRUE;
                break;
            case 't':
                tty_data.tstamp = TRUE;
                break;
            case 'n':
                tty_data.nolock = TRUE;
                break;
            case 'l':
                logName = (optarg[0] == '=' ? optarg + 1 : optarg);
                break;
            case 's':
                i = 0;
                while (baudrates[i].spdstr &&
                        strcmp(optarg, baudrates[i].spdstr)) i++;
                if (baudrates[i].spdstr) {
                    tty_data.baudrate = baudrates[i].speed;
                    strcat(baudstr, baudrates[i].spdstr);
                }
                break;
            case 'p':
                tty_data.ptyName = (optarg[0] == '=' ? optarg + 1 : optarg);
                break;
            case 'w':
                ptr1 = getColor(optarg);
                if (ptr1) {
                    tty_data.clr[0] = 0;
                    strcat(tty_data.clr, ptr1);
                }
                break;
            case 'x':
                tty_data.dsphex = TRUE;
                break;
            case 'u':
                tty_data.sysvpty = TRUE;
                break;
            case 'y':
                ptr1 = getColor(optarg);
                if (ptr1) {
                    tty_data.tclr[0] = 0;
                    strcat(tty_data.tclr, ptr1);
                }
                break;
            case 'z':
                ptr1 = getColor(optarg);
                if (ptr1) {
                    tty_data.bclr[0] = 0;
                    strcat(tty_data.bclr, ptr1);
                }
                break;
            case 'i':
            case 'o':
                    if (!(entry = malloc(sizeof(tee_entry)))) fatalError(MEMFAIL);
                    entryp = (optret == 'i' ? &tee_files[0] : &tee_files[1]);
                    if (!(entry->name = malloc((strlen(optarg) + 1) * sizeof(char))))
                        fatalError(MEMFAIL);
                    strcpy(entry->name, optarg);
                    entry->next = *entryp;
                    *entryp = entry;
                break;
            case 'h':
            case '?':
            default :
                usage();
                tty_data.ptyName = NULL;
                return -1;
        }
    }
    if (optind < argc) {
	   if (!(tty_data.portName = malloc(PORTNAMELEN))) fatalError(MEMFAIL);
	   ptr1 = argv[optind];
	   ptr2 = tty_data.portName;
	   while((*ptr1 == '/' || isalnum(*ptr1))
                && ptr2 - tty_data.portName < PORTNAMELEN - 1) *ptr2++ = *ptr1++;
	   *ptr2 = 0;
    }
    if (!tty_data.portName || !tty_data.portName[0]) {
        usage();
        tty_data.ptyName = NULL;
        return -1;
    }
    if (tty_data.ptyName && !strncmp(tty_data.portName, tty_data.ptyName, strlen(tty_data.portName))) {
        /* first and second port point to the same device */
	errno = EINVAL;
        perror(DIFFFAIL);
        return -1;
    }

    copyright();
    /* open logfile */
    if (!logName)
        tty_data.logfd = STDOUT_FILENO;
    else {
        if((tty_data.logfd = open(logName,
                    O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) < 0) {
            perror(LOGFAIL);
            tty_data.logfd = STDOUT_FILENO;
        } else {
            printf("Started logging data into file '%s'.\n", logName);
        }
    }
    /* open tee files */
    for (i = 0; i < 2; i++) {
        entry = tee_files[i];
        while (entry) {
            if ((entry->fd = open(entry->name, O_WRONLY|O_NOCTTY|O_CREAT|O_TRUNC, 0644)) < 0) {
                perror(TEEFAIL);
            } else {
                printf("Started logging raw data from %s into file '%s'.\n",
                                            i ? "host" : "device", entry->name);
            }
            entry = entry->next;
        }
    }
    /* create pipe */
    if (pipe(tty_data.ptypipefd) < 0 || pipe(tty_data.portpipefd) < 0) {
        perror(PIPEFAIL);
        return -1;
    }
    /* fork child process */
    switch (pid = fork()) {
    case 0:
        /* child process */
        /* close write pipe */
        close(tty_data.ptypipefd[1]);
        close(tty_data.portpipefd[1]);
        signal(SIGINT, sigintC);
        signal(SIGHUP, sighupC);
        pipeReader(&tty_data);
        break;
    case -1:
        /* fork() failed */
        perror(FORKFAIL);
        return -1;
    default:
        /* parent process */
        /* close read pipe */
        close(tty_data.ptypipefd[0]);
        close(tty_data.portpipefd[0]);
        break;
    }
    if (!tty_data.ptyName) {
        if (tty_data.sysvpty) {
#if defined(HAVE_GETPT) && defined(HAVE_GRANTPT) && defined(HAVE_PTSNAME) && defined(HAVE_UNLOCKPT)
            /* use unix98 pty */
            /* temprarily disable handler for SIGCHLD */
            signal(SIGCHLD, SIG_DFL);
            /* open pty */
            if ((tty_data.ptyfd = getpt()) < 0 ||
                 grantpt(tty_data.ptyfd) < 0 ||
                 unlockpt(tty_data.ptyfd) < 0 ||
                 !(tty_data.ptyName = ptsname(tty_data.ptyfd))) {
                perror(PTYFAIL);
                return -1;
            }
            /* re-enable SIGCHLD habdler */
            signal(SIGCHLD, sigchldP);
#else
            printf("No library support for Unix98 style ptys found.\n");
            return -1;
#endif
        } else {
            /* use BSD pty */
            /* Search for a free pty */
            if (!(tty_data.ptyName = strdup(DEFPTRNAME))) fatalError(MEMFAIL);
            for (i = 0; i < 16 && tty_data.ptyfd < 0; i++) {
                tty_data.ptyName[8] = "pqrstuvwxyzPQRST"[i];
                for (j = 0; j < 16 && tty_data.ptyfd < 0; j++) {
                    tty_data.ptyName[9] = "0123456789abcdef"[j];
                    /* try to open master side */
                    tty_data.ptyfd = open(tty_data.ptyName, O_RDWR|O_NONBLOCK|O_NOCTTY);
                }
            }
            if (tty_data.ptyfd < 0) {
                /* failed to find an available pty */
                free(tty_data.ptyName);
                /*set pointer to NULL as it will be checked in closeAll() */
                tty_data.ptyName = NULL;
                perror(PTYFAIL);
                return -1;
            }
            /* create the name of the slave pty */
            tty_data.ptyName[5] = 't';
        }
        printf("Opened pty: %s\n", tty_data.ptyName);
        /* save the name of pty opened in a file */
        if ((tmpfd = open(TMPPATH,
                     O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) < 0) {
            perror(TMPFAIL);
        } else {
            write(tmpfd, tty_data.ptyName, strlen(tty_data.ptyName));
            write(tmpfd, "\n", 1);
            printf("Saved name of the pty opened into file '%s'.\n", TMPPATH);
            if (close(tmpfd) < 0) perror(CLOSEFAIL);
        }        
        if (!tty_data.sysvpty) free(tty_data.ptyName);
        /* set pointer to NULL as it will be checked in closeAll() */
        tty_data.ptyName = NULL;
    } else {
        /* open port2 instead of pty */
        /* lock port2 */
        if (!tty_data.nolock && dev_setlock(tty_data.ptyName) == -1) {
            /* couldn't lock the device */
            return -1;
        }
        /* try to open port2 */
        if ((tty_data.ptyfd = open(tty_data.ptyName, O_RDWR|O_NONBLOCK)) < 0) {
            perror(PORTFAIL);
            return -1;
        }
        printf("Opened port: %s\n", tty_data.ptyName);
    }
    /* set raw mode on pty */
    if(!setRaw(tty_data.ptyfd, &tty_data.ptystate_orig)) {
        perror(RAWFAIL);
        return -1;
    }
    tty_data.ptyraw = TRUE;
    /* lock port */
    if (!tty_data.nolock && dev_setlock(tty_data.portName) == -1) {
        /* couldn't lock the device */
        return -1;
    }
    /* try to open port */
    if ((tty_data.portfd = open(tty_data.portName, O_RDWR|O_NONBLOCK)) < 0) {
        perror(PORTFAIL);
        return -1;
    }
    printf("Opened port: %s\n", tty_data.portName);
    /* set raw mode on port */
    if (!setRaw(tty_data.portfd, &tty_data.portstate_orig)) {
        perror(RAWFAIL);
        return -1;
    }
    tty_data.portraw = TRUE;
    printf("Baudrate is set to %s baud%s.\n",
                baudstr[0] ? baudstr : "9600",
                baudstr[0] ? "" : " (default)");
    /* start listening to the slave and port */
    maxfd = max(tty_data.ptyfd, tty_data.portfd);
    while (TRUE) {
        FD_ZERO(&rset);
        FD_SET(tty_data.ptyfd, &rset);
        FD_SET(tty_data.portfd, &rset);
        if (select(maxfd + 1, &rset, NULL, NULL, NULL) < 0) {
	    if (errno != EINTR) {
                perror(SELFAIL);
                return -1;
            } else {
	        continue;
	    }
        }
        if (FD_ISSET(tty_data.portfd, &rset)) {
            /* data coming from device */
            writeData(tty_data.portfd, tty_data.ptyfd, tty_data.portpipefd[1], 1);
        } else {
            if (FD_ISSET(tty_data.ptyfd, &rset)) {
                /* data coming from host */
                if (needsync) {
                    setColor(STDOUT_FILENO, colors[WHITE].color);
                    printf("\nSynchronizing ports...");
                    if (tcgetattr(tty_data.ptyfd, &tty_state) ||
                            tcsetattr(tty_data.portfd, TCSANOW, &tty_state)) {
                        perror(SYNCFAIL);
                        return -1;
                    }
                    needsync = FALSE;
                    printf("Done!\n");
                }
                writeData(tty_data.ptyfd, tty_data.portfd, tty_data.ptypipefd[1], 2);
            }
        }
    }
    return 1;
}
