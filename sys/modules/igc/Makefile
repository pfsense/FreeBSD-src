# $FreeBSD$

.PATH:  ${SRCTOP}/sys/dev/igc
KMOD	= if_igc
SRCS	= device_if.h bus_if.h pci_if.h opt_ddb.h opt_inet.h \
	  opt_inet6.h ifdi_if.h opt_rss.h
SRCS	+= if_igc.c igc_txrx.c igc_osdep.c
SRCS	+= igc_api.c igc_phy.c igc_nvm.c igc_mac.c igc_manage.c
SRCS	+= igc_i225.c igc_base.c 

CFLAGS += -I${SRCTOP}/sys/dev/igc

# DEVICE_POLLING for a non-interrupt-driven method
#CFLAGS  += -DDEVICE_POLLING

.include <bsd.kmod.mk>
