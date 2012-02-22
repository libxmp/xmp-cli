
SRC_OBJS	= sound_null.o terminal.o info.o commands.o \
		  options.o getopt.o getopt1.o main.o
SRC_DFILES	= Makefile xmp.1 $(SRC_OBJS:.o=.c) common.h getopt.h
SRC_PATH	= src

OBJS += $(addprefix $(SRC_PATH)/,$(SRC_OBJS))

dist-src:
	mkdir -p $(DIST)/$(SRC_PATH)
	cp -RPp $(addprefix $(SRC_PATH)/,$(SRC_DFILES)) $(DIST)/$(SRC_PATH)

$(SRC_PATH)/xmp: $(OBJS) $(SRC_PATH)/$(DRIVER).o
	@CMD='$(LD) -o $@ $(LDFLAGS) $(OBJS) $(SRC_PATH)/$(DRIVER).o $(LIBS) -lasound'; \
	if [ "$(V)" -gt 0 ]; then echo $$CMD; else echo LD $@ ; fi; \
	eval $$CMD

install-xmp: $(SRC_PATH)/xmp
	@echo Installing xmp in $(DESTDIR)$(BINDIR)
	@[ -d $(DESTDIR)$(BINDIR) ] || mkdir -p $(DESTDIR)$(BINDIR)
	@$(INSTALL_PROGRAM) $+ $(DESTDIR)$(BINDIR)

