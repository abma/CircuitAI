/*
 * CircuitUnit.h
 *
 *  Created on: Sep 2, 2014
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_UNIT_CIRCUITUNIT_H_
#define SRC_CIRCUIT_UNIT_CIRCUITUNIT_H_

#include "util/ActionList.h"

#include "Unit.h"

namespace springai {
	class Weapon;
}

namespace circuit {

//#define CMD_RETREAT_ZONE		10001
//#define CMD_SETHAVEN			CMD_RETREAT_ZONE
#define CMD_MORPH				31210
#define CMD_MORPH_STOP			32210
#define CMD_PRIORITY			34220
#define CMD_MISC_PRIORITY		34221
#define CMD_RETREAT				34223
#define CMD_ONECLICK_WEAPON		35000
#define CMD_JUMP				38521
#define CMD_TERRAFORM_INTERNAL	39801

class CCircuitDef;
class IUnitTask;
class IUnitManager;
struct STerrainMapArea;

class CCircuitUnit: public CActionList {
public:
	using Id = int;

	CCircuitUnit(const CCircuitUnit& that) = delete;
	CCircuitUnit& operator=(const CCircuitUnit&) = delete;
	CCircuitUnit(springai::Unit* unit, CCircuitDef* cdef);
	virtual ~CCircuitUnit();

	Id GetId() const { return id; }
	springai::Unit* GetUnit() const { return unit; }
	CCircuitDef* GetCircuitDef() const { return circuitDef; }

	void SetTask(IUnitTask* task);
	IUnitTask* GetTask() const { return task; }
	int GetTaskFrame() const { return taskFrame; }

	void SetManager(IUnitManager* mgr) { manager = mgr; }
	IUnitManager* GetManager() const { return manager; }

	void SetArea(STerrainMapArea* area) { this->area = area; }
	STerrainMapArea* GetArea() const { return area; }

	bool IsMoveFailed(int frame);

	void ForceExecute() { isForceExecute = true; }
	bool IsForceExecute();

	springai::Weapon* GetDGun() const { return dgun; }
	void ManualFire(springai::Unit* enemy, int timeOut);
	bool IsDisarmed();
	float GetDPS();

	void Morph();
	void StopMorph();
	bool IsMorphing() const { return isMorphing; }

	bool operator==(const CCircuitUnit& rhs) { return id == rhs.id; }
	bool operator!=(const CCircuitUnit& rhs) { return id != rhs.id; }

private:
	Id id;
	springai::Unit* unit;  // owner
	CCircuitDef* circuitDef;  // TODO: Replace with CCircuitDef::Id?
	IUnitTask* task;
	int taskFrame;
	IUnitManager* manager;
	STerrainMapArea* area;  // = nullptr if a unit flies

	int moveFails;
	int failFrame;
	bool isForceExecute;  // TODO: Replace by CExecuteAction?

	springai::Weapon* dgun;
	springai::UnitRulesParam* disarmParam;
	bool isMorphing;
};

} // namespace circuit

#endif // SRC_CIRCUIT_UNIT_CIRCUITUNIT_H_
