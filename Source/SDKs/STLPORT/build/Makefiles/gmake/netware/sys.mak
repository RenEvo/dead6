# Time-stamp: <05/09/09 21:12:00 ptr>
# $Id: sys.mak,v 1.1.2.3 2005/09/11 06:27:35 complement Exp $

INSTALL := /usr/bin/install

INSTALL_SO := ${INSTALL} -c -m 0755
INSTALL_A := ${INSTALL} -c -m 0644
INSTALL_EXE := ${INSTALL} -c -m 0755

# this work only with cygwin, so use built-in test:
EXT_TEST := test
