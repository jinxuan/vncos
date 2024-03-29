XEN_ROOT = ..
MINI_OS = $(XEN_ROOT)/extras/mini-os

export XEN_OS=MiniOS

export stubdom=y
export debug=y
include $(XEN_ROOT)/Config.mk

ZLIB_URL?=http://www.zlib.net
#ZLIB_URL=$(XEN_EXTFILES_URL)
ZLIB_VERSION=1.2.3

LIBPCI_URL?=http://www.kernel.org/pub/software/utils/pciutils
#LIBPCI_URL?=$(XEN_EXTFILES_URL)
LIBPCI_VERSION=2.2.9

NEWLIB_URL?=ftp://sources.redhat.com/pub/newlib
#NEWLIB_URL?=$(XEN_EXTFILES_URL)
NEWLIB_VERSION=1.16.0

LWIP_URL?=http://download.savannah.gnu.org/releases/lwip
#LWIP_URL?=$(XEN_EXTFILES_URL)
LWIP_VERSION=1.3.0

WGET=wget -c

GNU_TARGET_ARCH:=$(XEN_TARGET_ARCH)
ifeq ($(XEN_TARGET_ARCH),x86_32)
GNU_TARGET_ARCH:=i686
endif

ifeq ($(GNU_TARGET_ARCH), i686)
TARGET_CFLAGS=
NEWLIB_CFLAGS+=-D_I386MACH_ALLOW_HW_INTERRUPTS
STUBDOM_SUPPORTED=1
endif
ifeq ($(GNU_TARGET_ARCH), x86_64)
TARGET_CFLAGS=-mno-red-zone
NEWLIB_CFLAGS+=-D_I386MACH_ALLOW_HW_INTERRUPTS
STUBDOM_SUPPORTED=1
endif
ifeq ($(GNU_TARGET_ARCH), ia64)
TARGET_CFLAGS=-mconstant-gp
endif

CROSS_ROOT=cross-root-$(GNU_TARGET_ARCH)
CROSS_PREFIX=$(CURDIR)/$(CROSS_ROOT)

# Disable PIE/SSP if GCC supports them. They can break us.
TARGET_CFLAGS += $(CFLAGS)
TARGET_CPPFLAGS += $(CPPFLAGS)
TARGET_CFLAGS += $(call cc-option,$(CC),-nopie,)
TARGET_CFLAGS += $(call cc-option,$(CC),-fno-stack-protector,)
TARGET_CFLAGS += $(call cc-option,$(CC),-fno-stack-protector-all,)

# Do not use host headers and libs
GCC_INSTALL = $(shell LANG=C gcc -print-search-dirs | sed -n -e 's/install: \(.*\)/\1/p')
TARGET_CPPFLAGS += -U __linux__ -U __FreeBSD__ -U __sun__
TARGET_CPPFLAGS += -nostdinc
TARGET_CPPFLAGS += -isystem $(CURDIR)/$(MINI_OS)/include/posix
TARGET_CPPFLAGS += -isystem $(CROSS_PREFIX)/$(GNU_TARGET_ARCH)-xen-elf/include
TARGET_CPPFLAGS += -isystem $(GCC_INSTALL)include
TARGET_CPPFLAGS += -isystem $(CURDIR)/lwip-$(XEN_TARGET_ARCH)/src/include
TARGET_CPPFLAGS += -isystem $(CURDIR)/lwip-$(XEN_TARGET_ARCH)/src/include/ipv4
TARGET_CPPFLAGS += -I$(CURDIR)/include

TARGET_LDFLAGS += -nostdlib -L$(CROSS_PREFIX)/$(GNU_TARGET_ARCH)-xen-elf/lib

TARGETS=c

.PHONY: all
all: build
ifeq ($(STUBDOM_SUPPORTED),1)
build: c-stubdom
else
build:
endif

##############
# Cross-newlib
##############

newlib-$(NEWLIB_VERSION).tar.gz:
	$(WGET) $(NEWLIB_URL)/$@

newlib-$(NEWLIB_VERSION): newlib-$(NEWLIB_VERSION).tar.gz
	tar xzf $<
	patch -d $@ -p0 < newlib.patch
	patch -d $@ -p0 < newlib-chk.patch
	touch $@

NEWLIB_STAMPFILE=$(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/lib/libc.a
.PHONY: cross-newlib
cross-newlib: $(NEWLIB_STAMPFILE)
$(NEWLIB_STAMPFILE): mk-headers-$(XEN_TARGET_ARCH) newlib-$(NEWLIB_VERSION)
	mkdir -p newlib-$(XEN_TARGET_ARCH)
	( cd newlib-$(XEN_TARGET_ARCH) && \
	  CC_FOR_TARGET="$(CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(NEWLIB_CFLAGS)" AR_FOR_TARGET=$(AR) LD_FOR_TARGET=$(LD) RANLIB_FOR_TARGET=$(RANLIB) ../newlib-$(NEWLIB_VERSION)/configure --prefix=$(CROSS_PREFIX) --verbose --target=$(GNU_TARGET_ARCH)-xen-elf --enable-newlib-io-long-long --disable-multilib && \
	  $(MAKE) && \
	  DESTDIR= $(MAKE) install )

############
# Cross-zlib
############

zlib-$(ZLIB_VERSION).tar.gz:
	$(WGET) $(ZLIB_URL)/$@

zlib-$(XEN_TARGET_ARCH): zlib-$(ZLIB_VERSION).tar.gz 
	tar xzf $<
	mv zlib-$(ZLIB_VERSION) $@

ZLIB_STAMPFILE=$(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/lib/libz.a
.PHONY: cross-zlib
cross-zlib: $(ZLIB_STAMPFILE)
$(ZLIB_STAMPFILE): zlib-$(XEN_TARGET_ARCH) $(NEWLIB_STAMPFILE)
	( cd $< && \
	  CFLAGS="$(TARGET_CPPFLAGS) $(TARGET_CFLAGS)" CC=$(CC) ./configure --prefix=$(CROSS_PREFIX)/$(GNU_TARGET_ARCH)-xen-elf && \
	  $(MAKE) libz.a && \
	  $(MAKE) install )

##############
# Cross-libpci
##############

pciutils-$(LIBPCI_VERSION).tar.bz2:
	$(WGET) $(LIBPCI_URL)/$@

pciutils-$(XEN_TARGET_ARCH): pciutils-$(LIBPCI_VERSION).tar.bz2
	tar xjf $<
	mv pciutils-$(LIBPCI_VERSION) $@
	patch -d $@ -p1 < pciutils.patch
	touch $@

LIBPCI_STAMPFILE=$(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/lib/libpci.a
.PHONY: cross-libpci
cross-libpci: $(LIBPCI_STAMPFILE)
$(LIBPCI_STAMPFILE): pciutils-$(XEN_TARGET_ARCH) $(NEWLIB_STAMPFILE) $(ZLIB_STAMPFILE)
	( cd $< && \
	  cp ../libpci.config.h lib/config.h && \
	  echo '#define PCILIB_VERSION "$(LIBPCI_VERSION)"' >> lib/config.h && \
	  cp ../libpci.config.mak lib/config.mk && \
	  $(MAKE) CC="$(CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) -I$(realpath $(MINI_OS)/include)" lib/libpci.a && \
	  $(INSTALL_DATA) lib/libpci.a $(CROSS_PREFIX)/$(GNU_TARGET_ARCH)-xen-elf/lib/ && \
	  $(INSTALL_DIR) $(CROSS_PREFIX)/$(GNU_TARGET_ARCH)-xen-elf/include/pci && \
	  $(INSTALL_DATA) lib/config.h lib/header.h lib/pci.h lib/types.h $(CROSS_PREFIX)/$(GNU_TARGET_ARCH)-xen-elf/include/pci/ \
	)

######
# lwIP
######

lwip-$(LWIP_VERSION).tar.gz:
	$(WGET) $(LWIP_URL)/$@

lwip-$(XEN_TARGET_ARCH): lwip-$(LWIP_VERSION).tar.gz
	tar xzf $<
	mv lwip $@
	patch -d $@ -p0 < lwip.patch-cvs
	touch $@

#######
# Links
#######

.PHONY: $(CROSS_ROOT)
$(CROSS_ROOT): cross-newlib cross-zlib cross-libpci

mk-headers-$(XEN_TARGET_ARCH):
	patch -d $(XEN_ROOT) -p1 < mini-os.patch
	mkdir -p include/xen && \
          ln -sf $(addprefix ../../,$(wildcard $(XEN_ROOT)/xen/include/public/*.h)) include/xen && \
          ln -sf $(addprefix ../../$(XEN_ROOT)/xen/include/public/,arch-ia64 arch-x86 hvm io xsm) include/xen && \
          ( [ -h include/xen/sys ] || ln -sf ../../$(XEN_ROOT)/tools/include/xen-sys/MiniOS include/xen/sys ) && \
          ( [ -h include/xen/libelf ] || ln -sf ../../$(XEN_ROOT)/tools/include/xen/libelf include/xen/libelf ) && \
	mkdir -p include/xen-foreign && \
	  ln -sf $(addprefix ../../,$(wildcard $(XEN_ROOT)/tools/include/xen-foreign/*)) include/xen-foreign/ && \
	  $(MAKE) -C include/xen-foreign/ && \
	  ( [ -h include/xen/foreign ] || ln -sf ../xen-foreign include/xen/foreign )
	mkdir -p libxc-$(XEN_TARGET_ARCH)
	[ -h libxc-$(XEN_TARGET_ARCH)/Makefile ] || ( cd libxc-$(XEN_TARGET_ARCH) && \
	  ln -sf ../$(XEN_ROOT)/tools/libxc/*.h . && \
	  ln -sf ../$(XEN_ROOT)/tools/libxc/*.c . && \
	  ln -sf ../$(XEN_ROOT)/tools/libxc/Makefile . )
	mkdir -p libxc-$(XEN_TARGET_ARCH)/$(XEN_TARGET_ARCH)
	[ -h libxc-$(XEN_TARGET_ARCH)/$(XEN_TARGET_ARCH) ] || ( cd libxc-$(XEN_TARGET_ARCH)/$(XEN_TARGET_ARCH) && \
	  ln -sf ../../$(XEN_ROOT)/tools/libxc/$(XEN_TARGET_ARCH)/*.c . && \
	  ln -sf ../../$(XEN_ROOT)/tools/libxc/$(XEN_TARGET_ARCH)/*.h . && \
	  ln -sf ../../$(XEN_ROOT)/tools/libxc/$(XEN_TARGET_ARCH)/Makefile . )
	$(MAKE) -C $(MINI_OS) links
	touch mk-headers-$(XEN_TARGET_ARCH)

TARGETS_MINIOS=$(addprefix mini-os-$(XEN_TARGET_ARCH)-,$(TARGETS))
$(TARGETS_MINIOS): mini-os-%:
	[ -d $@ ] || \
	for i in $$(cd $(MINI_OS) ; find . -type d) ; do \
                mkdir -p $@/$$i ; \
	done

#######
# libxc
#######

.PHONY: libxc
libxc: libxc-$(XEN_TARGET_ARCH)/libxenctrl.a libxc-$(XEN_TARGET_ARCH)/libxenguest.a
libxc-$(XEN_TARGET_ARCH)/libxenctrl.a libxc-$(XEN_TARGET_ARCH)/libxenguest.a:: cross-zlib
	CPPFLAGS="$(TARGET_CPPFLAGS)" CFLAGS="$(TARGET_CFLAGS)" $(MAKE) -C libxc-$(XEN_TARGET_ARCH)


###
# C
###

.PHONY: c
c: $(CROSS_ROOT)
	CPPFLAGS="$(TARGET_CPPFLAGS)" CFLAGS="$(TARGET_CFLAGS)" $(MAKE) -C $@ LWIPDIR=$(CURDIR)/lwip-$(XEN_TARGET_ARCH) 

########
# minios
########

.PHONY: c-stubdom
c-stubdom: mini-os-$(XEN_TARGET_ARCH)-c lwip-$(XEN_TARGET_ARCH) libxc c
	mkdir -p $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/mini-os && \
	  ln -sf $(addprefix ../../../../,$(wildcard $(MINI_OS)/include/*.h)) $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/mini-os && \
	  ln -sf $(addprefix ../../../../$(MINI_OS)/include/,arch ia64 linux posix sys x86) $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/mini-os 
	mkdir -p $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/xen && \
          ln -sf $(addprefix ../../../../,$(wildcard $(XEN_ROOT)/xen/include/public/*.h)) $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/xen && \
          ln -sf $(addprefix ../../../../$(XEN_ROOT)/xen/include/public/,arch-ia64 arch-x86 hvm io xsm) $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/xen && \
          ( [ -h $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/xen/sys ] || ln -sf ../../../../$(XEN_ROOT)/tools/include/xen-sys/MiniOS $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/xen/sys ) && \
          ( [ -h $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/xen/libelf ] || ln -sf ../../../../$(XEN_ROOT)/tools/include/xen/libelf $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/xen/libelf ) && \
	mkdir -p $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/xen-foreign && \
	  ln -sf $(addprefix ../../../../,$(wildcard $(XEN_ROOT)/tools/include/xen-foreign/*.h)) $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/xen-foreign
	mkdir -p  $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/libxc
	  ln -sf $(addprefix ../../../../,$(wildcard $(XEN_ROOT)/tools/libxc/*.h)) $(CROSS_ROOT)/$(GNU_TARGET_ARCH)-xen-elf/include/libxc
	DEF_CPPFLAGS="$(TARGET_CPPFLAGS)" DEF_CFLAGS="-DCONFIG_C $(TARGET_CFLAGS)" DEF_LDFLAGS="$(TARGET_LDFLAGS)" $(MAKE) -C $(MINI_OS) OBJ_DIR=$(CURDIR)/$< LWIPDIR=$(CURDIR)/lwip-$(XEN_TARGET_ARCH) APP_OBJS=$(CURDIR)/c/main.a

#######
# clean
#######

# Only clean the libxc/ioemu/mini-os part
.PHONY: clean
clean:
	rm -fr mini-os-$(XEN_TARGET_ARCH)-c
	$(MAKE) -C c clean
	[ ! -d libxc-$(XEN_TARGET_ARCH) ] || $(MAKE) -C libxc-$(XEN_TARGET_ARCH) clean

# clean the cross-compilation result
.PHONY: crossclean
crossclean: clean
	rm -fr $(CROSS_ROOT)
	rm -fr newlib-$(XEN_TARGET_ARCH)
	rm -fr zlib-$(XEN_TARGET_ARCH) pciutils-$(XEN_TARGET_ARCH)
	rm -f mk-headers-$(XEN_TARGET_ARCH)
	rm -fr include
	rm -fr libxc-$(XEN_TARGET_ARCH)

# clean patched sources
.PHONY: patchclean
patchclean: crossclean
	rm -fr newlib-$(NEWLIB_VERSION)
	rm -fr lwip-$(XEN_TARGET_ARCH)
	patch -d $(XEN_ROOT) -R -p1 <mini-os.patch

# clean downloads
.PHONY: downloadclean
downloadclean: patchclean
	rm -f newlib-$(NEWLIB_VERSION).tar.gz
	rm -f zlib-$(ZLIB_VERSION).tar.gz
	rm -f pciutils-$(LIBPCI_VERSION).tar.bz2
	rm -f lwip-$(LWIP_VERSION).tar.gz

.PHONY: distclean
distclean: downloadclean
