# Makefile for cWServer - with automatic path detection using find

# --- Customizable path variables ---
SEARCH_BASE_DIRS ?= /usr/local /usr /opt /

# --- Commands to find directories ---
FIND_INCLUDE_CMD = find $(SEARCH_BASE_DIRS) -name include -type d -print -quit 2>/dev/null
FIND_SYSROOT_CMD = find $(SEARCH_BASE_DIRS) -name lib -type d -print -quit 2>/dev/null | sed 's/\/lib$$//'

# --- Automatic path detection ---
INCLUDE_DIR := $(shell $(FIND_INCLUDE_CMD))
SYSROOT_DIR := $(shell $(FIND_SYSROOT_CMD))

# --- Fallback defaults if find fails ---
ifeq ($(INCLUDE_DIR),)
	$(warning Include directory not found automatically. Using default /usr/include)
	INCLUDE_DIR := /usr/include
endif

ifeq ($(SYSROOT_DIR),)
	$(warning Sysroot directory not found automatically. Using default /)
	SYSROOT_DIR := /
endif

# --- Compiler and linker settings ---
CC = gcc
CFLAGS =   -std=c99 \
           -march=74kc \
           -mips16 -mdsp \
           -O3 \
           -flto \
           -pipe \
           -EB \
           -I$(INCLUDE_DIR)  \
           --sysroot=$(SYSROOT_DIR) \
	   -D_GNU_SOURCE \
           -s -ffunction-sections -Wl,--gc-sections -fno-asynchronous-unwind-tables -Wl,--strip-all

LDFLAGS = -pthread -lresolv

STRIP = strip

all: cwserver

cwserver: cwserver_v0.1a.c
	$(CC) $(CFLAGS) -o cwserver cwserver_v0.1a.c $(LDFLAGS)
	$(STRIP) --remove-section=.note.ABI-tag --remove-section=.comment --remove-section=.gnu.version -g -s cwserver

clean:
	rm -f *.o cwserver *~

# --- User instructions ---
.PHONY: help
help:
	@echo "Makefile for building cWServer with automatic path detection."
	@echo ""
	@echo "Variables that can be overridden:"
	@echo "  SEARCH_BASE_DIRS:  List of directories to search for include and lib (default: '$(SEARCH_BASE_DIRS)')"
	@echo "  INCLUDE_DIR:       Include directory (automatically detected or fallback to /usr/include)"
	@echo "  SYSROOT_DIR:       Sysroot directory (automatically detected or fallback to /)"
	@echo ""
	@echo "Make targets:"
	@echo "  make all         : Build the 'cwserver' executable"
	@echo "  make clean       : Delete object files and the executable"
	@echo "  make help        : Show this help message"
	@echo ""
	@echo "Example of overriding SEARCH_BASE_DIRS:"
	@echo "  make SEARCH_BASE_DIRS='/opt/my_libs /usr' all"
