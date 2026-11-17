# Hardware objects are retained by the driver bundle. Link just the
# support libraries on demand, rather than the whole Circle IP stack.
REI_DRIVER_BUNDLE ?= $(CIRCLEHOME)
LIBS = $(REI_DRIVER_BUNDLE)/addon/fatfs/libfatfs.a \
       $(REI_DRIVER_BUNDLE)/lib/fs/libfs.a \
       $(REI_DRIVER_BUNDLE)/lib/net/libnet.a \
       $(REI_DRIVER_BUNDLE)/lib/sched/libsched.a

include $(CIRCLEHOME)/Rules.mk

# Do not discard an applicable driver merely because the boot diagnostic
# does not instantiate it yet.
LDFLAGS := $(filter-out --gc-sections,$(LDFLAGS))
