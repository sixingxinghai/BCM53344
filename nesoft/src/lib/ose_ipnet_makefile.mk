############################################################################
#                           PROGRAM MODULE
#
#	$Workfile:   makefile.mk  $ (lib)
#	Document no: @(#) 550/OSE137-makefile
#	$Version:   /main/tb_ri24/18  $
#	venu
#	2003/06/16 19:20:23
#
#   Copyright (C) 2002-2003  All rights reserved.
#
############################################################################

include C:$/PacOS5$/platform$/ose_ipnet$/Rules.options
include C:$/PacOS5$/platform$/ose_ipnet$/Rules.platform

ZEBLIBOBJ *= ..$/..$/platform$/ose_ipnet$/obj

ZEBLIB_SRC *= $(PACOS_ROOT)$/lib 
ZEBLIB_VR_SRC *= $(PACOS_ROOT)$/lib$/vr
ZEBLIB_MPLS_SRC *= $(PACOS_ROOT)$/lib$/mpls_client
ZEBLIB_MPLS *= $(PACOS_ROOT)$/lib$/mpls
ZEBLIB_CSPF_SRC *= $(PACOS_ROOT)$/lib$/cspf_client
ZEBLIB_OSPFCOM_SRC *= $(PACOS_ROOT)$/lib$/ospf_common
ZEBLIB_LIB *= $(ZEBLIBOBJ)$/zeblib.a

ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/admin_grp.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/api.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/bitmap.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/buffer.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/cfg.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/checksum.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/command.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/distribute.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/filter.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/hash.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/if.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/if_rmap.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/keychain.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/label_pool.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/lib.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/linklist.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/log.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/lp_client.o
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/ls_table.o
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/ls_prefix.o
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/message.o
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/mpls.o
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/nsm_message.o
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/nsm_client.o
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/network.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/plist.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/prefix.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/ptree.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/qos_client.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/qos_common.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/routemap.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/smux.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/snprintf.o
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/sockunion.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/stream.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/table.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/tlv.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/thread.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/vector.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/version.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/vty.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/zffs.o 
# lib\vr files
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/gma_client.o
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/vr_cli.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/vr_lib.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/vr_login.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/vr_user.o 
# lib\ospf_comon files
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/ospf_const.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/ospf_debug_msg.o 
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/ospf_util.o 
# lib\mpls_client files
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/mpls_client.o
# lib\cspf_client
.IF $(ENABLE_CSPF) == yes
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/cspf_api.o
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/cspf_client.o
ZEBLIB_OBJECTS += $(ZEBLIBOBJ)$/cspf_common.o
.END

#
# Targets
#

all: $(ZEBLIBOBJ) $(ZEBLIB_LIB)


$(ZEBLIBOBJ)$/%.o:	$(ZEBLIB_SRC)$/%.c
	$(ECHO) Compile (CC) $< to $@ $(ECHOEND)
	$(CC) $(CFLAGS) $(DEFINES) $(IPNET_INCLUDE) $(ZEBROOT_INCLUDE) $(ZEBPAL_INCLUDE) $(ZEBPALAPI_INCLUDE) $(ZEBLIB_INCLUDE) $(ZEBNSM_INCLUDE) $(INCLUDE) $(CCOUT) $<

$(ZEBLIBOBJ)$/%.o:	$(ZEBLIB_VR_SRC)$/%.c
	$(ECHO) Compile (CC) $< to $@ $(ECHOEND)
	$(CC) $(CFLAGS) $(DEFINES) $(IPNET_INCLUDE) $(ZEBROOT_INCLUDE) $(ZEBPAL_INCLUDE) $(ZEBPALAPI_INCLUDE) $(ZEBLIB_INCLUDE) $(ZEBNSM_INCLUDE) $(INCLUDE) $(CCOUT) $<

$(ZEBLIBOBJ)$/%.o:	$(ZEBLIB_MPLS_SRC)$/%.c
	$(ECHO) Compile (CC) $< to $@ $(ECHOEND)
	$(CC) $(CFLAGS) $(DEFINES) $(IPNET_INCLUDE) $(ZEBROOT_INCLUDE) $(ZEBPAL_INCLUDE) $(ZEBPALAPI_INCLUDE) $(ZEBLIB_INCLUDE) $(ZEBNSM_INCLUDE) $(INCLUDE) $(CCOUT) $<
	
$(ZEBLIBOBJ)$/%.o:	$(ZEBLIB_MPLS)$/%.c
	$(ECHO) Compile (CC) $< to $@ $(ECHOEND)
	$(CC) $(CFLAGS) $(DEFINES) $(IPNET_INCLUDE) $(ZEBROOT_INCLUDE) $(ZEBPAL_INCLUDE) $(ZEBPALAPI_INCLUDE) $(ZEBLIB_INCLUDE) $(ZEBNSM_INCLUDE) $(INCLUDE) $(CCOUT) $<

$(ZEBLIBOBJ)$/%.o:	$(ZEBLIB_CSPF_SRC)$/%.c
	$(ECHO) Compile (CC) $< to $@ $(ECHOEND)
	$(CC) $(CFLAGS) $(DEFINES) $(IPNET_INCLUDE) $(ZEBROOT_INCLUDE) $(ZEBPAL_INCLUDE) $(ZEBPALAPI_INCLUDE) $(ZEBLIB_INCLUDE) $(ZEBNSM_INCLUDE) $(INCLUDE) $(CCOUT) $<

$(ZEBLIBOBJ)$/%.o:	$(ZEBLIB_OSPFCOM_SRC)$/%.c
	$(ECHO) Compile (CC) $< to $@ $(ECHOEND)
	$(CC) $(CFLAGS) $(DEFINES) $(IPNET_INCLUDE) $(ZEBROOT_INCLUDE) $(ZEBPAL_INCLUDE) $(ZEBPALAPI_INCLUDE) $(ZEBLIB_INCLUDE) $(ZEBNSM_INCLUDE) $(INCLUDE) $(CCOUT) $<

$(ZEBLIB_LIB): $(ZEBLIB_OBJECTS)
	$(AR) $(ZEBLIB_LIB) $<
