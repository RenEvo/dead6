# -*- Makefile -*- Time-stamp: <03/10/12 20:35:49 ptr>
# $Id: gcc.mak,v 1.1.2.1 2005/11/27 18:08:27 complement Exp $

SRCROOT := ../..
COMPILER_NAME := gcc

include Makefile.inc
include ${SRCROOT}/Makefiles/top.mak

INCLUDES += -I$(STLPORT_INCLUDE_DIR)

ifeq ($(OSNAME),linux)
DEFS += -D_GNU_SOURCE
endif
