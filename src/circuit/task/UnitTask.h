/*
 * UnitTask.h
 *
 *  Created on: Sep 2, 2014
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_TASK_UNITTASK_H_
#define SRC_CIRCUIT_TASK_UNITTASK_H_

#include "AIFloat3.h"

#include <set>

namespace circuit {

class CCircuitUnit;
class CEnemyUnit;
class ITaskManager;

class IUnitTask {  // CSquad, IAction
public:
	enum class Priority: char {LOW = 0, NORMAL = 1, HIGH = 2};
	enum class Type: char {PLAYER, STUCK, IDLE, RETREAT, BUILDER, FACTORY, FIGHTER};

protected:
	IUnitTask(ITaskManager* mgr, Priority priority, Type type);
public:
	virtual ~IUnitTask();

	virtual bool CanAssignTo(CCircuitUnit* unit);
	virtual void AssignTo(CCircuitUnit* unit);
	virtual void RemoveAssignee(CCircuitUnit* unit);

	virtual void Execute(CCircuitUnit* unit) = 0;  // <=> IAction::OnStart()
	virtual void Update() = 0;
	virtual void Close(bool done);  // <=> IAction::OnEnd()
protected:
	// NOTE: Do not run time consuming code here. Instead create separate task.
	virtual void Finish();
	virtual void Cancel();  // TODO: Make pure virtual?

public:
	virtual void OnUnitIdle(CCircuitUnit* unit) = 0;
	virtual void OnUnitDamaged(CCircuitUnit* unit, CEnemyUnit* attacker) = 0;
	virtual void OnUnitDestroyed(CCircuitUnit* unit, CEnemyUnit* attacker) = 0;

	const std::set<CCircuitUnit*>& GetAssignees() const { return units; }
	Priority GetPriority() const { return priority; }
	Type GetType() const { return type; }
	ITaskManager* GetManager() const { return manager; }

protected:
	ITaskManager* manager;
	std::set<CCircuitUnit*> units;
	Priority priority;
	Type type;

	unsigned int updCount;
};

} // namespace circuit

#endif // SRC_CIRCUIT_TASK_UNITTASK_H_
