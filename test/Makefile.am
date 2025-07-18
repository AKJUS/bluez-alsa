# BlueALSA - Makefile.am
# Copyright (c) 2016-2024 Arkadiusz Bokowy

SUBDIRS = mock

TESTS = \
	test-a2dp \
	test-alsa-ctl \
	test-alsa-pcm \
	test-alsa-pcm-hwcompat-busy \
	test-alsa-pcm-hwcompat-none \
	test-alsa-pcm-hwcompat-silence \
	test-at \
	test-audio \
	test-ba \
	test-dbus \
	test-h2 \
	test-io \
	test-rfcomm \
	test-rtp \
	test-utils

check_PROGRAMS = \
	test-a2dp \
	test-alsa-ctl \
	test-alsa-pcm \
	test-at \
	test-audio \
	test-ba \
	test-dbus \
	test-h2 \
	test-io \
	test-rfcomm \
	test-rtp \
	test-utils

if ENABLE_APLAY
TESTS += test-utils-aplay
check_PROGRAMS += test-utils-aplay
endif

if ENABLE_CTL
TESTS += test-utils-ctl
check_PROGRAMS += test-utils-ctl
endif

if ENABLE_LC3_SWB
TESTS += test-lc3-swb
check_PROGRAMS += test-lc3-swb
endif

if ENABLE_MIDI
TESTS += test-alsa-midi test-ble-midi
check_PROGRAMS += test-alsa-midi test-ble-midi
endif

if ENABLE_MSBC
TESTS += test-msbc
check_PROGRAMS += test-msbc
endif

if HAVE_SNDFILE
check_PROGRAMS += sndalign
endif

check_LTLIBRARIES = \
	libaloader.la
libaloader_la_LDFLAGS = \
	-rpath /nowhere \
	-avoid-version \
	-shared
libaloader_la_LIBADD = \
	@ALSA_LIBS@

BUILT_SOURCES = \
	test-dbus-iface.c

MOSTLYCLEANFILES = \
	$(BUILT_SOURCES)

test_a2dp_SOURCES = \
	../src/shared/a2dp-codecs.c \
	../src/shared/ffb.c \
	../src/shared/log.c \
	../src/shared/rt.c \
	../src/ba-config.c \
	../src/a2dp.c \
	../src/a2dp-sbc.c \
	../src/audio.c \
	../src/codec-sbc.c \
	../src/io.c \
	../src/rtp.c \
	../src/utils.c \
	test-a2dp.c

test_alsa_ctl_SOURCES = \
	../src/shared/log.c \
	test-alsa-ctl.c

test_alsa_midi_SOURCES = \
	../src/shared/log.c \
	test-alsa-midi.c

test_alsa_pcm_SOURCES = \
	../src/shared/log.c \
	../src/shared/rt.c \
	test-alsa-pcm.c

test_at_SOURCES = \
	../src/shared/log.c \
	../src/at.c \
	test-at.c

test_audio_SOURCES = \
	../src/shared/log.c \
	../src/audio.c \
	test-audio.c

test_ba_SOURCES = \
	../src/shared/a2dp-codecs.c \
	../src/shared/ffb.c \
	../src/shared/log.c \
	../src/shared/rt.c \
	../src/audio.c \
	../src/ba-adapter.c \
	../src/ba-config.c \
	../src/ba-device.c \
	../src/ba-transport.c \
	../src/ba-transport-pcm.c \
	../src/codec-sbc.c \
	../src/dbus.c \
	../src/h2.c \
	../src/hci.c \
	../src/hfp.c \
	../src/io.c \
	../src/sco.c \
	../src/sco-cvsd.c \
	../src/storage.c \
	../src/utils.c \
	test-ba.c

test_dbus_SOURCES = \
	../src/shared/log.c \
	../src/dbus.c \
	test-dbus-iface.c \
	test-dbus.c

test_h2_SOURCES = \
	../src/h2.c \
	test-h2.c

test_io_SOURCES = \
	../src/shared/a2dp-codecs.c \
	../src/shared/ffb.c \
	../src/shared/log.c \
	../src/shared/rt.c \
	../src/a2dp-sbc.c \
	../src/audio.c \
	../src/ba-adapter.c \
	../src/ba-config.c \
	../src/ba-device.c \
	../src/ba-transport-pcm.c \
	../src/codec-sbc.c \
	../src/dbus.c \
	../src/h2.c \
	../src/hci.c \
	../src/hfp.c \
	../src/io.c \
	../src/rtp.c \
	../src/sco.c \
	../src/sco-cvsd.c \
	../src/utils.c \
	test-io.c

if ENABLE_MIDI
test_ble_midi_SOURCES = \
	../src/shared/log.c \
	../src/shared/rt.c \
	../src/ble-midi.c \
	test-ble-midi.c
endif

if ENABLE_LC3_SWB
test_lc3_swb_SOURCES = \
	../src/shared/ffb.c \
	../src/shared/log.c \
	../src/codec-lc3-swb.c \
	../src/h2.c \
	test-lc3-swb.c
endif

if ENABLE_MSBC
test_msbc_SOURCES = \
	../src/shared/ffb.c \
	../src/shared/log.c \
	../src/codec-msbc.c \
	../src/codec-sbc.c \
	../src/h2.c \
	test-msbc.c
endif

test_rfcomm_SOURCES = \
	../src/shared/a2dp-codecs.c \
	../src/shared/ffb.c \
	../src/shared/log.c \
	../src/shared/rt.c \
	../src/at.c \
	../src/audio.c \
	../src/ba-adapter.c \
	../src/ba-config.c \
	../src/ba-device.c \
	../src/ba-rfcomm.c \
	../src/ba-transport.c \
	../src/ba-transport-pcm.c \
	../src/dbus.c \
	../src/h2.c \
	../src/hci.c \
	../src/hfp.c \
	../src/io.c \
	../src/sco.c \
	../src/sco-cvsd.c \
	../src/utils.c \
	test-rfcomm.c

test_rtp_SOURCES = \
	../src/shared/log.c \
	../src/rtp.c \
	test-rtp.c

test_utils_SOURCES = \
	../src/shared/ffb.c \
	../src/shared/hex.c \
	../src/shared/log.c \
	../src/shared/nv.c \
	../src/shared/rt.c \
	../src/ba-config.c \
	../src/hci.c \
	../src/utils.c \
	test-utils.c

if ENABLE_AAC
test_a2dp_SOURCES += ../src/a2dp-aac.c
test_io_SOURCES += ../src/a2dp-aac.c
endif

if ENABLE_APTX
test_a2dp_SOURCES += ../src/a2dp-aptx.c
test_io_SOURCES += ../src/a2dp-aptx.c
endif

if ENABLE_APTX_HD
test_a2dp_SOURCES += ../src/a2dp-aptx-hd.c
test_io_SOURCES += ../src/a2dp-aptx-hd.c
endif

if ENABLE_APTX_OR_APTX_HD
test_a2dp_SOURCES += ../src/codec-aptx.c
test_io_SOURCES += ../src/codec-aptx.c
endif

if ENABLE_FASTSTREAM
test_a2dp_SOURCES += ../src/a2dp-faststream.c
test_io_SOURCES += ../src/a2dp-faststream.c
endif

if ENABLE_LC3PLUS
test_a2dp_SOURCES += ../src/a2dp-lc3plus.c
test_io_SOURCES += ../src/a2dp-lc3plus.c
endif

if ENABLE_LC3_SWB
test_ba_SOURCES += \
	../src/codec-lc3-swb.c \
	../src/sco-lc3-swb.c
test_io_SOURCES += \
	../src/codec-lc3-swb.c \
	../src/sco-lc3-swb.c
test_rfcomm_SOURCES += \
	../src/codec-lc3-swb.c \
	../src/sco-lc3-swb.c
endif

if ENABLE_LDAC
test_a2dp_SOURCES += ../src/a2dp-ldac.c
test_io_SOURCES += ../src/a2dp-ldac.c
endif

if ENABLE_LHDC
test_a2dp_SOURCES += ../src/a2dp-lhdc.c
test_io_SOURCES += ../src/a2dp-lhdc.c
endif

if ENABLE_MPEG
test_a2dp_SOURCES += ../src/a2dp-mpeg.c
test_io_SOURCES += ../src/a2dp-mpeg.c
endif

if ENABLE_MSBC
test_ba_SOURCES += \
	../src/codec-msbc.c \
	../src/sco-msbc.c
test_io_SOURCES += \
	../src/codec-msbc.c \
	../src/sco-msbc.c
test_rfcomm_SOURCES += \
	../src/codec-msbc.c \
	../src/codec-sbc.c \
	../src/sco-msbc.c
endif

if ENABLE_OPUS
test_a2dp_SOURCES += ../src/a2dp-opus.c
test_io_SOURCES += ../src/a2dp-opus.c
endif

AM_TESTS_ENVIRONMENT = \
	export G_DEBUG=fatal-warnings;

AM_CFLAGS = \
	-I$(top_srcdir)/src \
	@AAC_CFLAGS@ \
	@ALSA_CFLAGS@ \
	@APTX_CFLAGS@ \
	@APTX_HD_CFLAGS@ \
	@BLUEZ_CFLAGS@ \
	@CHECK_CFLAGS@ \
	@GIO2_CFLAGS@ \
	@GLIB2_CFLAGS@ \
	@LC3_CFLAGS@ \
	@LDAC_ABR_CFLAGS@ \
	@LDAC_DEC_CFLAGS@ \
	@LDAC_ENC_CFLAGS@ \
	@LHDC_DEC_CFLAGS@ \
	@LHDC_ENC_CFLAGS@ \
	@LIBBSD_CFLAGS@ \
	@LIBUNWIND_CFLAGS@ \
	@MP3LAME_CFLAGS@ \
	@MPG123_CFLAGS@ \
	@OPENMP_CFLAGS@ \
	@OPUS_CFLAGS@ \
	@SBC_CFLAGS@ \
	@SNDFILE_CFLAGS@ \
	@SPANDSP_CFLAGS@

LDADD = \
	@AAC_LIBS@ \
	@ALSA_LIBS@ \
	@APTX_LIBS@ \
	@APTX_HD_LIBS@ \
	@BLUEZ_LIBS@ \
	@CHECK_LIBS@ \
	@GIO2_LIBS@ \
	@GLIB2_LIBS@ \
	@LC3_LIBS@ \
	@LC3PLUS_LIBS@ \
	@LDAC_ABR_LIBS@ \
	@LDAC_DEC_LIBS@ \
	@LDAC_ENC_LIBS@ \
	@LHDC_DEC_LIBS@ \
	@LHDC_ENC_LIBS@ \
	@LIBUNWIND_LIBS@ \
	@MP3LAME_LIBS@ \
	@MPG123_LIBS@ \
	@OPUS_LIBS@ \
	@SBC_LIBS@ \
	@SNDFILE_LIBS@ \
	@SPANDSP_LIBS@

.xml.c:
	$(top_srcdir)/src/dbus-codegen.py --output $@ \
		--interface-info-body --interface-skeleton-body $<
