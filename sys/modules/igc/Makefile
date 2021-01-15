# $FreeBSD$

.PATH:  ${SRCTOP}/sys/dev/igc

KMOD	= if_igc
SRCS	= device_if.h bus_if.h pci_if.h ifdi_if.h
SRCS	+= opt_inet.h opt_inet6.h opt_rss.h
SRCS	+= if_igc.c igc_api.c igc_base.c igc_i225.c igc_mac.c igc_manage.c
SRCS	+= igc_nvm.c igc_osdep.c igc_phy.c igc_txrx.c

.include <bsd.kmod.mk>
