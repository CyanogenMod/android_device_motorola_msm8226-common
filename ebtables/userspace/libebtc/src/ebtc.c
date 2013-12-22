/*
 * ==[ FILENAME: ebtc.c ]=======================================================
 *
 *  Project
 *
 *      Library for ethernet bridge tables.
 *
 *
 *  Description
 *
 *      See project
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/netfilter_bridge/ebtables.h>
#include <libebtc.h>


/* Macros */

#define SUCCESS                         0
#define ERR_CHAINNOTFOUND               1
#define ERR_CHAINEXIST                  2
#define ERR_POLICYNOTFOUND              3
#define ERR_ALLOCATEMEM                 4
#define ERR_RULENUM                     5
#define ERR_BUILTINCHAIN                6
#define ERR_RAWSOCKET                   7
#define ERR_GETINFO                     8
#define ERR_GETENTRIES                  9
#define ERR_SETENTRIES                  10
#define ERR_SETCOUNTERS                 11
#define ERR_BADENTRY                    12
#define ERR_STDCHAINNOTALLOW            13
#define ERR_ENTRYTARGETINVALID          14


/* Types */

typedef struct rule_list_st rule_list_t;

typedef struct chain_list_st chain_list_t;

typedef struct chain2id_st chain2id_t;


/* Structures */

struct rule_list_st {

    rule_list_t *next;

    /* Payload */

    struct ebt_entry *entry;

    struct {

        int changed;

        int offset;

        struct ebt_counter counter;

    } counter;

};

struct chain_list_st {

    chain_list_t *next;

    int id; /* used for chain jump */

    int offset;

    /* Payload */

    int hookid;

    struct ebt_entries entries;

    struct {

        int count;

        unsigned int size;

        rule_list_t *first;

        rule_list_t *cur;

        rule_list_t *last;

    } rules;

};

struct chain2id_st {

    const char name[EBT_CHAIN_MAXNAMELEN];

    int id;

};

struct ebtc_handle_st {

    /* Communication backend to kernel */

    int fd;

    struct ebt_replace replace;

    /* Errorhandling */

    unsigned int error_no;

    /* Cache */

    struct {

        unsigned int num_counters;

    } cache;

    /* Payload */

    int changed;

    struct {

        int count;

        int last_id;

        chain_list_t *first;

        chain_list_t *cur;

        chain_list_t *last;

    } chains;

};


/* Global variables */

static chain2id_t targets[] = {
    { "ACCEPT",         EBT_ACCEPT          },
    { "DROP",           EBT_DROP            },
    { "CONTINUE",       EBT_CONTINUE        },
    { "RETURN",         EBT_RETURN          },
    { "",               0                   }
};

static chain2id_t builtinchains[] = {
    { "PREROUTING",     NF_BR_PRE_ROUTING,  },
    { "INPUT",          NF_BR_LOCAL_IN      },
    { "FORWARD",        NF_BR_FORWARD       },
    { "OUTPUT",         NF_BR_LOCAL_OUT     },
    { "POSTROUTING",    NF_BR_POST_ROUTING, },
    { "BROUTING",       NF_BR_BROUTING,     },
    { "",               0                   }
};

static struct {

    unsigned int id;

    char *msg;

} error_msg[] = {
    { SUCCESS,                  "success" },
    { ERR_CHAINNOTFOUND,        "chain not found" },
    { ERR_CHAINEXIST,           "chain already exist" },
    { ERR_POLICYNOTFOUND,       "policy not exist" },
    { ERR_ALLOCATEMEM,          "can't allocate memory" },
    { ERR_RULENUM,              "rule number is out of range" },
    { ERR_BUILTINCHAIN,         "delete denied this chain is a builtin chain" },
    { ERR_RAWSOCKET,            "can't open raw socket" },
    { ERR_GETINFO,              "can't get informations" },
    { ERR_GETENTRIES,           "can't get entries" },
    { ERR_SETENTRIES,           "can't set entries" },
    { ERR_SETCOUNTERS,          "can't set counters" },
    { ERR_BADENTRY,             "entry is not valid" },
    { ERR_STDCHAINNOTALLOW,     "standard chain is not allowed" },
    { ERR_ENTRYTARGETINVALID,   "target of entry is invalid" }
};

static struct {

    /* Errorhandling */

    unsigned int error_no;

} self_private;


static chain_list_t *find_chain (const char *chainname,
                                 const ebtc_handle_t handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain = handle->chains.first;



/* ---- CODE ---- */

    for (; chain; chain = chain->next) {

        if (!strcmp(chain->entries.name, chainname))
            return chain;

    }

    return NULL;

}


static int check_entry (const ebt_entry_t *entry)
{

/* ---- VAR ---- */

    ebt_standard_target_t   *std_target;

    ebt_entry_target_t      *target;



/* ---- CODE ---- */

    if (!(entry->bitmask & EBT_ENTRY_OR_ENTRIES))
        return -1;

    /* Check offsets */

    if (!entry->watchers_offset)
        return -1;

    if (!entry->target_offset)
        return -1;

    if (!entry->next_offset)
        return -1;

    if (entry->watchers_offset > entry->target_offset)
        return -1;

    if (entry->target_offset > entry->next_offset)
        return -1;

    return 0;

}


int ebtc_is_chain (const char *chainname, const ebtc_handle_t handle)
{

/* ---- CODE ---- */

    return find_chain(chainname, handle) ? 0 : -1;

}


const char *ebtc_first_chain (ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;



/* ---- CODE ---- */

    (*handle)->chains.cur = chain = (*handle)->chains.first;

    return chain->entries.name;

}


const char *ebtc_next_chain (ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;



/* ---- CODE ---- */

    chain = (*handle)->chains.cur;

    if (!chain)
        return NULL;

    (*handle)->chains.cur = chain = chain->next;

    return chain ? chain->entries.name : NULL;

}


const struct ebt_entry *ebtc_first_rule (const char *chainname,
                                         ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;

    rule_list_t             *rule;



/* ---- CODE ---- */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return NULL;

    }

    rule = chain->rules.cur = chain->rules.first;

    return rule ? rule->entry : NULL;

}


const struct ebt_entry *ebtc_next_rule (const char *chainname,
                                        ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;

    rule_list_t             *rule;



/* ---- CODE ---- */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return NULL;

    }

    if (!chain->rules.cur)
        return NULL;

    rule = chain->rules.cur = chain->rules.cur->next;

    (*handle)->error_no = SUCCESS;

    return rule ? rule->entry : NULL;

}


const char *ebtc_get_target (const struct ebt_entry *entry,
                             const ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    char                    *ptr;

    ebt_entry_target_t      *target;

    ebt_standard_target_t   *std_target;

    chain2id_t              *chain2id;



/* ---- CODE ---- */

    ptr = (char *)entry;
    ptr += entry->target_offset;

    target = (struct ebt_entry_target *)ptr;

    if (strcmp(target->u.name, "standard"))
        return target->u.name;

    /* Convert id to ascii */

    std_target = (ebt_standard_target_t *)target;

    for (chain2id = targets; *chain2id->name; chain2id++) {

        if (std_target->verdict == chain2id->id)
            return chain2id->name;

    }

    return NULL;

}


int ebtc_is_builtin (const char *chainname, const ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;



/* ---- CODE ---- */

    chain = find_chain(chainname, *handle);

    if (!chain)
        return -1;

    return chain->hookid == NF_BR_NUMHOOKS ? -1 : 0;

}


int ebtc_set_policy (const char *chainname, const char *policy,
                     ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    int                     i;

    int                     id = 0;

    chain_list_t            *chain = (*handle)->chains.first;



/* ---- CODE ---- */

    /* Transform policy to id */

    for (i = 0; ; i++) {

        if (!*targets[i].name) {

            (*handle)->error_no = ERR_POLICYNOTFOUND;

            return -1;

        }

        if (!strcmp(policy, targets[i].name)) {

            id = targets[i].id;
            break;

        }

    }

    /* Search chain and set policy */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return -1;

    }

    if (chain->entries.policy != id) {

        chain->entries.policy = id;
        (*handle)->changed = EBTC_TRUE;

    }

    (*handle)->error_no = SUCCESS;

    return 0;

}


const char *ebtc_get_policy (const char *chainname, const ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    int                     i;

    chain_list_t            *chain;

    chain2id_t              *chain2id;



/* ---- CODE ---- */

    /* Find chain */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return NULL;

    }

    /* Convert id to ascii */

    for (chain2id = targets; *chain2id->name; chain2id++) {

        if (chain->entries.policy == chain2id->id) {

            (*handle)->error_no = SUCCESS;

            return chain2id->name;

        }

    }

    return NULL;

}


int ebtc_insert_entry (const char *chainname, const struct ebt_entry *entry,
                       unsigned int rulenum, ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;

    rule_list_t             *rule_last = NULL;

    rule_list_t             *rule_cur;

    rule_list_t             *rule;

    unsigned int            size;



/* ---- CODE ---- */

    if (check_entry(entry)) {

        (*handle)->error_no = ERR_BADENTRY;

        return -1;

    }

    /* Search for chain in chain list */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return -1;

    }

    /* Copy entry and append to chain */

    size = sizeof(rule_list_t) + entry->next_offset;
    rule = (rule_list_t *)malloc(size);

    if (!rule) {

        (*handle)->error_no = ERR_ALLOCATEMEM;

        return -1;

    }

    memset(rule, 0, size);

    rule->entry = (ebt_entry_t *)(rule + 1);
    memcpy(rule->entry, entry, entry->next_offset);

    rule_cur = chain->rules.first;

    if (rule_cur) {

        for (; rulenum > 0 && rule_cur; rulenum--) {

            rule_last = rule_cur;
            rule_cur = rule_cur->next;

        }

        if (rulenum || !rule_cur) {

            (*handle)->error_no = ERR_RULENUM;

            return -1;

        }

        if (rule_last)
            rule_last->next = rule;
        else
            chain->rules.first = rule;

        rule->next = rule_cur;

    } else {

        if (rulenum != 0) {

            (*handle)->error_no = ERR_RULENUM;

            return -1;

        }

        chain->rules.first = chain->rules.last = rule;

    }

    chain->rules.count++;
    chain->rules.size += entry->next_offset;

    (*handle)->changed = EBTC_TRUE;
    (*handle)->error_no = SUCCESS;

    return 0;

}


int ebtc_replace_entry (const char *chainname, const struct ebt_entry *entry,
                        unsigned int rulenum, ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;

    rule_list_t             *rule_last = NULL;

    rule_list_t             *rule_old;

    rule_list_t             *rule_new;

    unsigned int            size;



/* ---- CODE ---- */

    if (check_entry(entry)) {

        (*handle)->error_no = ERR_BADENTRY;

        return -1;

    }

    /* Search for chain in chain list */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return -1;

    }

    /* Copy entry and replace in chain */

    size = sizeof(rule_list_t) + entry->next_offset;
    rule_new = (rule_list_t *)malloc(size);

    if (!rule_new) {

        (*handle)->error_no = ERR_ALLOCATEMEM;

        return -1;

    }

    memset(rule_new, 0, size);

    rule_new->entry = (ebt_entry_t *)(rule_new + 1);
    memcpy(rule_new->entry, entry, entry->next_offset);

    rule_old = chain->rules.first;

    if (!rule_old) {

        (*handle)->error_no = ERR_RULENUM;

        return -1;

    }

    for (; rulenum > 0 && rule_old; rulenum--) {

        rule_last = rule_old;
        rule_old = rule_old->next;

    }

    if (rulenum || !rule_old) {

        (*handle)->error_no = ERR_RULENUM;

        return -1;

    }

    if (rule_last)
        rule_last->next = rule_new;
    else
        chain->rules.first = rule_new;

    rule_new->next = rule_old->next;

    if (!rule_new->next)
        chain->rules.last = rule_new;

    chain->rules.size -= rule_old->entry->next_offset;
    chain->rules.size += entry->next_offset;

    (*handle)->changed = EBTC_TRUE;

    /* Clean up */

    free(rule_old);

    (*handle)->error_no = SUCCESS;

    return 0;

}


int ebtc_append_entry (const char *chainname, const struct ebt_entry *entry,
                       ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain = (*handle)->chains.first;

    rule_list_t             *rule;

    unsigned int            size;



/* ---- CODE ---- */

    if (check_entry(entry)) {

        (*handle)->error_no = ERR_BADENTRY;

        return -1;

    }

    /* Search for chain in chain list */

    while (chain) {

        if (!strcmp(chain->entries.name, chainname))
            break;

        chain = chain->next;

    }

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return -1;

    }

    /* Copy entry and insert into chain */

    size = sizeof(rule_list_t) + entry->next_offset;
    rule = (rule_list_t *)malloc(size);

    if (!rule) {

        (*handle)->error_no = ERR_ALLOCATEMEM;

        return -1;

    }

    memset(rule, 0, size);

    rule->entry = (ebt_entry_t *)(rule + 1);
    memcpy(rule->entry, entry, entry->next_offset);

    if (chain->rules.last) {

        chain->rules.last->next = rule;
        chain->rules.last = rule;

    } else
        chain->rules.first = chain->rules.last = rule;

    chain->rules.count++;
    chain->rules.size += entry->next_offset;

    (*handle)->changed = EBTC_TRUE;
    (*handle)->error_no = SUCCESS;

    return 0;

}


int ebtc_delete_entry (const char *chainname, unsigned int rulenum,
                       ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;

    rule_list_t             *rule_last = NULL;

    rule_list_t             *rule;



/* ---- CODE ---- */

    /* Find chain */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return -1;

    }

    /* Find and delete rule */

    rule = chain->rules.first;

    if (!rule) {

        (*handle)->error_no = ERR_RULENUM;

        return -1;

    }

    for (; rulenum > 0 && rule; rulenum--) {

        rule_last = rule;
        rule = rule->next;

    }

    if (rulenum || !rule) {

        (*handle)->error_no = ERR_RULENUM;

        return -1;

    }

    if (rule_last)
        rule_last->next = rule->next;
    else
        chain->rules.first = rule->next;

    if (!rule->next)
        chain->rules.last = rule_last;

    chain->rules.count--;
    chain->rules.size -= rule->entry->next_offset;

    (*handle)->changed = EBTC_TRUE;

    /* Clean up */

    free(rule);

    (*handle)->error_no = SUCCESS;

    return 0;

}


int ebtc_flush_entries (const char *chainname, ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;

    rule_list_t             *rule;

    rule_list_t             *rule_next;



/* ---- CODE ---- */

    /* Find chain */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return -1;

    }

    /* Free all rules */

    rule = chain->rules.first;

    while (rule) {

        rule_next = rule->next;

        free(rule);

        rule = rule_next;

    }

    memset(&chain->rules, 0, sizeof(chain->rules));

    (*handle)->changed = EBTC_TRUE;
    (*handle)->error_no = SUCCESS;

    return 0;

}


int ebtc_zero_entries (const char *chainname, ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;

    rule_list_t             *rule;



/* ---- CODE ---- */

    /* Find chain */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return -1;

    }

    /* Iterate set zero in rule list */

    rule = chain->rules.first;

    while (rule) {

        memset(&rule->counter.counter, 0, sizeof(ebt_counter_t));
        rule->counter.changed = EBTC_TRUE;

        rule = rule->next;

    }

    (*handle)->changed = EBTC_TRUE;
    (*handle)->error_no = SUCCESS;

    return 0;

}


int ebtc_rename_chain (const char *chainname_old, const char *chainname_new,
                       ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;



/* ---- CODE ---- */

    /* Find chain */

    chain = find_chain(chainname_old, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return -1;

    }

    if (chain->hookid != NF_BR_NUMHOOKS) { 

        (*handle)->error_no = ERR_BUILTINCHAIN;

        return -1;

    }

    /* Set new chainname */

    snprintf(chain->entries.name, EBT_CHAIN_MAXNAMELEN, "%s", chainname_new);

    (*handle)->changed = EBTC_TRUE;

    return 0;

}


int ebtc_create_chain (const char *chainname, ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;



/* ---- CODE ---- */

    /* Exist chain */

    chain = find_chain(chainname, *handle);

    if (chain) {

        (*handle)->error_no = ERR_CHAINEXIST;

        return -1;

    }

    /* Allocate new chain */

    chain = (chain_list_t *)malloc(sizeof(chain_list_t));

    if (!chain) {

        (*handle)->error_no = ERR_ALLOCATEMEM;

        return -1;

    }

    memset(chain, 0, sizeof(chain_list_t));

    chain->hookid = NF_BR_NUMHOOKS;
    chain->id = (*handle)->chains.last_id++;

    snprintf(chain->entries.name, EBT_CHAIN_MAXNAMELEN, "%s", chainname);
    chain->entries.policy = EBT_ACCEPT;

    if ((*handle)->chains.last) {

        (*handle)->chains.last->next = chain;
        (*handle)->chains.last = chain;

    } else
        (*handle)->chains.last = (*handle)->chains.first = chain;

    (*handle)->changed = EBTC_TRUE;
    (*handle)->error_no = SUCCESS;

    return 0;

}


int ebtc_delete_chain (const char *chainname, ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain = (*handle)->chains.first;

    chain_list_t            *chain_last = NULL;

    rule_list_t             *rule;

    rule_list_t             *rule_next;



/* ---- CODE ---- */

    /* Find chain */

    for (; chain; chain_last = chain, chain = chain->next) {

        if (!strcmp(chain->entries.name, chainname))
            break;

    }

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return -1;

    }

    if (chain->hookid != NF_BR_NUMHOOKS) {

        (*handle)->error_no = ERR_BUILTINCHAIN;

        return -1;

    }

    /* Free all rules */

    rule = chain->rules.first;

    while (rule) {

        rule_next = rule->next;

        free(rule);

        rule = rule_next;

    }

    /* Remove chain from list */

    if (chain_last)
        chain_last->next = chain->next;
    else
        (*handle)->chains.first = chain->next;

    if (!chain->next)
        (*handle)->chains.last = chain_last;

    (*handle)->changed = EBTC_TRUE;
    (*handle)->error_no = SUCCESS;

    /* Clean up */

    free(chain);

    return 0;

}


const struct ebt_counter *ebtc_read_counter (const char *chainname,
                                             unsigned int rulenum,
                                             ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;

    rule_list_t             *rule;



/* ---- CODE ---- */

    /* Find chain */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return NULL;

    }

    /* Find rule */

    rule = chain->rules.first;

    for (; rulenum > 0 && rule; rulenum--)
        rule = rule->next;

    if (rulenum || !rule) {

        (*handle)->error_no = ERR_RULENUM;

        return NULL;

    }

    (*handle)->error_no = SUCCESS;

    return &rule->counter.counter;

}


int ebtc_zero_counter (const char *chainname, unsigned int rulenum,
                       ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;

    rule_list_t             *rule;



/* ---- CODE ---- */

    /* Find chain */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return -1;

    }

    /* Find rule */

    rule = chain->rules.first;

    for (; rulenum > 0 && rule; rulenum--)
        rule = rule->next;

    if (rulenum || !rule) {

        (*handle)->error_no = ERR_RULENUM;

        return -1;

    }

    memset(&rule->counter.counter, 0, sizeof(ebt_counter_t));
    rule->counter.changed = EBTC_TRUE;

    (*handle)->changed = EBTC_TRUE;
    (*handle)->error_no = SUCCESS;

    return 0;

}


int ebtc_set_counter (const char *chainname, unsigned int rulenum,
                      const struct ebt_counter *counter, ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    chain_list_t            *chain;

    rule_list_t             *rule;



/* ---- CODE ---- */

    /* Find chain */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return -1;

    }

    /* Find rule */

    rule = chain->rules.first;

    for (; rulenum > 0 && rule; rulenum--)
        rule = rule->next;

    if (rulenum || !rule) {

        (*handle)->error_no = ERR_RULENUM;

        return -1;

    }

    memcpy(&rule->counter.counter, counter, sizeof(ebt_counter_t));
    rule->counter.changed = EBTC_TRUE;

    (*handle)->changed = EBTC_TRUE;
    (*handle)->error_no = SUCCESS;

    return 0;

}


int ebtc_target_jumptochain (ebt_standard_target_t *target, char *chainname,
                             ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    int                     i;

    chain_list_t            *chain;



/* ---- CODE ---- */

    /* Prepare basics of target */

    target->target.target_size = EBTC_SIZEOF(ebt_standard_target_t);
    target->target.target_size -= EBTC_SIZEOF(ebt_entry_target_t);

    snprintf(target->target.u.name, EBT_FUNCTION_MAXNAMELEN, "standard");

    /* Check for standard targets */

    for (i = 0; *targets[i].name; i++) {

        if (!strcmp(chainname, targets[i].name)) {

            target->verdict = targets[i].id;

            return 0;

        }

    }

    /* Check for standard targets */

    for (i = 0; *targets[i].name; i++) {

        if (!strcmp(chainname, targets[i].name)) {

            target->verdict = targets[i].id;

            return 0;

        }

    }

    /* Find chain */

    chain = find_chain(chainname, *handle);

    if (!chain) {

        (*handle)->error_no = ERR_CHAINNOTFOUND;

        return -1;

    }

    if (chain->hookid != NF_BR_NUMHOOKS) {

        (*handle)->error_no = ERR_STDCHAINNOTALLOW;

        return -1;

    }

    target->verdict = chain->id;

    return 0;

}


ebtc_handle_t ebtc_init (const char *tablename, int options)
{

/* ---- VAR ---- */

    int                     i;

    int                     optlen;

    int                     optname;

    int                     counter;

    unsigned int            size;

    unsigned int            entry_count;

    unsigned int            rule_size;

    ebt_replace_t           *replace;

    ebt_entries_t           *entries;

    ebt_entry_t             *entry;

    chain_list_t            *chain;

    rule_list_t             *rule;

    ebtc_handle_t           handle;



/* ---- CODE ---- */

    /* Allocate memory for handle and initialize this object */

    handle = (ebtc_handle_t)malloc(sizeof(struct ebtc_handle_st));

    if (!handle) {

        self_private.error_no = ERR_ALLOCATEMEM;

        return NULL;

    }

    memset(handle, 0, sizeof(struct ebtc_handle_st));

    handle->changed = options & EBTC_INIT_WITHFLUSH ? EBTC_TRUE : EBTC_FALSE;
    replace = &handle->replace;

    /* Open raw socket */

    handle->fd = socket(AF_INET, SOCK_RAW, PF_INET);

    if (handle->fd == -1) {

        free(handle);

        self_private.error_no = ERR_RAWSOCKET;

        return NULL;

    }

    /* Get infomations */

    memset(replace, 0, sizeof(ebt_replace_t));
    snprintf(replace->name, EBT_TABLE_MAXNAMELEN, "%s", tablename);

    optlen = sizeof(ebt_replace_t);
    optname = options & EBTC_INIT_WITHFLUSH ? EBT_SO_GET_INIT_INFO :
                                              EBT_SO_GET_INFO;

    if (getsockopt(handle->fd, IPPROTO_IP, optname, replace, &optlen)) {

        free(handle);

        self_private.error_no = ERR_GETINFO;

        return NULL;

    }

    /* Get entries */

    if (replace->nentries) {

        size = replace->nentries * sizeof(ebt_counter_t);
        replace->counters = (ebt_counter_t *)malloc(size);

        if (!replace->counters) {

            close(handle->fd);
            free(handle);

            self_private.error_no = ERR_ALLOCATEMEM;

            return NULL;

        }

    } else {

        size = 0;
        replace->counters = NULL;

    }

    handle->cache.num_counters = replace->num_counters = replace->nentries;
    replace->entries = malloc(replace->entries_size);

    if (!replace->entries) {

        if (replace->counters)
            free(replace->counters);

        close(handle->fd);
        free(handle);

        self_private.error_no = ERR_ALLOCATEMEM;

        return NULL;

    }

    optlen += replace->entries_size;
    optlen += replace->num_counters * sizeof(ebt_counter_t);
    optname = options & EBTC_INIT_WITHFLUSH ? EBT_SO_GET_INIT_ENTRIES :
                                              EBT_SO_GET_ENTRIES;

    if (getsockopt(handle->fd, IPPROTO_IP, optname, replace, &optlen)) {

        close(handle->fd);

        if (replace->counters)
            free(replace->counters);

        free(replace->entries);
        free(handle);

        self_private.error_no = ERR_GETENTRIES;

        return NULL;

    }

    /* Split one block from kernel into lists */

    size = replace->entries_size;
    entries = (ebt_entries_t *)replace->entries;

    for (; size > 0; size -= sizeof(ebt_entries_t)) {

        /* Allocate and fill a container for chain */

        chain = (chain_list_t *)malloc(sizeof(chain_list_t));

        if (!chain) {

            ebtc_free(&handle);

            self_private.error_no = ERR_ALLOCATEMEM;

            return NULL;

        }

        memset(chain, 0, sizeof(chain_list_t));
        memcpy(&chain->entries, entries, sizeof(ebt_entries_t));

        chain->hookid = NF_BR_NUMHOOKS;
        chain->id = handle->chains.last_id++;

        for (i = 0; i < NF_BR_NUMHOOKS; i++) {

            if (!strcmp(entries->name, builtinchains[i].name)) {

                chain->hookid = builtinchains[i].id;
                break;

            }

        }

        if (handle->chains.last) {

            handle->chains.last->next = chain;
            handle->chains.last = chain;

        } else
            handle->chains.last = handle->chains.first = chain;

        handle->chains.count++;

        /* Put entries into rule list of current chain */

        entry_count = entries->nentries;
        counter = entries->counter_offset;

        entries++;

        if (entry_count) {

            entry = (ebt_entry_t *)entries;
            counter = chain->entries.counter_offset;

            for (; entry_count > 0; entry_count--) {

                /* Allocate memory for rule */

                rule_size = sizeof(rule_list_t) + entry->next_offset;
                rule = (rule_list_t *)malloc(rule_size);

                if (!rule) {

                    ebtc_free(&handle);

                    self_private.error_no = ERR_ALLOCATEMEM;

                    return NULL;

                }

                /* Initialize rule */

                memset(rule, 0, sizeof(rule_list_t));

                rule->counter.changed = EBTC_FALSE;
                rule->counter.offset = counter;

                rule->entry = (ebt_entry_t *)(rule + 1);
                memcpy(rule->entry, entry, entry->next_offset);

                chain->rules.size += entry->next_offset;

                memcpy(&rule->counter.counter, &replace->counters[counter++],
                       sizeof(ebt_counter_t));

                if (chain->rules.last) {

                    chain->rules.last->next = rule;
                    chain->rules.last = rule;

                } else
                    chain->rules.last = chain->rules.first = rule;

                chain->rules.count++;

                /* Jump to next entry */

                size -= entry->next_offset;
                entry = (ebt_entry_t *)((char *)entry +
                                             entry->next_offset);

            }

            entries = (ebt_entries_t *)entry;

        }

    }

    /* Clean up */

    if (replace->counters) {

        free(replace->counters);
        replace->counters = NULL;

    }

    free(replace->entries);
    replace->entries = NULL;

    handle->error_no = SUCCESS;

    return handle;

}


static int precommit_standard_target (chain_list_t *chain, ebt_entry_t *entry)
{

/* ---- VAR ---- */

    ebt_standard_target_t   *target;



/* ---- CODE ---- */

    /* Check for standard target */

    target = (ebt_standard_target_t *)EBTC_ADDOFFSET(entry,
                                                     entry->target_offset);

    if (strncmp(target->target.u.name, "standard", EBT_FUNCTION_MAXNAMELEN))
        return 0;

    for (; chain; chain = chain->next) {

        if (chain->id == target->verdict) {

            target->verdict = chain->offset;

            return 0;

        }

    }

    return 0;

}


int ebtc_commit (ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    socklen_t               optlen;

    int                     result;

    int                     fd;

    int                     i;

    unsigned int            size;

    unsigned int            size_entry;

    unsigned int            size_entries = 0;

    unsigned int            size_counters = 0;

    unsigned int            counter_offset = 0;

    unsigned int            count_entries = 0;

    chain_list_t            *chain;

    rule_list_t             *rule;

    ebt_replace_t           *replace;

    ebt_entries_t           *entries;

    ebt_entry_t             *entry;

    ebt_counter_t           *counters;

    ebt_counter_t           *counters_back;



/* ---- CODE ---- */

    /* Check of all conditions */

    if ((*handle)->changed == EBTC_FALSE) {

        ebtc_free(handle);

        self_private.error_no = SUCCESS;

        return 0;

    }

    /* Collect all size informations for entries */

    for (chain = (*handle)->chains.first; chain; chain = chain->next) {

        chain->offset = size_entries;

        size_entries += sizeof(ebt_entries_t);
        size_entries += chain->rules.size;

        size_counters += sizeof(ebt_counter_t) * chain->rules.count;
        count_entries += chain->rules.count;

    }

    /* Allocate */

    replace = (ebt_replace_t *)malloc(sizeof(ebt_replace_t));

    if (!replace) {

        ebtc_free(handle);

        self_private.error_no = ERR_ALLOCATEMEM;

        return -1;

    }

    entries = (ebt_entries_t *)malloc(size_entries);

    if (!entries) {

        free(replace);
        ebtc_free(handle);

        self_private.error_no = ERR_ALLOCATEMEM;

        return -1;

    }

    /* Pack all entries in a big block */

    memcpy(replace, &(*handle)->replace, sizeof(ebt_replace_t));

    replace->entries_size = size_entries;
    replace->nentries = count_entries;
    replace->entries = (char *)entries;
    replace->counters = counters;
    replace->num_counters = 0;

    for (chain = (*handle)->chains.first; chain; chain = chain->next) {

        /* Copy entries */

        memcpy(entries, &chain->entries, sizeof(ebt_entries_t));

        entries->distinguisher = 0;
        entries->counter_offset = counter_offset;
        entries->nentries = chain->rules.count;

        /* Is chain a hook chain? */

        if (chain->hookid != NF_BR_NUMHOOKS)
            replace->hook_entry[chain->hookid] = entries;

        /* Copy list of entries */

        entries++;
        entry = (ebt_entry_t *)entries;

        for (rule = chain->rules.first; rule; rule = rule->next) {

            size_entry = rule->entry->next_offset;

            result = precommit_standard_target((*handle)->chains.first,
                                               rule->entry);

            if (result) {

                free(entries);
                free(replace);
                ebtc_free(handle);

                self_private.error_no = ERR_ENTRYTARGETINVALID;

                return -1;

            }

            memcpy(entry, rule->entry, size_entry);
            entry = (ebt_entry_t *)((char *)entry + size_entry);

        }

        /* Setup for next entries */

        counter_offset += chain->rules.count;
        entries = (ebt_entries_t *)entry;

    }

    entries = (ebt_entries_t *)replace->entries;

    /* Create container for counter refresh */

    if (size_counters > 0 && (*handle)->cache.num_counters > 0) {

        size = (*handle)->cache.num_counters * sizeof(ebt_counter_t);
        replace->num_counters = (*handle)->cache.num_counters;
        counters_back = replace->counters = (ebt_counter_t *)malloc(size);

        if (!counters_back) {

            free(entries);
            free(replace);
            ebtc_free(handle);

            self_private.error_no = ERR_ALLOCATEMEM;

            return -1;

        }

    } else {

        counters_back = replace->counters = NULL;
        replace->num_counters = 0;

    }

    /* Submit entries to kernel */

    fd = (*handle)->fd;

    optlen = sizeof(ebt_replace_t);
    optlen += size_entries;

    if (setsockopt(fd, IPPROTO_IP, EBT_SO_SET_ENTRIES, replace, optlen)) {

        if (counters_back)
            free(counters_back);

        free(entries);
        free(replace);
        ebtc_free(handle);

        self_private.error_no = ERR_SETENTRIES;

        return -1;

    }

    if (size_counters > 0) {

        /* Allocate memory for counters */

        replace->counters = counters = (ebt_counter_t *)malloc(size_counters);

        if (!counters) {

            if (counters_back)
                free(counters_back);

            free(replace);
            ebtc_free(handle);

            self_private.error_no = ERR_ALLOCATEMEM;

            return -1;

        }

        /* Fill counters */

        for (chain = (*handle)->chains.first; chain; chain = chain->next) {

            for (rule = chain->rules.first; rule; rule = rule->next) {

                if (rule->counter.changed == EBTC_FALSE) {

                    memcpy(counters, &counters_back[rule->counter.offset],
                           sizeof(ebt_counter_t));

                } else {

                    memcpy(counters, &rule->counter.counter,
                           sizeof(ebt_counter_t));

                }

                counters++;

            }

        }

        counters = replace->counters;

        if (counters_back)
            free(counters_back);

        /* Submit counters to kernel */

        replace->num_counters = count_entries;
        replace->entries = NULL;

        optlen = sizeof(ebt_replace_t);
        optlen += sizeof(ebt_counter_t) * count_entries;

        if (setsockopt(fd, IPPROTO_IP, EBT_SO_SET_COUNTERS, replace, optlen)) {

            free(counters);
            free(entries);
            free(replace);
            ebtc_free(handle);

            self_private.error_no = ERR_SETCOUNTERS;

            return -1;

        }

        free(counters);

    }

    /* Clean up */

    free(entries);
    free(replace);
    ebtc_free(handle);

    self_private.error_no = SUCCESS;

    return 0;

}


void ebtc_free (ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    ebt_replace_t           *replace = &(*handle)->replace;

    chain_list_t            *chain = (*handle)->chains.first;

    chain_list_t            *chain_next;

    rule_list_t             *rule;

    rule_list_t             *rule_next;



/* ---- CODE ---- */

    /* Free chain list with all rules */

    for (; chain; chain = chain_next) {

        for (rule = chain->rules.first; rule; rule = rule_next) {

            rule_next = rule->next;
            free(rule);

        }

        chain_next = chain->next;
        free(chain);

    }

    /* Close gateway into the kernel */

    close((*handle)->fd);

    /* Clean up*/

    if (replace->counters)
        free(replace->counters);

    if (replace->entries)
        free(replace->entries);

    free(*handle);

    (*handle)->error_no = SUCCESS;

    return;

}


const char *ebtc_strerror (const ebtc_handle_t *handle)
{

/* ---- VAR ---- */

    int                     i;



/* ---- CODE ---- */

    if (handle)
        i = (*handle)->error_no;
    else
        i = self_private.error_no;

    if (i > sizeof(error_msg) / sizeof(error_msg[0]))
        return NULL;

    return error_msg[i].msg;

}


