/*
 * ==[ FILENAME: ebtc_test.c ]==================================================
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


int insert_entry (const char *chain, ebtc_handle_t *handle)
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

    if (ebtc_insert_entry(chain, entry, 0, handle)) {

        eprintf("Can't insert entry\n");

        free(entry);

        return -1;

    }

    free(entry);

    return 0;

}


int replace_entry (const char *chain, ebtc_handle_t *handle)
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

    std_target->verdict = EBT_ACCEPT;

    /* Append */

    if (ebtc_replace_entry(chain, entry, 1, handle)) {

        eprintf("Can't replace entry\n");

        free(entry);

        return -1;

    }

    free(entry);

    return 0;

}


int append_entry (const char *chain, ebtc_handle_t *handle, char *chainname)
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

    if (ebtc_target_jumptochain(std_target, chainname, handle)) {

        eprintf("Can't jump to chain\n");

        return -1;

    }

    /* Append */

    if (ebtc_append_entry(chain, entry, handle)) {

        eprintf("Can't append entry\n");

        free(entry);

        return -1;

    }

    free(entry);

    return 0;

}


int main ()
{

/* ---- VAR ---- */

    int                     i;

    const char              *chain;

    const char              *policy;

    ebtc_handle_t           handle;

    const struct ebt_entry  *rule;

    struct ebt_counter      counter_new;

    const struct ebt_counter
                            *counter;



/* ---- CODE ---- */

    /* Open ebtables handle */

    handle = ebtc_init("filter", EBTC_INIT_WITHFLUSH);

    if (!handle) {

        eprintf("Can't open ebtables (%s)\n", ebtc_strerror(NULL));

        return 1;

    }

    if (ebtc_commit(&handle)) {

        eprintf("Can't commit ebtables changes\n");

        return 1;

    }

    /* Open ebtables handle */

    handle = ebtc_init("filter", EBTC_INIT_WITHFLUSH);

    if (!handle) {

        eprintf("Can't open ebtables (%s)\n", ebtc_strerror(NULL));

        return 1;

    }

    /* Entry functions */

    if (append_entry("FORWARD", &handle, "CONTINUE")) {

        eprintf("Can't append entry\n");

        ebtc_free(&handle);

        return 1;

    }

    if (append_entry("FORWARD", &handle, "CONTINUE")) {

        eprintf("Can't append entry\n");

        ebtc_free(&handle);

        return 1;

    }

    if (insert_entry("FORWARD", &handle)) {

        eprintf("Can't insert entry\n");

        ebtc_free(&handle);

        return 1;

    }

    if (replace_entry("FORWARD", &handle)) {

        eprintf("Can't replace entry\n");

        ebtc_free(&handle);

        return 1;

    }

    /* Set policies */

    policy = ebtc_get_policy("FORWARD", &handle);

    if (!policy) {

        eprintf("Can't get policy of chain\n");

        ebtc_free(&handle);

        return 1;

    }

    printf("Chain policy before change: %s\n", policy);

    if (ebtc_set_policy("FORWARD", "DROP", &handle)) {

        eprintf("Can't set policy\n");

        ebtc_free(&handle);

        return 1;

    }

    policy = ebtc_get_policy("FORWARD", &handle);

    if (!policy) {

        eprintf("Can't get policy of chain\n");

        ebtc_free(&handle);

        return 1;

    }

    printf("Chain policy after change:  %s\n", policy);
    printf("\n");

    /* Create chain */

    if (ebtc_create_chain("test_prerename", &handle)) {

        eprintf("Can't create chain\n");

        ebtc_free(&handle);

        return 1;

    }

    if (ebtc_rename_chain("test_prerename", "test", &handle)) {

        eprintf("Can't rename chain\n");

        ebtc_free(&handle);

        return 1;

    }

    /* Delete chain */

    printf("Chains before delete:\n");

    chain = ebtc_first_chain(&handle);

    while (chain) {

        printf("  Chain: '%s'\n", chain);

        chain = ebtc_next_chain(&handle);

    }

    if (ebtc_delete_chain("test", &handle)) {

        eprintf("Can't delete chain\n");

        ebtc_free(&handle);

        return 1;

    }

    printf("Chains after delete:\n");

    chain = ebtc_first_chain(&handle);

    while (chain) {

        printf("  Chain: '%s'\n", chain);

        chain = ebtc_next_chain(&handle);

    }

    printf("\n");

    /* Delete entry test */

    if (append_entry("INPUT", &handle, "RETURN")) {

        eprintf("Can't append entry\n");

        ebtc_free(&handle);

        return 1;

    }

    counter_new.bcnt = 1;
    counter_new.pcnt = 2;

    if (ebtc_set_counter("INPUT", 0, &counter_new, &handle)) {

        eprintf("Can't set counter\n");

        ebtc_free(&handle);

        return -1;

    }

    if (insert_entry("INPUT", &handle)) {

        eprintf("Can't insert entry\n");

        ebtc_free(&handle);

        return 1;

    }

    printf("Rule list in chain INPUT before delete\n");

    i = 0;
    rule = ebtc_first_rule("INPUT", &handle);

    while (rule) {

        counter = ebtc_read_counter("INPUT", i++, &handle);

        printf("  Entry jump to '%s' (bcnt = %lld; pcnt = %lld)\n",
               ebtc_get_target(rule, &handle), counter->bcnt, counter->pcnt);

        rule = ebtc_next_rule("INPUT", &handle);

    }

    if (ebtc_delete_entry("INPUT", 0, &handle)) {

        eprintf("Can't delete entry\n");

        ebtc_free(&handle);

        return 1;

    }

    if (ebtc_zero_counter("INPUT", 0, &handle)) {

        eprintf("Can't zero counter\n");

        ebtc_free(&handle);

        return 1;

    }

    printf("Rule list in chain INPUT after delete\n");

    i = 0;
    rule = ebtc_first_rule("INPUT", &handle);

    while (rule) {

        counter = ebtc_read_counter("INPUT", i++, &handle);

        printf("  Entry jump to '%s' (bcnt = %lld; pcnt = %lld)\n",
               ebtc_get_target(rule, &handle), counter->bcnt, counter->pcnt);

        rule = ebtc_next_rule("INPUT", &handle);

    }

    printf("\n");

    /* Counter test */

    if (append_entry("OUTPUT", &handle, "DROP")) {

        eprintf("Can't append entry\n");

        ebtc_free(&handle);

        return 1;

    }

    if (ebtc_zero_entries("OUTPUT", &handle)) {

        eprintf("Can't zero counter of chain\n");

        return 1;

    }

    /* Jump to chain */

    if (ebtc_create_chain("test2", &handle)) {

        eprintf("Can't create chain\n");

        ebtc_free(&handle);

        return 1;

    }

    if (append_entry("OUTPUT", &handle, "test2")) {

        eprintf("Can't append entry\n");

        ebtc_free(&handle);

        return 1;

    }

    /* Submit changes to kernel */

    if (ebtc_commit(&handle)) {

        eprintf("Can't commit ebtables changes (%s)\n", ebtc_strerror(NULL));

        return 1;

    }

    printf("Test successful finished.\n\n");

    return 0;

}


