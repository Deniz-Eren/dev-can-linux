/*
 * interrupt.c
 *
 *  Created on: Dec 2, 2022
 *      Author: Deniz Eren
 */

#include <linux/interrupt.h>

struct sigevent event;
int id = -1;

struct func_t {
	irq_handler_t handler;
	struct net_device *dev;
	unsigned int irq;
};

struct func_t funcs[16];
int funcs_size = 0;

const struct sigevent *irq_handler( void *area, int id ) {
	return( &event );
}

extern int
request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)
{
	log_trace("request_irq; irq: %d, name: %s\n", irq, name);

	struct net_device *ndev = (struct net_device *)dev;

	struct func_t f = {
			.handler = handler,
			.dev = ndev,
			.irq = irq
	};

	funcs[funcs_size++] = f;

	if (id == -1) {
		SIGEV_SET_TYPE(&event, SIGEV_INTR);

	    if ((id = InterruptAttach(irq, &irq_handler, NULL, 0, 0)) == -1) {
	    	log_err("internal error; interrupt attach failure\n");
	    }
	}

	return 0;
}

void free_irq(unsigned int irq, void *dev)
{
	log_trace("free_irq; irq: %d\n", irq);

	// Disconnect the ISR handler
    InterruptDetach(id);
}

void run_interrupt_wait()
{
	while (1) {
		int i;
		InterruptWait( 0, NULL );

		for (i = 0; i < funcs_size; ++i) {
			funcs[i].handler(funcs[i].irq, funcs[i].dev);
		}
	}
}
