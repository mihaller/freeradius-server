TARGETNAME	:= rlm_ratelimit

ifneq "$(TARGETNAME)" ""
TARGET		:= $(TARGETNAME).a
endif

SOURCES		:= $(TARGETNAME).c hashtable.c

SRC_CFLAGS	:=
TGT_LDLIBS	:=

