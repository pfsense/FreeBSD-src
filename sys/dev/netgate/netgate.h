/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2022-2023 Rubicon Communications, LLC (Netgate).
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

#ifndef __NETGATE_H__
#define __NETGATE_H__

#define	NETGATE_MOD_VER		"0.1"

#define	NETGATE_UNKNOWN		0

/* Old devices. */
#define	NETGATE_APU1		10
#define	NETGATE_APU		11
#define	NETGATE_SG2200		12
#define	NETGATE_RCCVE		13
#define	NETGATE_SG4860		14
#define	NETGATE_SG8860		15
#define	NETGATE_XG2758		16
#define	NETGATE_MBT4220		17
#define	NETGATE_MBT4220_1	18
#define	NETGATE_MBT2220		19
#define	NETGATE_MBT2220_1	20
#define	NETGATE_TURBOT		21
#define	NETGATE_TURBOT_1	22
#define	NETGATE_C2558		23
#define	NETGATE_C2558_1		24
#define	NETGATE_C2758		25
#define	NETGATE_C2758_1		26
#define	NETGATE_APU2		27
#define	NETGATE_APU2_1		28

/* VMs */
#define	NETGATE_VM_QEMU		50
#define	NETGATE_VM_GOOGLE	51
#define	NETGATE_VM_KVM		52
#define	NETGATE_VM_AWS		53
#define	NETGATE_VM_GOOGLE_1	54
#define	NETGATE_VM_VIRTUALBOX	55
#define	NETGATE_VM_HYPERV	56
#define	NETGATE_VM_AZURE	57
#define	NETGATE_VM_VMWARE	58
#define	NETGATE_VM_AWS_1    59
#define	NETGATE_VM_ORACLE   60

/* Netgate */
#define	NETGATE_SG1000		1000
#define	NETGATE_1100		1100
#define	NETGATE_2100		2100
#define	NETGATE_1537		1537
#define	NETGATE_1540		1540
#define	NETGATE_1541		1541
#define	NETGATE_3100		3100
#define	NETGATE_4100		4100
#define	NETGATE_4200		4200
#define	NETGATE_5100		5100
#define	NETGATE_6100		6100
#define	NETGATE_6200		6200
#define	NETGATE_7100		7100
#define	NETGATE_7541		7541
#define	NETGATE_8200		8200

char *netgate_get_device_desc(void);
char *netgate_get_device_model(void);
int netgate_get_device_id(void);

#endif	/* __NETGATE_H__ */
