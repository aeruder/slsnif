/*  devlck.c
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

#include "devlck.h"

int dev_getstat(char *dev, devrec *dptr) {
/* validates device name, retreives stat structure for it,
 * and generates names for the lock files.
 * input: device name in *dev,
 *        pointer to the devrec structure in *dptr
 * returns: 0 on success, -1 on error.
 */
 
    char buffer[256];
    char *ptr;
    int  tmp = 0;

    /* check if device name looks good */
    if (!(ptr = strrchr(dev, '/')) || ptr - dev >= strlen(dev)) {
	   fprintf(stderr, "\n%s %s\n", DINVNAME, dev);
	   return -1;
    }
    /* check if device exists and get major and minor numbers */
    if (stat(dev, &(dptr->strec)) == -1) {
	   sprintf(buffer, "%s %s", DACCFAIL, dev);
	   perror(buffer);
	   return -1;
    }
    /* generate FSSTND-1.2 lock file name */
    sprintf(dptr->fsstnd, "%s/LCK..%s", LOCK_PATH, ++ptr);
    /* generate SVr4 lock file name */
    if ((dptr->strec).st_rdev >= TTYAUX_MAJOR * 256 + 64 &&
                        (dptr->strec).st_rdev <= TTYAUX_MAJOR * 256 + 127) {
	   tmp = (TTY_MAJOR - TTYAUX_MAJOR) * 256;
    }
    sprintf(dptr->svr4, "%s/LCK.%03d.%03d", LOCK_PATH,
                (int) MAJOR(tmp + (dptr->strec).st_rdev),
                (int) MINOR(tmp + (dptr->strec).st_rdev));    
    return 0;
}

pid_t dev_readpid(const char *fname) {
/* reads pid from a specified lock file
 * input: name of the file in fname
 * returns: pid on success, -1 on error
 */
    
    FILE *fd;
    int  pid, tmp;

    if (!(fd = fopen(fname, "r"))) {
        return -1;
    } else {
        /* get owner's pid from the file */
        tmp = fscanf(fd, "%d", &pid);
        /* close file */
        fclose(fd);
        return tmp == 1 ? pid : -1;
    }
}
pid_t dev_checklock(char *dev, devrec *drec) {
/* performs actual check of the locks, called
 * internally from dev_getlock() and dev_unlock
 * returns: 0 if device is not locked, -1 on error,
 *          pid of the process that owns device otherwise.
 */
    char        buffer[256];
    char        *lckname;
    pid_t       pid;
    int         j;

    for (j = 0; j < 2; j++) {
        if (!j)
            /* check for FSSTND-1.2 lock */
            lckname = drec->fsstnd;
        else
            /* check for a SVr4 style lock */
            lckname = drec->svr4;
        /* try to open the lock file */
        if ((pid = dev_readpid(lckname)) == -1) {
            /* don't bail out if file doesn't exist */
            if (errno != ENOENT) {
                sprintf(buffer, "Failed to open lock file %s", lckname);
                perror(buffer);
                return -1;
            }
        } else {
            /* check if we got a valid pid */
            if (!kill(pid, 0)) {
                return pid;
            }
            /* it's a stale lock, let's remove it */
            if (unlink(lckname) == -1) {
                sprintf(buffer, "Failed to remove stale lock %s", lckname);
                perror(buffer);
                return -1;
            }
        }
    } /* end for */
    return 0; /* no valid lock found */
}

pid_t dev_getlock(char *dev) {
/* 
 * checks if device is locked
 * input: device name in *dev, i.e. "/dev/ttyS0" 
 * returns: 0 if device is not locked, -1 on error,
 *          pid of the process that owns device otherwise.
 */

    pid_t       pid;
    devrec      *drec;
    
    /* allocate memory for drec */
    if (!(drec = malloc(sizeof(devrec)))) {
        fprintf(stderr, "\nMemory allocation failed! \n");
        return -1;
    }
    /* get drec for the device */
    if (dev_getstat(dev, drec) == -1) {
        free(drec);
        return -1;
    }
    pid = dev_checklock(dev, drec);
    free(drec);
    return pid;
}

pid_t dev_setlock(char *dev) {
/* locks specified device
 * input: device name in *dev, i.e. "/dev/ttyS0" 
 * returns: lock owner's pid on success, -1 on error
 */
 
    mode_t 	oldmask;
    devrec  *drec;
    pid_t   ourpid, pid1, pid2 = 0;
    char 	buffer[256];
    char    lckname[256];
    FILE    *fd;

    /* allocate memory for drec */
    if (!(drec = malloc(sizeof(devrec)))) {
        fprintf(stderr, "\nMemory allocation failed! \n");
        return -1;
    }	
    /* give rw permissions to lock files created */
    oldmask = umask(0);
    /* get drec for the device */
    if (dev_getstat(dev, drec) == -1) {
        free(drec);
        return -1;
    }
    /* get our own pid */
    ourpid = getpid();
    /* check if device is already locked */
    if ((pid1 = dev_checklock(dev, drec)) == -1) {
        /* something went wrong while checking the lock - bail out */
        free(drec);
        return -1;
    } else if (pid1) {
        /* it's locked already */
        if (pid1 == ourpid) {
            /* we own this device, do nothing */
            free(drec);
            return ourpid;
        } else {
            /* it's locked by someone else */
            fprintf(stderr, "Unable to obtain lock. Device %s is locked by pid %li\n",
		              dev, (long int) pid1);
            free(drec);
            return -1;
        }
    } else {
        /* create a file with our pid in the name - should be unique */
        sprintf(lckname, "%s/LCK...%d", LOCK_PATH, (int) ourpid);
        if (!(fd = fopen(lckname, "w"))) {
            sprintf(buffer, "Failed to create lock file %s", lckname);
            perror(buffer);
            free(drec);
            return -1;
        } else {
            fprintf(fd, "%d\n", (int) ourpid);
            fclose(fd);
        }
        /* create SVr4 lock */
        if (link(lckname, drec->svr4) == -1) {
            sprintf(buffer, "Failed to create lock file %s", drec->svr4);
            perror(buffer);
            free(drec);
            unlink(lckname);
            return -1;
        }
        /* create FSSTND-1.2 lock */
        if (link(lckname, drec->fsstnd) == -1) {
            sprintf(buffer, "Failed to create lock file %s", drec->fsstnd);
            perror(buffer);
            unlink(drec->svr4);
            unlink(lckname);
            free(drec);
            return -1;
        }
        /* both locks created succesfully, let's make sure nothing went wrong */
        if ((pid1 = dev_readpid(drec->fsstnd)) != ourpid ||
                        (pid2 = dev_readpid(drec->svr4)) != ourpid) {
            /* something went wrong, we don't own at least one of the locks */
            /* unlink files we own and bail out */
            unlink(lckname);
            if (pid1 == ourpid)
                unlink(drec->fsstnd);
            else if (pid2 == ourpid)
                unlink(drec->svr4);
            if (pid1 != ourpid) {
                sprintf(buffer,"%s %li", RACECOND, (long int) pid1);
                perror(buffer);
            } 
            if (pid2 != ourpid && pid2 != pid1) {
                sprintf(buffer, "%s %li", RACECOND, (long int) pid2);
                perror(buffer);
            }
            free(drec);
            return -1;
        }
        /* we own the lock */
        unlink(lckname);
        free(drec);
        return ourpid;    
    }
}

int dev_unlock(char *dev) {
/* unlocks specified device
 * input: device name in *dev, i.e. "/dev/ttyS0"
 * returns: 0 on success, -1 on error
 */

    pid_t   ourpid, pid1;
    devrec  *drec;
    char 	buffer[256];

    /* allocate memory for drec */
    if (!(drec = malloc(sizeof(devrec)))) {
        fprintf(stderr, "\nMemory allocation failed! \n");
        return -1;
    }	
    /* get drec for the device */
    if (dev_getstat(dev, drec) == -1) {
        free(drec);
        return -1;
    }
    /* get our own pid */
    ourpid = getpid();
    /* check if device is already locked */
    if ((pid1 = dev_checklock(dev, drec)) == -1) {
        /* something went wrong while checking the lock - bail out */
        free(drec);
        return -1;
    } else if (!pid1) {
        /* it's not locked, so we have nothing to do */
        free(drec);
        return 0;
    } else {
        /* it's locked */
        if (pid1 == ourpid) {
            /* we own this device, remove lock */
            if (unlink(drec->fsstnd) == -1) {
                sprintf(buffer, "%s %s", REMFAIL, drec->fsstnd);
                perror(buffer);
            }
            if (unlink(drec->svr4) == -1) {
                sprintf(buffer, "%s %s", REMFAIL, drec->svr4);
                perror(buffer);
            }                
            free(drec);
            return 0;
        } else {
            /* it's locked by someone else */
            fprintf(stderr, "Unable to remove lock. Device %s is locked by pid %li\n",
		              dev, (long int) pid1);
            free(drec);
            return -1;
        }
    }    
}
