#ifndef recorder_h_INCLUDED
#define recorder_h_INCLUDED

#include "qpn_port.h"
#include "hc.h"
#include "titime.h"

struct Recorder {
	QActive super;
	int16_t ti;
	int16_t calibration;
	struct TiTime max_today;
	struct TiTime max_yesterday;
	struct TiTime min_today;
	struct TiTime min_yesterday;
};


extern struct Recorder recorder;

void recorder_ctor(void);

inline const struct TiTime *get_max_today(void) {
	return &recorder.max_today;
}

inline const struct TiTime *get_max_yesterday(void) {
	return &recorder.max_yesterday;
}

inline const struct TiTime *get_min_today(void) {
	return &recorder.min_today;
}

inline const struct TiTime *get_min_yesterday(void) {
	return &recorder.min_yesterday;
}

inline int16_t get_calibration(void) {
	return recorder.calibration;
}

void set_calibration(int16_t cal);

#endif
