#ifndef _IEEE80211_H_
#define _IEEE80211_H_

#include "type.h"

#define IEEE80211_HT_MCS_MASK_LEN		10
#define HT_CAP  45
#define VHT_CAP 191


/**
 * struct ieee80211_mcs_info - MCS information
 * @rx_mask: RX mask
 * @rx_highest: highest supported RX rate. If set represents
 *	the highest supported RX data rate in units of 1 Mbps.
 *	If this field is 0 this value should not be used to
 *	consider the highest RX data rate supported.
 * @tx_params: TX parameters
 */
struct ieee80211_mcs_info {
	u8 rx_mask[IEEE80211_HT_MCS_MASK_LEN];
	u16 rx_highest;
	u8 tx_params;
	u8 reserved[3];
}  __attribute__ ((__packed__));

/**
 * struct ieee80211_vht_mcs_info - VHT MCS information
 * @rx_mcs_map: RX MCS map 2 bits for each stream, total 8 streams
 * @rx_highest: Indicates highest long GI VHT PPDU data rate
 *	STA can receive. Rate expressed in units of 1 Mbps.
 *	If this field is 0 this value should not be used to
 *	consider the highest RX data rate supported.
 *	The top 3 bits of this field are reserved.
 * @tx_mcs_map: TX MCS map 2 bits for each stream, total 8 streams
 * @tx_highest: Indicates highest long GI VHT PPDU data rate
 *	STA can transmit. Rate expressed in units of 1 Mbps.
 *	If this field is 0 this value should not be used to
 *	consider the highest TX data rate supported.
 *	The top 3 bits of this field are reserved.
 */
struct ieee80211_vht_mcs_info {
	u16 rx_mcs_map;
	u16 rx_highest;
	u16 tx_mcs_map;
	u16 tx_highest;
}  __attribute__ ((__packed__));


/**
 * struct ieee80211_sta_ht_cap - STA's HT capabilities
 *
 * This structure describes most essential parameters needed
 * to describe 802.11n HT capabilities for an STA.
 *
 * @ht_supported: is HT supported by the STA
 * @cap: HT capabilities map as described in 802.11n spec
 * @ampdu_factor: Maximum A-MPDU length factor
 * @ampdu_density: Minimum A-MPDU spacing
 * @mcs: Supported MCS rates
 */
struct ieee80211_sta_ht_cap {
	u16 cap; /* use IEEE80211_HT_CAP_ */
	_Bool ht_supported;
	u8 ampdu_factor;
	u8 ampdu_density;
	struct ieee80211_mcs_info mcs;
};

/**
 * struct ieee80211_sta_vht_cap - STA's VHT capabilities
 *
 * This structure describes most essential parameters needed
 * to describe 802.11ac VHT capabilities for an STA.
 *
 * @vht_supported: is VHT supported by the STA
 * @cap: VHT capabilities map as described in 802.11ac spec
 * @vht_mcs: Supported VHT MCS rates
 */
struct ieee80211_sta_vht_cap {
	_Bool vht_supported;
	u32 cap; /* use IEEE80211_VHT_CAP_ */
	struct ieee80211_vht_mcs_info vht_mcs;
};

struct IEEEtypes_HT_Element_t
{
	u8 ElementId;
	u8 Len;
	u16 HTCapabilitiesInfo;
	u8   MacHTParamInfo;
	u8 SupportedMCSset[16];
}  __attribute__ ((__packed__));

struct IEEEtypes_vht_cap {
    u8 id;
    u8 len;
    u32 cap;
    u32 SupportedRxMcsSet;
    u32 SupportedTxMcsSet;
}  __attribute__ ((__packed__));


/* VHT channel widths */
#define VHT_CHANWIDTH_USE_HT	0
#define VHT_CHANWIDTH_80MHZ		1
#define VHT_CHANWIDTH_160MHZ	2
#define VHT_CHANWIDTH_80P80MHZ	3

#define BAND_2G 0
#define BAND_5G 1

enum hw_mode {
	HOSTAPD_MODE_IEEE80211B,
	HOSTAPD_MODE_IEEE80211G,
	HOSTAPD_MODE_IEEE80211A,
	HOSTAPD_MODE_IEEE80211AD,
	HOSTAPD_MODE_IEEE80211ANY,
	NUM_HOSTAPD_MODES
};

#endif
