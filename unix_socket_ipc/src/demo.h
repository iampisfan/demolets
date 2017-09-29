struct tlv {
	unsigned int messagetype;
	unsigned int tlv_t;
	unsigned int tlv_l;
	char *tlv_v;
};

struct message {
	unsigned int alid[6];
	unsigned int macaddr[6];
	struct tlv tlv;
};
