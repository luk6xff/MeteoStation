/*
 * LUTouch.h
 *
 *  Created on: 28-09-2016
 *      Author: igbt6
 */

#ifndef LUTOUCH_H_
#define LUTOUCH_H_
#include <stdint.h>

typedef struct {
	uint16_t x;
	uint16_t y;
} TouchPoint;

class LUTouch {
public:
	LUTouch();
	virtual void init() = 0;
	virtual bool read() = 0;
	virtual bool dataAvailable()= 0;
	virtual TouchPoint getTouchedPoint() = 0;
	virtual ~LUTouch();
protected:
	TouchPoint m_currentTouchedPoint;
};

#endif /* LUTOUCH_H_ */
