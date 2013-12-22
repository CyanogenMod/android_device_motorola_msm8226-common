#ifndef __LINUX_BRIDGE_EBT_NAT_H
#define __LINUX_BRIDGE_EBT_NAT_H

struct ebt_inat_tuple
{
	int enabled;
	unsigned char mac[ETH_ALEN];
	// EBT_ACCEPT, EBT_DROP or EBT_CONTINUE
	int target;
};

struct ebt_inat_info
{
	uint32_t ip_subnet;
	struct ebt_inat_tuple a[256];
	// EBT_ACCEPT, EBT_DROP, EBT_CONTINUE or EBT_RETURN
	int target;
};

#define EBT_ISNAT_TARGET "isnat"
#define EBT_IDNAT_TARGET "idnat"

#endif
