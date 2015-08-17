/*-
 * Copyright (c) 2014 The FreeBSD Foundation
 * All rights reserved.
 *
 * This software was developed by John-Mark Gurney under
 * the sponsorship from the FreeBSD Foundation and
 * Rubicon Communications, LLC (Netgate)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 *	$Id$
 *
 */

/*
 * Figure 5, 7, and 11 are copied from the Intel white paper: Intel
 * s Multiplication Instruction and its Usage for Computing the GCM Mode
 * 
 * and as such are: Copyright © 2010 Intel Corporation. All rights reserved.
 * 
 * Please see white paper for complete license.
 */

#ifdef _KERNEL
#include <crypto/aesni/aesni.h>
#else
#include <stdint.h>
#endif

#include <wmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>

/* some code from carry-less-multiplication-instruction-in-gcm-mode-paper.pdf */

#define REFLECT(X) \
 hlp1 = _mm_srli_epi16(X,4);\
 X = _mm_and_si128(AMASK,X);\
 hlp1 = _mm_and_si128(AMASK,hlp1);\
 X = _mm_shuffle_epi8(MASKH,X);\
 hlp1 = _mm_shuffle_epi8(MASKL,hlp1);\
 X = _mm_xor_si128(X,hlp1)

static inline int
m128icmp(__m128i a, __m128i b)
{
	__m128i cmp;

	cmp = _mm_cmpeq_epi32(a, b);

	return _mm_movemask_epi8(cmp) == 0xffff;
}

/* Figure 5. Code Sample - Performing Ghash Using Algorithms 1 and 5 (C) */
static void 
gfmul_decrypt(__m128i a, __m128i b, __m128i * res)
{
	__m128i /* tmp0, tmp1, tmp2, */ tmp3, tmp4, tmp5, tmp6,
	tmp7, tmp8, tmp9, tmp10, tmp11, tmp12;
	__m128i		XMMMASK = _mm_setr_epi32(0xffffffff, 0x0, 0x0, 0x0);

	tmp3 = _mm_clmulepi64_si128(a, b, 0x00);
	tmp6 = _mm_clmulepi64_si128(a, b, 0x11);
	tmp4 = _mm_shuffle_epi32(a, 78);
	tmp5 = _mm_shuffle_epi32(b, 78);
	tmp4 = _mm_xor_si128(tmp4, a);
	tmp5 = _mm_xor_si128(tmp5, b);
	tmp4 = _mm_clmulepi64_si128(tmp4, tmp5, 0x00);
	tmp4 = _mm_xor_si128(tmp4, tmp3);
	tmp4 = _mm_xor_si128(tmp4, tmp6);
	tmp5 = _mm_slli_si128(tmp4, 8);
	tmp4 = _mm_srli_si128(tmp4, 8);
	tmp3 = _mm_xor_si128(tmp3, tmp5);
	tmp6 = _mm_xor_si128(tmp6, tmp4);
	tmp7 = _mm_srli_epi32(tmp6, 31);
	tmp8 = _mm_srli_epi32(tmp6, 30);
	tmp9 = _mm_srli_epi32(tmp6, 25);
	tmp7 = _mm_xor_si128(tmp7, tmp8);
	tmp7 = _mm_xor_si128(tmp7, tmp9);
	tmp8 = _mm_shuffle_epi32(tmp7, 147);

	tmp7 = _mm_and_si128(XMMMASK, tmp8);
	tmp8 = _mm_andnot_si128(XMMMASK, tmp8);
	tmp3 = _mm_xor_si128(tmp3, tmp8);
	tmp6 = _mm_xor_si128(tmp6, tmp7);
	tmp10 = _mm_slli_epi32(tmp6, 1);
	tmp3 = _mm_xor_si128(tmp3, tmp10);
	tmp11 = _mm_slli_epi32(tmp6, 2);
	tmp3 = _mm_xor_si128(tmp3, tmp11);
	tmp12 = _mm_slli_epi32(tmp6, 7);
	tmp3 = _mm_xor_si128(tmp3, tmp12);

	*res = _mm_xor_si128(tmp3, tmp6);
}

void 
AES_GCM_encrypt(const unsigned char *in,
		unsigned char *out,
		const unsigned char *addt,
		const unsigned char *ivec,
		unsigned char *tag,
		int nbytes,
		int abytes,
		int ibytes,
		const unsigned char *key,
		int nr)
{
	int		i         , j, k;
	__m128i		hlp1 /* , hlp2, hlp3, hlp4 */ ;
	__m128i		tmp1  , tmp2, tmp3, tmp4;
	__m128i		H     , T;
	__m128i        *KEY = (__m128i *) key;
	__m128i		ctr1  , ctr2, ctr3, ctr4;
	__m128i		last_block = _mm_setzero_si128();
	__m128i		ONE = _mm_set_epi32(0, 1, 0, 0);
	__m128i		FOUR = _mm_set_epi32(0, 4, 0, 0);
	__m128i		BSWAP_EPI64 = _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5,
					    6, 7);
	/*
	 * __m128i BSWAP_MASK = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	 * 10, 11, 12, 13, 14, 15);
	 */
	__m128i		X = _mm_setzero_si128(), Y = _mm_setzero_si128();
	__m128i		AMASK = _mm_set_epi32(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f);
	__m128i		MASKL = _mm_set_epi32(0x0f070b03, 0x0d050901, 0x0e060a02, 0x0c040800);
	__m128i		MASKH = _mm_set_epi32(0xf070b030, 0xd0509010, 0xe060a020, 0xc0408000);
	__m128i		MASKF = _mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f);

	if (ibytes == 96 / 8) {
		Y = _mm_loadu_si128((__m128i *) ivec);
		Y = _mm_insert_epi32(Y, 0x1000000, 3);
		/* (Compute E[ZERO, KS] and E[Y0, KS] together */
		tmp1 = _mm_xor_si128(X, KEY[0]);
		tmp2 = _mm_xor_si128(Y, KEY[0]);
		for (j = 1; j < nr - 1; j += 2) {
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j]);
			tmp2 = _mm_aesenc_si128(tmp2, KEY[j]);
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j + 1]);
			tmp2 = _mm_aesenc_si128(tmp2, KEY[j + 1]);
		}
		tmp1 = _mm_aesenc_si128(tmp1, KEY[nr - 1]);
		tmp2 = _mm_aesenc_si128(tmp2, KEY[nr - 1]);
		H = _mm_aesenclast_si128(tmp1, KEY[nr]);
		T = _mm_aesenclast_si128(tmp2, KEY[nr]);
		REFLECT(H);
	} else {
		tmp1 = _mm_xor_si128(X, KEY[0]);
		for (j = 1; j < nr; j++)
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j]);
		H = _mm_aesenclast_si128(tmp1, KEY[nr]);
		REFLECT(H);
		Y = _mm_xor_si128(Y, Y);
		for (i = 0; i < ibytes / 16; i++) {
			tmp1 = _mm_loadu_si128(&((__m128i *) ivec)[i]);
			REFLECT(tmp1);
			Y = _mm_xor_si128(Y, tmp1);
			gfmul_decrypt(Y, H, &Y);
		}
		if (ibytes % 16) {
			for (j = 0; j < ibytes % 16; j++)
				((unsigned char *)&last_block)[j] = ivec[i * 16 + j];
			tmp1 = last_block;
			REFLECT(tmp1);
			Y = _mm_xor_si128(Y, tmp1);
			gfmul_decrypt(Y, H, &Y);
		}
		tmp1 = _mm_insert_epi64(tmp1, ibytes * 8, 0);
		tmp1 = _mm_insert_epi64(tmp1, 0, 1);
		REFLECT(tmp1);
		tmp1 = _mm_shuffle_epi8(tmp1, MASKF);
		Y = _mm_xor_si128(Y, tmp1);
		gfmul_decrypt(Y, H, &Y);
		REFLECT(Y);
		/* Compute E(K, Y0) */
		tmp1 = _mm_xor_si128(Y, KEY[0]);
		for (j = 1; j < nr; j++)
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j]);
		T = _mm_aesenclast_si128(tmp1, KEY[nr]);
	}

	for (i = 0; i < abytes / 16; i++) {
		tmp1 = _mm_loadu_si128(&((__m128i *) addt)[i]);
		REFLECT(tmp1);
		X = _mm_xor_si128(X, tmp1);
		gfmul_decrypt(X, H, &X);
	}
	if (abytes % 16) {
		last_block = _mm_setzero_si128();
		for (j = 0; j < abytes % 16; j++)
			((unsigned char *)&last_block)[j] = addt[i * 16 + j];
		tmp1 = last_block;
		REFLECT(tmp1);
		X = _mm_xor_si128(X, tmp1);
		gfmul_decrypt(X, H, &X);
	}
	ctr1 = _mm_shuffle_epi8(Y, BSWAP_EPI64);
	ctr1 = _mm_add_epi64(ctr1, ONE);
	ctr2 = _mm_add_epi64(ctr1, ONE);
	ctr3 = _mm_add_epi64(ctr2, ONE);
	ctr4 = _mm_add_epi64(ctr3, ONE);
	for (i = 0; i < nbytes / 16 / 4; i++) {
		tmp1 = _mm_shuffle_epi8(ctr1, BSWAP_EPI64);
		tmp2 = _mm_shuffle_epi8(ctr2, BSWAP_EPI64);
		tmp3 = _mm_shuffle_epi8(ctr3, BSWAP_EPI64);
		tmp4 = _mm_shuffle_epi8(ctr4, BSWAP_EPI64);
		ctr1 = _mm_add_epi64(ctr1, FOUR);
		ctr2 = _mm_add_epi64(ctr2, FOUR);
		ctr3 = _mm_add_epi64(ctr3, FOUR);
		ctr4 = _mm_add_epi64(ctr4, FOUR);
		tmp1 = _mm_xor_si128(tmp1, KEY[0]);
		tmp2 = _mm_xor_si128(tmp2, KEY[0]);
		tmp3 = _mm_xor_si128(tmp3, KEY[0]);
		tmp4 = _mm_xor_si128(tmp4, KEY[0]);
		for (j = 1; j < nr - 1; j += 2) {
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j]);
			tmp2 = _mm_aesenc_si128(tmp2, KEY[j]);
			tmp3 = _mm_aesenc_si128(tmp3, KEY[j]);
			tmp4 = _mm_aesenc_si128(tmp4, KEY[j]);
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j + 1]);
			tmp2 = _mm_aesenc_si128(tmp2, KEY[j + 1]);
			tmp3 = _mm_aesenc_si128(tmp3, KEY[j + 1]);
			tmp4 = _mm_aesenc_si128(tmp4, KEY[j + 1]);
		}
		tmp1 = _mm_aesenc_si128(tmp1, KEY[nr - 1]);
		tmp2 = _mm_aesenc_si128(tmp2, KEY[nr - 1]);
		tmp3 = _mm_aesenc_si128(tmp3, KEY[nr - 1]);
		tmp4 = _mm_aesenc_si128(tmp4, KEY[nr - 1]);
		tmp1 = _mm_aesenclast_si128(tmp1, KEY[nr]);
		tmp2 = _mm_aesenclast_si128(tmp2, KEY[nr]);
		tmp3 = _mm_aesenclast_si128(tmp3, KEY[nr]);
		tmp4 = _mm_aesenclast_si128(tmp4, KEY[nr]);
		tmp1 = _mm_xor_si128(tmp1, _mm_loadu_si128(&((__m128i *) in)[i * 4 + 0]));
		tmp2 = _mm_xor_si128(tmp2, _mm_loadu_si128(&((__m128i *) in)[i * 4 + 1]));
		tmp3 = _mm_xor_si128(tmp3, _mm_loadu_si128(&((__m128i *) in)[i * 4 + 2]));
		tmp4 = _mm_xor_si128(tmp4, _mm_loadu_si128(&((__m128i *) in)[i * 4 + 3]));
		_mm_storeu_si128(&((__m128i *) out)[i * 4 + 0], tmp1);
		_mm_storeu_si128(&((__m128i *) out)[i * 4 + 1], tmp2);
		_mm_storeu_si128(&((__m128i *) out)[i * 4 + 2], tmp3);
		_mm_storeu_si128(&((__m128i *) out)[i * 4 + 3], tmp4);
		REFLECT(tmp1);
		REFLECT(tmp2);
		REFLECT(tmp3);
		REFLECT(tmp4);
		X = _mm_xor_si128(X, tmp1);
		gfmul_decrypt(X, H, &X);
		X = _mm_xor_si128(X, tmp2);
		gfmul_decrypt(X, H, &X);
		X = _mm_xor_si128(X, tmp3);
		gfmul_decrypt(X, H, &X);
		X = _mm_xor_si128(X, tmp4);
		gfmul_decrypt(X, H, &X);
	}
	for (k = i * 4; k < nbytes / 16; k++) {
		tmp1 = _mm_shuffle_epi8(ctr1, BSWAP_EPI64);
		ctr1 = _mm_add_epi64(ctr1, ONE);
		tmp1 = _mm_xor_si128(tmp1, KEY[0]);
		for (j = 1; j < nr - 1; j += 2) {
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j]);
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j + 1]);
		}
		tmp1 = _mm_aesenc_si128(tmp1, KEY[nr - 1]);
		tmp1 = _mm_aesenclast_si128(tmp1, KEY[nr]);
		tmp1 = _mm_xor_si128(tmp1, _mm_loadu_si128(&((__m128i *) in)[k]));
		_mm_storeu_si128(&((__m128i *) out)[k], tmp1);
		REFLECT(tmp1);
		X = _mm_xor_si128(X, tmp1);
		gfmul_decrypt(X, H, &X);
	}
	//If one partial block remains
		if (nbytes % 16) {
		tmp1 = _mm_shuffle_epi8(ctr1, BSWAP_EPI64);
		tmp1 = _mm_xor_si128(tmp1, KEY[0]);
		for (j = 1; j < nr - 1; j += 2) {
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j]);
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j + 1]);
		}
		tmp1 = _mm_aesenc_si128(tmp1, KEY[nr - 1]);
		tmp1 = _mm_aesenclast_si128(tmp1, KEY[nr]);
		tmp1 = _mm_xor_si128(tmp1, _mm_loadu_si128(&((__m128i *) in)[k]));
		last_block = tmp1;
		for (j = 0; j < nbytes % 16; j++)
			out[k * 16 + j] = ((unsigned char *)&last_block)[j];
		for (; j < 16; j++)
			((unsigned char *)&last_block)[j] = 0;
		tmp1 = last_block;
		REFLECT(tmp1);
		X = _mm_xor_si128(X, tmp1);
		gfmul_decrypt(X, H, &X);
	}
	tmp1 = _mm_insert_epi64(tmp1, nbytes * 8, 0);
	tmp1 = _mm_insert_epi64(tmp1, abytes * 8, 1);
	REFLECT(tmp1);
	tmp1 = _mm_shuffle_epi8(tmp1, MASKF);
	X = _mm_xor_si128(X, tmp1);
	gfmul_decrypt(X, H, &X);
	REFLECT(X);
	T = _mm_xor_si128(X, T);
	_mm_storeu_si128((__m128i *) tag, T);
}

int 
AES_GCM_decrypt(const unsigned char *in,
		unsigned char *out,
		const unsigned char *addt,
		const unsigned char *ivec,
		unsigned char *tag,
		int nbytes,
		int abytes,
		int ibytes,
		const unsigned char *key,
		int nr)
{
	int		i         , j, k;
	__m128i		hlp1 /* , hlp2, hlp3, hlp4 */ ;
	__m128i		tmp1  , tmp2, tmp3, tmp4;
	__m128i		H     , T;
	__m128i        *KEY = (__m128i *) key;
	__m128i		ctr1  , ctr2, ctr3, ctr4;
	__m128i		last_block = _mm_setzero_si128();
	__m128i		ONE = _mm_set_epi32(0, 1, 0, 0);
	__m128i		FOUR = _mm_set_epi32(0, 4, 0, 0);
	__m128i		BSWAP_EPI64 = _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5,
					    6, 7);
	__m128i		BSWAP_MASK = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
					   14, 15);
	__m128i		X = _mm_setzero_si128(), Y = _mm_setzero_si128();
	__m128i		AMASK = _mm_set_epi32(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f);
	__m128i		MASKL = _mm_set_epi32(0x0f070b03, 0x0d050901, 0x0e060a02, 0x0c040800);
	__m128i		MASKH = _mm_set_epi32(0xf070b030, 0xd0509010, 0xe060a020, 0xc0408000);
	__m128i		MASKF = _mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f);

	if (ibytes == 96 / 8) {
		Y = _mm_loadu_si128((__m128i *) ivec);
		Y = _mm_insert_epi32(Y, 0x1000000, 3);
		/* (Compute E[ZERO, KS] and E[Y0, KS] together */
		tmp1 = _mm_xor_si128(X, KEY[0]);
		tmp2 = _mm_xor_si128(Y, KEY[0]);
		for (j = 1; j < nr - 1; j += 2) {
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j]);
			tmp2 = _mm_aesenc_si128(tmp2, KEY[j]);
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j + 1]);
			tmp2 = _mm_aesenc_si128(tmp2, KEY[j + 1]);
		};
		tmp1 = _mm_aesenc_si128(tmp1, KEY[nr - 1]);
		tmp2 = _mm_aesenc_si128(tmp2, KEY[nr - 1]);
		H = _mm_aesenclast_si128(tmp1, KEY[nr]);
		T = _mm_aesenclast_si128(tmp2, KEY[nr]);
		REFLECT(H);
	} else {
		tmp1 = _mm_xor_si128(X, KEY[0]);
		for (j = 1; j < nr; j++)
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j]);
		H = _mm_aesenclast_si128(tmp1, KEY[nr]);
		REFLECT(H);
		Y = _mm_xor_si128(Y, Y);
		for (i = 0; i < ibytes / 16; i++) {
			tmp1 = _mm_loadu_si128(&((__m128i *) ivec)[i]);
			REFLECT(tmp1);
			Y = _mm_xor_si128(Y, tmp1);
			gfmul_decrypt(Y, H, &Y);
		}
		if (ibytes % 16) {
			for (j = 0; j < ibytes % 16; j++)
				((unsigned char *)&last_block)[j] = ivec[i * 16 + j];
			tmp1 = last_block;
			REFLECT(tmp1);
			Y = _mm_xor_si128(Y, tmp1);
			gfmul_decrypt(Y, H, &Y);
		}
		tmp1 = _mm_insert_epi64(tmp1, ibytes * 8, 0);
		tmp1 = _mm_insert_epi64(tmp1, 0, 1);
		REFLECT(tmp1);
		tmp1 = _mm_shuffle_epi8(tmp1, MASKF);
		Y = _mm_xor_si128(Y, tmp1);
		gfmul_decrypt(Y, H, &Y);
		REFLECT(Y);
		/* Compute E(K, Y0) */
		tmp1 = _mm_xor_si128(Y, KEY[0]);
		for (j = 1; j < nr; j++)
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j]);
		T = _mm_aesenclast_si128(tmp1, KEY[nr]);
	}
	for (i = 0; i < abytes / 16; i++) {
		tmp1 = _mm_loadu_si128(&((__m128i *) addt)[i]);
		REFLECT(tmp1);
		X = _mm_xor_si128(X, tmp1);
		gfmul_decrypt(X, H, &X);
	}
	if (abytes % 16) {
		last_block = _mm_setzero_si128();
		for (j = 0; j < abytes % 16; j++)
			((unsigned char *)&last_block)[j] = addt[i * 16 + j];
		tmp1 = last_block;
		REFLECT(tmp1);
		X = _mm_xor_si128(X, tmp1);
		gfmul_decrypt(X, H, &X);
	}
	for (i = 0; i < nbytes / 16; i++) {
		tmp1 = _mm_loadu_si128(&((__m128i *) in)[i]);
		REFLECT(tmp1);
		X = _mm_xor_si128(X, tmp1);
		gfmul_decrypt(X, H, &X);
	}
	if (nbytes % 16) {
		last_block = _mm_setzero_si128();
		for (j = 0; j < nbytes % 16; j++)
			((unsigned char *)&last_block)[j] = in[i * 16 + j];
		tmp1 = last_block;
		REFLECT(tmp1);
		X = _mm_xor_si128(X, tmp1);
		gfmul_decrypt(X, H, &X);
	}
	tmp1 = _mm_insert_epi64(tmp1, nbytes * 8, 0);
	tmp1 = _mm_insert_epi64(tmp1, abytes * 8, 1);
	REFLECT(tmp1);
	tmp1 = _mm_shuffle_epi8(tmp1, MASKF);
	X = _mm_xor_si128(X, tmp1);
	gfmul_decrypt(X, H, &X);
	REFLECT(X);
	T = _mm_xor_si128(X, T);
	if (!m128icmp(T, _mm_loadu_si128((__m128i*)tag)))
		return 0; //in case the authentication failed

	//in case the authentication failed
	ctr1 = _mm_shuffle_epi8(Y, BSWAP_EPI64);
	ctr1 = _mm_add_epi64(ctr1, ONE);
	ctr2 = _mm_add_epi64(ctr1, ONE);
	ctr3 = _mm_add_epi64(ctr2, ONE);
	ctr4 = _mm_add_epi64(ctr3, ONE);
	for (i = 0; i < nbytes / 16 / 4; i++) {
		tmp1 = _mm_shuffle_epi8(ctr1, BSWAP_EPI64);
		tmp2 = _mm_shuffle_epi8(ctr2, BSWAP_EPI64);
		tmp3 = _mm_shuffle_epi8(ctr3, BSWAP_EPI64);
		tmp4 = _mm_shuffle_epi8(ctr4, BSWAP_EPI64);
		ctr1 = _mm_add_epi64(ctr1, FOUR);
		ctr2 = _mm_add_epi64(ctr2, FOUR);
		ctr3 = _mm_add_epi64(ctr3, FOUR);
		ctr4 = _mm_add_epi64(ctr4, FOUR);
		tmp1 = _mm_xor_si128(tmp1, KEY[0]);
		tmp2 = _mm_xor_si128(tmp2, KEY[0]);
		tmp3 = _mm_xor_si128(tmp3, KEY[0]);
		tmp4 = _mm_xor_si128(tmp4, KEY[0]);
		for (j = 1; j < nr - 1; j += 2) {
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j]);
			tmp2 = _mm_aesenc_si128(tmp2, KEY[j]);
			tmp3 = _mm_aesenc_si128(tmp3, KEY[j]);
			tmp4 = _mm_aesenc_si128(tmp4, KEY[j]);
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j + 1]);
			tmp2 = _mm_aesenc_si128(tmp2, KEY[j + 1]);
			tmp3 = _mm_aesenc_si128(tmp3, KEY[j + 1]);
			tmp4 = _mm_aesenc_si128(tmp4, KEY[j + 1]);
		}
		tmp1 = _mm_aesenc_si128(tmp1, KEY[nr - 1]);
		tmp2 = _mm_aesenc_si128(tmp2, KEY[nr - 1]);
		tmp3 = _mm_aesenc_si128(tmp3, KEY[nr - 1]);
		tmp4 = _mm_aesenc_si128(tmp4, KEY[nr - 1]);
		tmp1 = _mm_aesenclast_si128(tmp1, KEY[nr]);
		tmp2 = _mm_aesenclast_si128(tmp2, KEY[nr]);
		tmp3 = _mm_aesenclast_si128(tmp3, KEY[nr]);
		tmp4 = _mm_aesenclast_si128(tmp4, KEY[nr]);
		tmp1 = _mm_xor_si128(tmp1, _mm_loadu_si128(&((__m128i *) in)[i * 4 + 0]));
		tmp2 = _mm_xor_si128(tmp2, _mm_loadu_si128(&((__m128i *) in)[i * 4 + 1]));
		tmp3 = _mm_xor_si128(tmp3, _mm_loadu_si128(&((__m128i *) in)[i * 4 + 2]));
		tmp4 = _mm_xor_si128(tmp4, _mm_loadu_si128(&((__m128i *) in)[i * 4 + 3]));
		_mm_storeu_si128(&((__m128i *) out)[i * 4 + 0], tmp1);
		_mm_storeu_si128(&((__m128i *) out)[i * 4 + 1], tmp2);
		_mm_storeu_si128(&((__m128i *) out)[i * 4 + 2], tmp3);
		_mm_storeu_si128(&((__m128i *) out)[i * 4 + 3], tmp4);
		tmp1 = _mm_shuffle_epi8(tmp1, BSWAP_MASK);
		tmp2 = _mm_shuffle_epi8(tmp2, BSWAP_MASK);
		tmp3 = _mm_shuffle_epi8(tmp3, BSWAP_MASK);
		tmp4 = _mm_shuffle_epi8(tmp4, BSWAP_MASK);
	}
	for (k = i * 4; k < nbytes / 16; k++) {
		tmp1 = _mm_shuffle_epi8(ctr1, BSWAP_EPI64);
		ctr1 = _mm_add_epi64(ctr1, ONE);
		tmp1 = _mm_xor_si128(tmp1, KEY[0]);
		for (j = 1; j < nr - 1; j += 2) {
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j]);
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j + 1]);
		}
		tmp1 = _mm_aesenc_si128(tmp1, KEY[nr - 1]);
		tmp1 = _mm_aesenclast_si128(tmp1, KEY[nr]);
		tmp1 = _mm_xor_si128(tmp1, _mm_loadu_si128(&((__m128i *) in)[k]));
		_mm_storeu_si128(&((__m128i *) out)[k], tmp1);
	}
	//If one partial block remains
	if (nbytes % 16) {
		tmp1 = _mm_shuffle_epi8(ctr1, BSWAP_EPI64);
		tmp1 = _mm_xor_si128(tmp1, KEY[0]);
		for (j = 1; j < nr - 1; j += 2) {
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j]);
			tmp1 = _mm_aesenc_si128(tmp1, KEY[j + 1]);
		}
		tmp1 = _mm_aesenc_si128(tmp1, KEY[nr - 1]);
		tmp1 = _mm_aesenclast_si128(tmp1, KEY[nr]);
		tmp1 = _mm_xor_si128(tmp1, _mm_loadu_si128(&((__m128i *) in)[k]));
		last_block = tmp1;
		for (j = 0; j < nbytes % 16; j++)
			out[k * 16 + j] = ((unsigned char *)&last_block)[j];
	}
	return 1;
	//when sucessfull returns 1
}
