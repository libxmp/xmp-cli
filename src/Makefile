
#SRC_OBJS	= getopt.o getopt1.o options.o drivers.o main.o
SRC_OBJS	= sound_alsa.o terminal.o info.o options.o main.o
SRC_DFILES	= Makefile xmp.1 $(SRC_OBJS:.o=.c) common.h
SRC_PATH	= src

OBJS += $(addprefix $(SRC_PATH)/,$(SRC_OBJS))

dist-src:
	mkdir -p $(DIST)/$(SRC_PATH)
	cp -RPp $(addprefix $(SRC_PATH)/,$(SRC_DFILES)) $(DIST)/$(SRC_PATH)

$(SRC_PATH)/xmp: $(OBJS)
	@CMD='$(LD) -o $@ $(LDFLAGS) $(OBJS) $(LIBS) -lasound'; \
	if [ "$(V)" -gt 0 ]; then echo $$CMD; else echo LD $@ ; fi; \
	eval $$CMD

install-xmp: $(SRC_PATH)/xmp
	@echo Installing xmp in $(DESTDIR)$(BINDIR)
	@[ -d $(DESTDIR)$(BINDIR) ] || mkdir -p $(DESTDIR)$(BINDIR)
	@$(INSTALL_PROGRAM) $+ $(DESTDIR)$(BINDIR)

