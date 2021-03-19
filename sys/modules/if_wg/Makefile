.if !defined(WITH_CTF)
WITH_CTF=1
.endif

.include <bsd.sys.mk>

SRCDIR= ${.CURDIR}/module
INCDIR=${.CURDIR}/include
ZINCDIR= ${SRCDIR}/crypto/zinc

KMOD=   if_wg

.PATH: ${SRCDIR}
.PATH: ${ZINCDIR}
.PATH: ${ZINCDIR}/chacha20
.PATH: ${ZINCDIR}/poly1305

CFLAGS+= -I${INCDIR}

CFLAGS+= -D__KERNEL__ -D__BSD_VISIBLE=1 -DINET6=1 -DINET=1
CFLAGS+= -ferror-limit=7
CFLAGS+= -include ${INCDIR}/sys/support.h

.if defined(WITH_DEBUG) && ${WITH_DEBUG} == "true"
CFLAGS+= -DINVARIANTS -DWITNESS -g -O0
.else
CFLAGS += -DNDEBUG
.endif

DEBUG_FLAGS=-g

SRCS+= opt_inet.h opt_inet6.h device_if.h bus_if.h ifdi_if.h

#SRCS+= module.c cookie.c noise.c peer.c whitelist.c
SRCS+= wg_noise.c wg_cookie.c
SRCS+= if_wg_session.c module.c curve25519.c blake2s.c
SRCS+= chacha20poly1305.c chacha20.c poly1305.c
.if ${MACHINE_ARCH} == "amd64"
SRCS += poly1305-x86_64.S chacha20-x86_64.S
SIMD_FLAGS = -DCONFIG_AS_SSSE3=1 -DCONFIG_AS_AVX=1
SIMD_FLAGS+= -DCONFIG_AS_AVX512=1 -DCONFIG_AS_AVX2=1
.endif
.include <bsd.kmod.mk>
CFLAGS.poly1305-x86_64.S = -D__LOCORE -gdwarf-4 ${SIMD_FLAGS}
CFLAGS.chacha20-x86_64.S = -D__LOCORE -gdwarf-4 ${SIMD_FLAGS}
.if ${MACHINE_ARCH} == "amd64"
CFLAGS.chacha20poly1305.c = -DCONFIG_ZINC_ARCH_X86_64
CFLAGS.chacha20.c = -DCONFIG_ZINC_ARCH_X86_64
CFLAGS.poly1305.c = -DCONFIG_ZINC_ARCH_X86_64
.endif
