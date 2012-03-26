
SRC_OBJS	= sound.o sound_null.o terminal.o info.o commands.o \
		  options.o getopt.o getopt1.o main.o
SRC_DFILES	= Makefile xmp.1 $(SRC_OBJS:.o=.c) common.h getopt.h \
		  sound_alsa.c sound_coreaudio.c sound_oss.c sound_sndio.c \
		  sound_bsd.c sound_solaris.c
SRC_PATH	= src

SRC_OBJS += $(DRIVERS)

OBJS += $(addprefix $(SRC_PATH)/,$(SRC_OBJS))

dist-src:
	mkdir -p $(DIST)/$(SRC_PATH)
	cp -RPp $(addprefix $(SRC_PATH)/,$(SRC_DFILES)) $(DIST)/$(SRC_PATH)

$(SRC_PATH)/xmp: $(OBJS)
	@CMD='$(LD) -o $@ $(LDFLAGS) $(OBJS) $(LIBS)'; \
	if [ "$(V)" -gt 0 ]; then echo $$CMD; else echo LD $@ ; fi; \
	eval $$CMD

install-xmp: $(SRC_PATH)/xmp
	@echo Installing xmp in $(DESTDIR)$(BINDIR)
	@[ -d $(DESTDIR)$(BINDIR) ] || mkdir -p $(DESTDIR)$(BINDIR)
	@$(INSTALL_PROGRAM) $+ $(DESTDIR)$(BINDIR)

