/* Shared library add-on to arptables for standard target support. */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <getopt.h>
#include <arptables.h>

/* Function which prints out usage message. */
static void
help(void)
{
	printf(
"Standard v%s options:\n"
"(If target is DROP, ACCEPT, RETURN or nothing)\n", ARPTABLES_VERSION);
}

static struct option opts[] = {
	{0}
};

/* Initialize the target. */
static void
init(struct arpt_entry_target *t)
{
}

/* Function which parses command options; returns true if it
   ate an option */
static int
parse(int c, char **argv, int invert, unsigned int *flags,
      const struct arpt_entry *entry,
      struct arpt_entry_target **target)
{
	return 0;
}

/* Final check; don't care. */
static void final_check(unsigned int flags)
{
}

/* Saves the targinfo in parsable form to stdout. */
static void
save(const struct arpt_arp *ip, const struct arpt_entry_target *target)
{
}

static
struct arptables_target standard
= { NULL,
    "standard",
    ARPTABLES_VERSION,
    ARPT_ALIGN(sizeof(int)),
    ARPT_ALIGN(sizeof(int)),
    &help,
    &init,
    &parse,
    &final_check,
    NULL, /* print */
    &save,
    opts
};

static void _init(void) __attribute__ ((constructor));
static void _init(void)
{
	register_target(&standard);
}
