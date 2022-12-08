#include <linux/skbuff.h>

void kfree_skb (struct sk_buff* skb) {
    // TODO: Check if any of the Linux implementation house-keeping is needed
    fixed_free(skb->data);
    fixed_free(skb);
}
