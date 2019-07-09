#include <stdio.h>
#include <stdlib.h>

/* TODO: fill with true value, but not hardcode */


enum index {
	Class,
	MaxTxPower,
	NonOperable,
	NumberOfNonOperChan,
	IndexMax
};

const struct data_element *capable_op_class_nodes[] = {
	[Class] = {
		"Class", FLAG_LIST_KEY, TYPE_UINT8,
	},
	[MaxTxPower] = {
		"MaxTxPower", FLAG_NONE, TYPE_INT8,
	},
	[NonOperable] = {
		"NonOperable", FLAG_NONE, TYPE_ARRAY,
	},
	[NumberOfNonOperChan] = {
		"NumberOfNonOperChan", FLAG_NONE, TYPE_UINT8,
	},
};
