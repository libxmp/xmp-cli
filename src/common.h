#ifndef __COMMON_H
#define __COMMON_H

#define MAX_DRV_PARM 20

struct options {
	int start;		/* start order */
	int amplify;		/* amplification factor */
	int rate;		/* sampling rate */
	int format;		/* sample format */
	int max_time;		/* max. replay time */
	int mix;		/* channel separation */
	int interp;		/* interpolation type */
	int dsp;		/* dsp effects */
	int loop;		/* loop module */
	int random;		/* play in random order */
	int reverse;		/* reverse stereo channels */
	int verbose;
	int silent;		/* silent output */
	int info;		/* display information and exit */
	int probeonly;		/* probe sound driver and exit */
	int nocmd;		/* disable interactive commands */
	int dparm;		/* driver parameter index */
	char *driver_id;	/* sound driver ID */
	char *out_file;		/* output file name */
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
};


int report(char *, ...);

/* option */
void get_options(int, char **, struct options *);
int read_config(struct options *);

/* terminal */
int set_tty(void);
int reset_tty(void);

/* info */
void info_mod(struct xmp_module_info *);
void info_frame_init(struct xmp_module_info *);
void info_frame(struct xmp_module_info *, struct control *, int);
void info_ins_smp(struct xmp_module_info *);
void info_instruments(struct xmp_module_info *);
void info_samples(struct xmp_module_info *);
void info_help(void);

/* commands */
void read_command(xmp_context, struct control *);

#endif
