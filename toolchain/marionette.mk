
# Directories for MARIONETTE configuration

MARIONETTE_TOP           = ../..
MARIONETTE_TOOLCHAIN     = $(MARIONETTE_TOP)/toolchain/
MARIONETTE_OPENOCD       = $(MARIONETTE_TOOLCHAIN)/openocd

# source directories
MARIONETTE_UTIL          = $(MARIONETTE_TOP)/src/util
MARIONETTE_USB           = $(MARIONETTE_TOP)/src/usb
MARIONETTE_FETCH         = $(MARIONETTE_TOP)/src/fetch
MARIONETTE_MSHELL        = $(MARIONETTE_TOP)/src/mshell
MARIONETTE_MPIPE         = $(MARIONETTE_TOP)/src/mpipe
MARIONETTE_BOARDS        = $(MARIONETTE_TOP)/src/boards

# make rules
MARIONETTE_RULES         = $(MARIONETTE_OPENOCD)/openocd.mk \
													 $(MARIONETTE_TOOLCHAIN)/dfu.mk

ifeq ($(shell git diff-index --quiet HEAD $(MARIONETTE_TOP)/src ; echo $$?), 1)
INDEX_DIRTY = modified
else
INDEX_DIRTY =
endif

VERSION_PREFIX = git
GIT_HASH = $(shell git rev-parse --short HEAD)

MARIONETTE_VERSION = "\"$(VERSION_PREFIX)-$(GIT_HASH)-$(INDEX_DIRTY)\""

