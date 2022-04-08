# $FreeBSD$

PACKAGE=lib${LIB}
SHLIBDIR?= /lib

.include <src.opts.mk>

LIB=	nv
SHLIB_MAJOR= 0

VERSION_DEF=	${SRCTOP}/lib/libc/Versions.def
SYMBOL_MAPS=	${.CURDIR}/Symbol.map

.PATH: ${SRCTOP}/sys/contrib/libnv ${SRCTOP}/sys/sys
CFLAGS+=-I${.CURDIR}

SRCS=	cnvlist.c
SRCS+=	dnvlist.c
SRCS+=	msgio.c
SRCS+=	nvlist.c
SRCS+=	nvpair.c

HAS_TESTS=
SUBDIR.${MK_TESTS}+= tests

.include <bsd.lib.mk>
