/*
 * Defines.h
 *
 *  Created on: Sep 18, 2015
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_UTIL_DEFINES_H_
#define SRC_CIRCUIT_UTIL_DEFINES_H_

#include "Sim/Misc/GlobalConstants.h"
#include "AIFloat3.h"

#include <vector>

#define FRAMES_PER_SEC		GAME_SPEED
#define WATCHDOG_COUNT		2
#define PYLON_RANGE			500.0f
#define TASK_RETRIES		10
#define DEBUG_MARK			0xBAD0C0DE
#define THREAT_RES			8
#define MIN_THREAT			20.0f

// z++
#define UNIT_FACING_SOUTH	0
// x++
#define UNIT_FACING_EAST	1
// z--
#define UNIT_FACING_NORTH	2
// x--
#define UNIT_FACING_WEST	3

typedef std::vector<springai::AIFloat3> F3Vec;

#endif // SRC_CIRCUIT_UTIL_DEFINES_H_
