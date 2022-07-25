/*-
 * Copyright (c) 2017 Microsoft Corp.
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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/module.h>

#include <contrib/dev/acpica/include/acpi.h>
#include <dev/acpica/acpivar.h>

#include <dev/hyperv/include/hyperv.h>

#include "acpi_if.h"
#include "bus_if.h"
#include "pcib_if.h"

static int		vmbus_res_probe(device_t);
static int		vmbus_res_attach(device_t);
static int		vmbus_res_detach(device_t);
static int          acpi_syscont_alloc_msi(device_t, device_t,
                    int count, int maxcount, int *irqs);
static int          acpi_syscont_release_msi(device_t bus, device_t dev,
                    int count, int *irqs);
static int          acpi_syscont_alloc_msix(device_t bus, device_t dev,
                    int *irq);
static int          acpi_syscont_release_msix(device_t bus, device_t dev,
                    int irq);
static int          acpi_syscont_map_msi(device_t bus, device_t dev,
                    int irq, uint64_t *addr, uint32_t *data);

static device_method_t vmbus_res_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,			vmbus_res_probe),
	DEVMETHOD(device_attach,		vmbus_res_attach),
	DEVMETHOD(device_detach,		vmbus_res_detach),
	DEVMETHOD(device_shutdown,		bus_generic_shutdown),
	DEVMETHOD(device_suspend,		bus_generic_suspend),
	DEVMETHOD(device_resume,		bus_generic_resume),
	DEVMETHOD(bus_setup_intr,		bus_generic_setup_intr),
	DEVMETHOD(bus_alloc_resource,	bus_generic_alloc_resource),
    /* pcib interface */
    DEVMETHOD(pcib_alloc_msi,       acpi_syscont_alloc_msi),
    DEVMETHOD(pcib_release_msi,     acpi_syscont_release_msi),
    DEVMETHOD(pcib_alloc_msix,      acpi_syscont_alloc_msix),
    DEVMETHOD(pcib_release_msix,    acpi_syscont_release_msix),
    DEVMETHOD(pcib_map_msi,     acpi_syscont_map_msi),
	DEVMETHOD_END
};

static driver_t vmbus_res_driver = {
	"vmbus_res",
	vmbus_res_methods,
	1
};

DRIVER_MODULE(vmbus_res, acpi, vmbus_res_driver, NULL, NULL);
MODULE_DEPEND(vmbus_res, acpi, 1, 1, 1);
MODULE_VERSION(vmbus_res, 1);

static int
vmbus_res_probe(device_t dev)
{
	char *id[] = { "VMBUS", NULL };
	int rv;
	device_printf(dev, "vmbus res probe called\n");	/*
	if (device_get_unit(dev) != 0 || vm_guest != VM_GUEST_HV ||
	    (hyperv_features & CPUID_HV_MSR_SYNIC) == 0)
		return (ENXIO);*/
	rv = ACPI_ID_PROBE(device_get_parent(dev), dev, id, NULL);
	if (rv <= 0)
		device_set_desc(dev, "Hyper-V Vmbus Resource");
	return (rv);
}

static int
vmbus_res_attach(device_t dev __unused)
{
#if 1
	bus_generic_probe(dev);
	bus_generic_attach(dev);
#endif
	return (0);
}

static int
vmbus_res_detach(device_t dev __unused)
{

	return (0);
}

static int
acpi_syscont_alloc_msi(device_t bus, device_t dev, int count, int maxcount,
    int *irqs)
{
    device_t parent = device_get_parent(bus);

    return (PCIB_ALLOC_MSI(device_get_parent(parent), dev, count, maxcount,
    irqs));
}

static int
acpi_syscont_release_msi(device_t bus, device_t dev, int count, int *irqs)
{
    device_t parent = device_get_parent(bus);

    return (PCIB_RELEASE_MSI(device_get_parent(parent), dev, count, irqs));
}

static int
acpi_syscont_alloc_msix(device_t bus, device_t dev, int *irq)
{
    device_t parent = device_get_parent(bus);

    return (PCIB_ALLOC_MSIX(device_get_parent(parent), dev, irq));
}

static int
acpi_syscont_release_msix(device_t bus, device_t dev, int irq)
{
    device_t parent = device_get_parent(bus);

    return (PCIB_RELEASE_MSIX(device_get_parent(parent), dev, irq));
}

static int
acpi_syscont_map_msi(device_t bus, device_t dev, int irq, uint64_t *addr,
    uint32_t *data)
{
    device_t parent = device_get_parent(bus);

    return (PCIB_MAP_MSI(device_get_parent(parent), dev, irq, addr, data));
}
