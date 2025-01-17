# Time-stamp: <05/09/09 21:56:22 ptr>
# $Id: top.mak,v 1.1.2.4 2005/09/19 19:55:47 dums Exp $

.SUFFIXES:
.SCCS_GET:
.RCS_GET:

PHONY ?=

RULESBASE ?= $(SRCROOT)/Makefiles

ALL_TAGS ?= release-shared	dbg-shared	stldbg-shared

all:	$(OUTPUT_DIRS) $(ALL_TAGS)

all-static:	release-static	dbg-static	stldbg-static
all-shared:	release-shared	dbg-shared	stldbg-shared

# include file, generated by configure, if available
-include ${RULESBASE}/config.mak
# define what make clone we use
include ${RULESBASE}/make.mak
ifndef OSNAME
# identify OS and build date
include ${RULESBASE}/$(USE_MAKE)/sysid.mak
endif
# OS-specific definitions, like ln, install, etc. (guest host)
include ${RULESBASE}/$(USE_MAKE)/$(BUILD_OSNAME)/sys.mak
# target OS-specific definitions, like ar, etc.
include ${RULESBASE}/$(USE_MAKE)/$(OSNAME)/targetsys.mak
# compiler, compiler options
include ${RULESBASE}/$(USE_MAKE)/$(COMPILER_NAME).mak
# rules to make dirs for targets
include ${RULESBASE}/$(USE_MAKE)/targetdirs.mak
# extern libraries
include ${RULESBASE}/$(USE_MAKE)/$(OSNAME)/extern.mak

# os-specific local rules
-include specific.mak

# derive common targets (*.o, *.d),
# build rules (including output catalogs)
include ${RULESBASE}/$(USE_MAKE)/targets.mak
# dependency
ifneq ($(OSNAME),windows)
include ${RULESBASE}/$(USE_MAKE)/depend.mak
endif

# general clean
include ${RULESBASE}/clean.mak

# if target is library, rules for library
ifdef LIBNAME
include ${RULESBASE}/$(USE_MAKE)/lib/top.mak
endif

# if target is program, rules for executable
ifdef PRGNAME
include ${RULESBASE}/$(USE_MAKE)/app/top.mak
endif

.PHONY: $(PHONY)
