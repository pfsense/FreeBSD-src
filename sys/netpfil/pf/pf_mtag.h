/*	$FreeBSD$	*/
/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2001 Daniel Hartmeier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _NET_PF_MTAG_H_
#define _NET_PF_MTAG_H_

#ifdef _KERNEL

#define	PF_TAG_GENERATED		0x0001
#define	PF_TAG_DUMMYNET			0x0002
#define	PF_TAG_TRANSLATE_LOCALHOST	0x0004
#define	PF_PACKET_LOOPED		0x0008
#define	PF_FASTFWD_OURS_PRESENT		0x0010
#define	PF_REASSEMBLED			0x0020
#define	PF_TAG_SYNCOOKIE_RECREATED	0x0080
#define	PF_TAG_ROUTE_TO			0x0100

struct pf_mtag {
	void		*hdr;		/* saved hdr pos in mbuf, for ECN */
	u_int16_t	 qid;		/* queue id */
	u_int32_t	 qid_hash;	/* queue hashid used by WFQ like algos */
	u_int16_t	 tag;		/* tag id */
	u_int16_t	 flags;
	u_int8_t	 routed;
	u_int16_t	 dnpipe;
	u_int32_t	 dnflags;

	struct ifnet	*ifp;
	struct sockaddr_storage dst;
};

static __inline struct pf_mtag *
pf_find_mtag(struct mbuf *m)
{
	struct m_tag	*mtag;

	if ((mtag = m_tag_find(m, PACKET_TAG_PF, NULL)) == NULL)
		return (NULL);

	return ((struct pf_mtag *)(mtag + 1));
}
#endif /* _KERNEL */
#endif /* _NET_PF_MTAG_H_ */
