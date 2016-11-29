#
#  UNPUBLISHED PROPRIETARY SOURCE CODE
#  Copyright (c) 2016 MXCHIP Inc.
#
#  The contents of this file may not be disclosed to third parties, copied or
#  duplicated in any form, in whole or in part, without the prior written
#  permission of MXCHIP Corporation.
#

NAME := Lib_fog_v2_api_test

$(NAME)_SOURCES := 	hello_fog.c
				
$(NAME)_LINK_FILES := hello_fog.o
				
$(NAME)_COMPONENTS := protocols/fog_v2