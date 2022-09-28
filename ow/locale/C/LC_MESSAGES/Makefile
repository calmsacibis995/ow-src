# @(#)Makefile	1.4 91/01/29 locale SMI
#
#         Copyright (C) 1991  Sun Microsystems, Inc
#                   All rights reserved.
#           Notice of copyright on this source code
#           product does not indicate publication.
# 
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
# the U.S. Government is subject to restrictions as set forth 
# in subparagraph (c)(1)(ii) of the Rights in Technical Data
# and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
# and FAR 52.227-19 (c) (June 1987).
#
# Sun Microsystems, Inc.
# 2550 Garcia Avenue
# Mountain View, California 94043
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

PSFFM_FILES		= # *.po
MESSAGE_FILES		= ${PSFFM_FILES:%.po=%.mo}

FILE			= ${MESSAGE_FILES}
DERIVEDFILES		= ${MESSAGE_FILES}

MAKE.install.target	= # install_file
INSTALL.file.dir	= # ${DEST.locale.C.LC}

include ${RULESET.include}
