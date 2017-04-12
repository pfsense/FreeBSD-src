# $FreeBSD$

.PATH: ${SRCTOP}/sys/arm/ti/cpsw

KMOD=   if_cpsw
SRCS=   if_cpsw.c device_if.h bus_if.h ofw_bus_if.h miibus_if.h opt_cpsw.h

.if !defined(OPT_CPSW_ETHERSWITCH) && defined(KERNBUILDDIR)
OPT_CPSW_ETHERSWITCH!= sed -n '/CPSW_ETHERSWITCH/p' ${KERNBUILDDIR}/opt_cpsw.h
.endif

.if !empty(OPT_CPSW_ETHERSWITCH)
SRCS+=	etherswitch_if.h
.endif

.include <bsd.kmod.mk>
