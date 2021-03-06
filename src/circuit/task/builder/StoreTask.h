/*
 * StoreTask.h
 *
 *  Created on: Jan 30, 2015
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_TASK_BUILDER_STORETASK_H_
#define SRC_CIRCUIT_TASK_BUILDER_STORETASK_H_

#include "task/builder/BuilderTask.h"

namespace circuit {

class CBStoreTask: public IBuilderTask {
public:
	CBStoreTask(ITaskManager* mgr, Priority priority,
				CCircuitDef* buildDef, const springai::AIFloat3& position,
				float cost, bool isShake, int timeout);
	virtual ~CBStoreTask();
};

} // namespace circuit

#endif // SRC_CIRCUIT_TASK_BUILDER_STORETASK_H_
