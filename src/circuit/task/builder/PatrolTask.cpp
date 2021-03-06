/*
 * PatrolTask.cpp
 *
 *  Created on: Jan 31, 2015
 *      Author: rlcevg
 */

#include "task/builder/PatrolTask.h"
#include "task/TaskManager.h"
#include "terrain/TerrainManager.h"
#include "CircuitAI.h"
#include "util/utils.h"

#include "Map.h"

namespace circuit {

using namespace springai;

CBPatrolTask::CBPatrolTask(ITaskManager* mgr, Priority priority,
						   const AIFloat3& position,
						   float cost, int timeout)
		: IBuilderTask(mgr, priority, nullptr, position, BuildType::PATROL, cost, false, timeout)
{
}

CBPatrolTask::~CBPatrolTask()
{
	PRINT_DEBUG("Execute: %s\n", __PRETTY_FUNCTION__);
}

void CBPatrolTask::AssignTo(CCircuitUnit* unit)
{
	IBuilderTask::AssignTo(unit);

	lastTouched = manager->GetCircuit()->GetLastFrame();
}

void CBPatrolTask::RemoveAssignee(CCircuitUnit* unit)
{
	IBuilderTask::RemoveAssignee(unit);

	manager->AbortTask(this);
}

void CBPatrolTask::Execute(CCircuitUnit* unit)
{
	Unit* u = unit->GetUnit();
	u->ExecuteCustomCommand(CMD_PRIORITY, {0.0f});

	const float size = SQUARE_SIZE * 100;
	CTerrainManager* terrainManager = manager->GetCircuit()->GetTerrainManager();
	AIFloat3 pos = position;
	pos.x += (pos.x > terrainManager->GetTerrainWidth() / 2) ? -size : size;
	pos.z += (pos.z > terrainManager->GetTerrainHeight() / 2) ? -size : size;
	u->PatrolTo(pos);
}

void CBPatrolTask::Update()
{
}

void CBPatrolTask::Finish()
{
}

void CBPatrolTask::Cancel()
{
}

} // namespace circuit
