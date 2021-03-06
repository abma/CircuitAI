/*
 * UnitAction.h
 *
 *  Created on: Jan 12, 2015
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_UNIT_ACTION_UNITACTION_H_
#define SRC_CIRCUIT_UNIT_ACTION_UNITACTION_H_

#include "util/Action.h"

namespace circuit {

class CCircuitUnit;

class IUnitAction: public IAction {
public:
	enum class Type: char {IDLE, MOVE, PRE_BUILD, BUILD, ATTACK, FIGHT, PATROL, RECLAIM, TERRAFORM, WAIT, DGUN};

protected:
	IUnitAction(CCircuitUnit* owner, Type type);
public:
	virtual ~IUnitAction();

	Type GetType() const { return type; }

protected:
	Type type;
};

} // namespace circuit

#endif // SRC_CIRCUIT_UNIT_ACTION_UNITACTION_H_
