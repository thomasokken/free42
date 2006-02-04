###############################################################################
# Free42 -- a free HP-42S calculator clone
# Copyright (C) 2004-2006  Thomas Okken
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
###############################################################################

all: palmos motif

palmos: FORCE
	cd palmos && $(MAKE)

motif: FORCE
	cd motif && $(MAKE)

MacDashboard: FORCE
	cd MacDashboard && $(MAKE)

clean: FORCE
	cd palmos && $(MAKE) clean
	cd motif && $(MAKE) clean
	find . -name 'core.*' -exec rm -f {} \;

FORCE:
