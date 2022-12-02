/*
 * led.c
 *
 *  Created on: Dec 2, 2022
 *      Author: Deniz Eren
 */

#include <linux/can/led.h>


void can_led_event(struct net_device *netdev, enum can_led_event event)
{
}

void devm_can_led_init(struct net_device *netdev)
{
}

int can_led_notifier_init(void)
{
    return 0;
}

void can_led_notifier_exit(void)
{
}
