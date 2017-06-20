/*-
 * Copyright (c) 2017 Rubicon Communications, LLC (Netgate)
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
__FBSDID("$FreeBSD$");

/*
 * Driver for the NXP PCA9552 - I2C LED driver with programmable blink rates.
 */

#include "opt_platform.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/gpio.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/sysctl.h>

#include <dev/iicbus/iicbus.h>
#include <dev/iicbus/iiconf.h>

#include <dev/gpio/gpiobusvar.h>
#ifdef FDT
#include <dev/ofw/ofw_bus.h>
#endif

#include <dev/iicbus/pca9552reg.h>

#include "gpio_if.h"
#include "iicbus_if.h"

#define	PCA9552_GPIO_PINS	16
#define	PCA9552_GPIO_CAPS	GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |	\
    GPIO_PIN_OPENDRAIN

struct pca9552_softc {
	device_t	sc_dev;
	device_t	sc_gpio_busdev;
	uint16_t	sc_addr;
};

static int
pca9552_read(device_t dev, uint16_t addr, uint8_t ctrl, uint8_t *data,
    size_t len)
{
	struct iic_msg msg[2] = {
	    { addr, IIC_M_WR | IIC_M_NOSTOP, 1, &ctrl },
	    { addr, IIC_M_RD, len, data },
	};

	return (iicbus_transfer(dev, msg, nitems(msg)));
}

static int
pca9552_write(device_t dev, uint16_t addr, uint8_t *data, size_t len)
{
	struct iic_msg msg[1] = {
	    { addr, IIC_M_WR, len, data },
	};

	return (iicbus_transfer(dev, msg, nitems(msg)));
}

static int
pca9552_probe(device_t dev)
{
#ifdef FDT
	phandle_t node;

	if (!ofw_bus_status_okay(dev))
		return (ENXIO);
	if (!ofw_bus_is_compatible(dev, "nxp,pca9552"))
		return (ENXIO);
	node = ofw_bus_get_node(dev);
	if (!OF_hasprop(node, "gpio-controller"))
		/* Node is not a GPIO controller. */
		return (ENXIO);
#endif
	device_set_desc(dev, "NXP PCA9552 LED driver");

	return (BUS_PROBE_DEFAULT);
}

static int
pca9552_period_sysctl(SYSCTL_HANDLER_ARGS)
{
	int error, new;
	struct pca9552_softc *sc;
	uint8_t data[2], psc;

	sc = (struct pca9552_softc *)arg1;
	error = pca9552_read(sc->sc_dev, sc->sc_addr, PCA9552_PSC(arg2), &psc,
	    sizeof(psc));
	if (error != 0)
		return (error);

	new = ((((int)psc) + 1) * 1000) / 44;
	error = sysctl_handle_int(oidp, &new, sizeof(new), req);
	if (error != 0 || req->newptr == NULL)
		return (error);
	new = ((new * 44) / 1000) - 1;
	if (new != psc && new >= 0 && new <= 255) {
		data[0] = PCA9552_PSC(arg2);
		data[1] = new;
		error = pca9552_write(sc->sc_dev, sc->sc_addr, data,
		    sizeof(data));
		if (error != 0)
			return (error);
	}

	return (error);
}

static int
pca9552_duty_sysctl(SYSCTL_HANDLER_ARGS)
{
	int error, new;
	struct pca9552_softc *sc;
	uint8_t data[2], duty;

	sc = (struct pca9552_softc *)arg1;
	error = pca9552_read(sc->sc_dev, sc->sc_addr, PCA9552_PWM(arg2), &duty,
	    sizeof(duty));
	if (error != 0)
		return (error);

	new = duty;
	error = sysctl_handle_int(oidp, &new, sizeof(new), req);
	if (error != 0 || req->newptr == NULL)
		return (error);
	if (new != duty && new >= 0 && new <= 255) {
		data[0] = PCA9552_PWM(arg2);
		data[1] = new;
		error = pca9552_write(sc->sc_dev, sc->sc_addr, data,
		    sizeof(data));
		if (error != 0)
			return (error);
	}

	return (error);
}

static int
pca9552_attach(device_t dev)
{
	char pwmbuf[4];
	int i;
	struct pca9552_softc *sc;
	struct sysctl_ctx_list *ctx;
	struct sysctl_oid *pwm_node, *pwmN_node, *tree_node;
	struct sysctl_oid_list *pwm_tree, *pwmN_tree, *tree;
	uint8_t data[2];

	sc = device_get_softc(dev);
	sc->sc_dev = dev;
	sc->sc_addr = iicbus_get_addr(dev);

	ctx = device_get_sysctl_ctx(dev);
	tree_node = device_get_sysctl_tree(dev);
	tree = SYSCTL_CHILDREN(tree_node);

	/* Reset output. */
	for (i = 0; i < 4; i++) {
		data[0] = PCA9552_LS(i * 4);
		data[1] = 0x55;
		if (pca9552_write(dev, sc->sc_addr, data, sizeof(data)) != 0)
			return (ENXIO);
	}

	/* Attach gpiobus. */
	sc->sc_gpio_busdev = gpiobus_attach_bus(dev);
	if (sc->sc_gpio_busdev == NULL)
		return (ENXIO);

	pwm_node = SYSCTL_ADD_NODE(ctx, tree, OID_AUTO, "pwm",
	    CTLFLAG_RD, NULL, "PWM settings");
	pwm_tree = SYSCTL_CHILDREN(pwm_node);

	for (i = 0; i < 2; i++) {
		snprintf(pwmbuf, sizeof(pwmbuf), "%d", i);
		pwmN_node = SYSCTL_ADD_NODE(ctx, pwm_tree, OID_AUTO, pwmbuf,
		    CTLFLAG_RD, NULL, "PWM settings");
		pwmN_tree = SYSCTL_CHILDREN(pwmN_node);

		SYSCTL_ADD_PROC(ctx, pwmN_tree, OID_AUTO, "period",
		    CTLFLAG_RW | CTLTYPE_UINT | CTLFLAG_MPSAFE, sc, i,
		    pca9552_period_sysctl, "IU", "PCA9552 PWM period (in ms)");
		SYSCTL_ADD_PROC(ctx, pwmN_tree, OID_AUTO, "duty",
		    CTLFLAG_RW | CTLTYPE_UINT | CTLFLAG_MPSAFE, sc, i,
		    pca9552_duty_sysctl, "IU", "PCA9552 PWM duty cycle");
	}

	return (0);
}

static device_t
pca9552_gpio_get_bus(device_t dev)
{
	struct pca9552_softc *sc;

	sc = device_get_softc(dev);

	return (sc->sc_gpio_busdev);
}

static int
pca9552_gpio_pin_max(device_t dev, int *maxpin)
{

	*maxpin = PCA9552_GPIO_PINS - 1;

	return (0);
}

static int
pca9552_gpio_pin_getname(device_t dev, uint32_t pin, char *name)
{

	if (pin >= PCA9552_GPIO_PINS)
		return (EINVAL);

	memset(name, 0, GPIOMAXNAME);
	snprintf(name, GPIOMAXNAME, "LED %d", pin);

	return (0);
}

static int
pca9552_gpio_pin_getcaps(device_t dev, uint32_t pin, uint32_t *caps)
{

	if (pin >= PCA9552_GPIO_PINS)
		return (EINVAL);

	*caps = PCA9552_GPIO_CAPS;

	return (0);
}

static int
pca9552_gpio_pin_getflags(device_t dev, uint32_t pin, uint32_t *flags)
{
	struct pca9552_softc *sc;
	uint8_t	curr;

	if (pin >= PCA9552_GPIO_PINS)
		return (EINVAL);

	sc = device_get_softc(dev);
	if (pca9552_read(dev, sc->sc_addr, PCA9552_LS(pin), &curr,
	    sizeof(curr)) != 0)
		return (ENXIO);

	switch ((curr >> PCA9552_LS_SHIFT(pin)) & 0x3) {
	case 0:
		*flags = GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN;
		break;
	case 1:
		*flags = GPIO_PIN_INPUT;
		break;
	default:
		*flags = 0;
	}

	return (0);
}

static int
pca9552_gpio_pin_setflags(device_t dev, uint32_t pin, uint32_t flags)
{
	struct pca9552_softc *sc;
	uint8_t	curr, new[2];

	if (pin >= PCA9552_GPIO_PINS)
		return (EINVAL);
	if ((flags & (GPIO_PIN_INPUT | GPIO_PIN_OUTPUT)) == 0)
		return (0);

	sc = device_get_softc(dev);
	if (pca9552_read(dev, sc->sc_addr, PCA9552_LS(pin), &curr,
	    sizeof(curr)) != 0)
		return (ENXIO);

	curr &= ~(0x3 << PCA9552_LS_SHIFT(pin));
	if ((flags & GPIO_PIN_INPUT) != 0)
		curr |= (0x1 << PCA9552_LS_SHIFT(pin));
	new[0] = PCA9552_LS(pin);
	new[1] = curr;
	if (pca9552_write(dev, sc->sc_addr, new, sizeof(new)) != 0)
		return (ENXIO);

	return (0);
}

static int
pca9552_gpio_pin_set(device_t dev, uint32_t pin, unsigned int value)
{
	struct pca9552_softc *sc;
	uint8_t	curr, new[2];

	if (pin >= PCA9552_GPIO_PINS)
		return (EINVAL);

	sc = device_get_softc(dev);
	if (pca9552_read(dev, sc->sc_addr, PCA9552_LS(pin), &curr,
	    sizeof(curr)) != 0)
		return (ENXIO);

	curr &= ~(0x3 << PCA9552_LS_SHIFT(pin));
	if (value != 0)
		curr |= (0x1 << PCA9552_LS_SHIFT(pin));
	new[0] = PCA9552_LS(pin);
	new[1] = curr;
	if (pca9552_write(dev, sc->sc_addr, new, sizeof(new)) != 0)
		return (ENXIO);

	return (0);
}

static int
pca9552_gpio_pin_get(device_t dev, uint32_t pin, unsigned int *val)
{
	struct pca9552_softc *sc;
	uint8_t data;

	if (pin >= PCA9552_GPIO_PINS)
		return (EINVAL);

	sc = device_get_softc(dev);
	if (pca9552_read(dev, sc->sc_addr, PCA9552_INPUT(pin), &data,
	    sizeof(data)) != 0)
		return (ENXIO);

	*val = ((data & (1 << (pin % 8))) != 0) ? 1 : 0;

	return (0);
}

static int
pca9552_gpio_pin_toggle(device_t dev, uint32_t pin)
{
	unsigned int val;

	if (pca9552_gpio_pin_get(dev, pin, &val) != 0)
		return (ENXIO);

	return (pca9552_gpio_pin_set(dev, pin, val ^ 1));
}

static phandle_t
pca9552_gpio_get_node(device_t bus, device_t dev)
{

	/* Used by ofw_gpiobus. */
	return (ofw_bus_get_node(bus));
}

static device_method_t pca9552_methods[] = {
	DEVMETHOD(device_probe,		pca9552_probe),
	DEVMETHOD(device_attach,	pca9552_attach),

	/* GPIO protocol */
	DEVMETHOD(gpio_get_bus,		pca9552_gpio_get_bus),
	DEVMETHOD(gpio_pin_max,		pca9552_gpio_pin_max),
	DEVMETHOD(gpio_pin_getname,	pca9552_gpio_pin_getname),
	DEVMETHOD(gpio_pin_getcaps,	pca9552_gpio_pin_getcaps),
	DEVMETHOD(gpio_pin_getflags,	pca9552_gpio_pin_getflags),
	DEVMETHOD(gpio_pin_setflags,	pca9552_gpio_pin_setflags),
	DEVMETHOD(gpio_pin_get,		pca9552_gpio_pin_get),
	DEVMETHOD(gpio_pin_set,		pca9552_gpio_pin_set),
	DEVMETHOD(gpio_pin_toggle,	pca9552_gpio_pin_toggle),

	/* ofw_bus interface */
	DEVMETHOD(ofw_bus_get_node,	pca9552_gpio_get_node),

	DEVMETHOD_END
};

static driver_t pca9552_driver = {
	"gpio",
	pca9552_methods,
	sizeof(struct pca9552_softc),
};

static devclass_t pca9552_devclass;

DRIVER_MODULE(pca9552, iicbus, pca9552_driver, pca9552_devclass, NULL, NULL);
MODULE_VERSION(pca9552, 1);
MODULE_DEPEND(pca9552, iicbus, 1, 1, 1);
