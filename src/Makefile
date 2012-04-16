
SRC_OBJS	= sound.o sound_null.o terminal.o info.o commands.o \
		  options.o getopt.o getopt1.o main.o sound_wav.o sound_file.o
SRC_DFILES	= Makefile xmp.1 $(SRC_OBJS:.o=.c) common.h getopt.h list.h \
		  sound.h sound_alsa.c sound_coreaudio.c sound_oss.c \
		  sound_sndio.c sound_netbsd.c sound_bsd.c sound_solaris.c \
		  sound_beos.c sound_ahi.c sound_dart.c sound_hpux.c \
		  sound_aix.c sound_pulseaudio.c sound_qnx.c sound_alsa05.c \
		  sound_win32.c
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

$(SRC_PATH)/sound_beos.o: src/sound_beos.c
	@CMD='g++ -c $(CFLAGS) -o $@ src/sound_beos.c'; \
	if [ "$(V)" -gt 0 ]; then echo $$CMD; else echo CXX $@ ; fi; \
	eval $$CMD

install-xmp: $(SRC_PATH)/xmp
	@echo Installing xmp in $(DESTDIR)$(BINDIR)
	@[ -d $(DESTDIR)$(BINDIR) ] || mkdir -p $(DESTDIR)$(BINDIR)
	@$(INSTALL_PROGRAM) $+ $(DESTDIR)$(BINDIR)

