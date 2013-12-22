/* Library which manipulates firewall rules.  Version 0.1. */

/* Architecture of firewall rules is as follows:
 *
 * Chains go INPUT, FORWARD, OUTPUT then user chains.
 * Each user chain starts with an ERROR node.
 * Every chain ends with an unconditional jump: a RETURN for user chains,
 * and a POLICY for built-ins.
 */

/* (C)1999 Paul ``Rusty'' Russell - Placed under the GNU GPL (See
   COPYING for details). */

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

#ifdef DEBUG_CONNTRACK
#define inline
#endif

#if !defined(__GLIBC__) || (__GLIBC__ < 2)
typedef unsigned int socklen_t;
#endif

#include "libarptc/libarptc.h"

#define IP_VERSION	4
#define IP_OFFSET	0x1FFF

#define STRUCT_ENTRY_TARGET	struct arpt_entry_target
#define STRUCT_ENTRY		struct arpt_entry
#define STRUCT_ENTRY_MATCH	struct arpt_entry_match
#define STRUCT_GETINFO		struct arpt_getinfo
#define STRUCT_GET_ENTRIES	struct arpt_get_entries
#define STRUCT_COUNTERS		struct arpt_counters
#define STRUCT_COUNTERS_INFO	struct arpt_counters_info
#define STRUCT_STANDARD_TARGET	struct arpt_standard_target
#define STRUCT_REPLACE		struct arpt_replace

#define STRUCT_TC_HANDLE	struct arptc_handle
#define TC_HANDLE_T		arptc_handle_t

#define ENTRY_ITERATE		ARPT_ENTRY_ITERATE
#define TABLE_MAXNAMELEN	ARPT_TABLE_MAXNAMELEN
#define FUNCTION_MAXNAMELEN	ARPT_FUNCTION_MAXNAMELEN

#define GET_TARGET		arpt_get_target

#define ERROR_TARGET		ARPT_ERROR_TARGET

#define ARPT_CHAINLABEL		arpt_chainlabel

#define TC_DUMP_ENTRIES		dump_entries
#define TC_IS_CHAIN		arptc_is_chain
#define TC_FIRST_CHAIN		arptc_first_chain
#define TC_NEXT_CHAIN		arptc_next_chain
#define TC_FIRST_RULE		arptc_first_rule
#define TC_NEXT_RULE		arptc_next_rule
#define TC_GET_TARGET		arptc_get_target
#define TC_BUILTIN		arptc_builtin
#define TC_GET_POLICY		arptc_get_policy
#define TC_INSERT_ENTRY		arptc_insert_entry
#define TC_REPLACE_ENTRY	arptc_replace_entry
#define TC_APPEND_ENTRY		arptc_append_entry
#define TC_DELETE_ENTRY		arptc_delete_entry
#define TC_DELETE_NUM_ENTRY	arptc_delete_num_entry
#define TC_CHECK_PACKET		arptc_check_packet
#define TC_FLUSH_ENTRIES	arptc_flush_entries
#define TC_ZERO_ENTRIES		arptc_zero_entries
#define TC_READ_COUNTER		arptc_read_counter
#define TC_ZERO_COUNTER		arptc_zero_counter
#define TC_SET_COUNTER		arptc_set_counter
#define TC_CREATE_CHAIN		arptc_create_chain
#define TC_GET_REFERENCES	arptc_get_references
#define TC_DELETE_CHAIN		arptc_delete_chain
#define TC_RENAME_CHAIN		arptc_rename_chain
#define TC_SET_POLICY		arptc_set_policy
#define TC_GET_RAW_SOCKET	arptc_get_raw_socket
#define TC_INIT			arptc_init
#define TC_COMMIT		arptc_commit
#define TC_STRERROR		arptc_strerror

#define TC_AF			AF_INET
#define TC_IPPROTO		IPPROTO_IP

#define SO_SET_REPLACE		ARPT_SO_SET_REPLACE
#define SO_SET_ADD_COUNTERS	ARPT_SO_SET_ADD_COUNTERS
#define SO_GET_INFO		ARPT_SO_GET_INFO
#define SO_GET_ENTRIES		ARPT_SO_GET_ENTRIES
#define SO_GET_VERSION		ARPT_SO_GET_VERSION

#define STANDARD_TARGET		ARPT_STANDARD_TARGET
#define LABEL_RETURN		ARPTC_LABEL_RETURN
#define LABEL_ACCEPT		ARPTC_LABEL_ACCEPT
#define LABEL_DROP		ARPTC_LABEL_DROP
#define LABEL_QUEUE		ARPTC_LABEL_QUEUE

#define ALIGN			ARPT_ALIGN
#define RETURN			ARPT_RETURN

#include "libarptc_incl.c"

#define IP_PARTS_NATIVE(n)			\
(unsigned int)((n)>>24)&0xFF,			\
(unsigned int)((n)>>16)&0xFF,			\
(unsigned int)((n)>>8)&0xFF,			\
(unsigned int)((n)&0xFF)

#define IP_PARTS(n) IP_PARTS_NATIVE(ntohl(n))

int
dump_entry(STRUCT_ENTRY *e, const TC_HANDLE_T handle)
{
	size_t i;
	STRUCT_ENTRY_TARGET *t;

	printf("Entry %u (%lu):\n", entry2index(handle, e),
	       entry2offset(handle, e));
	printf("SRC IP: %u.%u.%u.%u/%u.%u.%u.%u\n",
	       IP_PARTS(e->arp.src.s_addr),IP_PARTS(e->arp.smsk.s_addr));
	printf("DST IP: %u.%u.%u.%u/%u.%u.%u.%u\n",
	       IP_PARTS(e->arp.tgt.s_addr),IP_PARTS(e->arp.tmsk.s_addr));
	printf("Interface: `%s'/", e->arp.iniface);
	for (i = 0; i < IFNAMSIZ; i++)
		printf("%c", e->arp.iniface_mask[i] ? 'X' : '.');
	printf("to `%s'/", e->arp.outiface);
	for (i = 0; i < IFNAMSIZ; i++)
		printf("%c", e->arp.outiface_mask[i] ? 'X' : '.');
	printf("Flags: %02X\n", e->arp.flags);
	printf("Invflags: %02X\n", e->arp.invflags);
	printf("Counters: %"PRIu64" packets, %"PRIu64" bytes\n",
	       e->counters.pcnt, e->counters.bcnt);
/*
	printf("Cache: %08X ", e->nfcache);
	if (e->nfcache & NFC_ALTERED) printf("ALTERED ");
	if (e->nfcache & NFC_UNKNOWN) printf("UNKNOWN ");
	if (e->nfcache & NFC_IP_SRC) printf("IP_SRC ");
	if (e->nfcache & NFC_IP_DST) printf("IP_DST ");
	if (e->nfcache & NFC_IP_IF_IN) printf("IP_IF_IN ");
	if (e->nfcache & NFC_IP_IF_OUT) printf("IP_IF_OUT ");
	if (e->nfcache & NFC_IP_TOS) printf("IP_TOS ");
	if (e->nfcache & NFC_IP_PROTO) printf("IP_PROTO ");
	if (e->nfcache & NFC_IP_OPTIONS) printf("IP_OPTIONS ");
	if (e->nfcache & NFC_IP_TCPFLAGS) printf("IP_TCPFLAGS ");
	if (e->nfcache & NFC_IP_SRC_PT) printf("IP_SRC_PT ");
	if (e->nfcache & NFC_IP_DST_PT) printf("IP_DST_PT ");
	if (e->nfcache & NFC_IP_PROTO_UNKNOWN) printf("IP_PROTO_UNKNOWN ");
*/
	printf("\n");

/*
	ARPT_MATCH_ITERATE(e, print_match);
*/

	t = GET_TARGET(e);
	printf("Target name: `%s' [%u]\n", t->u.user.name, t->u.target_size);
	if (strcmp(t->u.user.name, STANDARD_TARGET) == 0) {
		const unsigned char *data = t->data;
		const int pos = *(const int *)data;
		if (pos < 0)
			printf("verdict=%s\n",
			       pos == -NF_ACCEPT-1 ? "NF_ACCEPT"
			       : pos == -NF_DROP-1 ? "NF_DROP"
			       : pos == -NF_QUEUE-1 ? "NF_QUEUE"
			       : pos == RETURN ? "RETURN"
			       : "UNKNOWN");
		else
			printf("verdict=%u\n", pos);
	} else if (strcmp(t->u.user.name, ARPT_ERROR_TARGET) == 0)
		printf("error=`%s'\n", t->data);

	printf("\n");
	return 0;
}

static int
is_same(const STRUCT_ENTRY *a, const STRUCT_ENTRY *b, unsigned char *matchmask)
{
	unsigned int i;
	STRUCT_ENTRY_TARGET *ta, *tb;
	unsigned char *mptr;

	/* Always compare head structures: ignore mask here. */
	if (a->arp.src.s_addr != b->arp.src.s_addr
	    || a->arp.tgt.s_addr != b->arp.tgt.s_addr
	    || a->arp.smsk.s_addr != b->arp.smsk.s_addr
	    || a->arp.tmsk.s_addr != b->arp.tmsk.s_addr
	    || a->arp.arhln != b->arp.arhln
	    || a->arp.arhln_mask != b->arp.arhln_mask
	    || a->arp.arpop != b->arp.arpop
	    || a->arp.arpop_mask != b->arp.arpop_mask
	    || a->arp.arhrd != b->arp.arhrd
	    || a->arp.arhrd_mask != b->arp.arhrd_mask
	    || a->arp.arpro != b->arp.arpro
	    || a->arp.arpro_mask != b->arp.arpro_mask
	    || a->arp.flags != b->arp.flags
	    || a->arp.invflags != b->arp.invflags)
		return 0;

	for (i = 0; i < ARPT_DEV_ADDR_LEN_MAX; i++) {
		if (a->arp.src_devaddr.addr[i] != b->arp.src_devaddr.addr[i] ||
		    a->arp.src_devaddr.mask[i] != b->arp.src_devaddr.mask[i])
			return 0;
		if (a->arp.tgt_devaddr.addr[i] != b->arp.tgt_devaddr.addr[i] ||
		    a->arp.tgt_devaddr.mask[i] != b->arp.tgt_devaddr.mask[i])
			return 0;
	}

	for (i = 0; i < IFNAMSIZ; i++) {
		if (a->arp.iniface_mask[i] != b->arp.iniface_mask[i])
			return 0;
		if ((a->arp.iniface[i] & a->arp.iniface_mask[i])
		    != (b->arp.iniface[i] & b->arp.iniface_mask[i]))
			return 0;
		if (a->arp.outiface_mask[i] != b->arp.outiface_mask[i])
			return 0;
		if ((a->arp.outiface[i] & a->arp.outiface_mask[i])
		    != (b->arp.outiface[i] & b->arp.outiface_mask[i]))
			return 0;
	}

	if (/* a->nfcache != b->nfcache
	    || */a->target_offset != b->target_offset
	    || a->next_offset != b->next_offset)
		return 0;

	mptr = matchmask + sizeof(STRUCT_ENTRY);
/*
	if (ARPT_MATCH_ITERATE(a, match_different, a->elems, b->elems, &mptr))
		return 0;
*/

	ta = GET_TARGET((STRUCT_ENTRY *)a);
	tb = GET_TARGET((STRUCT_ENTRY *)b);
	if (ta->u.target_size != tb->u.target_size)
		return 0;
	if (strcmp(ta->u.user.name, tb->u.user.name) != 0)
		return 0;

	mptr += sizeof(*ta);
	if (target_different(ta->data, tb->data,
			     ta->u.target_size - sizeof(*ta), mptr))
		return 0;

   	return 1;
}

/***************************** DEBUGGING ********************************/
static inline int
unconditional(const struct arpt_arp *arp)
{
	unsigned int i;

	for (i = 0; i < sizeof(*arp)/sizeof(u_int32_t); i++)
		if (((u_int32_t *)arp)[i])
			return 0;

	return 1;
}

/*
static inline int
check_match(const STRUCT_ENTRY_MATCH *m, unsigned int *off)
{
	assert(m->u.match_size >= sizeof(STRUCT_ENTRY_MATCH));
	assert(ALIGN(m->u.match_size) == m->u.match_size);

	(*off) += m->u.match_size;
	return 0;
}
*/

static inline int
check_entry(const STRUCT_ENTRY *e, unsigned int *i, unsigned int *off,
	    unsigned int user_offset, int *was_return,
	    TC_HANDLE_T h)
{
	unsigned int toff;
	STRUCT_STANDARD_TARGET *t;

	assert(e->target_offset >= sizeof(STRUCT_ENTRY));
	assert(e->next_offset >= e->target_offset
	       + sizeof(STRUCT_ENTRY_TARGET));
	toff = sizeof(STRUCT_ENTRY);
/*
	ARPT_MATCH_ITERATE(e, check_match, &toff);
*/

	assert(toff == e->target_offset);

	t = (STRUCT_STANDARD_TARGET *)
		GET_TARGET((STRUCT_ENTRY *)e);
	/* next_offset will have to be multiple of entry alignment. */
	assert(e->next_offset == ALIGN(e->next_offset));
	assert(e->target_offset == ALIGN(e->target_offset));
	assert(t->target.u.target_size == ALIGN(t->target.u.target_size));
	assert(!TC_IS_CHAIN(t->target.u.user.name, h));

	if (strcmp(t->target.u.user.name, STANDARD_TARGET) == 0) {
		assert(t->target.u.target_size
		       == ALIGN(sizeof(STRUCT_STANDARD_TARGET)));

		assert(t->verdict == -NF_DROP-1
		       || t->verdict == -NF_ACCEPT-1
		       || t->verdict == RETURN
		       || t->verdict < (int)h->entries.size);

		if (t->verdict >= 0) {
			STRUCT_ENTRY *te = get_entry(h, t->verdict);
			int idx;

			idx = entry2index(h, te);
			assert(strcmp(GET_TARGET(te)->u.user.name,
				      ARPT_ERROR_TARGET)
			       != 0);
			assert(te != e);

			/* Prior node must be error node, or this node. */
			assert(t->verdict == entry2offset(h, e)+e->next_offset
			       || strcmp(GET_TARGET(index2entry(h, idx-1))
					 ->u.user.name, ARPT_ERROR_TARGET)
			       == 0);
		}

		if (t->verdict == RETURN
		    && unconditional(&e->arp)
		    && e->target_offset == sizeof(*e))
			*was_return = 1;
		else
			*was_return = 0;
	} else if (strcmp(t->target.u.user.name, ARPT_ERROR_TARGET) == 0) {
		assert(t->target.u.target_size
		       == ALIGN(sizeof(struct arpt_error_target)));

		/* If this is in user area, previous must have been return */
		if (*off > user_offset)
			assert(*was_return);

		*was_return = 0;
	}
	else *was_return = 0;

	if (*off == user_offset)
		assert(strcmp(t->target.u.user.name, ARPT_ERROR_TARGET) == 0);

	(*off) += e->next_offset;
	(*i)++;
	return 0;
}

#ifdef ARPTC_DEBUG
/* Do every conceivable sanity check on the handle */
static void
do_check(TC_HANDLE_T h, unsigned int line)
{
	unsigned int i, n;
	unsigned int user_offset; /* Offset of first user chain */
	int was_return;

	assert(h->changed == 0 || h->changed == 1);
	if (strcmp(h->info.name, "filter") == 0) {
		assert(h->info.valid_hooks
		       == (1 << NF_IP_LOCAL_IN
			   | 1 << NF_IP_FORWARD
			   | 1 << NF_IP_LOCAL_OUT));

		/* Hooks should be first three */
		assert(h->info.hook_entry[NF_IP_LOCAL_IN] == 0);

		n = get_chain_end(h, 0);
		n += get_entry(h, n)->next_offset;
		assert(h->info.hook_entry[NF_IP_FORWARD] == n);

		n = get_chain_end(h, n);
		n += get_entry(h, n)->next_offset;
		assert(h->info.hook_entry[NF_IP_LOCAL_OUT] == n);

		user_offset = h->info.hook_entry[NF_IP_LOCAL_OUT];
	} else if (strcmp(h->info.name, "nat") == 0) {
		assert((h->info.valid_hooks
		        == (1 << NF_IP_PRE_ROUTING
			    | 1 << NF_IP_POST_ROUTING
			    | 1 << NF_IP_LOCAL_OUT)) ||
		       (h->info.valid_hooks
			== (1 << NF_IP_PRE_ROUTING
			    | 1 << NF_IP_LOCAL_IN
			    | 1 << NF_IP_POST_ROUTING
			    | 1 << NF_IP_LOCAL_OUT)));

		assert(h->info.hook_entry[NF_IP_PRE_ROUTING] == 0);

		n = get_chain_end(h, 0);

		n += get_entry(h, n)->next_offset;
		assert(h->info.hook_entry[NF_IP_POST_ROUTING] == n);
		n = get_chain_end(h, n);

		n += get_entry(h, n)->next_offset;
		assert(h->info.hook_entry[NF_IP_LOCAL_OUT] == n);
		user_offset = h->info.hook_entry[NF_IP_LOCAL_OUT];

		if (h->info.valid_hooks & (1 << NF_IP_LOCAL_IN)) {
			n = get_chain_end(h, n);
			n += get_entry(h, n)->next_offset;
			assert(h->info.hook_entry[NF_IP_LOCAL_IN] == n);
			user_offset = h->info.hook_entry[NF_IP_LOCAL_IN];
		}

	} else if (strcmp(h->info.name, "mangle") == 0) {
		/* This code is getting ugly because linux < 2.4.18-pre6 had
		 * two mangle hooks, linux >= 2.4.18-pre6 has five mangle hooks
		 * */
		assert((h->info.valid_hooks
			== (1 << NF_IP_PRE_ROUTING
			    | 1 << NF_IP_LOCAL_OUT)) || 
		       (h->info.valid_hooks
			== (1 << NF_IP_PRE_ROUTING
			    | 1 << NF_IP_LOCAL_IN
			    | 1 << NF_IP_FORWARD
			    | 1 << NF_IP_LOCAL_OUT
			    | 1 << NF_IP_POST_ROUTING)));

		/* Hooks should be first five */
		assert(h->info.hook_entry[NF_IP_PRE_ROUTING] == 0);

		n = get_chain_end(h, 0);

		if (h->info.valid_hooks & (1 << NF_IP_LOCAL_IN)) {
			n += get_entry(h, n)->next_offset;
			assert(h->info.hook_entry[NF_IP_LOCAL_IN] == n);
			n = get_chain_end(h, n);
		}

		if (h->info.valid_hooks & (1 << NF_IP_FORWARD)) {
			n += get_entry(h, n)->next_offset;
			assert(h->info.hook_entry[NF_IP_FORWARD] == n);
			n = get_chain_end(h, n);
		}

		n += get_entry(h, n)->next_offset;
		assert(h->info.hook_entry[NF_IP_LOCAL_OUT] == n);
		user_offset = h->info.hook_entry[NF_IP_LOCAL_OUT];

		if (h->info.valid_hooks & (1 << NF_IP_POST_ROUTING)) {
			n = get_chain_end(h, n);
			n += get_entry(h, n)->next_offset;
			assert(h->info.hook_entry[NF_IP_POST_ROUTING] == n);
			user_offset = h->info.hook_entry[NF_IP_POST_ROUTING];
		}

#ifdef NF_IP_DROPPING
	} else if (strcmp(h->info.name, "drop") == 0) {
		assert(h->info.valid_hooks == (1 << NF_IP_DROPPING));

		/* Hook should be first */
		assert(h->info.hook_entry[NF_IP_DROPPING] == 0);
		user_offset = 0;
#endif
	} else {
		fprintf(stderr, "Unknown table `%s'\n", h->info.name);
		abort();
	}

	/* User chain == end of last builtin + policy entry */
	user_offset = get_chain_end(h, user_offset);
	user_offset += get_entry(h, user_offset)->next_offset;

	/* Overflows should be end of entry chains, and unconditional
           policy nodes. */
	for (i = 0; i < RUNTIME_NF_ARP_NUMHOOKS; i++) {
		STRUCT_ENTRY *e;
		STRUCT_STANDARD_TARGET *t;

		if (!(h->info.valid_hooks & (1 << i)))
			continue;
		assert(h->info.underflow[i]
		       == get_chain_end(h, h->info.hook_entry[i]));

		e = get_entry(h, get_chain_end(h, h->info.hook_entry[i]));
		assert(unconditional(&e->ip));
		assert(e->target_offset == sizeof(*e));
		t = (STRUCT_STANDARD_TARGET *)GET_TARGET(e);
		assert(t->target.u.target_size == ALIGN(sizeof(*t)));
		assert(e->next_offset == sizeof(*e) + ALIGN(sizeof(*t)));

		assert(strcmp(t->target.u.user.name, STANDARD_TARGET)==0);
		assert(t->verdict == -NF_DROP-1 || t->verdict == -NF_ACCEPT-1);

		/* Hooks and underflows must be valid entries */
		entry2index(h, get_entry(h, h->info.hook_entry[i]));
		entry2index(h, get_entry(h, h->info.underflow[i]));
	}

	assert(h->info.size
	       >= h->info.num_entries * (sizeof(STRUCT_ENTRY)
					 +sizeof(STRUCT_STANDARD_TARGET)));

	assert(h->entries.size
	       >= (h->new_number
		   * (sizeof(STRUCT_ENTRY)
		      + sizeof(STRUCT_STANDARD_TARGET))));
	assert(strcmp(h->info.name, h->entries.name) == 0);

	i = 0; n = 0;
	was_return = 0;
	/* Check all the entries. */
	ENTRY_ITERATE(h->entries.entrytable, h->entries.size,
		      check_entry, &i, &n, user_offset, &was_return, h);

	assert(i == h->new_number);
	assert(n == h->entries.size);

	/* Final entry must be error node */
	assert(strcmp(GET_TARGET(index2entry(h, h->new_number-1))
		      ->u.user.name,
		      ERROR_TARGET) == 0);
}
#endif /*ARPTC_DEBUG*/
