/*
 * CircuitUnit.cpp
 *
 *  Created on: Sep 2, 2014
 *      Author: rlcevg
 */

#include "unit/CircuitUnit.h"
#include "unit/UnitManager.h"
#include "CircuitAI.h"
#include "util/utils.h"

#include "AISCommands.h"
#include "UnitRulesParam.h"
#include "Weapon.h"

namespace circuit {

using namespace springai;

CCircuitUnit::CCircuitUnit(Unit* unit, CCircuitDef* cdef)
		: id(unit->GetUnitId())
		, unit(unit)
		, circuitDef(cdef)
		, task(nullptr)
		, taskFrame(-1)
		, manager(nullptr)
		, area(nullptr)
		, moveFails(0)
		, failFrame(-1)
		, isForceExecute(false)
		, disarmParam(nullptr)
		, isMorphing(false)
{
	WeaponMount* wpMnt = circuitDef->GetDGunMount();
	dgun = (wpMnt == nullptr) ? nullptr : unit->GetWeapon(wpMnt);
}

CCircuitUnit::~CCircuitUnit()
{
	PRINT_DEBUG("Execute: %s\n", __PRETTY_FUNCTION__);
	delete unit;
	delete dgun;
	delete disarmParam;
}

void CCircuitUnit::SetTask(IUnitTask* task)
{
	this->task = task;
	taskFrame = manager->GetCircuit()->GetLastFrame();
}

bool CCircuitUnit::IsMoveFailed(int frame)
{
	if (frame - failFrame >= FRAMES_PER_SEC * 3) {
		moveFails = 0;
	}
	failFrame = frame;
	return ++moveFails > TASK_RETRIES * 2;
}

bool CCircuitUnit::IsForceExecute()
{
	bool result = isForceExecute;
	isForceExecute = false;
	return result;
}

void CCircuitUnit::ManualFire(Unit* enemy, int timeOut)
{
	if (circuitDef->IsManualFire()) {
		unit->DGun(enemy, UNIT_COMMAND_OPTION_ALT_KEY, timeOut);
	} else {
		unit->ExecuteCustomCommand(CMD_ONECLICK_WEAPON, {}, UNIT_COMMAND_OPTION_ALT_KEY, timeOut);
	}
}

bool CCircuitUnit::IsDisarmed()
{
	if (disarmParam == nullptr) {
		disarmParam = unit->GetUnitRulesParamByName("disarmed");
		if (disarmParam == nullptr) {
			return false;
		}
	}
	return disarmParam->GetValueFloat() > .0f;
}

float CCircuitUnit::GetDPS()
{
	float dps = circuitDef->GetDPS();
	if (dps < 0.1f) {
		return .0f;
	}
	if (unit->IsParalyzed() || IsDisarmed()) {
		return 1.0f;
	}
	// TODO: Mind the slow down: dps * WeaponDef->GetReload / Weapon->GetReloadTime;
	return dps;
}

void CCircuitUnit::Morph()
{
	isMorphing = true;
	unit->ExecuteCustomCommand(CMD_MORPH, {});
	unit->ExecuteCustomCommand(CMD_MISC_PRIORITY, {0.0f});
}

void CCircuitUnit::StopMorph()
{
	isMorphing = false;
	unit->ExecuteCustomCommand(CMD_MORPH_STOP, {});
	unit->ExecuteCustomCommand(CMD_MISC_PRIORITY, {1.0f});
}

} // namespace circuit
