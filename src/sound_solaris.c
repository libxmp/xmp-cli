/* CS4231 code tested on Sparc 20 and Ultra 1 running Solaris 2.5.1
 * with mono/stereo, 16 bit, 22.05 kHz and 44.1 kHz using the internal
 * speaker and headphones.
 *
 * AMD 7930 code tested on Axil 240 running Solaris 2.5.1 and an Axil
 * 220 running Linux 2.0.33.
 */

/* Fixed and improved by Keith Hargrove <Keith.Hargrove@Eng.Sun.COM>
 * Wed, 30 Jun 1999 14:23:52 -0700 (PDT)
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(HAVE_SYS_AUDIOIO_H)
#include <sys/audioio.h>
#elif defined(HAVE_SYS_AUDIO_IO_H)
#include <sys/audio.io.h>
#elif defined(HAVE_SUN_AUDIOIO_H)
#include <sun/audioio.h>
#endif
#include <sys/stropts.h>

/* This is for Linux on Sparc */
#if defined(HAVE_SBUS_AUDIO_AUDIO_H)
#include <sbus/audio/audio.h>
#endif

#include "sound.h"

static int audio_fd;
static int audioctl_fd;

static int init(struct options *options)
{
	char **parm = options->driver_parm;
	audio_info_t ainfo, ainfo2;
	int gain;
	int bsize = 32 * 1024;
	int port;
	AUDIO_INITINFO(&ainfo);

	if ((audio_fd = open("/dev/audio", O_WRONLY)) == -1)
		return -1;

	/* try to open audioctl device */
	if ((audioctl_fd = open("/dev/audioctl", O_RDWR)) < 0) {
		fprintf(stderr, "couldn't open audioctl device\n");
		close(audio_fd);
		return -1;
	}

	/* sleep(2); -- Really needed? */

	/* empty buffers before change config */
	ioctl(audio_fd, AUDIO_DRAIN, 0);	/* drain everything out */
	ioctl(audio_fd, I_FLUSH, FLUSHRW);	/* flush everything     */
	ioctl(audioctl_fd, I_FLUSH, FLUSHRW);	/* flush everything */

	/* get audio parameters. */
	if (ioctl(audioctl_fd, AUDIO_GETINFO, &ainfo) < 0) {
		fprintf(stderr, "AUDIO_GETINFO failed!\n");
		close(audio_fd);
		close(audioctl_fd);
		return -1;
	}

	close(audioctl_fd);

	/* KH: Sun Dbri doesn't support 8bits linear. I dont muck with the gain
	 *     or the port setting. I hate when a program does that. There is
	 *     nothing more frustrating then having a program change your volume
	 *     and change from external speakers to the tiny one
	 */

	gain = ainfo.play.gain;
	port = ainfo.play.port;

	parm_init(parm);
	chkparm1("gain", gain = strtoul(token, NULL, 0));
	chkparm1("buffer", bsize = strtoul(token, NULL, 0));
	chkparm1("port", port = (int)*token)
	parm_end();

	switch (port) {
	case 'h':
		port = AUDIO_HEADPHONE;
		break;
	case 'l':
		port = AUDIO_LINE_OUT;
		break;
	case 's':
		port = AUDIO_SPEAKER;
	}

	if (gain < AUDIO_MIN_GAIN)
		gain = AUDIO_MIN_GAIN;
	if (gain > AUDIO_MAX_GAIN)
		gain = AUDIO_MAX_GAIN;

	AUDIO_INITINFO(&ainfo);	/* For CS4231 */
	AUDIO_INITINFO(&ainfo2);	/* For AMD 7930 if needed */

	ainfo.play.sample_rate = options->rate;
	ainfo.play.channels = options->format & XMP_MIX_MONO ? 1 : 2;
	ainfo.play.precision = options->format & XMP_MIX_8BIT ? 8 : 16;
	ainfo.play.encoding = AUDIO_ENCODING_LINEAR;
	ainfo2.play.gain = ainfo.play.gain = gain;
	ainfo2.play.port = ainfo.play.port = port;
	ainfo2.play.buffer_size = ainfo.play.buffer_size = bsize;

	if (ioctl(audio_fd, AUDIO_SETINFO, &ainfo) == -1) {
		/* CS4231 initialization Failed, perhaps we have an AMD 7930 */
		if (ioctl(audio_fd, AUDIO_SETINFO, &ainfo2) == -1) {
			close(audio_fd);
			return -1;
		}

		/* Sorry, AMD7930 uLaw not supported anymore */
		/* sound_solaris.description = "Solaris AMD7930 PCM audio"; */
		return -1;
	} else {
		/* sound_solaris.description = "Solaris CS4231 PCM audio"; */
	}

	return 0;
}

static void play(void *b, int i)
{
	int j;

	while (i) {
		if ((j = write(audio_fd, b, i)) > 0) {
			i -= j;
			b = (char *)b + j;
		} else
			break;
	}
}

static void deinit()
{
	close(audio_fd);
}

static void flush()
{
}

static void onpause()
{
}

static void onresume()
{
}


static char *help[] = {
	"gain=val", "Audio output gain (0 to 255)",
	"port={s|h|l}", "Audio port (s[peaker], h[eadphones], l[ineout])",
	"buffer=val", "Audio buffer size (default is 32768)",
	NULL
};

struct sound_driver sound_solaris = {
	"solaris",
	"Solaris PCM audio",
	help,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};

