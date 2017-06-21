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
 *
 * $FreeBSD$
 */

/*
 * ISSI IS31FL319X [3|6|9]-Channel Light Effect LED Driver.
 */

#ifndef _IS31FL319XREG_H_
#define _IS31FL319XREG_H_

#define	IS31FL319X_SHUTDOWN		0x00
#define	IS31FL319X_LEDCTRL1		0x01
#define	IS31FL319X_LEDCTRL2		0x02
#define	IS31FL319X_CONF1		0x03
#define	IS31FL319X_CONF2		0x04
#define	IS31FL319X_RAMPMODE		0x05
#define	IS31FL319X_BREATHMARK		0x06
#define	IS31FL319X_PWM(out)		(0x07 + (out))
#define	IS31FL319X_PWM_MAX			0xff
#define	IS31FL319X_DATA_UPDATE		0x10
#define	IS31FL319X_T0(out)		(0x11 + (out))
#define	IS31FL319X_T1			0x1a
#define	IS31FL319X_T2			0x1b
#define	IS31FL319X_T3			0x1c
#define	IS31FL319X_T4(out)		(0x1d + (out))
#define	IS31FL319X_TIME_UPDATE		0x26
#define	IS31FL319X_RESET		0xff

#define	IS31FL319X_MAX_PINS		9

#endif	/* _IS31FL319XREG_H_ */
