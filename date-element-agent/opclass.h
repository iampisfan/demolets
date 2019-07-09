#ifndef _OP_CLASS_H_
#define _OP_CLASS_H

#include "ieee80211.h"
#include "type.h"

struct channel_info {
	u8 opclass;
	u32 freq;
	u8 channel;
	u8 width;
	u32 center_freq1;
	u32 center_freq2;
	u8 txpower;
	
};

struct oper_class_map {
	u8 op_class;
	u8 min_chan;
	u8 max_chan;
	u8 inc;
};

/**
 * ieee80211_freq_to_channel_ext - Convert frequency into channel info
 * for HT40 and VHT. DFS channels are not covered.
 * @freq: Frequency (MHz) to convert
 * @sec_channel: 0 = non-HT40, 1 = sec. channel above, -1 = sec. channel below
 * @vht: VHT channel width (VHT_CHANWIDTH_*)
 * @op_class: Buffer for returning operating class
 * @channel: Buffer for returning channel number
 * Returns: hw_mode on success, NUM_HOSTAPD_MODES on failure
 */
enum hw_mode freq_to_channel_opclass(unsigned int freq,
						   int sec_channel, int vht,
						   u8 *op_class, u8 *channel);
int channel_to_opclass(u8 channel, u8 *opclass);
const struct oper_class_map *get_opclass_channels(u8 opclass);

#endif
