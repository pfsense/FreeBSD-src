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

struct is31fl319x_reg {
	struct is31fl319x_softc *sc;
	uint8_t		data;
	uint8_t		id;
	uint8_t		reg;
};

struct is31fl319x_softc {
	device_t	sc_dev;
	device_t	sc_gpio_busdev;
	int		sc_max_pins;
	uint8_t		sc_pwm[IS31FL319X_MAX_PINS];
	uint8_t		sc_conf1;
	struct is31fl319x_reg	sc_t0[IS31FL319X_MAX_PINS];
	struct is31fl319x_reg	sc_t123[IS31FL319X_MAX_PINS / 3];
	struct is31fl319x_reg	sc_t4[IS31FL319X_MAX_PINS];
};

static __inline int
is31fl319x_write(device_t dev, uint8_t reg, uint8_t *data, size_t len)
{

	return (iicdev_writeto(dev, reg, data, len, IIC_INTRWAIT));
}

static __inline int
is31fl319x_reg_update(struct is31fl319x_softc *sc, uint8_t reg)
{
	uint8_t data = 0;

        return (iicdev_writeto(sc->sc_dev, reg, &data, 1, IIC_INTRWAIT));
}

static int
is31fl319x_pwm_sysctl(SYSCTL_HANDLER_ARGS)
{
	int error, led;
	int32_t enable;
	struct is31fl319x_softc *sc;

	sc = (struct is31fl319x_softc *)arg1;
	led = arg2;

	enable = ((sc->sc_conf1 & IS31FL319X_CONF1_PWM(led)) != 0) ? 0 : 1;
	error = sysctl_handle_int(oidp, &enable, sizeof(enable), req);
	if (error != 0 || req->newptr == NULL)
		return (error);

	sc->sc_conf1 &= ~IS31FL319X_CONF1_PWM(led);
	if (enable == 0)
		sc->sc_conf1 |= IS31FL319X_CONF1_PWM(led);
	if (is31fl319x_write(sc->sc_dev, IS31FL319X_CONF1, &sc->sc_conf1,
	    sizeof(sc->sc_conf1)) != 0)
		return (ENXIO);
	if (is31fl319x_reg_update(sc, IS31FL319X_DATA_UPDATE) != 0)
		return (ENXIO);
	if (is31fl319x_reg_update(sc, IS31FL319X_TIME_UPDATE) != 0)
		return (ENXIO);

	return (0);
}

static int
is31fl319x_pin_timer_sysctl(SYSCTL_HANDLER_ARGS)
{
	int error;
	int32_t a, b, ms;
	struct is31fl319x_reg *timer;
	struct is31fl319x_softc *sc;

	timer = (struct is31fl319x_reg *)arg1;
	sc = timer->sc;

	a = timer->data & IS31FL319X_T0_A_MASK;
	b = (timer->data & IS31FL319X_T0_B_MASK) >> 4;
	ms = 260 * a * (2 << b);
	error = sysctl_handle_int(oidp, &ms, sizeof(ms), req);
	if (error != 0 || req->newptr == NULL)
		return (error);

	if (ms > IS31FL319X_T0_MAX_TIME)
		ms = IS31FL319X_T0_MAX_TIME;

	a = b = 0;
	if (ms >= 260) {
		ms /= 260;
		while (ms / (2 << b) > 15) {
			if (ms / (2 << b) > 15)
				b++;
			else
				break;
		}
		a = ms / (2 << b);
	}
	timer->data = (b << 4) | a;

	if (is31fl319x_write(sc->sc_dev, timer->reg, &timer->data,
	    sizeof(timer->data)) != 0)
		return (ENXIO);
	if (is31fl319x_reg_update(sc, IS31FL319X_TIME_UPDATE) != 0)
		return (ENXIO);

	return (0);
}

static int
is31fl319x_dt_sysctl(SYSCTL_HANDLER_ARGS)
{
	int error;
	int32_t enable;
	struct is31fl319x_reg *led;
	struct is31fl319x_softc *sc;

	led = (struct is31fl319x_reg *)arg1;
	sc = led->sc;

	enable = ((led->data & IS31FL319X_DT) != 0) ? 1 : 0;
	error = sysctl_handle_int(oidp, &enable, sizeof(enable), req);
	if (error != 0 || req->newptr == NULL)
		return (error);

	if (enable)
		led->data |= IS31FL319X_DT;
	else
		led->data &= ~IS31FL319X_DT;
	if (is31fl319x_write(sc->sc_dev, IS31FL319X_T123(led->id), &led->data,
	    sizeof(led->data)) != 0)
		return (ENXIO);
	if (is31fl319x_reg_update(sc, IS31FL319X_TIME_UPDATE) != 0)
		return (ENXIO);

	return (0);
}

static int
is31fl319x_t1t3_sysctl(SYSCTL_HANDLER_ARGS)
{
	int error;
	int32_t a, ms;
	struct is31fl319x_reg *led;
	struct is31fl319x_softc *sc;

	led = (struct is31fl319x_reg *)arg1;
	sc = led->sc;

	a = (led->data & IS31FL319X_T1_A_MASK);
	if (a >= 5 && a <= 6)
		ms = 0;
	else if (a == 7)
		ms = 100;
	else
		ms = 260 * (2 << a);
	error = sysctl_handle_int(oidp, &ms, sizeof(ms), req);
	if (error != 0 || req->newptr == NULL)
		return (error);

	if (ms > IS31FL319X_T1_MAX_TIME)
		ms = IS31FL319X_T1_MAX_TIME;

	a = 0;
	if (ms == 0)
		a = 5;	/* Breathing function disabled. */
	if (ms == 100)
		a = 7;	/* 100 ms */
	else if (ms >= 260) {
		ms /= 260;
		while (ms / (2 << a) > 1) {
			if (ms / (2 << a) > 1)
				a++;
			else
				break;
		}
	}
	led->data &= ~IS31FL319X_T1_A_MASK;
	led->data |= (a & IS31FL319X_T1_A_MASK);

	if (is31fl319x_write(sc->sc_dev, IS31FL319X_T123(led->id), &led->data,
	    sizeof(led->data)) != 0)
		return (ENXIO);
	if (is31fl319x_reg_update(sc, IS31FL319X_TIME_UPDATE) != 0)
		return (ENXIO);

	return (0);
}

static int
is31fl319x_t2_sysctl(SYSCTL_HANDLER_ARGS)
{
	int error;
	int32_t b, ms;
	struct is31fl319x_reg *led;
	struct is31fl319x_softc *sc;

	led = (struct is31fl319x_reg *)arg1;
	sc = led->sc;

	b = (led->data & IS31FL319X_T2_B_MASK) >> 4;
	if (b > 0)
		ms = 260 * (2 << (b - 1));
	else
		ms = 0;
	error = sysctl_handle_int(oidp, &ms, sizeof(ms), req);
	if (error != 0 || req->newptr == NULL)
		return (error);

	if (ms > IS31FL319X_T2_MAX_TIME)
		ms = IS31FL319X_T2_MAX_TIME;

	b = 0;
	if (ms >= 260) {
		ms /= 260;
		b = 1;
		while (ms / (2 << (b - 1)) > 1) {
			if (ms / (2 << (b - 1)) > 1)
				b++;
			else
				break;
		}
	}
	led->data &= ~IS31FL319X_T2_B_MASK;
	led->data |= ((b << 4) & IS31FL319X_T2_B_MASK);

	if (is31fl319x_write(sc->sc_dev, IS31FL319X_T123(led->id), &led->data,
	    sizeof(led->data)) != 0)
		return (ENXIO);
	if (is31fl319x_reg_update(sc, IS31FL319X_TIME_UPDATE) != 0)
		return (ENXIO);

	return (0);
}

static void
is31fl319x_sysctl_attach(device_t dev)
{
 	char strbuf[4];
	struct is31fl319x_softc *sc;
	struct sysctl_ctx_list *ctx;
	struct sysctl_oid *tree_node, *led_node, *ledN_node, *pin_node;
	struct sysctl_oid *pinN_node;
	struct sysctl_oid_list *tree, *led_tree, *ledN_tree, *pin_tree;
	struct sysctl_oid_list *pinN_tree;
	int led, pin;

	ctx = device_get_sysctl_ctx(dev);
	tree_node = device_get_sysctl_tree(dev);
	tree = SYSCTL_CHILDREN(tree_node);
	pin_node = SYSCTL_ADD_NODE(ctx, tree, OID_AUTO, "pin", CTLFLAG_RD,
	    NULL, "Output Pins");
	pin_tree = SYSCTL_CHILDREN(pin_node);

	sc = device_get_softc(dev);
	for (pin = 0; pin < sc->sc_max_pins; pin++) {

		snprintf(strbuf, sizeof(strbuf), "%d", pin);
		pinN_node = SYSCTL_ADD_NODE(ctx, pin_tree, OID_AUTO, strbuf,
		    CTLFLAG_RD, NULL, "Output Pin");
		pinN_tree = SYSCTL_CHILDREN(pinN_node);

		sc->sc_t0[pin].sc = sc;
		sc->sc_t0[pin].data = 0;
		sc->sc_t0[pin].id = pin;
		sc->sc_t0[pin].reg = IS31FL319X_T0(pin);
		SYSCTL_ADD_PROC(ctx, pinN_tree, OID_AUTO, "T0",
		    CTLFLAG_RW | CTLTYPE_UINT | CTLFLAG_MPSAFE,
		    &sc->sc_t0[pin], 0, is31fl319x_pin_timer_sysctl, "IU",
		    "T0 timer in ms");
		sc->sc_t4[pin].sc = sc;
		sc->sc_t4[pin].data = 0;
		sc->sc_t4[pin].id = pin;
		sc->sc_t4[pin].reg = IS31FL319X_T4(pin);
		SYSCTL_ADD_PROC(ctx, pinN_tree, OID_AUTO, "T4",
		    CTLFLAG_RW | CTLTYPE_UINT | CTLFLAG_MPSAFE,
		    &sc->sc_t4[pin], 0, is31fl319x_pin_timer_sysctl, "IU",
		    "T4 timer in ms");
	}
	led_node = SYSCTL_ADD_NODE(ctx, tree, OID_AUTO, "led", CTLFLAG_RD,
	    NULL, "RGB LEDs");
	led_tree = SYSCTL_CHILDREN(led_node);
	for (led = 0; led < (sc->sc_max_pins / 3); led++) {
		snprintf(strbuf, sizeof(strbuf), "%d", led);
		ledN_node = SYSCTL_ADD_NODE(ctx, led_tree, OID_AUTO, strbuf,
		    CTLFLAG_RD, NULL, "RGB LED");
		ledN_tree = SYSCTL_CHILDREN(ledN_node);

		SYSCTL_ADD_PROC(ctx, ledN_tree, OID_AUTO, "pwm",
		    CTLFLAG_RW | CTLTYPE_UINT | CTLFLAG_MPSAFE, sc, led,
		    is31fl319x_pwm_sysctl, "IU", "Enable the PWM control");
		sc->sc_t123[led].sc = sc;
		sc->sc_t123[led].data = 0;
		sc->sc_t123[led].id = led;
		sc->sc_t123[led].reg = IS31FL319X_T123(led);
		SYSCTL_ADD_PROC(ctx, ledN_tree, OID_AUTO, "T1-T3",
		    CTLFLAG_RW | CTLTYPE_UINT | CTLFLAG_MPSAFE,
		    &sc->sc_t123[led], 0, is31fl319x_t1t3_sysctl, "IU",
		    "T1 and T3 timer");
		SYSCTL_ADD_PROC(ctx, ledN_tree, OID_AUTO, "DT",
		    CTLFLAG_RW | CTLTYPE_UINT | CTLFLAG_MPSAFE,
		    &sc->sc_t123[led], 0, is31fl319x_dt_sysctl, "IU",
		    "T3 Double Time (T3 = 2T1)");
		SYSCTL_ADD_PROC(ctx, ledN_tree, OID_AUTO, "T2",
		    CTLFLAG_RW | CTLTYPE_UINT | CTLFLAG_MPSAFE,
		    &sc->sc_t123[led], 0, is31fl319x_t2_sysctl, "IU",
		    "T2 timer");
	}
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
	uint8_t data[3];

	sc = device_get_softc(dev);
	sc->sc_dev = dev;

	/* Reset the LED driver. */
	data[0] = 0;
	if (is31fl319x_write(dev, IS31FL319X_RESET, data, 1) != 0)
		return (ENXIO);

	/* Disable the shutdown mode. */
	data[0] = 1;
	if (is31fl319x_write(dev, IS31FL319X_SHUTDOWN, data, 1) != 0)
		return (ENXIO);

	/* Attach gpiobus. */
	sc->sc_gpio_busdev = gpiobus_attach_bus(dev);
	if (sc->sc_gpio_busdev == NULL)
		return (ENXIO);

	is31fl319x_sysctl_attach(dev);

	/* Update the booting status, kernel is loading. */
	data[0] = 0;
	data[1] = 0;
	data[2] = 35;
	if (is31fl319x_write(dev, IS31FL319X_PWM(6), data, sizeof(data)) != 0)
		return (ENXIO);
	data[2] = 100;
	if (is31fl319x_write(dev, IS31FL319X_PWM(3), data, sizeof(data)) != 0)
		return (ENXIO);

	/* Enable breath on LED 2 and 3. */
	sc->sc_conf1 |= (6 << 4);
	if (is31fl319x_write(sc->sc_dev, IS31FL319X_CONF1, &sc->sc_conf1,
	    sizeof(sc->sc_conf1)) != 0)
		return (ENXIO);

	/* Update register data. */
	if (is31fl319x_reg_update(sc, IS31FL319X_DATA_UPDATE) != 0)
		return (ENXIO);
	if (is31fl319x_reg_update(sc, IS31FL319X_TIME_UPDATE) != 0)
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

static int
is31fl319x_gpio_pin_set(device_t dev, uint32_t pin, uint32_t value)
{
	struct is31fl319x_softc *sc;

	sc = device_get_softc(dev);
	if (pin >= sc->sc_max_pins)
		return (EINVAL);

	if (value != 0)
		sc->sc_pwm[pin] = IS31FL319X_PWM_MAX;
	else
		sc->sc_pwm[pin] = 0;
	if (is31fl319x_write(dev, IS31FL319X_PWM(pin),
	    &sc->sc_pwm[pin], 1) != 0)
		return (ENXIO);

	return (is31fl319x_reg_update(sc, IS31FL319X_DATA_UPDATE));
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

	sc = device_get_softc(dev);
	if (pin >= sc->sc_max_pins)
		return (EINVAL);

	if (pwm != -1 || reg != GPIO_PWM_DUTY)
		return (EINVAL);

	sc->sc_pwm[pin] = (uint8_t)val;
	if (is31fl319x_write(dev, IS31FL319X_PWM(pin),
	    &sc->sc_pwm[pin], 1) != 0)
		return (ENXIO);

	return (is31fl319x_reg_update(sc, IS31FL319X_DATA_UPDATE));
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
