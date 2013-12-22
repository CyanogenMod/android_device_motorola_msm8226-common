/*
 * ==[ FILENAME: ebtc_test2.c ]=================================================
 *
 *  Project
 *
 *      Library for ethernet bridge tables.
 *
 *
 *  Description
 *
 *      Test for this library.
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
#include <stdlib.h>
#include <util.h>
#include <libebtc.h>


int append_entry (const char *chain, ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    unsigned int            size = 0;

    struct ebt_entry        *entry;

    struct ebt_standard_target
                            *std_target;

    struct ebt_entry_target *target;



/* ---- CODE ---- */

    /* Allocate memory */

    size += EBTC_ALIGN(sizeof(struct ebt_entry));
    size += EBTC_ALIGN(sizeof(struct ebt_standard_target));

    entry = (struct ebt_entry *)malloc(size);

    if (!entry) {

        eprintf("Can't allocate memory\n");

        return -1;

    }

    memset(entry, 0, size);

    size = EBTC_ALIGN(sizeof(*entry));
    std_target = (struct ebt_standard_target *)((char *)entry + size);

    /* Prepare entry */

    entry->bitmask = EBT_ENTRY_OR_ENTRIES | EBT_NOPROTO;

    entry->watchers_offset = EBTC_ALIGN(sizeof(*entry));
    entry->target_offset = entry->watchers_offset;
    entry->next_offset = entry->target_offset + EBTC_ALIGN(sizeof(*std_target));

    /* Prepare target */

    target = &std_target->target;
    target->target_size = EBTC_ALIGN(sizeof(*std_target));
    target->target_size -= EBTC_ALIGN(sizeof(*target));

    snprintf(target->u.name, EBT_FUNCTION_MAXNAMELEN, "standard");

    std_target->verdict = EBT_CONTINUE;

    /* Append */

    if (ebtc_append_entry(chain, entry, handle)) {

        eprintf("Can't append entry\n");

        free(entry);

        return -1;

    }

    free(entry);

    return 0;

}


void printlist (ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    const struct ebt_entry *rule;

    int                     i = 0;

    const struct ebt_counter
                            *counter;



/* ---- CODE ---- */

    rule = ebtc_first_rule("INPUT", handle);

    while (rule) {

        counter = ebtc_read_counter("INPUT", i++, handle);

        printf("  Entry jump to '%s' (bcnt = %lld; pcnt = %lld)\n",
               ebtc_get_target(rule, handle), counter->bcnt, counter->pcnt);

        rule = ebtc_next_rule("INPUT", handle);

    }

}


int test_run ()
{

/* ---- VAR ---- */

    int                     i;

    const char              *chain;

    const char              *policy;

    ebtc_handle_t           handle;



/* ---- CODE ---- */

    /* Open ebtables handle */

    handle = ebtc_init("filter", EBTC_INIT);

    if (!handle) {

        eprintf("Can't open ebtables (%s)\n", ebtc_strerror(NULL));

        return 1;

    }

    /* Delete and append entry */

    printf("Rule list in chain INPUT before delete\n");
    printlist(&handle);
    printf("\n");

    printf("Delete second entry\n");

    if (ebtc_delete_entry("INPUT", 1, &handle)) {

        eprintf("Can't delete entry\n");

        return 1;

    }

    printf("\n");

    printf("Rule list in chain INPUT after delete\n");
    printlist(&handle);
    printf("\n");

    printf("Append entry\n");

    if (append_entry("INPUT", &handle)) {

        eprintf("Can't append entry\n");

        return 1;

    }

    printf("\n");

    /* Wait for packets */

    printf("Wait 5 seconds for network traffic...\n");
    sleep(5);
    printf("\n");

    /* Submit changes to kernel */

    if (ebtc_commit(&handle)) {

        eprintf("Can't commit ebtables changes\n");

        return 1;

    }

    /* Open ebtables handle */

    handle = ebtc_init("filter", EBTC_INIT);

    if (!handle) {

        eprintf("Can't open ebtables (%s)\n", ebtc_strerror(NULL));

        return 1;

    }

    /* List current entries */

    printf("Rule list in chain INPUT after commit\n");
    printlist(&handle);
    printf("\n");

    /* Submit changes to kernel */

    ebtc_free(&handle);

    return 0;

}


int test_init ()
{

/* ---- VAR ---- */

    int                     i;

    const char              *chain;

    const char              *policy;

    ebtc_handle_t           handle;



/* ---- CODE ---- */

    handle = ebtc_init("filter", EBTC_INIT_WITHFLUSH);

    if (!handle) {

        eprintf("Can't open ebtables (%s)\n", ebtc_strerror(NULL));

        return 1;

    }

    printf("Append entry\n");

    if (append_entry("INPUT", &handle)) {

        eprintf("Can't append entry\n");

        return 1;

    }

    printf("Append entry\n");

    if (append_entry("INPUT", &handle)) {

        eprintf("Can't append entry\n");

        return 1;

    }

    printf("\n");

    if (ebtc_commit(&handle)) {

        eprintf("Can't commit ebtables changes\n");

        return 1;

    }

    return 0;
    
}


int main (int argc, char **argv)
{

/* ---- CODE ---- */

    if (argc != 2) {

        printf("Usage: progname <init|run>\n\n");

        return 1;

    }

    if (!strcmp(argv[1], "init"))
        return test_init();
    else if (!strcmp(argv[1], "run"))
        return test_run();

    return 0;

}


