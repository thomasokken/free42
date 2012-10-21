///////////////////////////////////////////////////////////////////////////////
// Free42 -- an HP-42S calculator simulator
// Copyright (C) 2004-2012  Thomas Okken
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see http://www.gnu.org/licenses/.
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#define TDIFF(begin,end) (((double)(end.tv_sec - begin.tv_sec)*1000.0) + ((end.tv_usec - begin.tv_usec)/1000.0))

static unsigned int audio_sample_rate = 22050;
static snd_pcm_format_t audio_format = SND_PCM_FORMAT_S16;
static const char *audio_device = "plughw:0,0";
static int audio_channels = 1;

static snd_pcm_t *playback_handle = NULL;
static bool audio_initialized = false;
static snd_pcm_uframes_t buffer_size;

static struct timeval last_use;

static pthread_mutex_t closer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t closer_cond = PTHREAD_COND_INITIALIZER;
static pthread_t closer_thread;
static bool closer_initialized = false;

static int audio_set_hw_params(snd_pcm_hw_params_t *hw_params) {
    int err;
    snd_pcm_uframes_t size;

    if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
	fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
	return err;
    }
    if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
	fprintf (stderr, "cannot set access type (%s)\n", snd_strerror (err));
	return err;
    }
    if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params, audio_format)) < 0) {
	fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
	return err;
    }
    if ((err = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &audio_sample_rate, 0)) < 0) {
	fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
	return err;
    }
    if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, audio_channels)) < 0) {
	fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
	return err;
    }
    // set audio buffer size to about 125ms
    buffer_size = (audio_sample_rate + 7) / 8;
    if((err = snd_pcm_hw_params_set_buffer_size_near(playback_handle, hw_params, &buffer_size)) < 0) {
	printf("Unable to set buffer size %li for playback: %s\n", buffer_size, snd_strerror(err));
	return err;
    }
    if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
	fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
	return err;
    }
    if((err = snd_pcm_hw_params_get_buffer_size(hw_params, &size)) < 0) {
	printf("Unable to get buffer size for playback: %s\n", snd_strerror(err));
	return err;
    }
    buffer_size = size;
    return 0;
}

static int audio_set_sw_params(snd_pcm_sw_params_t *sw_params) {
    int err;
    snd_pcm_uframes_t boundary;

    /* get the current swparams */
    if((err = snd_pcm_sw_params_current(playback_handle, sw_params)) < 0) {
        fprintf(stderr, "Unable to determine current swparams for playback: %s\n", snd_strerror(err));
        return err;
    }
    if((err = snd_pcm_sw_params_set_silence_threshold(playback_handle, sw_params, 0)) < 0) {
        fprintf(stderr, "Unable to set silence threshold for playback: %s\n", snd_strerror(err));
        return err;
    }
    if((err = snd_pcm_sw_params_get_boundary(sw_params, &boundary)) < 0) {
        fprintf(stderr, "unable to get ring pointer boundary for playback: %s\n", snd_strerror(err));
	return err;
    }
    if((err = snd_pcm_sw_params_set_silence_size(playback_handle, sw_params, boundary)) < 0) {
	fprintf(stderr, "Unable to set silence size playback: %s\n", snd_strerror(err));
	return err;
    }
    if ((err = snd_pcm_sw_params (playback_handle, sw_params)) < 0) {
	fprintf (stderr, "cannot set sw parameters (%s)\n", snd_strerror (err));
	return err;
    }

    return 0;
}

static void *closer(void *) {
    pthread_mutex_lock(&closer_mutex);
    while (true) {
	struct timespec when;
	when.tv_sec = last_use.tv_sec + 5;
	when.tv_nsec = last_use.tv_usec * 1000L;
	struct timeval now;
	gettimeofday(&now, NULL);
	if (when.tv_sec < now.tv_sec || when.tv_sec == now.tv_sec && when.tv_nsec < now.tv_usec * 1000L)
	    break;
	pthread_cond_timedwait(&closer_cond, &closer_mutex, &when);
    }
    snd_pcm_close(playback_handle);
    audio_initialized = false;
    pthread_mutex_unlock(&closer_mutex);
    return NULL;
}

static bool audio_init() {
    pthread_mutex_lock(&closer_mutex);
    gettimeofday(&last_use, NULL);
    if (audio_initialized) {
	pthread_mutex_unlock(&closer_mutex);
	return true;
    }

    int err;

    if ((err = snd_pcm_open(&playback_handle, audio_device, SND_PCM_STREAM_PLAYBACK, 0)) == 0) {
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_hw_params_alloca(&hw_params);
	if ((err = audio_set_hw_params(hw_params)) < 0) {
	    snd_pcm_close(playback_handle);
	    goto fail;
	}

	snd_pcm_sw_params_t *sw_params;
	snd_pcm_sw_params_alloca(&sw_params);
	if ((err = audio_set_sw_params(sw_params)) < 0) {
	    snd_pcm_close(playback_handle);
	    goto fail;
	}
    } else {
	fail:
	fprintf (stderr, "cannot open audio device %s (%s)\n", audio_device, snd_strerror(err));
	pthread_mutex_unlock(&closer_mutex);
	return false;
    }

    audio_initialized = true;

    pthread_create(&closer_thread, NULL, closer, NULL);
    pthread_mutex_unlock(&closer_mutex);
    return true;
}

static int xrun_recovery(snd_pcm_t *handle, int err)
{
    if (err == -EPIPE) {    /* under-run */
	err = snd_pcm_prepare(handle);
	if (err < 0)
	    fprintf(stderr, "Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
	return 0;
    } else if (err == -ESTRPIPE) {
	while ((err = snd_pcm_resume(handle)) == -EAGAIN)
	    sleep(1);       /* wait until the suspend flag is released */
	if (err < 0) {
	    err = snd_pcm_prepare(handle);
	    if (err < 0)
		fprintf(stderr, "Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
	}
	return 0;
    }
    return err;
}

void alsa_beeper(int frequency, int duration) {
    if(!audio_init())
	return;

    /* roughly benchmark the function so that we can return only after the
     * sound has been played */
    struct timeval begin, end;
    gettimeofday(&begin, NULL);

    int format_bits = snd_pcm_format_width(audio_format);
    unsigned int maxval = (1 << (format_bits - 1)) - 1;
    int bps = format_bits / 8;
    int phys_bps = snd_pcm_format_physical_width(audio_format) / 8;
    int big_endian = snd_pcm_format_big_endian(audio_format) == 1;
    int to_unsigned = snd_pcm_format_unsigned(audio_format) == 1;
    int numSamples = duration * audio_sample_rate / 1000;
    int bufferSize, err, x;
    char *buffer, *p;

    bufferSize = numSamples * audio_channels * phys_bps;
    buffer = (char *)malloc(bufferSize);
    if(buffer != NULL) {
	// generate a triangle waveform
	p = buffer;
	for(x = 0; x < numSamples; x++) {
	    int res, i, chn;
	    double v;

	    v = fmod(((double) x) / audio_sample_rate * frequency, 1);
	    if (v >= 0.75)
		v -= 1;
	    else if (v >= 0.25)
		v = 0.5 - v;
	    res = v * maxval;
	    if (to_unsigned)
		res ^= 1U << (format_bits - 1);
	    for(chn = 0; chn < audio_channels; chn ++) {
		if (big_endian) {
		    for (i = 0; i < bps; i++)
			*(p + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
		} else {
		    for (i = 0; i < bps; i++)
			*(p + i)  = (res >>  i * 8) & 0xff;
		}
		p += phys_bps;
	    }
	}

	/* play the waveform */
	p = buffer;
	while(numSamples > 0) {
	    if((err = snd_pcm_writei (playback_handle, p, numSamples)) < 0) {
		if(err == -EAGAIN) {
		    continue;
		}
		if((err = xrun_recovery(playback_handle, err)) < 0) {
		    fprintf (stderr, "write to audio interface failed (%s)\n", snd_strerror (err));
		    break;
		}
	    }
	    else {
		numSamples -= err;
		p += err * phys_bps * audio_channels;
	    }
	}

	free(buffer);

	gettimeofday(&end, NULL);
	duration -= TDIFF(begin, end);
	if(duration > 0) usleep(duration * 1000);
    }
}
