/*
 * AttackTask.h
 *
 *  Created on: Jan 28, 2015
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_TASK_FIGHTER_ATTACKTASK_H_
#define SRC_CIRCUIT_TASK_FIGHTER_ATTACKTASK_H_

#include "task/fighter/FighterTask.h"

namespace circuit {

class CAttackTask: public IFighterTask {
public:
	CAttackTask(ITaskManager* mgr);
	virtual ~CAttackTask();

	virtual void Execute(CCircuitUnit* unit);
	virtual void Update();

private:
	void Execute(CCircuitUnit* unit, bool isUpdating);
	CEnemyUnit* FindBestTarget(CCircuitUnit* unit, float& minSqDist);
};

} // namespace circuit

#endif // SRC_CIRCUIT_TASK_FIGHTER_ATTACKTASK_H_
