/* sequence message */
struct picard_seq {
	uint32_t target;
	uint8_t *seq;
};

/* sequence info message */
struct picard_seq_info {
	uint16_t target;
	uint16_t current_seq;
};

/* voltage info message */
struct picard_v_info {
	uint16_t target;
	uint16_t vok;
	float vpp;
	float vnn;
};

/* lock/unlock message */
struct picard_lock {
	uint8_t target;
};
