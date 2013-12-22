/*
 * (C) 2010 by Frederic Leroy <fredo@starox.org>
 *
 * arpt_classify.c -- arptables extension to classify arp packet
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <getopt.h>
#include <arptables.h>
#include <linux/netfilter/xt_CLASSIFY.h>

#define TC_H_MAJ_MASK (0xFFFF0000U)
#define TC_H_MIN_MASK (0x0000FFFFU)
#define TC_H_MAJ(h) ((h)&TC_H_MAJ_MASK)
#define TC_H_MIN(h) ((h)&TC_H_MIN_MASK)
#define TC_H_MAKE(maj,min) (((maj)&TC_H_MAJ_MASK)|((min)&TC_H_MIN_MASK))

static void
help(void)
{
	printf(
"CLASSIFY target v%s options:\n"
"--set-class major:minor : set the major and minor class value\n",
	ARPTABLES_VERSION);
}

#define CLASSIFY_OPT 1

static struct option opts[] = {
	{ "set-class"   , required_argument, 0, CLASSIFY_OPT },
	{0}
};

static void
init(struct arpt_entry_target *t)
{
	struct xt_classify_target_info *classify = (struct xt_classify_target_info *) t->data;
	classify->priority = 0;
}

static int
parse(int c, char **argv, int invert, unsigned int *flags,
	const struct arpt_entry *e,
	struct arpt_entry_target **t)
{
	struct xt_classify_target_info *classify = (struct xt_classify_target_info *)(*t)->data;
	int i,j;

	switch (c) {
		case CLASSIFY_OPT:
			if (sscanf(argv[optind-1], "%x:%x", &i, &j) != 2) {
				exit_error(PARAMETER_PROBLEM,
						"Bad class value `%s'", optarg);
				return 0;
			}
			classify->priority = TC_H_MAKE(i<<16, j);
			if (*flags)
				exit_error(PARAMETER_PROBLEM,
						"CLASSIFY: Can't specify --set-class twice");
			*flags = 1;
			break;
		default:
			return 0;
	}
	return 1;
}

static void final_check(unsigned int flags)
{
	if (!flags)
		exit_error(PARAMETER_PROBLEM, "CLASSIFY: Parameter --set-class is required");
}

static void print(const struct arpt_arp *ip,
	const struct arpt_entry_target *target, int numeric)
{
	struct xt_classify_target_info *t = (struct xt_classify_target_info *)(target->data);

	printf("--set-class %x:%x ", TC_H_MAJ(t->priority)>>16, TC_H_MIN(t->priority));
}

static void
save(const struct arpt_arp *ip, const struct arpt_entry_target *target)
{
}

static
struct arptables_target classify
= { NULL,
	"CLASSIFY",
	ARPTABLES_VERSION,
	ARPT_ALIGN(sizeof(struct xt_classify_target_info)),
	ARPT_ALIGN(sizeof(struct xt_classify_target_info)),
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
	register_target(&classify);
}
