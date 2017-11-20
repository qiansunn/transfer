#include <asm/unaligned.h>
#include <linux/skbuff.h>
#include <linux/time.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/inet.h>
#include <net/tcp.h>
#include <net/xfrm.h>

//---------- Create a feebdack packet and prepare for transmission.  Returns 1 if successful.
static int generate_ack(struct Flow_entry *flow)
{
    //struct net_device *ndev = dev_get_by_name(&init_net, "p1p2");
    if(unlikely(!flow) || unlikely(!flow->dev))
        return 0;
    struct sk_buff *skb;
    struct ethhdr *eth_to;
    struct iphdr *iph_to;
    struct tcphdr *tcp_to;
    int length, tcplen, ret;
    unsigned long flags;         //variable for save current states of irq

    int tcpsize=sizeof(struct tcphdr) + tcp_options_size(flow);  // flow->h.optsize+ tcp_options_size(flow);
    int iphsize=sizeof(struct iphdr);
    int ethsize = ETH_HLEN;

    int size = ethsize + iphsize + tcpsize;
    if(size < ETH_ZLEN)
        size = ETH_ZLEN;

    skb = netdev_alloc_skb(flow->dev, size);
    if(likely(skb))
    {
        skb_set_queue_mapping(skb, 0);
        skb->len = size; //pkt->len;
        skb->protocol = __constant_htons(ETH_P_IP);
        skb->pkt_type =  flow->p_type; //PACKET_OUTGOING; //flow->p_type; //PACKET_HOST; //PACKET_USER;
        skb_set_tail_pointer(skb, size);

        //----------------MAC HEADER---------------------
        skb_reset_mac_header(skb);
        eth_to = eth_hdr(skb);

        //spin_lock_irqsave(&globalLock,flags);
        memcpy(eth_to->h_source, flow->f.local_eth, ETH_ALEN);
        memcpy(eth_to->h_dest, flow->f.remote_eth, ETH_ALEN);
        //spin_unlock_irqrestore(&globalLock,flags);

        //No lock is needed
        eth_to->h_proto =  __constant_htons(ETH_P_IP);//eth_from->h_proto;

        skb_pull(skb, ethsize);
        //skb_reserve(skb, ETH_ALEN);

        //-----------------------IP header------------------------------------
        skb_reset_network_header(skb);
        //iph_to = (void *)skb_put(skb, sizeof(struct iphdr));
        iph_to = ip_hdr(skb);

        //spin_lock_irqsave(&globalLock,flags);
        iph_to->saddr = flow->f.local_ip;
        iph_to->daddr = flow->f.remote_ip;
        iph_to->id = flow->last_bid + 1; //htons(atomic_inc_return(&ip_ident)); //flow->last_bid + 1;
        //flow->last_bid = flow->last_bid +1;
        //spin_unlock_irqrestore(&globalLock,flags);

        //No Lock is needed
        iph_to->ihl = iphsize>>2;
        iph_to->version = 4;
        iph_to->tot_len = __constant_htons(iphsize + tcpsize);
        iph_to->tos = 0;
        iph_to->frag_off =  0; //htons(0x4000);
        iph_to->frag_off |= ntohs(IP_DF);
        iph_to->ttl = OUR_TTL; //64;
        iph_to->protocol = IPPROTO_TCP;

        //calculate IP checksum Info and the IP header
        ip_send_check(iph_to);
        skb_pull(skb, iphsize);

        //------------------------TCP Header---------------------------------
        //skb_push(skb, tcpsize);
        skb_reset_transport_header(skb);
        //tcp_to = (void *)skb_put(skb, sizeof(struct tcphdr));
        tcp_to = tcp_hdr(skb);
        /*if(flow->h.optsize > 0)
        {
            tcp_opt=(unsigned char*)tcp_to + 20;
            memcpy(tcp_opt, flow->h.opt, flow->h.optsize);
        }*/

        //spin_lock_irqsave(&globalLock,flags);
        tcp_to->source    = flow->f.local_port;
        tcp_to->dest    = flow->f.remote_port;
        tcp_to->seq     = htonl(flow->last_seqb);
        tcp_to->ack_seq  = htonl(flow->last_ack);
        tcp_to->window  = flow->last_window;
        //spin_unlock_irqrestore(&globalLock,flags);

        // No Lock is needed
        tcp_to->doff    = tcpsize>>2;
        tcp_to->res1=0;
        tcp_to->syn=0;
        tcp_to->rst=0;
        tcp_to->urg=0;
        tcp_to->urg_ptr = 0;
        tcp_to->ece=0; //flow->ece;
        tcp_to->cwr=0; //ack->cwr;
        tcp_to->fin=0; //ack->fin;
        tcp_to->psh=0;//flow->psh;
        if(flow->psh)
        	tcp_to->psh = 1;
        if(flow->fin)
        	tcp_to->fin = 1;
        tcp_to->ack = 1;

        if(unlikely(flow->sack) || likely(flow->tstamp))
            tcp_opt_write((__be32 *)(tcp_to+1), flow);
        //calculate TCP checksum information
        tcp_to->check=0;
        tcp_to->check = csum_tcpudp_magic(iph_to->saddr, iph_to->daddr, tcpsize, iph_to->protocol, csum_partial((char *)tcp_to, tcpsize, 0));
        skb->ip_summed = CHECKSUM_UNNECESSARY;
        //------------------PUSH ETHERNET AND IP-------------------------

        skb_push(skb, iphsize);
        skb_push(skb, ethsize);

        //------------------Padding-------------------------
        if (skb->len < ETH_ZLEN)
            skb_pad(skb, ETH_ZLEN - skb->len);

        /*if (skb->len < ETH_ZLEN) {
        length = ETH_ZLEN;
        memset(&skb->data + skb->len, 0, length - skb->len);
        } else {
        length = skb->len;
        }
        skb->len = length;*/

        /*if(padsize > 0)
        {
        	//add padding
        	padding = skb_put(skb, padsize);
        	memcpy(padding, junk, padsize);
        }*/

        /*if(debug)
        {
            dev_hold(skb->dev);
            trace_printk("%s:%pI4:%pI4 SKB dev is OK, ack:%u relack:%u \n", skb->dev->name, &iph_to->saddr, &iph_to->daddr, ntohl(tcp_to->ack_seq), flow->relastack);
            dev_put(skb->dev);
        }*/
        //ret = ip_local_out(skb);
        //ret = dev_loopback_xmit(skb);

        ret = dev_queue_xmit(skb);
        if(ret<0)
        {
            if(debug>=1)
                trace_printk(KERN_INFO " Dev Queue XMIT failed: %d \n", ret);
            return 0;
        }
        /*else
        	trace_printk(KERN_INFO " Dev Queue XMIT success: %d %d %d %d %d\n", ret, size, ethsize, iphsize, tcpsize);*/

        return 1;
    }

    return 0;
}
