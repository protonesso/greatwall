# Copyright 2004-2010 SmoothWall Ltd

include ../Makefile
include ../Makefile.conf
include ../Makefile.versions

PACKAGE = xtables-addons
VERSION =  $(XTABLES_ADDONS_VER)

EXTENSION = .tar.xz

BASE_URL = http://downloads.sourceforge.net/project/xtables-addons/Xtables-addons/$(VERSION)

CONFIG_OPTS += --with-gnu-ld \
	      --with-kbuild=/build/sources/linux$(KERNEL_TYPE)/linux/ \
	      --with-ksource=/lib/modules/$(K_SWE_VERSION)$(KERNEL_TYPE)/source

include ../Makefile.rules
