# -*- makefile -*- Time-stamp: <03/10/26 16:17:03 ptr>
# $Id: clean.mak,v 1.1.2.4 2005/09/19 19:58:06 dums Exp $

clobber:	clean
	@if exist $(PRG) del /F /Q $(PRG)
	@if exist $(PRG_DBG) del /F /Q $(PRG_DBG)
	@if exist $(PRG_STLDBG) del /F /Q $(PRG_STLDBG)
	@if exist $(PRG_A) del /F /Q $(PRG_A)
	@if exist $(PRG_A_DBG) del /F /Q $(PRG_A_DBG)
	@if exist $(PRG_A_STLDBG) del /F /Q $(PRG_A_STLDBG)
	@if exist $(PDB_NAME_OUT) del /F /Q $(PDB_NAME_OUT)
	@if exist $(PDB_NAME_OUT_DBG) del /F /Q $(PDB_NAME_OUT_DBG)
	@if exist $(PDB_NAME_OUT_STLDBG) del /F /Q $(PDB_NAME_OUT_STLDBG)
	@if exist $(MANIFEST_NAME_OUT) del /F /Q $(MANIFEST_NAME_OUT)
	@if exist $(MANIFEST_NAME_OUT_DBG) del /F /Q $(MANIFEST_NAME_OUT_DBG)
	@if exist $(MANIFEST_NAME_OUT_STLDBG) del /F /Q $(MANIFEST_NAME_OUT_STLDBG)
	@if exist $(A_PDB_NAME_OUT) del /F /Q $(A_PDB_NAME_OUT)
	@if exist $(A_PDB_NAME_OUT_DBG) del /F /Q $(A_PDB_NAME_OUT_DBG)
	@if exist $(A_PDB_NAME_OUT_STLDBG) del /F /Q $(A_PDB_NAME_OUT_STLDBG)
	@-if exist $(OUTPUT_DIR) rd $(OUTPUT_DIR)
	@-if exist $(OUTPUT_DIR_DBG) rd $(OUTPUT_DIR_DBG)
	@-if exist $(OUTPUT_DIR_STLDBG) rd $(OUTPUT_DIR_STLDBG)
	@-if exist $(OUTPUT_DIR_A) rd $(OUTPUT_DIR_A)
	@-if exist $(OUTPUT_DIR_A_DBG) rd $(OUTPUT_DIR_A_DBG)
	@-if exist $(OUTPUT_DIR_A_STLDBG) rd $(OUTPUT_DIR_A_STLDBG)

distclean:	clobber
	@if exist $(INSTALL_BIN_DIR)\$(PRG_NAME_BASE)$(EXE) del /F /Q $(INSTALL_BIN_DIR)\$(PRG_NAME_BASE)$(EXE)
	@if exist $(INSTALL_BIN_DIR_DBG)\$(PRG_NAME_DBG_BASE)$(EXE) del /F /Q $(INSTALL_BIN_DIR_DBG)\$(PRG_NAME_DBG_BASE)$(EXE)
	@if exist $(INSTALL_BIN_DIR_STLDBG)\$(PRG_NAME_STLDBG_BASE)$(EXE) del /F /Q $(INSTALL_BIN_DIR_STLDBG)\$(PRG_NAME_STLDBG_BASE)$(EXE)
	@if exist $(INSTALL_STATIC_BIN_DIR)\$(PRG_NAME_A_BASE)$(EXE) del /F /Q $(INSTALL_STATIC_BIN_DIR)\$(PRG_NAME_A_BASE)$(EXE)
	@if exist $(INSTALL_STATIC_BIN_DIR_DBG)\$(PRG_NAME_A_DBG_BASE)$(EXE) del /F /Q $(INSTALL_STATIC_BIN_DIR_DBG)\$(PRG_NAME_A_DBG_BASE)$(EXE)
	@if exist $(INSTALL_STATIC_BIN_DIR_STLDBG)\$(PRG_NAME_A_STLDBG_BASE)$(EXE) del /F /Q $(INSTALL_STATIC_BIN_DIR_STLDBG)\$(PRG_NAME_A_STLDBG_BASE)$(EXE)
	@if exist $(INSTALL_BIN_DIR)\$(PRG_NAME_BASE).pdb del /F /Q $(INSTALL_BIN_DIR)\$(PRG_NAME_BASE).pdb
	@if exist $(INSTALL_BIN_DIR_DBG)\$(PRG_NAME_DBG_BASE).pdb del /F /Q $(INSTALL_BIN_DIR_DBG)\$(PRG_NAME_DBG_BASE).pdb
	@if exist $(INSTALL_BIN_DIR_STLDBG)\$(PRG_NAME_STLDBG_BASE).pdb del /F /Q $(INSTALL_BIN_DIR_STLDBG)\$(PRG_NAME_STLDBG_BASE).pdb
	@if exist $(INSTALL_STATIC_BIN_DIR)\$(PRG_NAME_A_BASE).pdb del /F /Q $(INSTALL_STATIC_BIN_DIR)\$(PRG_NAME_A_BASE).pdb
	@if exist $(INSTALL_STATIC_BIN_DIR_DBG)\$(PRG_NAME_A_DBG_BASE).pdb del /F /Q $(INSTALL_STATIC_BIN_DIR_DBG)\$(PRG_NAME_A_DBG_BASE).pdb
	@if exist $(INSTALL_STATIC_BIN_DIR_STLDBG)\$(PRG_NAME_A_STLDBG_BASE).pdb del /F /Q $(INSTALL_STATIC_BIN_DIR_STLDBG)\$(PRG_NAME_A_STLDBG_BASE).pdb
