/*
 * BlueALSA - sco-lc3-swb.c
 * Copyright (c) 2016-2024 Arkadiusz Bokowy
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#include "sco-lc3-swb.h"

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "ba-transport.h"
#include "ba-transport-pcm.h"
#include "codec-lc3-swb.h"
#include "io.h"
#include "shared/defs.h"
#include "shared/ffb.h"
#include "shared/log.h"
#include "shared/rt.h"

void *sco_lc3_swb_enc_thread(struct ba_transport_pcm *t_pcm) {

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_cleanup_push(PTHREAD_CLEANUP(ba_transport_pcm_thread_cleanup), t_pcm);

	struct ba_transport *t = t_pcm->t;
	struct io_poll io = { .timeout = -1 };
	const size_t mtu_write = t->mtu_write;

	struct esco_lc3_swb codec;
	lc3_swb_init(&codec);

	debug_transport_pcm_thread_loop(t_pcm, "START");
	for (ba_transport_pcm_state_set_running(t_pcm);;) {

		ssize_t samples = ffb_len_in(&codec.pcm);
		switch (samples = io_poll_and_read_pcm(&io, t_pcm, codec.pcm.tail, samples)) {
		case -1:
			if (errno == ESTALE) {
				/* reinitialize LC3-SWB encoder */
				lc3_swb_init(&codec);
				continue;
			}
			error("PCM poll and read error: %s", strerror(errno));
			/* fall-through */
		case 0:
			ba_transport_stop_if_no_clients(t);
			continue;
		}

		ffb_seek(&codec.pcm, samples);

		/* encode as much PCM data as possible */
		while (lc3_swb_encode(&codec) > 0) {

			uint8_t *data = codec.data.data;
			size_t data_len = ffb_blen_out(&codec.data);

			while (data_len >= mtu_write) {

				ssize_t len;
				if ((len = io_bt_write(t_pcm, data, mtu_write)) <= 0) {
					if (len == -1)
						error("BT write error: %s", strerror(errno));
					goto exit;
				}

				data += len;
				data_len -= len;

			}

			/* keep data transfer at a constant bit rate */
			asrsync_sync(&io.asrs, codec.frames * LC3_SWB_CODESAMPLES);
			/* update busy delay (encoding overhead) */
			t_pcm->delay = asrsync_get_busy_usec(&io.asrs) / 100;

			/* Move unprocessed data to the front of our linear
			 * buffer and clear the LC3-SWB frame counter. */
			ffb_shift(&codec.data, ffb_blen_out(&codec.data) - data_len);
			codec.frames = 0;

		}

	}

exit:
	debug_transport_pcm_thread_loop(t_pcm, "EXIT");
	pthread_cleanup_pop(1);
	return NULL;
}

void *sco_lc3_swb_dec_thread(struct ba_transport_pcm *t_pcm) {

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_cleanup_push(PTHREAD_CLEANUP(ba_transport_pcm_thread_cleanup), t_pcm);

	struct ba_transport *t = t_pcm->t;
	struct io_poll io = { .timeout = -1 };

	struct esco_lc3_swb codec;
	lc3_swb_init(&codec);

	debug_transport_pcm_thread_loop(t_pcm, "START");
	for (ba_transport_pcm_state_set_running(t_pcm);;) {

		ssize_t len = ffb_blen_in(&codec.data);
		if ((len = io_poll_and_read_bt(&io, t_pcm, codec.data.tail, len)) == -1)
			error("BT poll and read error: %s", strerror(errno));
		else if (len == 0)
			goto exit;

		if (!ba_transport_pcm_is_active(t_pcm))
			continue;

		if (len > 0)
			ffb_seek(&codec.data, len);

		int err;
		/* Process data until there is no more LC3-SWB frames to decode. This loop
		 * ensures that for MTU values bigger than the LC3-SWB frame size, the input
		 * buffer will not fill up causing short reads and LC3-SWB frame losses. */
		while ((err = lc3_swb_decode(&codec)) > 0)
			continue;

		ssize_t samples;
		if ((samples = ffb_len_out(&codec.pcm)) <= 0)
			continue;

		io_pcm_scale(t_pcm, codec.pcm.data, samples);
		if ((samples = io_pcm_write(t_pcm, codec.pcm.data, samples)) == -1)
			error("FIFO write error: %s", strerror(errno));
		else if (samples == 0)
			ba_transport_stop_if_no_clients(t);

		ffb_shift(&codec.pcm, samples);

	}

exit:
	debug_transport_pcm_thread_loop(t_pcm, "EXIT");
	pthread_cleanup_pop(1);
	return NULL;
}
