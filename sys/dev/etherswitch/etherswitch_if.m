# $FreeBSD$

#include <sys/bus.h>

# Needed for ifreq/ifmediareq
#include <sys/socket.h>
#include <net/if.h>

#include <dev/etherswitch/etherswitch.h>

INTERFACE etherswitch;

#
# Default implementation
#
CODE {
	static void
	null_etherswitch_lock(device_t dev)
	{
	}

	static void
	null_etherswitch_unlock(device_t dev)
	{
	}

	static int
	null_etherswitch_getlaggroup(device_t dev, etherswitch_laggroup_t *conf)
	{
		return (EINVAL);
	}

	static int
	null_etherswitch_setlaggroup(device_t dev, etherswitch_laggroup_t *conf)
	{
		return (EINVAL);
	}

	static int
	null_etherswitch_getconf(device_t dev, etherswitch_conf_t *conf)
	{
		return (0);
	}

	static int
	null_etherswitch_setconf(device_t dev, etherswitch_conf_t *conf)
	{
		return (0);
	}

	static ssize_t
	null_etherswitch_getioblksize(device_t dev)
	{
		return (-1);
	}

	static ssize_t
	null_etherswitch_getiosize(device_t dev)
	{
		return (-1);
	}

	static void *
	null_etherswitch_getiobuf(device_t dev)
	{
		return (NULL);
	}

	static int
	null_etherswitch_ioread(device_t dev, off_t off, ssize_t len)
	{
		return (EIO);
	}

	static int
	null_etherswitch_iowrite(device_t dev, off_t off, ssize_t len)
	{
		return (EIO);
	}
};

#
# Return device info
#
METHOD etherswitch_info_t* getinfo {
	device_t	dev;
}

#
# Lock access to switch registers
#
METHOD void lock {
	device_t	dev;
} DEFAULT null_etherswitch_lock;

#
# Unlock access to switch registers
#
METHOD void unlock {
	device_t	dev;
} DEFAULT null_etherswitch_unlock;

#
# Read switch register
#
METHOD int readreg {
	device_t	dev;
	int		reg;
};

#
# Write switch register
#
METHOD int writereg {
	device_t	dev;
	int		reg;
	int		value;
};

#
# Read PHY register
#
METHOD int readphyreg {
	device_t	dev;
	int		phy;
	int		reg;
};

#
# Write PHY register
#
METHOD int writephyreg {
	device_t	dev;
	int		phy;
	int		reg;
	int		value;
};

#
# Get port configuration
#
METHOD int getport {
	device_t	dev;
	etherswitch_port_t *vg;
}

#
# Set port configuration
#
METHOD int setport {
	device_t	dev;
	etherswitch_port_t *vg;
}

#
# Get VLAN group configuration
#
METHOD int getvgroup {
	device_t	dev;
	etherswitch_vlangroup_t *vg;
}

#
# Set VLAN group configuration
#
METHOD int setvgroup {
	device_t	dev;
	etherswitch_vlangroup_t *vg;
}

#
# Get LAGG configuration
#
METHOD int getlaggroup {
	device_t	dev;
	etherswitch_laggroup_t *vg;
} DEFAULT null_etherswitch_getlaggroup;

#
# Set LAGG configuration
#
METHOD int setlaggroup {
	device_t	dev;
	etherswitch_laggroup_t *vg;
} DEFAULT null_etherswitch_setlaggroup;

#
# Get the Switch configuration
#
METHOD int getconf {
	device_t	dev;
	etherswitch_conf_t	*conf;
} DEFAULT null_etherswitch_getconf;

#
# Set the Switch configuration
#
METHOD int setconf {
	device_t	dev;
	etherswitch_conf_t	*conf;
} DEFAULT null_etherswitch_setconf;

#
# Get the IO buffer block size
#
METHOD ssize_t getioblksize {
	device_t	dev;
} DEFAULT null_etherswitch_getioblksize;

#
# Get the IO buffer size
#
METHOD ssize_t getiosize {
	device_t	dev;
} DEFAULT null_etherswitch_getiosize;

#
# Get the IO buffer
#
METHOD void * getiobuf {
	device_t	dev;
} DEFAULT null_etherswitch_getiobuf;

#
# Perform a read operation and save data into IO buffer
#
METHOD int ioread {
	device_t	dev;
	off_t		off;
	ssize_t		len;
} DEFAULT null_etherswitch_ioread;

#
# Perform a write operation (write the data in the IO buffer)
#
METHOD int iowrite {
	device_t	dev;
	off_t		off;
	ssize_t		len;
} DEFAULT null_etherswitch_iowrite;
