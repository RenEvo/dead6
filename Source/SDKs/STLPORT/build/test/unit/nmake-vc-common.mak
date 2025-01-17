# -*- Makefile -*- Time-stamp: <04/03/30 14:53:15 ptr>
# $Id: nmake-vc-common.mak,v 1.1.2.7 2005/06/28 19:28:07 dums Exp $

SRCROOT=../..
STLPORT_DIR=../../..

!include Makefile.inc

INCLUDES=$(INCLUDES) /I$(STLPORT_INCLUDE_DIR) /I$(STLPORT_DIR)/src /FI vc_warning_disable.h

!ifdef STLP_BUILD_BOOST_PATH
INCLUDES=$(INCLUDES) /I "$(STLP_BUILD_BOOST_PATH)"
!endif

DEFS_DBG=/D_STLP_DEBUG_UNINITIALIZED
DEFS_STLDBG=/D_STLP_DEBUG_UNINITIALIZED
DEFS_STATIC_DBG=/D_STLP_DEBUG_UNINITIALIZED
DEFS_STATIC_STLDBG=/D_STLP_DEBUG_UNINITIALIZED

LDSEARCH=$(LDSEARCH) /LIBPATH:$(STLPORT_LIB_DIR)

!include $(SRCROOT)/Makefiles/nmake/top.mak
