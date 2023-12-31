SFX = 5.1
APPNAME = wlua$(SFX)

SRC = lua.c wmain.c

include lua_conf.inc

APPTYPE = windows

ifneq ($(findstring Win, $(TEC_SYSNAME)), )
  SRC += wlua.rc 
else
  ifneq ($(findstring cygw, $(TEC_UNAME)), )
    SRC += wlua.rc
  endif
endif
