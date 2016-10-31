#
#  UNPUBLISHED PROPRIETARY SOURCE CODE
#  Copyright (c) 2016 MXCHIP Inc.
#
#  The contents of this file may not be disclosed to third parties, copied or
#  duplicated in any form, in whole or in part, without the prior written
#  permission of MXCHIP Corporation.
#

NAME := Lib_fog_v2_subdevice_api_test

GLOBAL_INCLUDES += 	./config \

$(NAME)_SOURCES := 	hello_fog.c \
					subdevice_app/fog_v2_subdevice_app.c

$(NAME)_INCLUDES += ./subdevice_app 
				
$(NAME)_LINK_FILES := subdevice_app/fog_v2_subdevice_app.o
				
$(NAME)_COMPONENTS := protocols/fog_v2