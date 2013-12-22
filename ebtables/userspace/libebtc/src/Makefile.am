#
# ==[ Makefile ]================================================================
#
#  Project
#
#      Library for ethernet bridge tables.
#
#
#  Description
#
#      Process this file with automake to create Makefile.in
#
#
#  Copyright
#
#      Copyright 2005 by Jens Götze
#      All rights reserved.
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software
#      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307,
#      USA.
#
#
# ==============================================================================
#


LT_CUR                      = 1
LT_REV                      = 0
LT_AGE                      = 1

MAINTAINERCLEANFILES        = Makefile.in

AM_CFLAGS                   = @CFLAGS@ -I../include
AM_LDFLAGS                  = @LDFLAGS@

EXTRA_DIST                  =

lib_LTLIBRARIES             = libebtc.la

libebtc_la_SOURCES          = ebtc.c
libebtc_la_LDFLAGS          = -version-info ${LT_CUR}:${LT_REV}:${LT_AGE}


