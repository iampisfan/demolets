#include <stdio.h>

#include "opclass.h"
#include "ieee80211.h"

const struct oper_class_map global_op_class[] = {
	{ 81, 1, 13, 1 },
	{ 82, 14, 14, 1 },

	/* Do not enable HT40 on 2.4 GHz for P2P use for now */
	{ 83, 1, 9, 1 },
	{ 84, 5, 13, 1 },

	{ 115, 36, 48, 4 },
	{ 116, 36, 44, 8 },
	{ 117, 40, 48, 8 },
	{ 118, 52, 64, 4 },
	{ 119, 52, 60, 8 },
	{ 120, 56, 64, 8 },
	{ 121, 100, 140, 4 },
	{ 122, 100, 132, 8 },
	{ 123, 104, 136, 8 },
	{ 124, 149, 161, 4 },
	{ 125, 149, 169, 4 },
	{ 126, 149, 157, 8 },
	{ 127, 153, 161, 8 },

	/*
	 * IEEE P802.11ac/D7.0 Table E-4 actually talks about channel center
	 * frequency index 42, 58, 106, 122, 138, 155 with channel spacing of
	 * 80 MHz, but currently use the following definition for simplicity
	 * (these center frequencies are not actual channels, which makes
	 * wpas_p2p_allow_channel() fail). wpas_p2p_verify_80mhz() should take
	 * care of removing invalid channels.
	 */
	{ 128, 36, 161, 4 },
	{ 129, 50, 114, 16 },
	{ 130, 36, 161, 4 },
	{ 180, 1, 4, 1 },
	{ 0, 0, 0, 0 }
};

enum hw_mode freq_to_channel_opclass(unsigned int freq,
						   int sec_channel, int vht,
						   u8 *op_class, u8 *channel)
{
	u8 vht_opclass;

	/* TODO: more operating classes */

	if (sec_channel > 1 || sec_channel < -1)
		return NUM_HOSTAPD_MODES;

	if (freq >= 2412 && freq <= 2472) {
		if ((freq - 2407) % 5)
			return NUM_HOSTAPD_MODES;

		if (vht)
			return NUM_HOSTAPD_MODES;

		/* 2.407 GHz, channels 1..13 */
		if (sec_channel == 1)
			*op_class = 83;
		else if (sec_channel == -1)
			*op_class = 84;
		else
			*op_class = 81;

		*channel = (freq - 2407) / 5;

		return HOSTAPD_MODE_IEEE80211G;
	}

	if (freq == 2484) {
		if (sec_channel || vht)
			return NUM_HOSTAPD_MODES;

		*op_class = 82; /* channel 14 */
		*channel = 14;

		return HOSTAPD_MODE_IEEE80211B;
	}

	if (freq >= 4900 && freq < 5000) {
		if ((freq - 4000) % 5)
			return NUM_HOSTAPD_MODES;
		*channel = (freq - 4000) / 5;
		*op_class = 0; /* TODO */
		return HOSTAPD_MODE_IEEE80211A;
	}

	switch (vht) {
	case VHT_CHANWIDTH_80MHZ:
		vht_opclass = 128;
		break;
	case VHT_CHANWIDTH_160MHZ:
		vht_opclass = 129;
		break;
	case VHT_CHANWIDTH_80P80MHZ:
		vht_opclass = 130;
		break;
	default:
		vht_opclass = 0;
		break;
	}

	/* 5 GHz, channels 36..48 */
	if (freq >= 5180 && freq <= 5240) {
		if ((freq - 5000) % 5)
			return NUM_HOSTAPD_MODES;

		if (vht_opclass)
			*op_class = vht_opclass;
		else if (sec_channel == 1)
			*op_class = 116;
		else if (sec_channel == -1)
			*op_class = 117;
		else
			*op_class = 115;

		*channel = (freq - 5000) / 5;

		return HOSTAPD_MODE_IEEE80211A;
	}

	/* 5 GHz, channels 52..64 */
	if (freq >= 5260 && freq <= 5320) {
		if ((freq - 5000) % 5)
			return NUM_HOSTAPD_MODES;

		if (vht_opclass)
			*op_class = vht_opclass;
		else if (sec_channel == 1)
			*op_class = 119;
		else if (sec_channel == -1)
			*op_class = 120;
		else
			*op_class = 118;

		*channel = (freq - 5000) / 5;

		return HOSTAPD_MODE_IEEE80211A;
	}

	/* 5 GHz, channels 149..169 */
	if (freq >= 5745 && freq <= 5845) {
		if ((freq - 5000) % 5)
			return NUM_HOSTAPD_MODES;

		if (vht_opclass)
			*op_class = vht_opclass;
		else if (sec_channel == 1)
			*op_class = 126;
		else if (sec_channel == -1)
			*op_class = 127;
		else if (freq <= 5805)
			*op_class = 124;
		else
			*op_class = 125;

		*channel = (freq - 5000) / 5;

		return HOSTAPD_MODE_IEEE80211A;
	}

	/* 5 GHz, channels 100..140 */
	if (freq >= 5000 && freq <= 5700) {
		if ((freq - 5000) % 5)
			return NUM_HOSTAPD_MODES;

		if (vht_opclass)
			*op_class = vht_opclass;
		else if (sec_channel == 1)
			*op_class = 122;
		else if (sec_channel == -1)
			*op_class = 123;
		else
			*op_class = 121;

		*channel = (freq - 5000) / 5;

		return HOSTAPD_MODE_IEEE80211A;
	}

	if (freq >= 5000 && freq < 5900) {
		if ((freq - 5000) % 5)
			return NUM_HOSTAPD_MODES;
		*channel = (freq - 5000) / 5;
		*op_class = 0; /* TODO */
		return HOSTAPD_MODE_IEEE80211A;
	}

	/* 56.16 GHz, channel 1..4 */
	if (freq >= 56160 + 2160 * 1 && freq <= 56160 + 2160 * 4) {
		if (sec_channel || vht)
			return NUM_HOSTAPD_MODES;

		*channel = (freq - 56160) / 2160;
		*op_class = 180;

		return HOSTAPD_MODE_IEEE80211AD;
	}

	return NUM_HOSTAPD_MODES;
}

/* TODO: */
int channel_to_opclass(u8 channel, u8 *opclass)
{
	switch (channel) {
	case 6:
		*opclass = 81;
		return 0;
	case 36:
		*opclass = 115;
		return 0;
	default:
		return -1;
	}
}

const struct oper_class_map *get_opclass_channels(u8 opclass)
{
	int i;

	for (i = 0; i < sizeof(global_op_class)/sizeof(struct oper_class_map); i++) {
		if (global_op_class[i].op_class == opclass)
			return &global_op_class[i];
	}

	return NULL;
}

