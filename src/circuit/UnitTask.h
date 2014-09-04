/*
 * UnitTask.h
 *
 *  Created on: Sep 2, 2014
 *      Author: rlcevg
 */

#ifndef UNITTASK_H_
#define UNITTASK_H_

#include <memory>

namespace circuit {

class CUnitDetail;

class CUnitTask {
public:
	CUnitTask();
	virtual ~CUnitTask();

private:
	std::shared_ptr<CUnitDetail> unit;

};

} // namespace circuit

#endif // UNITTASK_H_