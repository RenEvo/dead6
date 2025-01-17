# Time-stamp: <04/04/30 23:36:48 ptr>
# $Id: evc-common.mak,v 1.1.2.4 2005/04/16 14:39:19 dums Exp $

# stuff not defined here: CXX, CC, DEFS_COMMON, CFLAGS_*, CXXFLAGS_*, OPT_COMMON

LINK = link.exe
RC = rc.exe

!ifdef DEFS
DEFS_REL = $(DEFS) $(DEFS_REL)
DEFS_DBG = $(DEFS) $(DEFS_DBG)
DEFS_STLDBG = $(DEFS) $(DEFS_STLDBG)
DEFS_STATIC_REL = $(DEFS) $(DEFS_STATIC_REL)
DEFS_STATIC_DBG = $(DEFS) $(DEFS_STATIC_DBG)
DEFS_STATIC_STLDBG = $(DEFS) $(DEFS_STATIC_STLDBG)
!endif
!ifdef OPT
OPT_REL = $(OPT) $(OPT_REL)
OPT_DBG = $(OPT) $(OPT_DBG)
OPT_STLDBG = $(OPT) $(OPT_STLDBG)
OPT_STATIC_REL = $(OPT) $(OPT_STATIC_REL)
OPT_STATIC_DBG = $(OPT) $(OPT_STATIC_DBG)
OPT_STATIC_STLDBG = $(OPT) $(OPT_STATIC_STLDBG)
!endif

OUTPUT_OPTION = /Fo$@ /Fd"$(PDB_NAME_OUT)"
OUTPUT_OPTION_DBG = /Fo$@ /Fd"$(PDB_NAME_OUT_DBG)"
OUTPUT_OPTION_STLDBG = /Fo$@ /Fd"$(PDB_NAME_OUT_STLDBG)"
OUTPUT_OPTION_STATIC = /Fo$@ /Fd"$(A_PDB_NAME_OUT)"
OUTPUT_OPTION_STATIC_DBG = /Fo$@ /Fd"$(A_PDB_NAME_OUT_DBG)"
OUTPUT_OPTION_STATIC_STLDBG = /Fo$@ /Fd"$(A_PDB_NAME_OUT_STLDBG)"
LINK_OUTPUT_OPTION = /OUT:$@
RC_OUTPUT_OPTION = /fo $@
RC_OUTPUT_OPTION_DBG = /fo $@
RC_OUTPUT_OPTION_STLDBG = /fo $@

DEFS_REL = $(DEFS_REL) $(DEFS_COMMON)
DEFS_STATIC_REL = $(DEFS_STATIC_REL) $(DEFS_COMMON)
DEFS_DBG = $(DEFS_DBG) $(DEFS_COMMON)
DEFS_STATIC_DBG = $(DEFS_STATIC_DBG) $(DEFS_COMMON)
DEFS_STLDBG = $(DEFS_STLDBG) $(DEFS_COMMON)
DEFS_STATIC_STLDBG = $(DEFS_STATIC_STLDBG) $(DEFS_COMMON)
CPPFLAGS_REL = $(DEFS_REL) $(INCLUDES)
CPPFLAGS_STATIC_REL = $(DEFS_STATIC_REL) $(INCLUDES)
CPPFLAGS_DBG = $(DEFS_DBG) $(INCLUDES)
CPPFLAGS_STATIC_DBG = $(DEFS_STATIC_DBG) $(INCLUDES)
CPPFLAGS_STLDBG = $(DEFS_STLDBG) $(INCLUDES)
CPPFLAGS_STATIC_STLDBG = $(DEFS_STATIC_STLDBG) $(INCLUDES)

COMPILE_c = $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) /c
COMPILE_c_REL = $(CC) $(CFLAGS_REL) $(CPPFLAGS_REL) $(TARGET_ARCH) /c
COMPILE_c_STATIC_REL = $(CC) $(CFLAGS_STATIC_REL) $(CPPFLAGS_STATIC_REL) $(TARGET_ARCH) /c
COMPILE_c_DBG = $(CC) $(CFLAGS_DBG) $(CPPFLAGS_DBG) $(TARGET_ARCH) /c
COMPILE_c_STATIC_DBG = $(CC) $(CFLAGS_STATIC_DBG) $(CPPFLAGS_STATIC_DBG) $(TARGET_ARCH) /c
COMPILE_cc_REL = $(CXX) $(CXXFLAGS_REL) $(CPPFLAGS_REL) $(TARGET_ARCH) /c
COMPILE_cc_STATIC_REL = $(CXX) $(CXXFLAGS_STATIC_REL) $(CPPFLAGS_STATIC_REL) $(TARGET_ARCH) /c
COMPILE_cc_DBG = $(CXX) $(CXXFLAGS_DBG) $(CPPFLAGS_DBG) $(TARGET_ARCH) /c
COMPILE_cc_STATIC_DBG = $(CXX) $(CXXFLAGS_STATIC_DBG) $(CPPFLAGS_STATIC_DBG) $(TARGET_ARCH) /c
COMPILE_cc_STLDBG = $(CXX) $(CXXFLAGS_STLDBG) $(CPPFLAGS_STLDBG) $(TARGET_ARCH) /c
COMPILE_cc_STATIC_STLDBG = $(CXX) $(CXXFLAGS_STATIC_STLDBG) $(CPPFLAGS_STATIC_STLDBG) $(TARGET_ARCH) /c
COMPILE_rc_REL = $(RC) $(RC_FLAGS_REL) /DBUILD=r /D "BUILD_INFOS=$(CPPFLAGS_REL)"
COMPILE_rc_STATIC_REL = $(RC) $(RC_FLAGS_REL) /DBUILD=r /D "BUILD_INFOS=$(CPPFLAGS_STATIC_REL)"
COMPILE_rc_DBG = $(RC) $(RC_FLAGS_DBG) /DBUILD=d /D "BUILD_INFOS=$(CPPFLAGS_DBG)"
COMPILE_rc_STATIC_DBG = $(RC) $(RC_FLAGS_DBG) /DBUILD=d /D "BUILD_INFOS=$(CPPFLAGS_STATIC_DBG)"
COMPILE_rc_STLDBG = $(RC) $(RC_FLAGS_STLDBG) /DBUILD=stld /D "BUILD_INFOS=$(CPPFLAGS_STLDBG) /D_STLP_DEBUG"
COMPILE_rc_STATIC_STLDBG = $(RC) $(RC_FLAGS_STLDBG) /DBUILD=stld /D "BUILD_INFOS=$(CPPFLAGS_STATIC_STLDBG) /D_STLP_DEBUG"
LINK_cc_REL = $(LINK) /nologo /incremental:no /debug /pdb:"$(PDB_NAME_OUT)" $(LDFLAGS_REL)
LINK_cc_DBG = $(LINK) /nologo /incremental:no /debug /pdb:"$(PDB_NAME_OUT_DBG)" $(LDFLAGS_DBG)
LINK_cc_STLDBG = $(LINK) /nologo /incremental:no /debug /pdb:"$(PDB_NAME_OUT_STLDBG)" $(LDFLAGS_STLDBG)
LINK_cc_A_REL = $(LINK) /nologo /incremental:no /debug /pdb:"$(A_PDB_NAME_OUT)" $(LDFLAGS_REL)
LINK_cc_A_DBG = $(LINK) /nologo /incremental:no /debug /pdb:"$(A_PDB_NAME_OUT_DBG)" $(LDFLAGS_DBG)
LINK_cc_A_STLDBG = $(LINK) /nologo /incremental:no /debug /pdb:"$(A_PDB_NAME_OUT_STLDBG)" $(LDFLAGS_STLDBG)

CDEPFLAGS = /FD /E
CCDEPFLAGS = /FD /E

# STLport DEBUG mode specific defines
DEFS_STLDBG = $(DEFS_STLDBG) /D_DEBUG /D_STLP_DEBUG /DDEBUG
DEFS_DBG = $(DEFS_DBG) /D_DEBUG /DDEBUG
DEFS_REL = $(DEFS_REL) /DNDEBUG
DEFS_STATIC_STLDBG = $(DEFS_STATIC_STLDBG) /D_DEBUG /D_STLP_DEBUG /DDEBUG /D_STLP_NO_FORCE_INSTANTIATE
DEFS_STATIC_DBG = $(DEFS_STATIC_DBG) /D_DEBUG /DDEBUG /D_STLP_NO_FORCE_INSTANTIATE
DEFS_STATIC_REL = $(DEFS_STATIC_REL) /DNDEBUG /D_STLP_NO_FORCE_INSTANTIATE

# optimization and debug compiler flags
OPT_REL = $(OPT_REL) /Zi /O2 /Og $(OPT_COMMON)
OPT_DBG = $(OPT_DBG) /Zi /Od $(OPT_COMMON)
OPT_STLDBG = $(OPT_STLDBG) /Zi /Od $(OPT_COMMON)
OPT_STATIC_REL = $(OPT_STATIC_REL) /Zi /O2 /Og $(OPT_COMMON)
OPT_STATIC_DBG = $(OPT_STATIC_DBG) /Zi /Od $(OPT_COMMON)
OPT_STATIC_STLDBG = $(OPT_STATIC_STLDBG) /Zi /Od $(OPT_COMMON)
