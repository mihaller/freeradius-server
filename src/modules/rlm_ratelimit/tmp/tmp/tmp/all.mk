# TARGET		:= rlm_ratelimit.a
# SOURCES		:= rlm_ratelimit.c

# TARGETNAME	:= rlm_ratelimit

# ifneq "$(TARGETNAME)" ""
# TARGET		:= $(TARGETNAME).a
# endif

# SOURCES		:= $(TARGETNAME).c tokenbucket.c

SOURCES         := rlm_ratelimit.c tokenbucket.c
TARGET          := rlm_ratelimit.a
