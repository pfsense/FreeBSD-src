/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Rubicon Communications, LLC (Netgate)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
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
#ifndef RESCUE
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/nv.h>

#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <net/route.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>		/* NB: for offsetof */
#include <locale.h>
#include <langinfo.h>

#include "ifconfig.h"

typedef enum {
	WGC_PEER_ADD = 0x1,
	WGC_PEER_DEL = 0x2,
	WGC_PEER_UPDATE = 0x3,
	WGC_PEER_LIST = 0x4,
	WGC_LOCAL_SHOW = 0x5,
} wg_cmd_t;

static nvlist_t *nvl_params;
static bool do_peer;
static int allowed_ips_count;
static int allowed_ips_max;
struct allowedip {
	struct sockaddr a_addr;
	struct sockaddr a_mask;
};
struct allowedip *allowed_ips;

#define	ALLOWEDIPS_START 16
#define	WG_KEY_LEN 32
#define	WG_KEY_LEN_BASE64 ((((WG_KEY_LEN) + 2) / 3) * 4 + 1)
#define	WG_KEY_LEN_HEX (WG_KEY_LEN * 2 + 1)
#define	WG_MAX_STRLEN 64

//CTASSERT(WG_MAX_STRLEN > WG_KEY_LEN_BASE64);
//CTASSERT(WG_MAX_STRLEN > INET6_ADDRSTRLEN);

static void encode_base64(u_int8_t *, const u_int8_t *, u_int16_t);
static bool decode_base64(u_int8_t *, u_int16_t, const u_int8_t *);

const static u_int8_t Base64Code[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const static u_int8_t index_64[128] = {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 62, 255, 255, 255, 63, 52, 53,
        54, 55, 56, 57, 58, 59, 60, 61, 255, 255,
        255, 255, 255, 255, 255, 0, 1, 2, 3, 4,
        5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
        255, 255, 255, 255, 255, 255, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
        39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, 255, 255, 255, 255, 255
};
#define CHAR64(c)  ( (c) > 127 ? 255 : index_64[(c)])
static bool
decode_base64(u_int8_t *buffer, u_int16_t len, const u_int8_t *data)
{
	const uint8_t *p = data;
	uint8_t *bp = buffer;
	uint8_t c1, c2, c3, c4;

	while (bp < buffer + len) {
		c1 = CHAR64(*p);
		c2 = CHAR64(*(p + 1));

		/* Invalid data */
		if (c1 == 255 || c2 == 255)
			break;

		*bp++ = (c1 << 2) | ((c2 & 0x30) >> 4);
		if (bp >= buffer + len)
			break;

		c3 = CHAR64(*(p + 2));
		if (c3 == 255)
			break;

		*bp++ = ((c2 & 0x0f) << 4) | ((c3 & 0x3c) >> 2);
		if (bp >= buffer + len)
			break;

		c4 = CHAR64(*(p + 3));
		if (c4 == 255)
			break;

		*bp++ = ((c3 & 0x03) << 6) | c4;

		p += 4;
	}
	if (bp < buffer + len)
		printf("len: %d filled: %d\n", len,
			   (int)(((uintptr_t)bp) - ((uintptr_t)buffer)));

	return (bp >= buffer + len);
}

static void
encode_base64(u_int8_t *buffer, const uint8_t *data, u_int16_t len)
{
	u_int8_t *bp = buffer;
	const u_int8_t *p = data;
	u_int8_t c1, c2;
	while (p < data + len) {
		c1 = *p++;
		*bp++ = Base64Code[(c1 >> 2)];
		c1 = (c1 & 0x03) << 4;
		if (p >= data + len) {
			*bp++ = Base64Code[c1];
			break;
		}
		c2 = *p++;
		c1 |= (c2 >> 4) & 0x0f;
		*bp++ = Base64Code[c1];
		c1 = (c2 & 0x0f) << 2;
		if (p >= data + len) {
			*bp++ = Base64Code[c1];
			break;
		}
		c2 = *p++;
		c1 |= (c2 >> 6) & 0x03;
		*bp++ = Base64Code[c1];
		*bp++ = Base64Code[c2 & 0x3f];
	}
	*bp = '\0';
}

static bool
key_from_base64(uint8_t key[static WG_KEY_LEN], const char *base64)
{

	if (strlen(base64) != WG_KEY_LEN_BASE64 - 1) {
		warnx("bad key len - need %d got %lu\n", WG_KEY_LEN_BASE64 - 1, strlen(base64));
		return false;
	}
	if (base64[WG_KEY_LEN_BASE64 - 2] != '=') {
		warnx("bad key terminator, expected '=' got '%c'", base64[WG_KEY_LEN_BASE64 - 2]);
		return false;
	}
	return (decode_base64(key, WG_KEY_LEN, base64));
}

static void
parse_endpoint(const char *endpoint_)
{
	int err;
	char *base, *endpoint, *port, *colon, *tmp;
	struct addrinfo hints, *res;

	endpoint = base = strdup(endpoint_);
	colon = rindex(endpoint, ':');
	if (colon == NULL)
		errx(1, "bad endpoint format %s - no port delimiter found", endpoint);
	*colon = '\0';
	port = colon + 1;

	/* [::]:<> */
	if (endpoint[0] == '[') {
		endpoint++;
		tmp = index(endpoint, ']');
		if (tmp == NULL)
			errx(1, "bad endpoint format %s - '[' found with no matching ']'", endpoint);
		*tmp = '\0';
	}
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_NUMERICHOST;
	err = getaddrinfo(endpoint, port, &hints, &res);
	if (err)
		errx(err, "address resolution for endpoint %s:%s failed\n", endpoint, port);
	nvlist_add_binary(nvl_params, "endpoint", res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);
	free(base);
}

static void
in_len2mask(struct in_addr *mask, u_int len)
{
	u_int i;
	u_char *p;

	p = (u_char *)mask;
	memset(mask, 0, sizeof(*mask));
	for (i = 0; i < len / NBBY; i++)
		p[i] = 0xff;
	if (len % NBBY)
		p[i] = (0xff00 >> (len % NBBY)) & 0xff;
}

static u_int
in_mask2len(struct in_addr *mask)
{
	u_int x, y;
	u_char *p;

	p = (u_char *)mask;
	for (x = 0; x < sizeof(*mask); x++) {
		if (p[x] != 0xff)
			break;
	}
	y = 0;
	if (x < sizeof(*mask)) {
		for (y = 0; y < NBBY; y++) {
			if ((p[x] & (0x80 >> y)) == 0)
				break;
		}
	}
	return x * NBBY + y;
}

static void
in6_prefixlen2mask(struct in6_addr *maskp, int len)
{
	static const u_char maskarray[NBBY] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};
	int bytelen, bitlen, i;

	/* sanity check */
	if (len < 0 || len > 128) {
		errx(1, "in6_prefixlen2mask: invalid prefix length(%d)\n",
		    len);
		return;
	}

	memset(maskp, 0, sizeof(*maskp));
	bytelen = len / NBBY;
	bitlen = len % NBBY;
	for (i = 0; i < bytelen; i++)
		maskp->s6_addr[i] = 0xff;
	if (bitlen)
		maskp->s6_addr[bytelen] = maskarray[bitlen - 1];
}

static int
in6_mask2len(struct in6_addr *mask, u_char *lim0)
{
	int x = 0, y;
	u_char *lim = lim0, *p;

	/* ignore the scope_id part */
	if (lim0 == NULL || lim0 - (u_char *)mask > sizeof(*mask))
		lim = (u_char *)mask + sizeof(*mask);
	for (p = (u_char *)mask; p < lim; x++, p++) {
		if (*p != 0xff)
			break;
	}
	y = 0;
	if (p < lim) {
		for (y = 0; y < NBBY; y++) {
			if ((*p & (0x80 >> y)) == 0)
				break;
		}
	}

	/*
	 * when the limit pointer is given, do a stricter check on the
	 * remaining bits.
	 */
	if (p < lim) {
		if (y != 0 && (*p & (0x00ff >> y)) != 0)
			return -1;
		for (p = p + 1; p < lim; p++)
			if (*p != 0)
				return -1;
	}

	return x * NBBY + y;
}

static bool
parse_ip(struct allowedip *aip, const char *value)
{
	struct sockaddr *sa = __DECONST(void *, &aip->a_addr);

	bzero(&aip->a_addr, sizeof(aip->a_addr));
	aip->a_addr.sa_family = AF_UNSPEC;

	if (strchr(value, ':')) {
		struct sockaddr_in6 *sin6 = (void *)sa;
		if (inet_pton(AF_INET6, value, &sin6->sin6_addr) == 1)
			aip->a_addr.sa_family = AF_INET6;
		aip->a_addr.sa_len = sizeof(struct sockaddr_in6);
	} else {
		struct sockaddr_in *sin = (void *)sa;
		if (inet_pton(AF_INET, value, &sin->sin_addr) == 1)
			aip->a_addr.sa_family = AF_INET;
		aip->a_addr.sa_len = sizeof(struct sockaddr_in);
	}
	if (aip->a_addr.sa_family == AF_UNSPEC)
		return (false);
	return (true);
}

static const char *
sa_ntop(const struct sockaddr *sa, char *buf, int *port)
{
	const struct sockaddr_in *sin;
	const struct sockaddr_in6 *sin6;
	const char *bufp;

	if (sa->sa_family == AF_INET) {
		sin = (const struct sockaddr_in *)sa;
		bufp = inet_ntop(AF_INET, &sin->sin_addr, buf,
						 INET6_ADDRSTRLEN);
		if (port)
			*port = sin->sin_port;
	} else if (sa->sa_family == AF_INET6) {
		sin6 = (const struct sockaddr_in6 *)sa;
		bufp = inet_ntop(AF_INET6, &sin6->sin6_addr, buf,
						 INET6_ADDRSTRLEN);
		if (port)
			*port = sin6->sin6_port;
	}  else {
		errx(1, "%s got invalid sockaddr family %d\n", __func__, sa->sa_family);
	}
	if (bufp == NULL) {
		perror("failed to convert address for peer\n");
		errx(1, "peer list failure");
	}
	return (bufp);
}

static void
dump_peer(const nvlist_t *nvl_peer)
{
	const void *key;
	const struct allowedip *aips;
	const struct sockaddr *endpoint;
	char outbuf[WG_MAX_STRLEN];
	char addr_buf[INET6_ADDRSTRLEN];
	const char *bufp;
	size_t size;
	int count, port;

	printf("[Peer]\n");
	if (nvlist_exists_binary(nvl_peer, "public-key")) {
		key = nvlist_get_binary(nvl_peer, "public-key", &size);
		encode_base64(outbuf, (const uint8_t *)key, size);
		printf("PublicKey = %s\n", outbuf);
	}
	if (nvlist_exists_binary(nvl_peer, "endpoint")) {
		endpoint = nvlist_get_binary(nvl_peer, "endpoint", &size);
		bufp = sa_ntop(endpoint, addr_buf, &port);
		printf("Endpoint = %s:%d\n", bufp, ntohs(port));
	}

	if (!nvlist_exists_binary(nvl_peer, "allowed-ips"))
		return;
	aips = nvlist_get_binary(nvl_peer, "allowed-ips", &size);
	if (size == 0 || size % sizeof(struct allowedip) != 0) {
		errx(1, "size %zu not integer multiple of allowedip", size);
	}
	printf("AllowedIPs = ");
	count = size / sizeof(struct allowedip);
	for (int i = 0; i < count; i++) {
		int mask;
		sa_family_t family;
		void *bitmask;
		struct sockaddr *sa;

		sa = __DECONST(void *, &aips->a_addr);
		bitmask = __DECONST(void *, &aips->a_mask.sa_data);
		family = aips[i].a_addr.sa_family;
		inet_ntop(family, sa->sa_data, addr_buf, INET6_ADDRSTRLEN);
		if (family == AF_INET)
			mask = in_mask2len(bitmask);
		else if (family == AF_INET6)
			mask = in6_mask2len(bitmask, NULL);
		else
			errx(1, "bad family in peer %d\n", family);
		printf("%s/%d", addr_buf, mask);
		if (i < count -1)
			printf(", ");
	}
	printf("\n");
}

static int
get_nvl_out_size(int sock, u_long op, size_t *size)
{
	struct ifdrv ifd;
	int err;

	memset(&ifd, 0, sizeof(ifd));

	strlcpy(ifd.ifd_name, name, sizeof(ifd.ifd_name));
	ifd.ifd_cmd = op;
	ifd.ifd_len = 0;
	ifd.ifd_data = NULL;

	err = ioctl(sock, SIOCGDRVSPEC, &ifd);
	if (err)
		return (err);
	*size = ifd.ifd_len;
	return (0);
}

static int
do_cmd(int sock, u_long op, void *arg, size_t argsize, int set)
{
	struct ifdrv ifd;

	memset(&ifd, 0, sizeof(ifd));

	strlcpy(ifd.ifd_name, name, sizeof(ifd.ifd_name));
	ifd.ifd_cmd = op;
	ifd.ifd_len = argsize;
	ifd.ifd_data = arg;

	return (ioctl(sock, set ? SIOCSDRVSPEC : SIOCGDRVSPEC, &ifd));
}

static
DECL_CMD_FUNC(peerlist, val, d)
{
	size_t size, peercount;
	void *packed;
	const nvlist_t *nvl, *nvl_peer;
	const nvlist_t *const *nvl_peerlist;

	if (get_nvl_out_size(s, WGC_PEER_LIST, &size))
		errx(1, "can't get peer list size");
	if ((packed = malloc(size)) == NULL)
		errx(1, "malloc failed for peer list");
	if (do_cmd(s, WGC_PEER_LIST, packed, size, 0))
		errx(1, "failed to obtain peer list");

	nvl = nvlist_unpack(packed, size, 0);
	nvl_peerlist = nvlist_get_nvlist_array(nvl, "peer-list", &peercount);

	for (int i = 0; i < peercount; i++, nvl_peerlist++) {
		nvl_peer = *nvl_peerlist;
		dump_peer(nvl_peer);
	}
}

static void
peerfinish(int s, void *arg)
{
	void *packed;
	size_t size;

	if (!nvlist_exists_binary(nvl_params, "public-key"))
		errx(1, "must specify a public-key for adding peer");
	if (!nvlist_exists_binary(nvl_params, "endpoint"))
		errx(1, "must specify an endpoint for adding peer");
	if (allowed_ips_count == 0)
		errx(1, "must specify at least one range of allowed-ips to add a peer");

	packed = nvlist_pack(nvl_params, &size);
	if (packed == NULL)
		errx(1, "failed to setup create request");
	if (do_cmd(s, WGC_PEER_ADD, packed, size, true))
		errx(1, "failed to install peer");
}

static
DECL_CMD_FUNC(peerstart, val, d)
{
	do_peer = true;
	callback_register(peerfinish, NULL);
	allowed_ips = malloc(ALLOWEDIPS_START * sizeof(struct allowedip));
	allowed_ips_max = ALLOWEDIPS_START;
	if (allowed_ips == NULL)
		errx(1, "failed to allocate array for allowedips");
}

static
DECL_CMD_FUNC(setwglistenport, val, d)
{
	char *endp;
	u_long ul;

	ul = strtoul(val, &endp, 0);
	if (*endp != '\0')
		errx(1, "invalid value for listen-port");

	nvlist_add_number(nvl_params, "listen-port", ul);
}

static
DECL_CMD_FUNC(setwgprivkey, val, d)
{
	uint8_t key[WG_KEY_LEN];

	if (!key_from_base64(key, val))
		errx(1, "invalid key %s", val);
	nvlist_add_binary(nvl_params, "private-key", key, WG_KEY_LEN);
}

static
DECL_CMD_FUNC(setwgpubkey, val, d)
{
	uint8_t key[WG_KEY_LEN];

	if (!do_peer)
		errx(1, "setting public key only valid when adding peer");

	if (!key_from_base64(key, val))
		errx(1, "invalid key %s", val);
	nvlist_add_binary(nvl_params, "public-key", key, WG_KEY_LEN);
}

static
DECL_CMD_FUNC(setallowedips, val, d)
{
	char *base, *allowedip, *mask;
	u_long ul;
	char *endp;
	struct allowedip *aip;

	if (!do_peer)
		errx(1, "setting allowed ip only valid when adding peer");
	if (allowed_ips_count == allowed_ips_max) {
		/* XXX grow array */
	}
	aip = &allowed_ips[allowed_ips_count];
	base = allowedip = strdup(val);
	mask = index(allowedip, '/');
	if (mask == NULL)
		errx(1, "mask separator not found in allowedip %s", val);
	*mask = '\0';
	mask++;
	parse_ip(aip, allowedip);
	ul = strtoul(mask, &endp, 0);
	if (*endp != '\0')
		errx(1, "invalid value for allowedip mask");
	bzero(&aip->a_mask, sizeof(aip->a_mask));
	if (aip->a_addr.sa_family == AF_INET)
		in_len2mask((struct in_addr *)&aip->a_mask.sa_data, ul);
	else if (aip->a_addr.sa_family == AF_INET6)
		in6_prefixlen2mask((struct in6_addr *)&aip->a_mask.sa_data, ul);
	else
		errx(1, "invalid address family %d\n", aip->a_addr.sa_family);
	allowed_ips_count++;
	if (allowed_ips_count > 1)
		nvlist_free_binary(nvl_params, "allowed-ips");
	nvlist_add_binary(nvl_params, "allowed-ips", allowed_ips,
					  allowed_ips_count*sizeof(*aip));

	dump_peer(nvl_params);
	free(base);
}

static
DECL_CMD_FUNC(setendpoint, val, d)
{
	if (!do_peer)
		errx(1, "setting endpoint only valid when adding peer");
	parse_endpoint(val);
}

static int
is_match(void)
{
	if (strncmp("wg", name, 2))
		return (-1);
	if (strlen(name) < 3)
		return (-1);
	if (!isdigit(name[2]))
		return (-1);
	return (0);
}

static void
wireguard_status(int s)
{
	size_t size;
	void *packed;
	nvlist_t *nvl;
	char buf[WG_KEY_LEN_BASE64];
	const void *key;
	uint16_t listen_port;

	if (is_match() < 0) {
		/* If it's not a wg interface just return */
		return;
	}
	if (get_nvl_out_size(s, WGC_LOCAL_SHOW, &size))
		return;
	if ((packed = malloc(size)) == NULL)
		return;
	if (do_cmd(s, WGC_LOCAL_SHOW, packed, size, 0))
		return;
	nvl = nvlist_unpack(packed, size, 0);
	if (nvlist_exists_number(nvl, "listen-port")) {
		listen_port = nvlist_get_number(nvl, "listen-port");
		printf("\tlisten-port: %d\n", listen_port);
	}
	if (nvlist_exists_binary(nvl, "private-key")) {
		key = nvlist_get_binary(nvl, "private-key", &size);
		encode_base64(buf, (const uint8_t *)key, size);
		printf("\tprivate-key: %s\n", buf);
	}
	if (nvlist_exists_binary(nvl, "public-key")) {
		key = nvlist_get_binary(nvl, "public-key", &size);
		encode_base64(buf, (const uint8_t *)key, size);
		printf("\tpublic-key:  %s\n", buf);
	}
}

static struct cmd wireguard_cmds[] = {
    DEF_CLONE_CMD_ARG("listen-port",  setwglistenport),
    DEF_CLONE_CMD_ARG("private-key",  setwgprivkey),
    DEF_CMD("peer-list",  0, peerlist),
    DEF_CMD("peer",  0, peerstart),
    DEF_CMD_ARG("public-key",  setwgpubkey),
    DEF_CMD_ARG("allowed-ips",  setallowedips),
    DEF_CMD_ARG("endpoint",  setendpoint),
};

static struct afswtch af_wireguard = {
	.af_name	= "af_wireguard",
	.af_af		= AF_UNSPEC,
	.af_other_status = wireguard_status,
};

static void
wg_create(int s, struct ifreq *ifr)
{
	struct iovec iov;
	void *packed;
	size_t size;

	setproctitle("ifconfig %s create ...\n", name);
	if (!nvlist_exists_number(nvl_params, "listen-port"))
		goto legacy;
	if (!nvlist_exists_binary(nvl_params, "private-key"))
		goto legacy;

	packed = nvlist_pack(nvl_params, &size);
	if (packed == NULL)
		errx(1, "failed to setup create request");
	iov.iov_len = size;
	iov.iov_base = packed;
	ifr->ifr_data = (caddr_t)&iov;
	if (ioctl(s, SIOCIFCREATE2, ifr) < 0)
		err(1, "SIOCIFCREATE2");
	return;
legacy:
	ifr->ifr_data == NULL;
	if (ioctl(s, SIOCIFCREATE, ifr) < 0)
		err(1, "SIOCIFCREATE");
}

static __constructor void
wireguard_ctor(void)
{
	int i;

	nvl_params = nvlist_create(0);
	for (i = 0; i < nitems(wireguard_cmds);  i++)
		cmd_register(&wireguard_cmds[i]);
	af_register(&af_wireguard);
	clone_setdefcallback("wg", wg_create);
}

#endif
