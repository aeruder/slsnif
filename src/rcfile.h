/*  rcfile.h
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

#define RCFNAME  ".slsnifrc"
#define NAMELEN  11
#define VALUELEN 256
#define ON       "ON"
#define OFF      "OFF"

typedef struct _rc_struct {
    char *name;
    void (*fn)();
} rc_struct;

/* external functions */
extern char *getColor(char *name);

/* callback function prototypes */
void rc_get_bytes(tty_struct *ptr, char *value);
void rc_get_tstamp(tty_struct *ptr, char *value);
void rc_get_hex(tty_struct *ptr, char *value);
void rc_get_nolock(tty_struct *ptr, char *value);
void rc_get_color(tty_struct *ptr, char *value);
void rc_get_timecolor(tty_struct *ptr, char *value);
void rc_get_bytescolor(tty_struct *ptr, char *value);
void rc_get_pty(tty_struct *ptr, char *value);

rc_struct rc_data[] = {
/*  identificator,  function to call */
    {"TOTALBYTES",  rc_get_bytes},
    {"TIMESTAMP",   rc_get_tstamp},
    {"DISPLAYHEX",  rc_get_hex},
    {"NOLOCK",      rc_get_nolock},
    {"COLOR",       rc_get_color},
    {"TIMECOLOR",   rc_get_timecolor},
    {"BYTESCOLOR",  rc_get_bytescolor},
    {"SYSVPTY",     rc_get_pty},
    {NULL,         NULL}
};
