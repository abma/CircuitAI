/*
 * TerraformTask.cpp
 *
 *  Created on: Jan 31, 2015
 *      Author: rlcevg
 */

#include "task/builder/TerraformTask.h"
#include "task/TaskManager.h"
#include "CircuitAI.h"
#include "util/utils.h"

#include "Map.h"

namespace circuit {

using namespace springai;

CBTerraformTask::CBTerraformTask(ITaskManager* mgr, Priority priority, CCircuitUnit* target, float cost, int timeout)
		: IBuilderTask(mgr, priority, target->GetCircuitDef(), target->GetUnit()->GetPos(), BuildType::TERRAFORM, cost, false, timeout)
		, targetId(target->GetId())
{
	facing = target->GetUnit()->GetBuildingFacing();
}

CBTerraformTask::~CBTerraformTask()
{
	PRINT_DEBUG("Execute: %s\n", __PRETTY_FUNCTION__);
}

void CBTerraformTask::Execute(CCircuitUnit* unit)
{
	CCircuitAI* circuit = manager->GetCircuit();
	if (circuit->GetTeamUnit(targetId) == nullptr) {  // is unit still alive?
		manager->AbortTask(this);
		return;
	}

	Unit* u = unit->GetUnit();

	std::vector<float> params;
	params.push_back(static_cast<float>(priority));
	u->ExecuteCustomCommand(CMD_PRIORITY, params);

	UnitDef* unitDef = buildDef->GetUnitDef();
	float offsetX = (((facing & 1) == 0) ? unitDef->GetXSize() : unitDef->GetZSize()) / 2 * SQUARE_SIZE + 3 * SQUARE_SIZE + 1;
	float offsetZ = (((facing & 1) == 1) ? unitDef->GetXSize() : unitDef->GetZSize()) / 2 * SQUARE_SIZE + 3 * SQUARE_SIZE + 1;
	params.clear();
	params.push_back(1.0f);  // 1: terraform_type, 1 == level
	params.push_back(manager->GetCircuit()->GetTeamId());  // 2: teamId
	params.push_back(0.0f);  // 3: terraform type - 0 == Wall, else == Area
	params.push_back(circuit->GetMap()->GetElevationAt(position.x, position.z) + unitDef->GetHeight());  // 4: terraformHeight
	params.push_back(5.0f);  // 5: number of control points
	params.push_back(1.0f);  // 6: units count?
	params.push_back(0.0f);  // 7: volumeSelection?
	params.push_back(position.x - offsetX);  //  8: i + 0 control point x
	params.push_back(position.z - offsetZ);  //  9: i + 1 control point z
	params.push_back(position.x - offsetX);  //  8: i + 2 control point x
	params.push_back(position.z + offsetZ);  //  9: i + 3 control point z
	params.push_back(position.x + offsetX);  //  8: i + 4 control point x
	params.push_back(position.z + offsetZ);  //  9: i + 5 control point z
	params.push_back(position.x + offsetX);  //  8: i + 6 control point x
	params.push_back(position.z - offsetZ);  //  9: i + 7 control point z
	params.push_back(position.x - offsetX);  //  8: i + 8 control point x
	params.push_back(position.z - offsetZ);  //  9: i + 9 control point z
	params.push_back(unit->GetId());  // 10: i + 10 unitId
	u->ExecuteCustomCommand(CMD_TERRAFORM_INTERNAL, params);

	// TODO: Enqueue "move out" action for nearby units
}

void CBTerraformTask::Finish()
{
}

void CBTerraformTask::Cancel()
{
}

void CBTerraformTask::OnUnitIdle(CCircuitUnit* unit)
{
	float range = unit->GetCircuitDef()->GetBuildDistance() * 2;
	const AIFloat3& pos = unit->GetUnit()->GetPos();
	if (position.SqDistance2D(pos) < range * range) {
		manager->DoneTask(this);
	} else {
		IBuilderTask::OnUnitIdle(unit);
	}
}

} // namespace circuit
