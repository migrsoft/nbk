# ============================================================================
#  Name	 : build_help.mk
#  Part of  : testapp
# ============================================================================
#  Name	 : build_help.mk
#  Part of  : testapp
#
#  Description: This make file will build the application help file (.hlp)
# 
# ============================================================================

do_nothing :
	@rem do_nothing

# build the help from the MAKMAKE step so the header file generated
# will be found by cpp.exe when calculating the dependency information
# in the mmp makefiles.

MAKMAKE : testapp_0xEA69F97C.hlp
testapp_0xEA69F97C.hlp : testapp.xml testapp.cshlp Custom.xml
	cshlpcmp testapp.cshlp
ifeq (WINSCW,$(findstring WINSCW, $(PLATFORM)))
	md $(EPOCROOT)epoc32\$(PLATFORM)\c\resource\help
	copy testapp_0xEA69F97C.hlp $(EPOCROOT)epoc32\$(PLATFORM)\c\resource\help
endif

BLD : do_nothing

CLEAN :
	del testapp_0xEA69F97C.hlp
	del testapp_0xEA69F97C.hlp.hrh

LIB : do_nothing

CLEANLIB : do_nothing

RESOURCE : do_nothing
		
FREEZE : do_nothing

SAVESPACE : do_nothing

RELEASABLES :
	@echo testapp_0xEA69F97C.hlp

FINAL : do_nothing
