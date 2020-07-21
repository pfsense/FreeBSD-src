#ifndef _CURVE25519_H_
#define _CURVE25519_H_

#include <sys/systm.h>

#define CURVE25519_KEY_SIZE 32

void curve25519_generic(u8 [CURVE25519_KEY_SIZE],
			const u8 [CURVE25519_KEY_SIZE],
			const u8 [CURVE25519_KEY_SIZE]);

static inline void curve25519_clamp_secret(u8 secret[CURVE25519_KEY_SIZE])
{
	secret[0] &= 248;
	secret[31] = (secret[31] & 127) | 64;
}

static const u8 null_point[CURVE25519_KEY_SIZE] = { 0 };

static inline int curve25519(u8 mypublic[CURVE25519_KEY_SIZE],
		const u8 secret[CURVE25519_KEY_SIZE],
		const u8 basepoint[CURVE25519_KEY_SIZE])
{
	curve25519_generic(mypublic, secret, basepoint);
	return timingsafe_bcmp(mypublic, null_point, CURVE25519_KEY_SIZE);
}

static inline int curve25519_generate_public(u8 pub[CURVE25519_KEY_SIZE],
				const u8 secret[CURVE25519_KEY_SIZE])
{
	static const u8 basepoint[CURVE25519_KEY_SIZE] __aligned(32) = { 9 };

	if (timingsafe_bcmp(secret, null_point, CURVE25519_KEY_SIZE) == 0)
		return 0;

	return curve25519(pub, secret, basepoint);
}

static inline void curve25519_generate_secret(u8 secret[CURVE25519_KEY_SIZE])
{
	arc4random_buf(secret, CURVE25519_KEY_SIZE);
	curve25519_clamp_secret(secret);
}




#endif /* _CURVE25519_H_ */
