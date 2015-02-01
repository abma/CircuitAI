/*
 * DefendTask.cpp
 *
 *  Created on: Jan 28, 2015
 *      Author: rlcevg
 */

#include "task/fighter/DefendTask.h"
#include "task/RetreatTask.h"
#include "task/TaskManager.h"
#include "unit/CircuitUnit.h"
#include "util/utils.h"

#include "Unit.h"

namespace circuit {

using namespace springai;

CDefendTask::CDefendTask(ITaskManager* mgr) :
		IUnitTask(mgr, Priority::NORMAL, Type::ATTACK)
{
}

CDefendTask::~CDefendTask()
{
	PRINT_DEBUG("Execute: %s\n", __PRETTY_FUNCTION__);
}

void CDefendTask::Execute(CCircuitUnit* unit)
{

}

void CDefendTask::Update()
{
	// TODO: Monitor threat? Or do it on EnemySeen/EnemyDestroyed?
}

void CDefendTask::OnUnitIdle(CCircuitUnit* unit)
{
	// TODO: Should be on patrol. Stuck?
}

void CDefendTask::OnUnitDamaged(CCircuitUnit* unit, CCircuitUnit* attacker)
{
	Unit* u = unit->GetUnit();
	if (u->GetHealth() > u->GetMaxHealth() * 0.6) {
		return;
	}

	RemoveAssignee(unit);
	manager->GetRetreatTask()->AssignTo(unit);
}

void CDefendTask::OnUnitDestroyed(CCircuitUnit* unit, CCircuitUnit* attacker)
{
	RemoveAssignee(unit);
}

} // namespace circuit