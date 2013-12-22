/*
 * ==[ FILENAME: libebtc.h ]====================================================
 *
 *  Project
 *
 *      Library for ethernet bridge tables.
 *
 *
 *  Description
 *
 *      See project.
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


#ifndef __LIB_EBTC_H__
#define __LIB_EBTC_H__ 1


#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/netfilter_bridge/ebtables.h>


/* Macros */

#ifndef EBTC_MIN_ALIGN
#  define EBTC_MIN_ALIGN                (__alignof__(struct ebt_entry_target))
#endif
#define EBTC_ALIGN(s)                   (((s) + (EBTC_MIN_ALIGN - 1)) & \
                                         ~(EBTC_MIN_ALIGN - 1))

#define EBTC_SIZEOF(a)                  EBTC_ALIGN(sizeof(a))
#define EBTC_NEXT(a)                    ((char *)(a) + EBTC_ALIGN(sizeof(*(a))))
#define EBTC_ADDOFFSET(a, b)            ((char *)(a) + (b))

#define EBTC_INIT                       0x0000
#define EBTC_INIT_WITHFLUSH             0x0001

#define EBTC_FALSE                      -1
#define EBTC_TRUE                       0


/* Types */

typedef struct ebtc_handle_st *ebtc_handle_t;

typedef struct ebt_replace ebt_replace_t;

typedef struct ebt_entries ebt_entries_t;

typedef struct ebt_entry ebt_entry_t;

typedef struct ebt_counter ebt_counter_t;

typedef struct ebt_entry_target ebt_entry_target_t;

typedef struct ebt_standard_target ebt_standard_target_t;


/* Functions */

/*
 * ==[ FUNCTION: ebtc_is_chain ]================================================
 *
 *  Description
 *
 *      ebtc_is_chain checks chainname for exist.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      I   handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_is_chain (const char *chainname, const ebtc_handle_t handle);


/*
 * ==[ FUNCTION: ebtc_first_chain ]=============================================
 *
 *  Description
 *
 *      ebtc_first_chain returns the first chain.
 *
 *
 *  Parameter
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns NULL if it end of list, or non-zero for pointer of chainname.
 *
 *
 * =============================================================================
 */
extern const char *ebtc_first_chain (ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_next_chain ]==============================================
 *
 *  Description
 *
 *      ebtc_next_chain returns the next chain.
 *
 *
 *  Parameter
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns NULL if it end of list, or non-zero for pointer of chainname.
 *
 *
 * =============================================================================
 */
extern const char *ebtc_next_chain (ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_first_rule ]==============================================
 *
 *  Description
 *
 *      ebtc_first_rule returns the first rule in chain.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns NULL if it end of list, or non-zero for pointer of entry.
 *
 *
 * =============================================================================
 */
extern const struct ebt_entry *ebtc_first_rule (const char *chainname,
                                                ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_next_rule ]===============================================
 *
 *  Description
 *
 *      ebtc_next_rule returns the next rule in chain.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns NULL if it end of list, or non-zero for pointer of entry.
 *
 *
 * =============================================================================
 */
extern const struct ebt_entry *ebtc_next_rule (const char *chainname,
                                               ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_get_target ]==============================================
 *
 *  Description
 *
 *      ebtc_get_target returns target of an rule.
 *
 *
 *  Parameter
 *
 *      I   entry
 *              Pointer to an ebt_entry object with target.
 *
 *      I   handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns NULL if it end of list, or non-zero for pointer of entry.
 *
 *
 * =============================================================================
 */
extern const char *ebtc_get_target (const struct ebt_entry *entry,
                                    const ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_is_builtin ]==============================================
 *
 *  Description
 *
 *      ebtc_is_builtin checks a chainname for built-in chain.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      I   handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it's a built-in chain, or non-zero for a non built-in
 *      chain.
 *
 *
 * =============================================================================
 */
extern int ebtc_is_builtin (const char *chainname, const ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_set_policy ]==============================================
 *
 *  Description
 *
 *      ebtc_set_policy set policy of a chain.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      I   policy
 *              C-String with policy (ACCEPT, DROP, CONTINUE or RETURN).
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_set_policy (const char *chainname, const char *policy,
                            ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_get_policy ]==============================================
 *
 *  Description
 *
 *      ebtc_get_policy get policy of a chain.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      I   handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns a c-ctring with policy (ACCEPT, DROP, CONTINUE or RETURN).
 *
 *
 * =============================================================================
 */
extern const char *ebtc_get_policy (const char *chainname,
                                    const ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_insert_entry ]============================================
 *
 *  Description
 *
 *      ebtc_insert_entry insert a entry. You find details for the entry-object
 *      in linux/netfilter_bridge/ebtables.h.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      I   entry
 *              Pointer for a entry-object.
 *
 *      I   rulenum
 *              Position of new rule.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_insert_entry (const char *chainname,
                              const struct ebt_entry *entry,
                              unsigned int rulenum, ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_replace_entry ]===========================================
 *
 *  Description
 *
 *      ebtc_replace_entry replace a entry. You find details for the entry-object
 *      in linux/netfilter_bridge/ebtables.h.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      I   entry
 *              Pointer for the new entry-object.
 *
 *      I   rulenum
 *              Position of new rule.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_replace_entry (const char *chainname,
                               const struct ebt_entry *entry,
                               unsigned int rulenum, ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_append_entry ]============================================
 *
 *  Description
 *
 *      ebtc_append_entry append a entry. You find details for the entry-object
 *      in linux/netfilter_bridge/ebtables.h.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      I   entry
 *              Pointer for a entry-object.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_append_entry (const char *chainname,
                              const struct ebt_entry *entry,
                              ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_delete_entry ]============================================
 *
 *  Description
 *
 *      ebtc_append_entry delete a entry.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      I   rulenum
 *              Position of rule.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_delete_entry (const char *chainname, unsigned int rulenum,
                              ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_target_jumptochain ]======================================
 *
 *  Description
 *
 *      ebtc_target_jumptochain initialize standard target for jump to a chain.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_target_jumptochain (ebt_standard_target_t *target,
                                    char *chainname, ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_flush_entries ]===========================================
 *
 *  Description
 *
 *      ebtc_flush_entries flush all entries in a chain.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_flush_entries (const char *chainname, ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_zero_entries ]============================================
 *
 *  Description
 *
 *      ebtc_zero_entries set all counter in a chain to zero.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_zero_entries (const char *chainname, ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_rename_chain ]============================================
 *
 *  Description
 *
 *      ebtc_rename_chain rename a chain. A built-in chain returns an error.
 *
 *
 *  Parameter
 *
 *      I   chainname_old
 *              C-String with chainname.
 *
 *      I   chainname_new
 *              C-String with chainname.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_rename_chain (const char *chainname_old,
                              const char *chainname_new, ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_create_chain ]============================================
 *
 *  Description
 *
 *      ebtc_create_chain create a chain.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_create_chain (const char *chainname, ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_delete_chain ]============================================
 *
 *  Description
 *
 *      ebtc_delete_chain delete a chain. A built-in chain returns a error.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_delete_chain (const char *chainname, ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_read_counter ]============================================
 *
 *  Description
 *
 *      ebtc_read_counter returns the current counter for packet and bytes.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with name of chain.
 *
 *      I   rulenum
 *              Position of rule.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns NULL for an error, or non-zero for the counter.
 *
 *
 * =============================================================================
 */
extern const struct ebt_counter *ebtc_read_counter (const char *chainname, 
                                                    unsigned int rulenum,
                                                    ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_zero_counter ]============================================
 *
 *  Description
 *
 *      ebtc_zero_counter set counter of a rule to zero.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      I   rulenum
 *              Postion of rule.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_zero_counter (const char *chainname, unsigned int rulenum,
                              ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_set_counter ]=============================================
 *
 *  Description
 *
 *      ebtc_set_counter set counter to a specific value.
 *
 *
 *  Parameter
 *
 *      I   chainname
 *              C-String with chainname.
 *
 *      I   rulenum
 *              Postion of rule.
 *
 *      I   counters
 *              New counter value.
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_set_counter (const char *chainname, unsigned int rulenum,
                             const struct ebt_counter *counters,
                             ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_init ]====================================================
 *
 *  Description
 *
 *      ebtc_init create a new handle for the tablename.
 *
 *
 *  Parameter
 *
 *      I   tablename
 *              C-String with name of table.
 *
 *      I   options
 *              Options, look at macros with the prefix "EBTC_INIT".
 *
 *
 *  Return value
 *
 *      Returns NULL for an error, or non-zero for the handle.
 *
 *
 * =============================================================================
 */
extern ebtc_handle_t ebtc_init (const char *tablename, int options);


/*
 * ==[ FUNCTION: ebtc_commit ]==================================================
 *
 *  Description
 *
 *      ebtc_commit submit all changes to kernel. After the submit, handle is
 *      freed.
 *
 *
 *  Parameter
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns 0 if it succeeds, or non-zero for an error.
 *
 *
 * =============================================================================
 */
extern int ebtc_commit (ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_free ]====================================================
 *
 *  Description
 *
 *      ebtc_free free a handle.
 *
 *
 *  Parameter
 *
 *      IO  handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns nothing.
 *
 *
 * =============================================================================
 */
extern void ebtc_free (ebtc_handle_t *handle);


/*
 * ==[ FUNCTION: ebtc_strerror ]================================================
 *
 *  Description
 *
 *      ebtc_strerror translate error id to a human readable message. If no
 *      handle exist (ebtc_init or ebtc_free return an error), use NULL. With
 *      option used ebtc_strerror an global variable. hint, in a
 *      multi-threading environment can return this method a wrong message.
 *
 *
 *  Parameter
 *
 *      I   handle
 *              EBTables handle
 *
 *
 *  Return value
 *
 *      Returns NULL if message not exist, or non-zero for the message.
 *
 *
 * =============================================================================
 */
extern const char *ebtc_strerror (const ebtc_handle_t *handle);


#endif


