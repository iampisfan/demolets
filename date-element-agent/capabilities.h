#ifndef _CAPABILITIES_H_
#define _CAPABILITIES_H

#include <arpa/inet.h>

#include "ieee80211.h"
#include "vendor.h"

union htcap {
	byte bytes[1];
	struct {
		byte reserved:1;
		byte ht40m:1;
		byte sgi_40m:1;
		byte sgi_20m:1;
		byte rx_stream:2;
		byte tx_stream:2;
	} binary;
};

union vhtcap {
	byte bytes[6];
	struct {
		u16 tx_mcs;
		u16 rx_mcs;
		byte sgi_160m:1;
		byte sgi_80m:1;
		byte rx_stream:3;
		byte tx_stream:3;
		byte reserved:4;
		byte mu_beamformer:1;
		byte su_beamformer:1;
		byte vht160m:1;
		byte vht80_80m:1;
	} binary;
};

static inline void fill_htcap(struct ht_vht_capability *capa, union htcap *ht)
{
	if (!capa || !ht)
		return;

	ht->binary.ht40m = !!(capa->htcap.cap & BIT(1));
	ht->binary.sgi_40m = !!(capa->htcap.cap & BIT(6));
	ht->binary.sgi_20m = !!(capa->htcap.cap & BIT(5));
	ht->binary.rx_stream = ((capa->htcap.mcs.rx_mask[1] == 0xff)? 1:0) +
				((capa->htcap.mcs.rx_mask[2] == 0xff)? 1:0) +
				((capa->htcap.mcs.rx_mask[3] == 0xff)? 1:0);
	ht->binary.tx_stream = ht->binary.rx_stream;
}

static int get_vht_streams(u16 mcs)
{
	int num = 0;

	while (mcs) {
		if (0x2 == (mcs & 0x3))
			num++;

		mcs = mcs >> 2;
	}

	if (num < 1)
		num = 1;

	return num;
}

static inline void fill_vhtcap(struct ht_vht_capability *capa, union vhtcap *vht)
{
	byte tmp;
	if (!capa || !vht)
		return;

	vht->binary.tx_mcs = ntohs(capa->vhtcap.vht_mcs.tx_mcs_map);
	vht->binary.rx_mcs = ntohs(capa->vhtcap.vht_mcs.rx_mcs_map);
	vht->binary.sgi_160m = !!(capa->vhtcap.cap & BIT(6));
	vht->binary.sgi_80m = !!(capa->vhtcap.cap & BIT(5));
	vht->binary.rx_stream = get_vht_streams(vht->binary.rx_mcs)-1;
	vht->binary.tx_stream = get_vht_streams(vht->binary.tx_mcs)-1;
	vht->binary.mu_beamformer = !!(capa->vhtcap.cap & BIT(19));
	vht->binary.su_beamformer = !!(capa->vhtcap.cap & BIT(11));
	vht->binary.vht160m = ((capa->vhtcap.cap >> 2) & 3) == 1 ? 1 : 0;
	vht->binary.vht80_80m = ((capa->vhtcap.cap >> 2) & 3) == 2 ? 1 : 0;
}

#endif
