/*
 * Based on the QNX4 port of nspmod by Mike Gorchak <malva@selena.kherson.ua>
 */

#include <sys/audio.h>
#include <sys/ioctl.h>
#include "sound.h"

static int fd_audio;

static int init(struct options *options)
{
	char **parm = options->driver_parm;
	int rc, rate, bits, stereo, bsize;
	char *dev;

	parm_init(parm);
	chkparm1("dev", dev = token);
	chkparm1("buffer", bsize = strtoul(token, NULL, 0));
	parm_end();

	rate = options->rate;
	bits = options->format & XMP_MIX_8BIT ? 8 : 16;
	stereo = 1;
	bufsize = 32 * 1024;

	fd_audio = open(dev, O_WRONLY);
	if (fd_audio < 0) {
		fprintf(stderr, "can't open audio device\n");
		return -1;
	}

	if (options->outfmt & XMP_MIX_MONO)
		stereo = 0;

	if (ioctl(fd_audio, SOUND_PCM_WRITE_BITS, &bits) < 0) {
		perror("can't set resolution");
		goto error;
	}

	if (ioctl(fd, SNDCTL_DSP_STEREO, &stereo) < 0) {
		perror("can't set channels");
		goto error;
	}

	if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) < 0) {
		perror("can't set rate");
		goto error;
	}

	if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &buf.size) < 0) {
		perror("can't set rate");
		goto error;
	}

	if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &bufsize) < 0) {
		perror("can't set buffer");
		goto error;
	}

	return 0;

    error:
	close(fd_audio);
	return -1;
}

static void play(void *b, int i)
{
	int j;

	do {
		if ((j = write(fd_audio, b, i)) > 0) {
			i -= j;
			b += j;
		} else {
			break;
		}
	} while (i);
}

static void deinit()
{
	close(fd_audio);
}

static void flush()
{
	ioctl(fd, SNDCTL_DSP_SYNC, NULL);
}

static void onpause()
{
}

static void onresume()
{
}


static char *help[] = {
	"dev=<device_name>", "Audio device name (default is /dev/dsp)",
	"buffer=val", "Audio buffer size (default is 32768)",
	NULL
};

struct sound_driver sound_qnx = {
	"QNX",
	"QNX PCM audio",
	help,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
