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
 * Driver for the ISSI IS31FL319x - 3/6/9 channel light effect LED driver.
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

#include <dev/iicbus/is31fl319xreg.h>

#include "gpio_if.h"
#include "iicbus_if.h"

#define	IS31FL3193	1
#define	IS31FL3196	2
#define	IS31FL3199	3

static struct ofw_compat_data compat_data[] = {
	{ "issi,is31fl3193",	IS31FL3193 },
	{ "issi,is31fl3196",	IS31FL3196 },
	{ "issi,is31fl3199",	IS31FL3199 },
        { NULL, 0 }
};

struct is31fl319x_softc {
	device_t	sc_dev;
	device_t	sc_gpio_busdev;
	int		sc_max_pins;
	uint16_t	sc_addr;
	uint8_t		sc_pwm[IS31FL319X_MAX_PINS];
};

static __inline int
is31fl319x_write(device_t dev, uint16_t addr, uint8_t *data, size_t len)
{
	struct iic_msg msg[1] = {
	    { addr, IIC_M_WR, len, data },
	};

	return (iicbus_transfer(dev, msg, nitems(msg)));
}

static int
is31fl319x_probe(device_t dev)
{
	const char *desc;
	struct is31fl319x_softc *sc;
#ifdef FDT
	phandle_t node;

	if (!ofw_bus_status_okay(dev))
		return (ENXIO);
	sc = device_get_softc(dev);
	switch (ofw_bus_search_compatible(dev, compat_data)->ocd_data) {
	case IS31FL3193:
		desc = "ISSI IS31FL3193 3 channel light effect LED driver";
		sc->sc_max_pins = 3;
		break;
	case IS31FL3196:
		desc = "ISSI IS31FL3196 6 channel light effect LED driver";
		sc->sc_max_pins = 6;
		break;
	case IS31FL3199:
		desc = "ISSI IS31FL3199 9 channel light effect LED driver";
		sc->sc_max_pins = 9;
		break;
	default:
		return (ENXIO);
	}
	node = ofw_bus_get_node(dev);
	if (!OF_hasprop(node, "gpio-controller"))
		/* Node is not a GPIO controller. */
		return (ENXIO);
#else
	sc = device_get_softc(dev);
	sc->sc_max_pins = IS31FL319X_MAX_PINS;
	desc = "ISSI IS31FL319x light effect LED driver";
#endif
	device_set_desc(dev, desc);

	return (BUS_PROBE_DEFAULT);
}

static int
is31fl319x_attach(device_t dev)
{
	struct is31fl319x_softc *sc;
	uint8_t data[2];

	sc = device_get_softc(dev);
	sc->sc_dev = dev;
	sc->sc_addr = iicbus_get_addr(dev);

	/* Reset the LED driver. */
	data[0] = IS31FL319X_RESET;
	data[1] = 0;
	if (is31fl319x_write(dev, sc->sc_addr, data, sizeof(data)) != 0)
		return (ENXIO);
	/* Disable the shutdown mode. */
	data[0] = IS31FL319X_SHUTDOWN;
	data[1] = 1;
	if (is31fl319x_write(dev, sc->sc_addr, data, sizeof(data)) != 0)
		return (ENXIO);

	/* Attach gpiobus. */
	sc->sc_gpio_busdev = gpiobus_attach_bus(dev);
	if (sc->sc_gpio_busdev == NULL)
		return (ENXIO);

	return (0);
}

static device_t
is31fl319x_gpio_get_bus(device_t dev)
{
	struct is31fl319x_softc *sc;

	sc = device_get_softc(dev);

	return (sc->sc_gpio_busdev);
}

static int
is31fl319x_gpio_pin_max(device_t dev, int *maxpin)
{
	struct is31fl319x_softc *sc;

	sc = device_get_softc(dev);
	*maxpin = sc->sc_max_pins - 1;

	return (0);
}

static int
is31fl319x_gpio_pin_getname(device_t dev, uint32_t pin, char *name)
{
	const char *buf[] = { "R", "G", "B" };
	struct is31fl319x_softc *sc;

	sc = device_get_softc(dev);
	if (pin >= sc->sc_max_pins)
		return (EINVAL);

	memset(name, 0, GPIOMAXNAME);
	snprintf(name, GPIOMAXNAME, "%s %d", buf[pin % 3], pin / 3);

	return (0);
}

static int
is31fl319x_gpio_pin_getcaps(device_t dev, uint32_t pin, uint32_t *caps)
{
	struct is31fl319x_softc *sc;

	sc = device_get_softc(dev);
	if (pin >= sc->sc_max_pins)
		return (EINVAL);

	*caps = GPIO_PIN_PWM;

	return (0);
}

static int
is31fl319x_gpio_pin_getflags(device_t dev, uint32_t pin, uint32_t *flags)
{
	struct is31fl319x_softc *sc;

	sc = device_get_softc(dev);
	if (pin >= sc->sc_max_pins)
		return (EINVAL);

	*flags = GPIO_PIN_PWM;

	return (0);
}

static int
is31fl319x_gpio_pin_setflags(device_t dev, uint32_t pin, uint32_t flags)
{
	struct is31fl319x_softc *sc;

	sc = device_get_softc(dev);
	if (pin >= sc->sc_max_pins)
		return (EINVAL);

	if ((flags & GPIO_PIN_PWM) == 0)
		return (EINVAL);

	return (0);
}

static __inline int
is31fl319x_pwm_update(struct is31fl319x_softc *sc)
{
	uint8_t data[2];

	data[0] = IS31FL319X_DATA_UPDATE;
	data[1] = 0;

	return (is31fl319x_write(sc->sc_dev, sc->sc_addr, data, sizeof(data)));
}

static int
is31fl319x_gpio_pin_set(device_t dev, uint32_t pin, uint32_t value)
{
	struct is31fl319x_softc *sc;
	uint8_t data[2];

	sc = device_get_softc(dev);
	if (pin >= sc->sc_max_pins)
		return (EINVAL);

	if (value != 0)
		sc->sc_pwm[pin] = IS31FL319X_PWM_MAX;
	else
		sc->sc_pwm[pin] = 0;
	data[0] = IS31FL319X_PWM(pin);
	data[1] = sc->sc_pwm[pin];
	if (is31fl319x_write(dev, sc->sc_addr, data, sizeof(data)) != 0)
		return (ENXIO);

	return (is31fl319x_pwm_update(sc));
}

static int
is31fl319x_gpio_pin_get(device_t dev, uint32_t pin, uint32_t *val)
{
	struct is31fl319x_softc *sc;

	sc = device_get_softc(dev);
	if (pin >= sc->sc_max_pins)
		return (EINVAL);

	*val = (sc->sc_pwm[pin] != 0) ? 1 : 0;

	return (0);
}

static int
is31fl319x_gpio_pin_toggle(device_t dev, uint32_t pin)
{
	struct is31fl319x_softc *sc;
	uint32_t val;

	sc = device_get_softc(dev);
	if (pin >= sc->sc_max_pins)
		return (EINVAL);

	val = (sc->sc_pwm[pin] != 0) ? 1 : 0;

	return (is31fl319x_gpio_pin_set(dev, pin, val ^ 1));
}

static int
is31fl319x_gpio_pwm_get(device_t dev, int32_t pwm, uint32_t pin, uint32_t reg,
    uint32_t *val)
{
	struct is31fl319x_softc *sc;

	sc = device_get_softc(dev);
	if (pin >= sc->sc_max_pins)
		return (EINVAL);

	if (pwm != -1 || reg != GPIO_PWM_DUTY)
		return (EINVAL);

	*val = (uint32_t)sc->sc_pwm[pin];

	return (0);
}

static int
is31fl319x_gpio_pwm_set(device_t dev, int32_t pwm, uint32_t pin, uint32_t reg,
    uint32_t val)
{
	struct is31fl319x_softc *sc;
	uint8_t data[2];

	sc = device_get_softc(dev);
	if (pin >= sc->sc_max_pins)
		return (EINVAL);

	if (pwm != -1 || reg != GPIO_PWM_DUTY)
		return (EINVAL);

	sc->sc_pwm[pin] = (uint8_t)val;
	data[0] = IS31FL319X_PWM(pin);
	data[1] = sc->sc_pwm[pin];
	if (is31fl319x_write(dev, sc->sc_addr, data, sizeof(data)) != 0)
		return (ENXIO);

	return (is31fl319x_pwm_update(sc));
}

static phandle_t
is31fl319x_gpio_get_node(device_t bus, device_t dev)
{

	/* Used by ofw_gpiobus. */
	return (ofw_bus_get_node(bus));
}

static device_method_t is31fl319x_methods[] = {
	DEVMETHOD(device_probe,		is31fl319x_probe),
	DEVMETHOD(device_attach,	is31fl319x_attach),

	/* GPIO protocol */
	DEVMETHOD(gpio_get_bus,		is31fl319x_gpio_get_bus),
	DEVMETHOD(gpio_pin_max,		is31fl319x_gpio_pin_max),
	DEVMETHOD(gpio_pin_getname,	is31fl319x_gpio_pin_getname),
	DEVMETHOD(gpio_pin_getcaps,	is31fl319x_gpio_pin_getcaps),
	DEVMETHOD(gpio_pin_getflags,	is31fl319x_gpio_pin_getflags),
	DEVMETHOD(gpio_pin_setflags,	is31fl319x_gpio_pin_setflags),
	DEVMETHOD(gpio_pin_get,		is31fl319x_gpio_pin_get),
	DEVMETHOD(gpio_pin_set,		is31fl319x_gpio_pin_set),
	DEVMETHOD(gpio_pin_toggle,	is31fl319x_gpio_pin_toggle),
	DEVMETHOD(gpio_pwm_get,		is31fl319x_gpio_pwm_get),
	DEVMETHOD(gpio_pwm_set,		is31fl319x_gpio_pwm_set),

	/* ofw_bus interface */
	DEVMETHOD(ofw_bus_get_node,	is31fl319x_gpio_get_node),

	DEVMETHOD_END
};

static driver_t is31fl319x_driver = {
	"gpio",
	is31fl319x_methods,
	sizeof(struct is31fl319x_softc),
};

static devclass_t is31fl319x_devclass;

DRIVER_MODULE(is31fl319x, iicbus, is31fl319x_driver, is31fl319x_devclass,
    NULL, NULL);
MODULE_VERSION(is31fl319x, 1);
MODULE_DEPEND(is31fl319x, iicbus, 1, 1, 1);
