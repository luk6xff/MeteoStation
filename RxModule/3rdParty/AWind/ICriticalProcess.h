/*
 * ICriticalProcess.h
 *
 *  Created on: 27-09-2016
 *      Author: igbt6
 */

#ifndef ICRITICALPROCESS_H_
#define ICRITICALPROCESS_H_

///User should derive his class from ICriticalProcess if such class has time critical functions, like check whether user touch the display
class ICriticalProcess
{
public:
	///In derived class contains time critical source code
	void virtual Idle()=0;
};


#endif /* ICRITICALPROCESS_H_ */
