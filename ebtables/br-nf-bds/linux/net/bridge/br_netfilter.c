/*
 *	Handle firewalling
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek               <buytenh@gnu.org>
 *	Bart De Schuymer		<bart.de.schuymer@pandora.be>
 *
 *	$Id: br_netfilter.c,v 1.3 2002/09/11 17:41:38 bdschuym Exp $
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *	Lennert dedicates this file to Kerstin Wurdinger.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ip.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/netfilter_bridge.h>
#include <linux/netfilter_ipv4.h>
#include <linux/in_route.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <asm/uaccess.h>
#include <asm/checksum.h>
#include "br_private.h"


#ifndef WE_REALLY_INSIST_ON_NOT_HAVING_NAT_SUPPORT
/* As the original source/destination addresses are variables private to this
 * file, we store them in unused space at the end of the control buffer.
 * On 64-bit platforms the TCP control buffer size still leaves us 8 bytes
 * of space at the end, so that fits.  Usage of the original source address
 * and the original destination address never overlaps (daddr is needed
 * around PRE_ROUTING, and saddr around POST_ROUTING), so that's okay as
 * well.
 */
#define skb_origaddr(skb)		(*((u32 *)((skb)->cb + sizeof((skb)->cb) - 4)))

#define store_orig_dstaddr(skb)		(skb_origaddr(skb) = (skb)->nh.iph->daddr)
#define store_orig_srcaddr(skb)		(skb_origaddr(skb) = (skb)->nh.iph->saddr)
#define dnat_took_place(skb)		(skb_origaddr(skb) != (skb)->nh.iph->daddr)
#define snat_took_place(skb)		(skb_origaddr(skb) != (skb)->nh.iph->saddr)
#else
#define store_orig_dstaddr(skb)
#define store_orig_srcaddr(skb)
#define dnat_took_place(skb)		(0)
#define snat_took_place(skb)		(0)
#endif


#define has_bridge_parent(device)	((device)->br_port != NULL)
#define bridge_parent(device)		(&((device)->br_port->br->dev))


/* As opposed to the DNAT case, for the SNAT case it's not quite
 * clear what we should do with ethernet addresses in NAT'ed
 * packets.  Use this heuristic for now.
 */
static inline void __maybe_fixup_src_address(struct sk_buff *skb)
{
	if (snat_took_place(skb) &&
	    inet_addr_type(skb->nh.iph->saddr) == RTN_LOCAL) {
		memcpy(skb->mac.ethernet->h_source,
			bridge_parent(skb->dev)->dev_addr,
			ETH_ALEN);
	}
}


/* We need these fake structures to make netfilter happy --
 * lots of places assume that skb->dst != NULL, which isn't
 * all that unreasonable.
 *
 * Currently, we fill in the PMTU entry because netfilter
 * refragmentation needs it, and the rt_flags entry because
 * ipt_REJECT needs it.  Future netfilter modules might
 * require us to fill additional fields.
 */
static struct net_device __fake_net_device = {
	hard_header_len:	ETH_HLEN
};

static struct rtable __fake_rtable = {
	u: {
		dst: {
			__refcnt:		ATOMIC_INIT(1),
			dev:			&__fake_net_device,
			pmtu:			1500
		}
	},

	rt_flags:	0
};


/* PF_BRIDGE/PRE_ROUTING *********************************************/
static void __br_dnat_complain(void)
{
	static unsigned long last_complaint = 0;

	if (jiffies - last_complaint >= 5 * HZ) {
		printk(KERN_WARNING "Performing cross-bridge DNAT requires IP "
			"forwarding to be enabled\n");
		last_complaint = jiffies;
	}
}


static int br_nf_pre_routing_finish_bridge(struct sk_buff *skb)
{
	skb->dev = bridge_parent(skb->dev);
#ifdef CONFIG_NETFILTER_DEBUG
	skb->nf_debug |= (1 << NF_BR_PRE_ROUTING) | (1 << NF_BR_FORWARD);
#endif
	skb->dst->output(skb);
	return 0;
}

#ifdef CONFIG_NETFILTER_DEBUG
#define __br_handle_frame_finish  br_nf_pre_routing_finish_route
static inline int br_nf_pre_routing_finish_route(struct sk_buff *skb)
{
	skb->nf_debug = 0;
	br_handle_frame_finish(skb);
	return 0;
}
#else
#define __br_handle_frame_finish br_handle_frame_finish
#endif

/* This requires some explaining.  If DNAT has taken place,
 * we will need to fix up the destination ethernet address,
 * and this is a tricky process.
 *
 * There are two cases to consider:
 * 1. The packet was DNAT'ed to a device in the same bridge
 *    port group as it was received on.  We can still bridge
 *    the packet.
 * 2. The packet was DNAT'ed to a different device, either
 *    a non-bridged device or another bridge port group.
 *    The packet will need to be routed.
 *
 * The way to distinguish between the two is by calling ip_route_input()
 * and looking at skb->dst->dev, which it changed to the destination device
 * if ip_route_input() succeeds.
 *
 * Let us first consider ip_route_input() succeeds:
 *
 * If skb->dst->dev equals the logical bridge device the packet came in on,
 * we can consider this bridging. We then call skb->dst->output() which will
 * make the packet enter br_nf_local_out() not much later. In that function
 * it is assured that the iptables FORWARD chain is traversed for the packet.
 *
 * Else, the packet is considered to be routed and we just change the
 * destination MAC address so that the packet will later be passed up to the ip
 * stack to be routed.
 *
 * Let us now consider ip_route_input() fails:
 *
 * After a "echo '0' > /proc/sys/net/ipv4/ip_forward" ip_route_input() will
 * fail, while ip_route_output() will return success. The source address for
 * ip_route_output() is set to zero, so ip_route_output()
 * thinks we're handling a locally generated packet and won't care if
 * ip forwarding is allowed. We send a warning message to the users's log
 * telling her to put ip forwarding on.
 *
 * ip_route_input() will also fail if there is no route available. Then we just
 * drop the packet.
 *
 * The other special thing happening here is putting skb->physoutdev on
 * &__fake_net_device (resp. NULL) for bridged (resp. routed) packets. This is
 * needed so that br_nf_local_out() can know that it has to give the packets to
 * the BR_NF_FORWARD (resp. BR_NF_LOCAL_OUT) bridge hook. See that function.
 * --Lennert, 20020411
 * --Bart, 20020416 (updated)
 */

static int br_nf_pre_routing_finish(struct sk_buff *skb)
{
	struct net_device *dev = skb->dev;
	struct iphdr *iph = skb->nh.iph;

#ifdef CONFIG_NETFILTER_DEBUG
	skb->nf_debug ^= (1 << NF_BR_PRE_ROUTING);
#endif
	if (dnat_took_place(skb)) {
		if (ip_route_input(skb, iph->daddr, iph->saddr, iph->tos, dev)) {
			struct rtable *rt;

			if (!ip_route_output(&rt, iph->daddr, 0, iph->tos, 0)) {
				// bridged dnated traffic isn't dependent on
				// disabled ip_forwarding
				if (((struct dst_entry *)rt)->dev == dev) {
					skb->dst = (struct dst_entry *)rt;
					goto bridged_dnat;
				}
				__br_dnat_complain();
				dst_release((struct dst_entry *)rt);
			}
			kfree_skb(skb);
			return 0;
		} else {
			if (skb->dst->dev == dev) {
bridged_dnat:
				// tell br_nf_local_out this is a bridged frame
				skb->physoutdev = &__fake_net_device;
				skb->dev = skb->physindev;
				NF_HOOK_THRESH(PF_BRIDGE, NF_BR_PRE_ROUTING, skb, skb->dev, NULL,
						br_nf_pre_routing_finish_bridge, 1);
				return 0;
			}
			// tell br_nf_local_out this is a routed frame
			skb->physoutdev = NULL;
			memcpy(skb->mac.ethernet->h_dest, dev->dev_addr, ETH_ALEN);
		}
	} else {
		skb->dst = (struct dst_entry *)&__fake_rtable;
		dst_hold(skb->dst);
	}
	skb->dev = skb->physindev;
	NF_HOOK_THRESH(PF_BRIDGE, NF_BR_PRE_ROUTING, skb, skb->dev, NULL,
			__br_handle_frame_finish, 1);

	return 0;
}

/* Replicate the checks that IPv4 does on packet reception.
 * Set skb->dev to the bridge device (i.e. parent of the
 * receiving device) to make netfilter happy, the REDIRECT
 * target in particular.  Save the original destination IP
 * address to be able to detect DNAT afterwards.
 */
static unsigned int br_nf_pre_routing(unsigned int hook, struct sk_buff **pskb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *iph;
	__u32 len;
	struct sk_buff *skb;

	if ((*pskb)->protocol != __constant_htons(ETH_P_IP))
		return NF_ACCEPT;

	if ((skb = skb_share_check(*pskb, GFP_ATOMIC)) == NULL)
		goto out;

	if (!pskb_may_pull(skb, sizeof(struct iphdr)))
		goto inhdr_error;

	iph = skb->nh.iph;
	if (iph->ihl < 5 || iph->version != 4)
		goto inhdr_error;

	if (!pskb_may_pull(skb, 4*iph->ihl))
		goto inhdr_error;

	iph = skb->nh.iph;
	if (ip_fast_csum((__u8 *)iph, iph->ihl) != 0)
		goto inhdr_error;

	len = ntohs(iph->tot_len);
	if (skb->len < len || len < 4*iph->ihl)
		goto inhdr_error;

	if (skb->len > len) {
		__pskb_trim(skb, len);
		if (skb->ip_summed == CHECKSUM_HW)
			skb->ip_summed = CHECKSUM_NONE;
	}

	skb->physindev = skb->dev;
	skb->dev = bridge_parent(skb->dev);
	if (skb->pkt_type == PACKET_OTHERHOST)
		skb->pkt_type = PACKET_HOST;
	store_orig_dstaddr(skb);

#ifdef CONFIG_NETFILTER_DEBUG
	(*pskb)->nf_debug ^= (1 << NF_IP_PRE_ROUTING);
#endif
	NF_HOOK(PF_INET, NF_IP_PRE_ROUTING, skb, skb->dev, NULL,
		br_nf_pre_routing_finish);

	return NF_STOLEN;

inhdr_error:
//	IP_INC_STATS_BH(IpInHdrErrors);
out:
	return NF_DROP;
}


/* PF_BRIDGE/LOCAL_IN ************************************************/
/* The packet is locally destined, which requires a real
 * dst_entry, so detach the fake one.  On the way up, the
 * packet would pass through PRE_ROUTING again (which already
 * took place when the packet entered the bridge), but we
 * register an IPv4 PRE_ROUTING 'sabotage' hook that will
 * prevent this from happening.
 */
static unsigned int br_nf_local_in(unsigned int hook, struct sk_buff **pskb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct sk_buff *skb = *pskb;

	if (skb->protocol != __constant_htons(ETH_P_IP))
		return NF_ACCEPT;

	if (skb->dst == (struct dst_entry *)&__fake_rtable) {
		dst_release(skb->dst);
		skb->dst = NULL;
	}

	return NF_ACCEPT;
}


/* PF_BRIDGE/FORWARD *************************************************/
static int br_nf_forward_finish(struct sk_buff *skb)
{
#ifdef CONFIG_NETFILTER_DEBUG
	skb->nf_debug ^= (1 << NF_BR_FORWARD);
#endif
	NF_HOOK_THRESH(PF_BRIDGE, NF_BR_FORWARD, skb, skb->physindev,
			skb->dev, br_forward_finish, 1);

	return 0;
}

/* This is the 'purely bridged' case.  We pass the packet to
 * netfilter with indev and outdev set to the bridge device,
 * but we are still able to filter on the 'real' indev/outdev
 * because another bit of the bridge-nf patch overloads the
 * '-i' and '-o' iptables interface checks to take
 * skb->phys{in,out}dev into account as well (so both the real
 * device and the bridge device will match).
 */
static unsigned int br_nf_forward(unsigned int hook, struct sk_buff **pskb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct sk_buff *skb = *pskb;

	// don't mess with non-ip frames, also don't mess with the ip-packets
	// when br_nf_local_out_finish explicitly says so.
	if (skb->protocol != __constant_htons(ETH_P_IP) || skb->physindev == NULL)
		return NF_ACCEPT;

	skb->physoutdev = skb->dev;
#ifdef CONFIG_NETFILTER_DEBUG
	skb->nf_debug ^= (1 << NF_IP_FORWARD);
#endif
	NF_HOOK(PF_INET, NF_IP_FORWARD, skb, bridge_parent(skb->physindev),
			bridge_parent(skb->dev), br_nf_forward_finish);

	return NF_STOLEN;
}


/* PF_BRIDGE/LOCAL_OUT
***********************************************/
static int br_nf_local_out_finish_forward(struct sk_buff *skb)
{
	struct net_device *dev;

	dev = skb->physindev;
	// tell br_nf_forward to stay away
	skb->physindev = NULL;
#ifdef CONFIG_NETFILTER_DEBUG
	skb->nf_debug &= ~(1 << NF_BR_FORWARD);
#endif
	NF_HOOK(PF_BRIDGE, NF_BR_FORWARD, skb, dev, skb->dev,
		br_forward_finish);

	return 0;
}

static int br_nf_local_out_finish(struct sk_buff *skb)
{
#ifdef CONFIG_NETFILTER_DEBUG
	skb->nf_debug &= ~(1 << NF_BR_LOCAL_OUT);
#endif
	NF_HOOK_THRESH(PF_BRIDGE, NF_BR_LOCAL_OUT, skb, NULL, skb->dev,
			br_forward_finish, INT_MIN + 1);

	return 0;
}


/* This hook sees both locally originated IP packets and forwarded
 * IP packets (in both cases the destination device is a bridge
 * device).  For the sake of interface transparency (i.e. properly
 * overloading the '-o' option), we steal packets destined to
 * a bridge device away from the IPv4 FORWARD and OUTPUT hooks,
 * and reinject them later, when we have determined the real
 * output device.  This reinjecting happens here.
 *
 * If skb->physindev is NULL, the bridge-nf code never touched
 * this packet before, and so the packet was locally originated.
 * We call the IPv4 LOCAL_OUT hook.
 *
 * If skb->physindev isn't NULL, there are two cases:
 * 1. The packet was IP routed.
 * 2. The packet was cross-bridge DNAT'ed (see the comment near
 *    PF_BRIDGE/PRE_ROUTING).
 * In both cases, we call the IPv4 FORWARD hook.  In case 1,
 * if the packet originally came from a bridge device, and in
 * case 2, skb->physindev will have a bridge device as parent,
 * so we use that parent device as indev.  Otherwise, we just
 * use physindev.
 *
 * If skb->physoutdev == NULL the bridge code never touched the
 * packet or the packet was routed in br_nf_pre_routing_finish().
 * We give the packet to the bridge NF_BR_LOCAL_OUT hook.
 * If not, the packet is actually a bridged one so we give it to
 * the NF_BR_FORWARD hook.
 */

static unsigned int br_nf_local_out(unsigned int hook, struct sk_buff **pskb, const struct net_device *in, const struct net_device *out, int (*_okfn)(struct sk_buff *))
{
	int hookno, prio;
	int (*okfn)(struct sk_buff *skb);
	struct net_device *realindev;
	struct sk_buff *skb = *pskb;

	if (skb->protocol != __constant_htons(ETH_P_IP))
		return NF_ACCEPT;

	/* Sometimes we get packets with NULL ->dst here (for example,
	 * running a dhcp client daemon triggers this).
	 */
	if (skb->dst == NULL)
		return NF_ACCEPT;

	// bridged, take forward
	// (see big note in front of br_nf_pre_routing_finish)
	if (skb->physoutdev == &__fake_net_device) {
		okfn = br_nf_local_out_finish_forward;
	} else if (skb->physoutdev == NULL) {
		// non-bridged: routed or locally generated traffic, take local_out
		// (see big note in front of br_nf_pre_routing_finish)
		okfn = br_nf_local_out_finish;
	} else {
		printk("ARGH: bridge_or_routed hack doesn't work\n");
		okfn = br_nf_local_out_finish;
	}

	skb->physoutdev = skb->dev;

#ifdef CONFIG_NETFILTER_DEBUG
	skb->nf_debug ^= (1 << NF_BR_LOCAL_OUT);
#endif
	hookno = NF_IP_LOCAL_OUT;
	prio = NF_IP_PRI_BRIDGE_SABOTAGE;
	if ((realindev = skb->physindev) != NULL) {
		hookno = NF_IP_FORWARD;
		// there is an iptables mangle table FORWARD chain with
		// priority -150. This chain should see the physical out-dev.
		prio = NF_IP_PRI_BRIDGE_SABOTAGE_FORWARD;
		if (has_bridge_parent(realindev))
			realindev = bridge_parent(realindev);
	}

	NF_HOOK_THRESH(PF_INET, hookno, skb, realindev,
			bridge_parent(skb->dev), okfn, prio + 1);

	return NF_STOLEN;
}


/* PF_BRIDGE/POST_ROUTING ********************************************/
static int br_nf_post_routing_finish(struct sk_buff *skb)
{
	__maybe_fixup_src_address(skb);
#ifdef CONFIG_NETFILTER_DEBUG
	skb->nf_debug ^= (1 << NF_BR_POST_ROUTING);
#endif
	NF_HOOK_THRESH(PF_BRIDGE, NF_BR_POST_ROUTING, skb, NULL,
			bridge_parent(skb->dev), br_dev_queue_push_xmit, 1);

	return 0;
}

static unsigned int br_nf_post_routing(unsigned int hook, struct sk_buff **pskb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct sk_buff *skb = *pskb;

	/* Be very paranoid.  */
	if (skb->mac.raw < skb->head || skb->mac.raw + ETH_HLEN > skb->data) {
		printk(KERN_CRIT "Argh!! Fuck me harder with a chainsaw. ");
		if (skb->dev != NULL) {
			printk("[%s]", skb->dev->name);
			if (has_bridge_parent(skb->dev))
				printk("[%s]", bridge_parent(skb->dev)->name);
		}
		printk("\n");
		return NF_ACCEPT;
	}

	if (skb->protocol != __constant_htons(ETH_P_IP))
		return NF_ACCEPT;

	/* Sometimes we get packets with NULL ->dst here (for example,
	 * running a dhcp client daemon triggers this).
	 */
	if (skb->dst == NULL)
		return NF_ACCEPT;

	store_orig_srcaddr(skb);
#ifdef CONFIG_NETFILTER_DEBUG
	skb->nf_debug ^= (1 << NF_IP_POST_ROUTING);
#endif
	NF_HOOK(PF_INET, NF_IP_POST_ROUTING, skb, NULL,
		bridge_parent(skb->dev), br_nf_post_routing_finish);

	return NF_STOLEN;
}


/* IPv4/SABOTAGE *****************************************************/
/* Don't hand locally destined packets to PF_INET/PRE_ROUTING
 * for the second time.  */
static unsigned int ipv4_sabotage_in(unsigned int hook, struct sk_buff **pskb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	if (in->hard_start_xmit == br_dev_xmit &&
	    okfn != br_nf_pre_routing_finish) {
		okfn(*pskb);
		return NF_STOLEN;
	}

	return NF_ACCEPT;
}

/* Postpone execution of PF_INET/FORWARD, PF_INET/LOCAL_OUT
 * and PF_INET/POST_ROUTING until we have done the forwarding
 * decision in the bridge code and have determined skb->physoutdev.
 */
static unsigned int ipv4_sabotage_out(unsigned int hook, struct sk_buff **pskb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	if (out->hard_start_xmit == br_dev_xmit &&
	    okfn != br_nf_forward_finish &&
	    okfn != br_nf_local_out_finish &&
	    okfn != br_nf_post_routing_finish) {
		struct sk_buff *skb = *pskb;

		if (hook == NF_IP_FORWARD && skb->physindev == NULL)
			skb->physindev = (struct net_device *)in;
		okfn(skb);
		return NF_STOLEN;
	}

	return NF_ACCEPT;
}


static struct nf_hook_ops br_nf_ops[] = {
	{ { NULL, NULL }, br_nf_pre_routing, PF_BRIDGE, NF_BR_PRE_ROUTING, 0 },
	{ { NULL, NULL }, br_nf_local_in, PF_BRIDGE, NF_BR_LOCAL_IN, 0 },
	{ { NULL, NULL }, br_nf_forward, PF_BRIDGE, NF_BR_FORWARD, 0 },
	// we need INT_MIN, so innocent NF_BR_LOCAL_OUT functions don't
	// get bridged traffic as input
	{ { NULL, NULL }, br_nf_local_out, PF_BRIDGE, NF_BR_LOCAL_OUT, INT_MIN },
	{ { NULL, NULL }, br_nf_post_routing, PF_BRIDGE, NF_BR_POST_ROUTING, 0 },

	{ { NULL, NULL }, ipv4_sabotage_in, PF_INET, NF_IP_PRE_ROUTING, NF_IP_PRI_FIRST },

	{ { NULL, NULL }, ipv4_sabotage_out, PF_INET, NF_IP_FORWARD, NF_IP_PRI_BRIDGE_SABOTAGE_FORWARD },
	{ { NULL, NULL }, ipv4_sabotage_out, PF_INET, NF_IP_LOCAL_OUT, NF_IP_PRI_BRIDGE_SABOTAGE },
	{ { NULL, NULL }, ipv4_sabotage_out, PF_INET, NF_IP_POST_ROUTING, NF_IP_PRI_FIRST },
};

#define NUMHOOKS (sizeof(br_nf_ops)/sizeof(br_nf_ops[0]))


int br_netfilter_init(void)
{
	int i;

#ifndef WE_REALLY_INSIST_ON_NOT_HAVING_NAT_SUPPORT
	if (sizeof(struct tcp_skb_cb) + 4 >= sizeof(((struct sk_buff *)NULL)->cb)) {
		extern int __too_little_space_in_control_buffer(void);
		__too_little_space_in_control_buffer();
	}
#endif

	for (i=0;i<NUMHOOKS;i++) {
		int ret;

		if ((ret = nf_register_hook(&br_nf_ops[i])) >= 0)
			continue;

		while (i--)
			nf_unregister_hook(&br_nf_ops[i]);

		return ret;
	}

	printk(KERN_NOTICE "Bridge firewalling registered\n");

	return 0;
}

void br_netfilter_fini(void)
{
	int i;

	for (i=NUMHOOKS-1;i>=0;i--)
		nf_unregister_hook(&br_nf_ops[i]);
}
