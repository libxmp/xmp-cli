#ifndef XMP_COMMON_H
#define XMP_COMMON_H

#ifdef _MSC_VER
#ifndef __cplusplus
#define inline __inline
#endif
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define kbhit _kbhit
#define getch _getch
#endif

#if defined(__MORPHOS__) || defined(__AROS__) || defined(__AMIGA__) \
 || defined(__amigaos__) || defined(__amigaos4__) || defined(AMIGA)
#define XMP_AMIGA	1
#endif

#define NUM_MODES 13
#define MAX_DRV_PARM 20


#include <xmp.h>

struct player_mode {
	const char *name;
	const char *desc;
	int mode;
};

struct options {
	int start;		/* start order */
	int amplify;		/* amplification factor */
	int rate;		/* sampling rate */
	int format;		/* sample format */
	int max_time;		/* max. replay time */
	int mix;		/* channel separation */
	int defpan;		/* default pan */
	int interp;		/* interpolation type */
	int dsp;		/* dsp effects */
	int loop;		/* loop module */
	int loop_time;		/* loop until time */
	int random;		/* play in random order */
	int reverse;		/* reverse stereo channels */
	int vblank;		/* vblank flag */
	int fx9bug;		/* fx9bug flag -- DEPRECATED */
	int numvoices;		/* maximum number of mixer voices */
	int fixloop;		/* fixloop flag */
	int verbose;		/* verbosity level */
	int silent;		/* silent output */
	int info;		/* display information and exit */
	int probeonly;		/* probe sound driver and exit */
	int nocmd;		/* disable interactive commands */
	int norc;		/* don't read the configuration files */
	int dparm;		/* driver parameter index */
	int sequence;		/* sequence to play */
	int explore;		/* play all sequences in module */
	int show_comment;	/* show module comment text */
	int player_mode;	/* force tracker emulation */
	int amiga_mixer;	/* enable amiga mixer */
	const char *driver_id;	/* sound driver ID */
	const char *out_file;	/* output file name */
	char *ins_path;		/* instrument path */
	char *driver_parm[MAX_DRV_PARM]; /* driver parameters */
	char mute[XMP_MAX_CHANNELS];
};

struct control {
	double time;		/* Replay time in ms */
	int skip;		/* Skip to next module */
	int loop;		/* Module is looped */
	int pause;		/* Replay paused */
	int display;		/* Info display flag */
	int sequence;		/* Current sequence */
	int explore;		/* Play all sequences */
	int cur_info;		/* Display current sequence or mixer type */
	int amiga_mixer;	/* Toggle amiga mixer mode */
	int mixer_type;		/* Mixer type (from player) */
};

extern const struct player_mode pmode[];

void delay_ms(unsigned int);
#ifdef XMP_AMIGA
void amiga_inittimer(void);
void amiga_getsystime(void *);
#endif

char *xmp_strdup(const char *);
int xmp_strcasecmp(const char *, const char *); /* locale-insensitive */
int report(const char *, ...);

/* option */
void get_options(int, char **, struct options *);
int read_config(struct options *);
void read_modconf(struct options *, const unsigned char *);

/* terminal */
int set_tty(void);
int reset_tty(void);

/* info */
void info_mod(const struct xmp_module_info *, int);
void info_message(const char *, ...);
void info_frame_init(void);
void info_frame(const struct xmp_module_info *, const struct xmp_frame_info *, struct control *, int);
void info_ins_smp(const struct xmp_module_info *);
void info_instruments(const struct xmp_module_info *);
void info_samples(const struct xmp_module_info *);
void info_comment(const struct xmp_module_info *);
void info_help(void);

/* commands */
void read_command(xmp_context, const struct xmp_module_info *, struct control *);

#endif
