/*-
 * Copyright (c) 2009-2012,2016-2017 Microsoft Corp.
 * Copyright (c) 2012 NetApp Inc.
 * Copyright (c) 2012 Citrix Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * VM Bus Driver Implementation
 */
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/linker.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/sbuf.h>
#include <sys/smp.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/taskqueue.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>

#include <machine/bus.h>
#include <machine/metadata.h>
#include <machine/md_var.h>
#include <machine/resource.h>
#include <contrib/dev/acpica/include/acpi.h>
#include <dev/acpica/acpivar.h>

#include <dev/hyperv/include/hyperv.h>
#include <dev/hyperv/include/vmbus_xact.h>
#include <dev/hyperv/vmbus/hyperv_var.h>
#include <dev/hyperv/vmbus/vmbus_reg.h>
#include <dev/hyperv/vmbus/vmbus_var.h>
#include <dev/hyperv/vmbus/vmbus_chanvar.h>
#if defined(__aarch64__)
#include <dev/psci/smccc.h>
#include <dev/hyperv/vmbus/aarch64/hyperv_machdep.h>
#include <dev/hyperv/vmbus/aarch64/hyperv_reg.h>
#else
#include <x86/include/apicvar.h>
#include <dev/hyperv/vmbus/amd64/hyperv_machdep.h>
#include <dev/hyperv/vmbus/amd64/hyperv_reg.h>
#include <machine/intr_machdep.h>
#endif
#include "acpi_if.h"
#include "pcib_if.h"
#include "vmbus_if.h"

#define VMBUS_GPADL_START		0xe1e10

struct vmbus_msghc {
	struct vmbus_xact		*mh_xact;
	struct hypercall_postmsg_in	mh_inprm_save;
};

static void			vmbus_identify(driver_t *, device_t);
static int			vmbus_probe(device_t);
static int			vmbus_attach(device_t);
static int			vmbus_detach(device_t);
static int			vmbus_read_ivar(device_t, device_t, int,
				    uintptr_t *);
static int			vmbus_child_pnpinfo(device_t, device_t, struct sbuf *);
static struct resource		*vmbus_alloc_resource(device_t dev,
				    device_t child, int type, int *rid,
				    rman_res_t start, rman_res_t end,
				    rman_res_t count, u_int flags);
static int			vmbus_alloc_msi(device_t bus, device_t dev,
				    int count, int maxcount, int *irqs);
static int			vmbus_release_msi(device_t bus, device_t dev,
				    int count, int *irqs);
static int			vmbus_alloc_msix(device_t bus, device_t dev,
				    int *irq);
static int			vmbus_release_msix(device_t bus, device_t dev,
				    int irq);
static int			vmbus_map_msi(device_t bus, device_t dev,
				    int irq, uint64_t *addr, uint32_t *data);
static uint32_t			vmbus_get_version_method(device_t, device_t);
static int			vmbus_probe_guid_method(device_t, device_t,
				    const struct hyperv_guid *);
static uint32_t			vmbus_get_vcpu_id_method(device_t bus,
				    device_t dev, int cpu);
static struct taskqueue		*vmbus_get_eventtq_method(device_t, device_t,
				    int);
#if defined(EARLY_AP_STARTUP) || defined(__aarch64__)
static void			vmbus_intrhook(void *);
#endif

static int			vmbus_init(struct vmbus_softc *);
static int			vmbus_connect(struct vmbus_softc *, uint32_t);
static int			vmbus_req_channels(struct vmbus_softc *sc);
static void			vmbus_disconnect(struct vmbus_softc *);
static int			vmbus_scan(struct vmbus_softc *);
static void			vmbus_scan_teardown(struct vmbus_softc *);
static void			vmbus_scan_done(struct vmbus_softc *,
				    const struct vmbus_message *);
static void			vmbus_chanmsg_handle(struct vmbus_softc *,
				    const struct vmbus_message *);
static void			vmbus_msg_task(void *, int);
static void			vmbus_synic_setup(void *);
static void			vmbus_synic_teardown(void *);
static int			vmbus_sysctl_version(SYSCTL_HANDLER_ARGS);
static int			vmbus_dma_alloc(struct vmbus_softc *);
static void			vmbus_dma_free(struct vmbus_softc *);
static int			vmbus_intr_setup(struct vmbus_softc *);
static void			vmbus_intr_teardown(struct vmbus_softc *);
static int			vmbus_doattach(struct vmbus_softc *);
static void			vmbus_event_proc_dummy(struct vmbus_softc *,
				    int);
static int			vmbus_handle_intr_new(void *);


void	vmbus_handle_timer_intr1(struct vmbus_message *msg_base,
		struct trapframe *frame);
void	vmbus_synic_setup1(void *xsc);
void	vmbus_synic_teardown1(void);
int		vmbus_setup_intr1(struct vmbus_softc *sc);
void	vmbus_intr_teardown1(struct vmbus_softc *sc);

void
vmbus_handle_timer_intr1(struct vmbus_message *msg_base, struct trapframe *frame)
{
	// do nothing for arm64, as we are using generic timer
	return;
}
static int
vmbus_handle_intr_new(void *arg)
{
	vmbus_handle_intr(NULL);
	return(FILTER_HANDLED);
}


void
vmbus_synic_setup1(void *xsc)
{
	return;
}

void
vmbus_synic_teardown1(void)
{
	return;
}


int
vmbus_setup_intr1(struct vmbus_softc *sc)
{
	int err;
	struct intr_map_data_acpi *irq_data;

	sc->ires = bus_alloc_resource_any(device_get_parent(sc->vmbus_dev),
				SYS_RES_IRQ, &sc->vector, RF_ACTIVE | RF_SHAREABLE);
	if (sc->ires == NULL) {
		device_printf(sc->vmbus_dev,
			"bus_alloc_resouce_any failed\n");
		return (ENXIO);
	} else {
		device_printf(sc->vmbus_dev,
			"irq 0x%lx, vector %d end 0x%lx\n",
		(uint64_t)rman_get_start(sc->ires), sc->vector, (uint64_t)rman_get_end(sc->ires));
	}
	err = bus_setup_intr(sc->vmbus_dev, sc->ires, INTR_TYPE_MISC ,
							 vmbus_handle_intr_new, NULL,  sc, &sc->icookie);
	if (err) {
		device_printf(sc->vmbus_dev, "failed to setup IRQ %d\n",err);
		return (err);
	}
	device_printf(sc->vmbus_dev, "vmbus	IRQ is set\n");
	irq_data = (struct intr_map_data_acpi *) rman_get_virtual(sc->ires);
	device_printf(sc->vmbus_dev,"the irq %u\n",irq_data->irq); 
	sc->vmbus_idtvec = irq_data->irq;
	return 0;
}

void
vmbus_intr_teardown1(struct vmbus_softc *sc)
{
	int cpu;
	
	sc->vmbus_idtvec = -1;
	bus_teardown_intr(sc->vmbus_dev, sc->ires, sc->icookie);
	
	CPU_FOREACH(cpu) {
		if (VMBUS_PCPU_GET(sc, event_tq, cpu) != NULL) {
			taskqueue_free(VMBUS_PCPU_GET(sc, event_tq, cpu));
			VMBUS_PCPU_GET(sc, event_tq, cpu) = NULL;
		}
		if (VMBUS_PCPU_GET(sc, message_tq, cpu) != NULL) {
			taskqueue_drain(VMBUS_PCPU_GET(sc, message_tq, cpu),
			    VMBUS_PCPU_PTR(sc, message_task, cpu));
			taskqueue_free(VMBUS_PCPU_GET(sc, message_tq, cpu));
			VMBUS_PCPU_GET(sc, message_tq, cpu) = NULL;
		}
	}
}
