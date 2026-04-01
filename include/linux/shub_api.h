#ifndef _SHUB_API_H
#define _SHUB_API_H

#include <linux/types.h>

struct shub_event_params {
	/*
	 * eHalSenData : 129
	 * eHalFlush : 130
	 */
	u8 Cmd;
	/*
	 * sizeof(struct shub_event_params)
	 */
	u8 Length;
	/*
	 * SENSOR_HANDLE_PROXIMITY : 8
	 * SENSOR_HANDLE_WAKE_UP_PROXIMITY : 58 (SensorHub use this)
	 */
	u16 HandleID;
	union {
		u8 udata[4];
		struct {
			s8 status;
			s8 type;
		};
	};
	union {
		/*
		 * For Proximity:
		 *   approaching : fdata[0] = 0x40a00000
		 *   away : fdata[0] = 0x0
		 */
		u32 fdata[3];
		struct {
			u32 x;
			u32 y;
			u32 z;
		};
		struct {
			u64 pedometer;
		};
		struct {
			u32 heart_rate;
		};
	};
	s64 timestamp;
};

int peri_send_sensor_event_to_iio(u8 *data, u16 len);

#endif