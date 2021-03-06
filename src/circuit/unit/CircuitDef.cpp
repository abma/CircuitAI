/*
 * CircuitDef.cpp
 *
 *  Created on: Dec 9, 2014
 *      Author: rlcevg
 */

#include "unit/CircuitDef.h"
#include "CircuitAI.h"
#include "util/utils.h"

#include "WeaponMount.h"
#include "WeaponDef.h"
#include "Damage.h"

#include <regex>

namespace circuit {

using namespace springai;

CCircuitDef::CCircuitDef(CCircuitAI* circuit, UnitDef* def, std::unordered_set<Id>& buildOpts, Resource* res)
		: def(def)
		, buildOptions(buildOpts)
		, count(0)
		, buildCounts(0)
//		, dgunReload(-1)
		, dgunRange(.0f)
		, dgunMount(nullptr)
		, dps(.0f)
		, targetCategory(0)
		, immobileTypeId(-1)
		, mobileTypeId(-1)
{
	id = def->GetUnitDefId();

	buildDistance = def->GetBuildDistance();
	buildSpeed = def->GetBuildSpeed();

	float bestReload = std::numeric_limits<float>::max();
	float bestRange;
	WeaponMount* bestMount = nullptr;
	bool isTracks = false;
//	bool isWater = false;
	auto mounts = std::move(def->GetWeaponMounts());
	for (WeaponMount* mount : mounts) {
		WeaponDef* wd = mount->GetWeaponDef();
		const std::map<std::string, std::string>& customParams = wd->GetCustomParams();

		float scale = wd->IsParalyzer() ? 0.5f : 1.0f;

		float extraDmg = .0f;
		auto it = customParams.find("extra_damage");
		if (it != customParams.end()) {
			extraDmg += utils::string_to_float(it->second);
		}

		it = customParams.find("disarmdamageonly");
		if ((it != customParams.end()) && (utils::string_to_int(it->second) == 1)) {
			scale = 0.5f;
		}

		it = customParams.find("timeslow_onlyslow");
		if ((it != customParams.end()) && (utils::string_to_int(it->second) == 1)) {
			scale = 0.5f;
		}

		it = customParams.find("is_capture");
		if ((it != customParams.end()) && (utils::string_to_int(it->second) == 1)) {
			scale = 2.0f;
		}

		it = customParams.find("area_damage_dps");
		if (it != customParams.end()) {
			extraDmg += utils::string_to_float(it->second);
			it = customParams.find("area_damage_is_impulse");
			if ((it != customParams.end()) && (utils::string_to_int(it->second) == 1)) {
				scale = 0.02f;
			}
		}

		float reloadTime = wd->GetReload();
		if (extraDmg > 0.1f) {
			dps += extraDmg * wd->GetSalvoSize() / reloadTime * scale;
		}
		Damage* damage = wd->GetDamage();
		const std::vector<float>& damages = damage->GetTypes();
		delete damage;
		float ldps = .0f;
		for (float dmg : damages) {
			ldps += dmg;
		}
		dps += ldps * wd->GetSalvoSize() / damages.size() / reloadTime * scale;
		targetCategory |= mount->GetOnlyTargetCategory();

		isTracks |= (wd->GetProjectileSpeed() * FRAMES_PER_SEC >= 500.0f) || wd->IsTracks();
		std::string weaponType(wd->GetType());  // Instant-hit
		isTracks |= (weaponType == "BeamLaser") || (weaponType == "LightningCannon") || (weaponType == "Rifle");
//		isWater |= wd->IsWaterWeapon();

		float reload;
		if (wd->IsManualFire() && ((reload = wd->GetReload()) < bestReload)) {
			bestReload = reload;
			bestRange = wd->GetRange();
			delete bestMount;
			bestMount = mount;
		} else {
			delete mount;
		}
		delete wd;
	}

	isManualFire = def->CanManualFire();
	if (bestReload < std::numeric_limits<float>::max()) {
//		dgunReload = math::ceil(bestReload * FRAMES_PER_SEC)/* + FRAMES_PER_SEC*/;
		dgunRange = bestRange;
		dgunMount = bestMount;
	}

	noChaseCategory = def->GetNoChaseCategory();

	const std::map<std::string, std::string>& customParams = def->GetCustomParams();
	auto it = customParams.find("is_drone");
	if ((it != customParams.end()) && (utils::string_to_int(it->second) == 1)) {
		category = ~circuit->GetGoodCategory();
	} else {
		category = def->GetCategory();
	}

	it = customParams.find("midposoffset");
	if (it != customParams.end()) {
		const std::string& str = it->second;
		std::string::const_iterator start = str.begin();
		std::string::const_iterator end = str.end();
		std::regex pattern("(-?\\d+)");
		std::smatch section;
		int index = 0;
		while (std::regex_search(start, end, section, pattern)) {
			midPosOffset[index++] = utils::string_to_float(section[1]);
			start = section[0].second;
		}
	} else {
		midPosOffset = ZeroVector;
	}

	// NOTE: isTracks filters units with slow weapon (hermit, recluse, rocko)
	isAntiAir   = (targetCategory & circuit->GetAirCategory()) && isTracks;
	isAntiLand  = (targetCategory & circuit->GetLandCategory());
	isAntiWater = (targetCategory & circuit->GetWaterCategory())/* || isWater*/;

	isMobile = def->GetSpeed() > .0f;
	isAbleToFly = def->IsAbleToFly();
	isFloater = def->IsFloater();

	if (isMobile && (dps < 0.1f)) {  // mobile bombs
		WeaponDef* wd = def->GetDeathExplosion();
		if (wd->GetAreaOfEffect() > 80.0f) {
			Damage* damage = wd->GetDamage();
			const std::vector<float>& damages = damage->GetTypes();
			delete damage;
			float ldps = .0f;
			for (float dmg : damages) {
				ldps += dmg;
			}
			dps = ldps * wd->GetSalvoSize() / damages.size();
			targetCategory = wd->GetOnlyTargetCategory();
			if (~targetCategory == 0) {
				targetCategory = circuit->GetGoodCategory();
			}
		}
		delete wd;
	}

	losRadius = def->GetLosRadius() * circuit->GetLosConv();
	cost = def->GetCost(res);
}

CCircuitDef::~CCircuitDef()
{
	PRINT_DEBUG("Execute: %s\n", __PRETTY_FUNCTION__);
	delete def;
	delete dgunMount;
}

CCircuitDef& CCircuitDef::operator++()
{
	++count;
	return *this;
}

// FIXME: ~CCircuitDef should fail with delete
//CCircuitDef CCircuitDef::operator++(int)
//{
//	CCircuitDef temp = *this;
//	count++;
//	return temp;
//}

CCircuitDef& CCircuitDef::operator--()
{
	--count;
	return *this;
}

//CCircuitDef CCircuitDef::operator--(int)
//{
//	CCircuitDef temp = *this;
//	count--;
//	return temp;
//}

} // namespace circuit
