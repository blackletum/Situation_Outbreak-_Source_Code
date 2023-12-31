SFX = 5.1
APPNAME = lua$(SFX)

SRC = lua.c

include lua_conf.inc

ifneq ($(findstring Win, $(TEC_SYSNAME)), )
  SRC += lua.rc 
else
  ifneq ($(findstring cygw, $(TEC_UNAME)), )
    SRC += lua.rc
  endif
endif
