#include <iostream>
#include <list>
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <hw/inout.h>

extern "C" {
#include <pci/pci.h>
}

#include "kernel/drivers/net/can/sja1000/sja1000.h"
#include "kernel/include/linux/pci.h"

extern pci_driver adv_pci_driver;
extern pci_driver kvaser_pci_driver;
extern net_device_ops sja1000_netdev_ops;

using namespace std;

/**
 * Command:
 *
 * 		pci-tool -vvvvv
 *
 * Output:
 *
 * B000:D05:F00 @ idx 7
 *         vid/did: 13fe/c302
 *                 <vendor id - unknown>, <device id - unknown>
 *         class/subclass/reg: 0c/09/00
 *                 CANbus Serial Bus Controller
 *         revid: 0
 *         cmd/status registers: 103/0
 *         Capabilities list (0):
 *
 *         Address Space list - 2 assigned
 *             [0] I/O, addr=c000, size=400, align: 400, attr: 32bit CONTIG ENABLED
 *             [1] I/O, addr=c400, size=400, align: 400, attr: 32bit CONTIG ENABLED
 *         Interrupt list - 0 assigned
 *         hdrType: 0
 *                 ssvid: 13fe  ?
 *                 ssid:  c302
 *
 *         Device Dependent Registers
 *                 [40] 00000000  00000000  00000000  00000000
 *                   :
 *                 [f0] 00000000  00000000  00000000  00000000
 *
 */

// NETDEVICE
#include "linux/netdevice.h"

void netif_wake_queue(struct net_device *dev)
{
	syslog(LOG_INFO, "netif_wake_queue");
}

int netif_rx(struct sk_buff *skb)
{
	can_frame* msg = (can_frame*)skb->data;

	syslog(LOG_INFO, "netif_rx; can%d: %x#%2x%2x%2x%2x%2x%2x%2x%2x",
			skb->dev_id,
			msg->can_id,
			msg->data[0],
			msg->data[1],
			msg->data[2],
			msg->data[3],
			msg->data[4],
			msg->data[5],
			msg->data[6],
			msg->data[7]);

	return 0;
}

void netif_start_queue(struct net_device *dev)
{
	syslog(LOG_INFO, "netif_start_queue");
}

void netif_carrier_on(struct net_device *dev)
{
	syslog(LOG_INFO, "netif_carrier_on");
	// Not called from SJA1000 driver
}

void netif_carrier_off(struct net_device *dev)
{
	syslog(LOG_INFO, "netif_carrier_off");
}

bool netif_queue_stopped(const struct net_device *dev)
{
	syslog(LOG_INFO, "netif_queue_stopped");
	return false;
}

void netif_stop_queue(struct net_device *dev)
{
	syslog(LOG_INFO, "netif_stop_queue");
}
// NETDEVICE

// DEV
#include "linux/can/dev.h"

int register_candev(struct net_device *dev) {
	std::snprintf(dev->name, IFNAMSIZ, "can%d", dev->dev_id);

	syslog(LOG_INFO, "register_candev: %s", dev->name);

	if (sja1000_netdev_ops.ndo_open(dev)) {
		return -1;
	}

	return 0;
}

void unregister_candev(struct net_device *dev) {
	syslog(LOG_INFO, "unregister_candev: %s", dev->name);

	if (sja1000_netdev_ops.ndo_stop(dev)) {
		syslog(LOG_ERR, "Internal error; ndo_stop failure");
	}
}

int open_candev(struct net_device *dev)
{
	syslog(LOG_INFO, "open_candev: %s", dev->name);
	return 0;
}

void close_candev(struct net_device *dev)
{
	syslog(LOG_INFO, "close_candev: %s", dev->name);
}
// DEV

// INTERRUPT
#include "linux/interrupt.h"

sigevent event;
int id = -1;

struct func_t {
	irq_handler_t handler;
	net_device *dev;
	unsigned int irq;
};
std::list<func_t> funcs;

const struct sigevent *irq_handler( void *area, int id ) {
	return( &event );
}

extern int
request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)
{
	syslog(LOG_INFO, "request_irq; irq: %d, name: %s", irq, name);

	struct net_device *ndev = (struct net_device *)dev;

	func_t f = {
			.handler = handler,
			.dev = ndev,
			.irq = irq
	};
	funcs.push_back(f);

	if (id == -1) {
		SIGEV_SET_TYPE(&event, SIGEV_INTR);

	    id=InterruptAttach( irq, &irq_handler,
	                        NULL, 0, 0 );

//		if ((id = InterruptAttachEvent(irq, &event, _NTO_INTR_FLAGS_TRK_MSK)) == -1) {
//			syslog(LOG_ERR, "Internal error; interrupt attach event failure");
//		}
	}

	return 0;
}

void free_irq(unsigned int irq, void *dev)
{
	syslog(LOG_INFO, "free_irq; irq: %d", irq);

	// Disconnect the ISR handler
    InterruptDetach(id);
}
// INTERRUPT

// PCI
int pci_enable_device(struct pci_dev *dev) {
	syslog(LOG_INFO, "pci_enable_device");

	uint_t idx = 0;
	pci_bdf_t bdf = 0;

	while ((bdf = pci_device_find(idx, dev->vendor, PCI_DID_ANY, PCI_CCODE_ANY)) != PCI_BDF_NONE)
	{
	    pci_did_t did;
	    pci_err_t r = pci_device_read_did(bdf, &did);

	    if (r == PCI_ERR_OK)
	    {
			syslog(LOG_INFO, "found device: %x:%x", dev->vendor, did);

	    	/* does this driver handle this device ? */
	    	if (did == dev->device) {
	    		pci_devhdl_t hdl = pci_device_attach(bdf, pci_attachFlags_EXCLUSIVE_OWNER, &r);
	    		dev->hdl = hdl;

	            if (hdl == NULL) {
	    			switch (r) {
	    			case PCI_ERR_EINVAL:
	    				syslog(LOG_ERR, "PCI device attach failed; Invalid flags.");
	    				break;
	    			case PCI_ERR_ENODEV:
	    				syslog(LOG_ERR, "PCI device attach failed; The bdf argument doesn't refer to a valid device.");
	    				break;
	    			case PCI_ERR_ATTACH_EXCLUSIVE:
	    				syslog(LOG_ERR, "PCI device attach failed; The device identified by bdf is already exclusively owned.");
	    				break;
	    			case PCI_ERR_ATTACH_SHARED:
	    				syslog(LOG_ERR, "PCI device attach failed; The request was for exclusive attachment, but the device identified by bdf has already been successfully attached to.");
	    				break;
	    			case PCI_ERR_ATTACH_OWNED:
	    				syslog(LOG_ERR, "PCI device attach failed; The request was for ownership of the device, but the device is already owned.");
	    				break;
	    			case PCI_ERR_ENOMEM:
	    				syslog(LOG_ERR, "PCI device attach failed; Memory for internal resources couldn't be obtained. This may be a temporary condition.");
	    				break;
	    			case PCI_ERR_LOCK_FAILURE:
	    				syslog(LOG_ERR, "PCI device attach failed; There was an error related to the creation, acquisition, or use of a synchronization object.");
	    				break;
	    			case PCI_ERR_ATTACH_LIMIT:
	    				syslog(LOG_ERR, "PCI device attach failed; There have been too many attachments to the device identified by bdf.");
	    				break;
	    			default:
	    				syslog(LOG_ERR, "PCI device attach failed; Unknown error: %d", (int)r);
	    				break;
	    			}
	            }
	            else {
	    			syslog(LOG_INFO, "PCI attach successful");

	                /* optionally determine capabilities of device */
	                uint_t capid_idx = 0;
	                pci_capid_t capid;

	                /* instead of looping could use pci_device_find_capid() to select which capabilities to use */
	                while ((r = pci_device_read_capid(bdf, &capid, capid_idx)) == PCI_ERR_OK)
	                {
	        			syslog(LOG_INFO, "found device capability: %x", capid);

	                    /* get next capability ID */
	                    ++capid_idx;
	                }

	                pci_ba_t ba[7];    // the maximum number of entries that can be returned
	                int_t nba = NELEMENTS(ba);

	                /* read the address space information */
	                r = pci_device_read_ba(hdl, &nba, ba, pci_reqType_e_UNSPECIFIED);
	                if ((r == PCI_ERR_OK) && (nba > 0))
	                {
	        			syslog(LOG_INFO, "reading device address space");

	        			dev->nba = nba;
	                	dev->ba = (pci_ba_t*)std::malloc(nba*sizeof(pci_ba_t));

	                    for (int_t i=0; i<nba; i++)
	                    {
	                    	dev->ba[i] = ba[i];

		        			syslog(LOG_INFO, "detected ba[%d] { addr: %x, size: %x }",
		        					i, ba[i].addr, ba[i].size);
	                    }
	                }

	                pci_irq_t irq[10];    // the maximum number of expected entries based on capabilities
	                int_t nirq = NELEMENTS(irq);

	                /* read the irq information */
                    r = pci_device_read_irq(hdl, &nirq, irq);
                    if ((r == PCI_ERR_OK) && (nirq > 0))
                    {
                    	dev->irq = irq[0];

                    	if (nirq > 1) {
                    		syslog(LOG_ERR, "detected multiple (%d) IRQs", nirq);

                    		return -1;
                    	}

                        for (int_t i=0; i<nirq; i++) {
                            //int_t iid = InterruptAttach(irq[i], my_isr, NULL, 0, 0);
        	    			syslog(LOG_INFO, "detected IRQ - irq[%d]: %d", i, irq[i]);
                        }
                    }
	            }
	    	}
	    }

	    /* get next device instance */
	    ++idx;
	}

	return 0;
}

void pci_disable_device(struct pci_dev *dev) {
	syslog(LOG_INFO, "pci_disable_device");

	if (dev != nullptr) {
		if (dev->hdl != nullptr) {
			pci_device_detach(dev->hdl);
		}
	}
}

uintptr_t pci_iomap(struct pci_dev *dev, int bar, unsigned long max) {
	syslog(LOG_INFO, "pci_iomap; bar: %d, max: %d", bar, max);

	if (bar >= dev->nba) {
		syslog(LOG_ERR, "Internal error; bar: %d, nba: %d", bar, dev->nba);
	}

	if (dev->ba[bar].size > max) {
		dev->ba[bar].size = max;
	}

	/* mmap() the address space(s) */

	if (mmap_device_io(dev->ba[bar].size, dev->ba[bar].addr) == MAP_DEVICE_FAILED) {
		switch (errno) {
		case EINVAL:
			syslog(LOG_ERR, "PCI device address mapping failed; Invalid flags type, or len is 0.");
			break;
		case ENOMEM:
			syslog(LOG_ERR, "PCI device address mapping failed; The address range requested is outside of the allowed process address range, or there wasn't enough memory to satisfy the request.");
			break;
		case ENXIO:
			syslog(LOG_ERR, "PCI device address mapping failed; The address from io for len bytes is invalid.");
			break;
		case EPERM:
			syslog(LOG_ERR, "PCI device address mapping failed; The calling process doesn't have the required permission; see procmgr_ability().");
			break;
		default:
			syslog(LOG_ERR, "PCI device address mapping failed; Unknown error: %d", errno);
			break;
		}
	}
	else {
		syslog(LOG_INFO, "ba[%d] mapping successful", bar);
	}

	return dev->ba[bar].addr;
}

void pci_iounmap(struct pci_dev *dev, uintptr_t p) {
	int bar = -1;
	uint64_t size = 0u;

	for (int i = 0; i < dev->nba; ++i) {
		if (dev->ba[i].addr == p) {
			bar = i;
			size = dev->ba[i].size;
			break;
		}
	}

	syslog(LOG_INFO, "pci_iounmap; bar: %d, size: %d", bar, size);

	if (bar == -1 || !size) {
		syslog(LOG_ERR, "Internal error; bar size failure during pci_iounmap");
	}

	if (munmap_device_io(p, size) == -1) {
		syslog(LOG_ERR, "Internal error; munmap_device_io failure");
	}
}

int pci_request_regions(struct pci_dev *, const char *) {
	syslog(LOG_INFO, "pci_request_regions");

	return 0;
}

void pci_release_regions(struct pci_dev *) {
	syslog(LOG_INFO, "pci_release_regions");
}
// PCI

void print_card (const pci_driver& driver) {
	std::cout << "  Name of driver: " << driver.name << std::endl;
	std::cout << "  Supported cards:" << std::endl;

	if (driver.id_table != nullptr) {
		const pci_device_id *id_table = driver.id_table;

		while (id_table->vendor != 0) {
			std::cout << std::hex << "    { "
					<< "vendor: " << id_table->vendor
					<< ", devide: " << id_table->device
					<< ", subvendor: " << id_table->subvendor
					<< ", subdevice: " << id_table->subdevice
					<< ", class: " << id_table->class_
					<< ", class_mask: " << id_table->class_mask
					<< " }" << std::endl;
			++id_table;
		}
	}
}

int main(void) {
	syslog(LOG_INFO, "start");

	ThreadCtl(_NTO_TCTL_IO, 0);

	std::cout << "Advantech CAN cards:" << std::endl;
	print_card(adv_pci_driver);

	std::cout << "Kvaser CAN cards:" << std::endl;
	print_card(kvaser_pci_driver);

	pci_dev pdev = {
			.ba = nullptr,
			.vendor = 0x13fe,
			.device = 0xc302,
			.dev = { .driver_data = nullptr },
			.irq = 10
	};

	if (adv_pci_driver.probe(&pdev, nullptr)) {
		return -1;
	}

	while (1) {
		InterruptWait( 0, NULL );

		for (auto it = funcs.begin(); it != funcs.end(); ++it) {
			it->handler(it->irq, it->dev);
		}
	}

	adv_pci_driver.remove(&pdev);

	return EXIT_SUCCESS;
}
