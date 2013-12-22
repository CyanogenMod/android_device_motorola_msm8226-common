/*
 * ==[ FILENAME: util.c ]=======================================================
 *
 *  Project
 *
 *      Library for ethernet bridge tables.
 *
 *
 *  Description
 *
 *      Utilities
 *
 *
 *  Copyright
 *
 *      Copyright 2005 by Jens Götze
 *      All rights reserved.
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307,
 *      USA.
 *
 *
 * =============================================================================
 */


#include <stdio.h>
#include <stdarg.h>


int __eprintf (char *filename, int line, char *format, ...)
{

/* ---- VAR ---- */

    va_list                 ap;



/* ---- CODE ---- */

    /* Open argument list */

    va_start(ap, format);

    /* Print message on screen */

    fprintf(stderr, "[ERROR] [%s:%d] ", filename, line);
    vfprintf(stderr, format, ap);
    fflush(stderr);

    /* Close argument list */

    va_end(ap);

    return 0;

}


void printhex (FILE *output, const char *buf, size_t size)
{

/* ---- VAR ---- */

    int                     i;

    char                    *ptr;

    char                    c;

    size_t                  tmp;



/* ---- CODE ---- */

    while (size) {

        if (size > 1) {

            fprintf(output, "%08X-%08X ", (unsigned int)buf,
                    (unsigned int)buf + (size > 15 ? 15 : size - 1));

        } else
            fprintf(output, "%08X %8.8s ", (unsigned int)buf, "");

        ptr = (char *)buf;
        tmp = size;

        for (i = 8; i && tmp; i--, tmp--)
            fprintf(output, " %02X", (unsigned char)*ptr++);

        for (; i; i--)
            fprintf(output, "   ");

        fprintf(output, " ");

        if (tmp) {

            for (i = 8; i && tmp; i--, tmp--)
                fprintf(output, " %02X", (unsigned char)*ptr++);

        } else
            i = 8;

        for (; i; i--)
            fprintf(output, "   ");

        fprintf(output, "  ");

        for (i = 16; i && size; i--, size--) {

            c = *buf++;
            fprintf(output, "%c", c < 32 ? '.' : c);

        }

        fprintf(output, "\n");

    }

}


