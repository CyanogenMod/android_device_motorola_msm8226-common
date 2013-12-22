/*
 *  ebt_idnat
 *
 *	Authors:
 *	Grzegorz Borowiak <grzes@gnu.univ.gda.pl>
 *
 *  September, 2003
 *
 */

#include <linux/netfilter_bridge/ebtables.h>
#include <linux/netfilter_bridge/ebt_inat.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <linux/if_arp.h>
#include <linux/types.h>
#include <net/sock.h>

static int get_ip_dst(const struct sk_buff *skb, uint32_t *addr)
{
	if (skb->mac.ethernet->h_proto == __constant_htons(ETH_P_IP)) {
		*addr = skb->nh.iph->daddr;
		return 1;
	}
	if (skb->mac.ethernet->h_proto == __constant_htons(ETH_P_ARP)) {

		uint32_t arp_len = sizeof(struct arphdr) +
		   (2 * (((*skb).nh.arph)->ar_hln)) +
		   (2 * (((*skb).nh.arph)->ar_pln));

		// Make sure the packet is long enough.
		if ((((*skb).nh.raw) + arp_len) > (*skb).tail)
			return 0;
		// IPv4 addresses are always 4 bytes.
		if (((*skb).nh.arph)->ar_pln != sizeof(uint32_t))
			return 0;

		memcpy(addr, ((*skb).nh.raw) + sizeof(struct arphdr) +
		   (2*(((*skb).nh.arph)->ar_hln)) +
		   (((*skb).nh.arph)->ar_pln), sizeof(uint32_t));
		
		return 2;
	}
	return 0;
}

static int ebt_target_idnat(struct sk_buff **pskb, unsigned int hooknr,
   const struct net_device *in, const struct net_device *out,
   const void *data, unsigned int datalen)
{
	struct ebt_inat_info *info = (struct ebt_inat_info *)data;
	uint32_t ip;
	int index;
	struct ebt_inat_tuple *tuple;
	
	if (!get_ip_dst(*pskb, &ip)) {
		/* not an ARP or IPV4 packet */
		return info->target;
	}

	if ((ip & __constant_htonl(0xffffff00)) != info->ip_subnet) {

		/* outside our range */
		return info->target;
	}

	index = ((unsigned char*)&ip)[3]; /* the last byte; network packets are big endian */
	tuple = &info->a[index];

	if (!tuple->enabled) {
		/* we do not want to alter packets with this IP */
		return info->target;
	}
	
	memcpy(((**pskb).mac.ethernet)->h_dest, tuple->mac,
	   ETH_ALEN * sizeof(unsigned char));

	if ((**pskb).mac.ethernet->h_proto == __constant_htons(ETH_P_ARP)) {
		/* change the payload */
		memcpy(
		   (**pskb).nh.raw + sizeof(struct arphdr) + 
		   (**pskb).nh.arph->ar_hln +
		   (**pskb).nh.arph->ar_pln, tuple->mac, ETH_ALEN);
	}
	
	return tuple->target;
}

static int ebt_target_idnat_check(const char *tablename, unsigned int hookmask,
   const struct ebt_entry *e, void *data, unsigned int datalen)
{
	struct ebt_inat_info *info = (struct ebt_inat_info *)data;

	if (BASE_CHAIN && info->target == EBT_RETURN)
		return -EINVAL;
	CLEAR_BASE_CHAIN_BIT;
	if ( (strcmp(tablename, "nat") ||
	   (hookmask & ~((1 << NF_BR_PRE_ROUTING) | (1 << NF_BR_LOCAL_OUT)))) &&
	   (strcmp(tablename, "broute") || hookmask & ~(1 << NF_BR_BROUTING)) )
		return -EINVAL;
	if (datalen != EBT_ALIGN(sizeof(struct ebt_inat_info)))
		return -EINVAL;
	if (INVALID_TARGET)
		return -EINVAL;
	return 0;
}

static struct ebt_target idnat =
{
	{NULL, NULL}, EBT_IDNAT_TARGET, ebt_target_idnat, ebt_target_idnat_check,
	NULL, THIS_MODULE
};

static int __init init(void)
{
	return ebt_register_target(&idnat);
}

static void __exit fini(void)
{
	ebt_unregister_target(&idnat);
}

module_init(init);
module_exit(fini);
EXPORT_NO_SYMBOLS;
MODULE_LICENSE("GPL");
