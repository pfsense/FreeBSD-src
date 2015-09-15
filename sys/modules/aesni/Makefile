# $FreeBSD$

.PATH: ${.CURDIR}/../../crypto/aesni

KMOD=	aesni
SRCS=	aesni.c
SRCS+=	aeskeys_${MACHINE_CPUARCH}.S
SRCS+=	device_if.h bus_if.h opt_bus.h cryptodev_if.h

OBJS+=	aesni_wrap.o

# Remove -nostdinc so we can get the intrinsics.
aesni_wrap.o: aesni_wrap.c
	${CC} -c ${CFLAGS:C/^-O2$/-O3/:N-nostdinc} ${WERROR} ${PROF} \
	     -mmmx -msse -maes ${.IMPSRC}
	${CTFCONVERT_CMD}

.if ${MACHINE_CPUARCH} == "amd64"
OBJS+=	aesni_ghash.o
aesni_ghash.o: aesni_ghash.c
	${CC} -c ${CFLAGS:C/^-O2$/-O3/:N-nostdinc} ${WERROR} ${PROF} \
	     -mmmx -mpclmul -msse -msse4.1 -maes ${.IMPSRC}
	${CTFCONVERT_CMD}
.endif

.include <bsd.kmod.mk>

