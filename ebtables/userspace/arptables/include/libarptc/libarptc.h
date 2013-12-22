#ifndef _LIBARPTC_H
#define _LIBARPTC_H
/* Library which manipulates filtering rules. */

#include <libarptc/arpt_kernel_headers.h>
#include <linux/netfilter_arp/arp_tables.h>

#ifndef ARPT_MIN_ALIGN
/* arpt_entry has pointers and u_int64_t's in it, so if you align to
   it, you'll also align to any crazy matches and targets someone
   might write */
#define ARPT_MIN_ALIGN (__alignof__(struct arpt_entry))
#endif

#define ARPT_ALIGN(s) (((s) + ((ARPT_MIN_ALIGN)-1)) & ~((ARPT_MIN_ALIGN)-1))

typedef char arpt_chainlabel[32];

#define ARPTC_LABEL_ACCEPT  "ACCEPT"
#define ARPTC_LABEL_DROP    "DROP"
#define ARPTC_LABEL_QUEUE   "QUEUE"
#define ARPTC_LABEL_RETURN  "RETURN"


/* NF_ARP_NUMHOOKS is different on 2.4 and 2.6; hack to support both */
extern int RUNTIME_NF_ARP_NUMHOOKS; /* boy, this is dirty */


/* Transparent handle type. */
typedef struct arptc_handle *arptc_handle_t;

/* Does this chain exist? */
int arptc_is_chain(const char *chain, const arptc_handle_t handle);

/* Take a snapshot of the rules.  Returns NULL on error. */
arptc_handle_t arptc_init(const char *tablename);

/* Iterator functions to run through the chains.  Returns NULL at end. */
const char *arptc_first_chain(arptc_handle_t *handle);
const char *arptc_next_chain(arptc_handle_t *handle);

/* Get first rule in the given chain: NULL for empty chain. */
const struct arpt_entry *arptc_first_rule(const char *chain,
					arptc_handle_t *handle);

/* Returns NULL when rules run out. */
const struct arpt_entry *arptc_next_rule(const struct arpt_entry *prev,
				       arptc_handle_t *handle);

/* Returns a pointer to the target name of this entry. */
const char *arptc_get_target(const struct arpt_entry *e,
			    arptc_handle_t *handle);

/* Is this a built-in chain? */
int arptc_builtin(const char *chain, const arptc_handle_t handle);

/* Get the policy of a given built-in chain */
const char *arptc_get_policy(const char *chain,
			    struct arpt_counters *counter,
			    arptc_handle_t *handle);

/* These functions return TRUE for OK or 0 and set errno.  If errno ==
   0, it means there was a version error (ie. upgrade libarptc). */
/* Rule numbers start at 1 for the first rule. */

/* Insert the entry `e' in chain `chain' into position `rulenum'. */
int arptc_insert_entry(const arpt_chainlabel chain,
		      const struct arpt_entry *e,
		      unsigned int rulenum,
		      arptc_handle_t *handle);

/* Atomically replace rule `rulenum' in `chain' with `e'. */
int arptc_replace_entry(const arpt_chainlabel chain,
		       const struct arpt_entry *e,
		       unsigned int rulenum,
		       arptc_handle_t *handle);

/* Append entry `e' to chain `chain'.  Equivalent to insert with
   rulenum = length of chain. */
int arptc_append_entry(const arpt_chainlabel chain,
		      const struct arpt_entry *e,
		      arptc_handle_t *handle);

/* Delete the first rule in `chain' which matches `e', subject to
   matchmask (array of length == origfw) */
int arptc_delete_entry(const arpt_chainlabel chain,
		      const struct arpt_entry *origfw,
		      unsigned char *matchmask,
		      arptc_handle_t *handle);

/* Delete the rule in position `rulenum' in `chain'. */
int arptc_delete_num_entry(const arpt_chainlabel chain,
			  unsigned int rulenum,
			  arptc_handle_t *handle);

/* Check the packet `e' on chain `chain'.  Returns the verdict, or
   NULL and sets errno. */
const char *arptc_check_packet(const arpt_chainlabel chain,
			      struct arpt_entry *entry,
			      arptc_handle_t *handle);

/* Flushes the entries in the given chain (ie. empties chain). */
int arptc_flush_entries(const arpt_chainlabel chain,
		       arptc_handle_t *handle);

/* Zeroes the counters in a chain. */
int arptc_zero_entries(const arpt_chainlabel chain,
		      arptc_handle_t *handle);

/* Creates a new chain. */
int arptc_create_chain(const arpt_chainlabel chain,
		      arptc_handle_t *handle);

/* Deletes a chain. */
int arptc_delete_chain(const arpt_chainlabel chain,
		      arptc_handle_t *handle);

/* Renames a chain. */
int arptc_rename_chain(const arpt_chainlabel oldname,
		      const arpt_chainlabel newname,
		      arptc_handle_t *handle);

/* Sets the policy on a built-in chain. */
int arptc_set_policy(const arpt_chainlabel chain,
		    const arpt_chainlabel policy,
		    struct arpt_counters *counters,
		    arptc_handle_t *handle);

/* Get the number of references to this chain */
int arptc_get_references(unsigned int *ref,
			const arpt_chainlabel chain,
			arptc_handle_t *handle);

/* read packet and byte counters for a specific rule */
struct arpt_counters *arptc_read_counter(const arpt_chainlabel chain,
				       unsigned int rulenum,
				       arptc_handle_t *handle);

/* zero packet and byte counters for a specific rule */
int arptc_zero_counter(const arpt_chainlabel chain,
		      unsigned int rulenum,
		      arptc_handle_t *handle);

/* set packet and byte counters for a specific rule */
int arptc_set_counter(const arpt_chainlabel chain,
		     unsigned int rulenum,
		     struct arpt_counters *counters,
		     arptc_handle_t *handle);

/* Makes the actual changes. */
int arptc_commit(arptc_handle_t *handle);

/* Get raw socket. */
int arptc_get_raw_socket();

/* Translates errno numbers into more human-readable form than strerror. */
const char *arptc_strerror(int err);



#endif /* _LIBARPTC_H */
