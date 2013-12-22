#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <getopt.h>
#include <arptables.h>
#include <linux/netfilter_arp/arpt_mangle.h>

static void
help(void)
{
	printf(
"mangle target v%s options:\n"
"--mangle-ip-s IP address\n"
"--mangle-ip-d IP address\n"
"--mangle-mac-s MAC address\n"
"--mangle-mac-d MAC address\n"
"--mangle-target target (DROP, CONTINUE or ACCEPT -- default is ACCEPT)\n",
	ARPTABLES_VERSION);
}

#define MANGLE_IPS    '1'
#define MANGLE_IPT    '2'
#define MANGLE_DEVS   '3'
#define MANGLE_DEVT   '4'
#define MANGLE_TARGET '5'
static struct option opts[] = {
	{ "mangle-ip-s"   , required_argument, 0, MANGLE_IPS    },
	{ "mangle-ip-d"   , required_argument, 0, MANGLE_IPT    },
	{ "mangle-mac-s"  , required_argument, 0, MANGLE_DEVS   },
	{ "mangle-mac-d"  , required_argument, 0, MANGLE_DEVT   },
	{ "mangle-target" , required_argument, 0, MANGLE_TARGET },
	{0}
};

static void
init(struct arpt_entry_target *t)
{
	struct arpt_mangle *mangle = (struct arpt_mangle *) t->data;

	mangle->target = NF_ACCEPT;
}

static int
parse(int c, char **argv, int invert, unsigned int *flags,
      const struct arpt_entry *e,
      struct arpt_entry_target **t)
{
	struct arpt_mangle *mangle = (struct arpt_mangle *)(*t)->data;
	struct in_addr *ipaddr;
	struct ether_addr *macaddr;
	int ret = 1;

	switch (c) {
	case MANGLE_IPS:
/*
		if (e->arp.arpln_mask == 0)
			exit_error(PARAMETER_PROBLEM, "no pln defined");

		if (e->arp.invflags & ARPT_INV_ARPPLN)
			exit_error(PARAMETER_PROBLEM,
				   "! pln not allowed for --mangle-ip-s");
*/
/*
		if (e->arp.arpln != 4)
			exit_error(PARAMETER_PROBLEM, "only pln=4 supported");
*/
		{
			unsigned int nr;
			ipaddr = parse_hostnetwork(argv[optind-1], &nr);
		}
		mangle->u_s.src_ip.s_addr = ipaddr->s_addr;
		free(ipaddr);
		mangle->flags |= ARPT_MANGLE_SIP;
		break;
	case MANGLE_IPT:
/*
		if (e->arp.arpln_mask == 0)
			exit_error(PARAMETER_PROBLEM, "no pln defined");

		if (e->arp.invflags & ARPT_INV_ARPPLN)
			exit_error(PARAMETER_PROBLEM,
				   "! pln not allowed for --mangle-ip-d");
*/
/*
		if (e->arp.arpln != 4)
			exit_error(PARAMETER_PROBLEM, "only pln=4 supported");
*/
		{
			unsigned int nr;
			ipaddr = parse_hostnetwork(argv[optind-1], &nr);
		}
		mangle->u_t.tgt_ip.s_addr = ipaddr->s_addr;
		free(ipaddr);
		mangle->flags |= ARPT_MANGLE_TIP;
		break;
	case MANGLE_DEVS:
		if (e->arp.arhln_mask == 0)
			exit_error(PARAMETER_PROBLEM, "no --h-length defined");
		if (e->arp.invflags & ARPT_INV_ARPHLN)
			exit_error(PARAMETER_PROBLEM,
				   "! --h-length not allowed for "
				   "--mangle-mac-s");
		if (e->arp.arhln != 6)
			exit_error(PARAMETER_PROBLEM, "only --h-length 6 "
						      "supported");
		macaddr = ether_aton(argv[optind-1]);
		if (macaddr == NULL)
			exit_error(PARAMETER_PROBLEM, "invalid source MAC");
		memcpy(mangle->src_devaddr, macaddr, e->arp.arhln);
		mangle->flags |= ARPT_MANGLE_SDEV;
		break;
	case MANGLE_DEVT:
		if (e->arp.arhln_mask == 0)
			exit_error(PARAMETER_PROBLEM, "no --h-length defined");
		if (e->arp.invflags & ARPT_INV_ARPHLN)
			exit_error(PARAMETER_PROBLEM,
				   "! hln not allowed for --mangle-mac-d");
		if (e->arp.arhln != 6)
			exit_error(PARAMETER_PROBLEM, "only --h-length 6 "
						      "supported");
		macaddr = ether_aton(argv[optind-1]);
		if (macaddr == NULL)
			exit_error(PARAMETER_PROBLEM, "invalid target MAC");
		memcpy(mangle->tgt_devaddr, macaddr, e->arp.arhln);
		mangle->flags |= ARPT_MANGLE_TDEV;
		break;
	case MANGLE_TARGET:
		if (!strcmp(argv[optind-1], "DROP"))
			mangle->target = NF_DROP;
		else if (!strcmp(argv[optind-1], "ACCEPT"))
			mangle->target = NF_ACCEPT;
		else if (!strcmp(argv[optind-1], "CONTINUE"))
			mangle->target = ARPT_CONTINUE;
		else
			exit_error(PARAMETER_PROBLEM, "bad target for "
						      "--mangle-target");
		break;
	default:
		ret = 0;
	}

	return ret;
}

static void final_check(unsigned int flags)
{
}

static void print(const struct arpt_arp *ip,
   const struct arpt_entry_target *target, int numeric)
{
	struct arpt_mangle *m = (struct arpt_mangle *)(target->data);
	char buf[100];

	if (m->flags & ARPT_MANGLE_SIP) {
		if (numeric)
			sprintf(buf, "%s", addr_to_dotted(&(m->u_s.src_ip)));
		else
			sprintf(buf, "%s", addr_to_anyname(&(m->u_s.src_ip)));
		printf("--mangle-ip-s %s ", buf);
	}
	if (m->flags & ARPT_MANGLE_SDEV) {
		printf("--mangle-mac-s ");
		print_mac((unsigned char *)m->src_devaddr, 6);
		printf(" ");
	}
	if (m->flags & ARPT_MANGLE_TIP) {
		if (numeric)
			sprintf(buf, "%s", addr_to_dotted(&(m->u_t.tgt_ip)));
		else
			sprintf(buf, "%s", addr_to_anyname(&(m->u_t.tgt_ip)));
		printf("--mangle-ip-d %s ", buf);
	}
	if (m->flags & ARPT_MANGLE_TDEV) {
		printf("--mangle-mac-d ");
		print_mac((unsigned char *)m->tgt_devaddr, 6);
		printf(" ");
	}
	if (m->target != NF_ACCEPT) {
		printf("--mangle-target ");
		if (m->target == NF_DROP)
			printf("DROP ");
		else
			printf("CONTINUE ");
	}
}

static void
save(const struct arpt_arp *ip, const struct arpt_entry_target *target)
{
}

static
struct arptables_target change
= { NULL,
    "mangle",
    ARPTABLES_VERSION,
    ARPT_ALIGN(sizeof(struct arpt_mangle)),
    ARPT_ALIGN(sizeof(struct arpt_mangle)),
    &help,
    &init,
    &parse,
    &final_check,
    &print,
    &save,
    opts
};

static void _init(void) __attribute__ ((constructor));
static void _init(void)
{
	register_target(&change);
}
