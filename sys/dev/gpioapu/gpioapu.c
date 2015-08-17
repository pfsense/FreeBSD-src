/*-
 * Copyright (c) 2013 Ermal Luci <eri@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/priv.h>
#include <sys/bus.h>
#include <machine/bus.h>
#include <sys/rman.h>
#include <machine/resource.h>
#include <sys/proc.h>
#include <sys/uio.h>

#include <dev/pci/pcivar.h>
#include <isa/isavar.h>

/* SB7xx RRG 2.3.3.1.1. */
#define	AMDSB_PMIO_INDEX		0xcd6
#define	AMDSB_PMIO_DATA			(PMIO_INDEX + 1)
#define	AMDSB_PMIO_WIDTH		2

#define AMDSB_SMBUS_DEVID               0x43851002
#define AMDSB8_SMBUS_REVID              0x40

#define IOMUX_OFFSET  0xD00
#define GPIO_SPACE_OFFSET 0x100 
#define GPIO_SPACE_SIZE 0x100
/* SB8xx RRG 2.3.3. */
#define	AMDSB8_PM_WDT_EN		0x24

#define GPIO_187      187	// MODESW
#define GPIO_189      189	// LED1#
#define GPIO_190      190	// LED2#
#define GPIO_191      191	// LED3#
#define GPIO_OUTPUT	  0x08
#define GPIO_INPUT    0x28

struct gpioapu_softc {
	device_t		dev;
	struct cdev		*cdev;
	struct resource		*res_ctrl;
	struct resource		*res_count;
	int			rid_ctrl;
	int			rid_count;
};

static int	gpioapu_open(struct cdev *dev, int flags, int fmt,
			struct thread *td);
static int	gpioapu_close(struct cdev *dev, int flags, int fmt,
			struct thread *td);
static int	gpioapu_write(struct cdev *dev, struct uio *uio, int ioflag);
static int	gpioapu_read(struct cdev *dev, struct uio *uio, int ioflag);

static struct cdevsw gpioapu_cdevsw = {
	.d_version =    D_VERSION,
	.d_open =       gpioapu_open,
	.d_read	=	gpioapu_read,
	.d_write = 	gpioapu_write,
	.d_close =      gpioapu_close,
	.d_name =       "gpioapu",
};

static void	gpioapu_identify(driver_t *driver, device_t parent);
static int	gpioapu_probe(device_t dev);
static int	gpioapu_attach(device_t dev);
static int	gpioapu_detach(device_t dev);

static device_method_t gpioapu_methods[] = {
	DEVMETHOD(device_identify,	gpioapu_identify),
	DEVMETHOD(device_probe,		gpioapu_probe),
	DEVMETHOD(device_attach,	gpioapu_attach),
	DEVMETHOD(device_detach,	gpioapu_detach),
	{0, 0}
};

static devclass_t	gpioapu_devclass;

static driver_t		gpioapu_driver = {
	"gpioapu",
	gpioapu_methods,
	sizeof(struct gpioapu_softc)
};

DRIVER_MODULE(gpioapu, isa, gpioapu_driver, gpioapu_devclass, NULL, NULL);


static uint8_t
pmio_read(struct resource *res, uint8_t reg)
{
	bus_write_1(res, 0, reg);	/* Index */
	return (bus_read_1(res, 1));	/* Data */
}

#if 0
static void
pmio_write(struct resource *res, uint8_t reg, uint8_t val)
{
	bus_write_1(res, 0, reg);	/* Index */
	bus_write_1(res, 1, val);	/* Data */
}
#endif

/* ARGSUSED */
static int
gpioapu_open(struct cdev *dev __unused, int flags __unused, int fmt __unused,
    struct thread *td)
{
	int error;

	error = priv_check(td, PRIV_IO);
	if (error != 0)
		return (error);
	error = securelevel_gt(td->td_ucred, 0);
	if (error != 0)
		return (error);

	return (error);
}

static int
gpioapu_read(struct cdev *dev, struct uio *uio, int ioflag) {
	struct gpioapu_softc *sc = dev->si_drv1;
	uint8_t tmp;
        char ch;
        int error;

	tmp = bus_read_1(sc->res_ctrl, GPIO_187);
#ifdef DEBUG
	device_printf(sc->dev, "returned %x\n", (u_int)tmp);
#endif
	if (tmp & 0x80)
		ch = '1';
	else
		ch = '0';

	error = uiomove(&ch, sizeof(ch), uio);

	return (error);
}
static int
gpioapu_write(struct cdev *dev, struct uio *uio, int ioflag) {
        struct gpioapu_softc *sc = dev->si_drv1;
        char ch[3];
        uint8_t old;
        int error, i, start;

        error = uiomove(ch, sizeof(ch), uio);
        if (error)
                return (error);

	start = GPIO_189;
	for (i = 0; i < 3; i++) {
		old = bus_read_1(sc->res_ctrl, start + i);
#ifdef DEBUG
		device_printf(sc->dev, "returned %x - %c\n", (u_int)old, ch[i]);
#endif
		if (ch[i] == '1')
			old &= 0x80;
		else
			old = 0xc8;
		bus_write_1(sc->res_ctrl, start + i, old);
	}

	return (error);
}

static int
gpioapu_close(struct cdev *dev __unused, int flags __unused, int fmt __unused,
    struct thread *td)
{
        struct gpioapu_softc *sc = dev->si_drv1;
        int i, start;

	start = GPIO_187;
	for (i = 0; i < 2; i++) {
		bus_write_1(sc->res_ctrl, start, 0xc8);
	}

	return (0);
}

static void
gpioapu_identify(driver_t *driver, device_t parent)
{
	device_t		child;
	device_t		smb_dev;

	if (resource_disabled("gpioapu", 0))
		return;

	
	if (device_find_child(parent, "gpioapu", -1) != NULL)
		return;

	/*
	 * Try to identify SB600/SB7xx by PCI Device ID of SMBus device
	 * that should be present at bus 0, device 20, function 0.
	 */
	smb_dev = pci_find_bsf(0, 20, 0);
	if (smb_dev == NULL)
		return;

	if (pci_get_devid(smb_dev) != AMDSB_SMBUS_DEVID)
		return;

	child = BUS_ADD_CHILD(parent, ISA_ORDER_SPECULATIVE, "gpioapu", -1);
	if (child == NULL)
		device_printf(parent, "add gpioapu child failed\n");
}

static int
gpioapu_probe(device_t dev)
{
	struct resource		*res;
	uint32_t		addr;
	int			rid;
	int			rc, i;
	char *value;

	value = getenv("smbios.system.product");
	device_printf(dev, "Environment returned %s\n", value);
	if (value == NULL || strncmp(value, "APU", strlen("APU")))
		return (ENXIO);

	/* Do not claim some ISA PnP device by accident. */
	if (isa_get_logicalid(dev) != 0)
		return (ENXIO);

	rc = bus_set_resource(dev, SYS_RES_IOPORT, 0, AMDSB_PMIO_INDEX,
	    AMDSB_PMIO_WIDTH);
	if (rc != 0) {
		device_printf(dev, "bus_set_resource for IO failed\n");
		return (ENXIO);
	}
	rid = 0;
	res = bus_alloc_resource(dev, SYS_RES_IOPORT, &rid, 0ul, ~0ul,
	    AMDSB_PMIO_WIDTH, RF_ACTIVE | RF_SHAREABLE);
	if (res == NULL) {
		device_printf(dev, "bus_alloc_resource for IO failed\n");
		return (ENXIO);
	}

	/* Find base address of memory mapped WDT registers. */
	for (addr = 0, i = 0; i < 4; i++) {
		addr <<= 8;
		addr |= pmio_read(res, AMDSB8_PM_WDT_EN + 3 - i);
	}
	addr &= 0xFFFFF000;
	device_printf(dev, "Address on reg 0x24 is 0x%x/%u\n", addr, addr);

	bus_release_resource(dev, SYS_RES_IOPORT, rid, res);
	bus_delete_resource(dev, SYS_RES_IOPORT, rid);

	rc = bus_set_resource(dev, SYS_RES_MEMORY, 0, addr + GPIO_SPACE_OFFSET,
	    GPIO_SPACE_SIZE);
	if (rc != 0) {
		device_printf(dev, "bus_set_resource for memory failed\n");
		return (ENXIO);
	}

	return (0);
}

static int
gpioapu_attach(device_t dev)
{
	struct gpioapu_softc	*sc;

	sc = device_get_softc(dev);
	sc->dev = dev;
	sc->rid_ctrl = 0;

	sc->res_ctrl = bus_alloc_resource(dev, SYS_RES_MEMORY, &sc->rid_ctrl, 0ul, ~0ul,
	    GPIO_SPACE_SIZE, RF_ACTIVE | RF_SHAREABLE);
	if (sc->res_ctrl == NULL) {
		device_printf(dev, "bus_alloc_resource for memory failed\n");
		return (ENXIO);
	}

	sc->dev = dev;
	sc->cdev = make_dev(&gpioapu_cdevsw, 0,
  			UID_ROOT, GID_WHEEL, 0600, "gpioapu");

	sc->cdev->si_drv1 = sc;

	return (0);

}

static int
gpioapu_detach(device_t dev)
{
	struct gpioapu_softc *sc;

	sc = device_get_softc(dev);

	if (sc->res_ctrl != NULL) {
		bus_release_resource(dev, SYS_RES_MEMORY, sc->rid_ctrl,
		    sc->res_ctrl);
		bus_delete_resource(dev, SYS_RES_MEMORY, sc->rid_ctrl);
	}

	destroy_dev(sc->cdev);

	return (0);
}
