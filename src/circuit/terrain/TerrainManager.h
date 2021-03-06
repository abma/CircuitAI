/*
 * TerrainManager.h
 *
 *  Created on: Dec 6, 2014
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_TERRAIN_TERRAINMANAGER_H_
#define SRC_CIRCUIT_TERRAIN_TERRAINMANAGER_H_

#include "terrain/BlockingMap.h"
#include "unit/CircuitUnit.h"
#include "unit/CircuitDef.h"

#include "AIFloat3.h"

#include <unordered_map>
#include <deque>
#include <functional>

namespace circuit {

class CCircuitAI;
class IBlockMask;
class CTerrainData;
struct STerrainMapArea;
struct STerrainMapMobileType;
struct STerrainMapImmobileType;
struct STerrainMapAreaSector;
struct STerrainMapSector;
struct SAreaData;

class CTerrainManager {  // <=> RAI's cBuilderPlacement
public:
	using TerrainPredicate = std::function<bool (const springai::AIFloat3& p)>;

	CTerrainManager(CCircuitAI* circuit, CTerrainData* terrainData);
	virtual ~CTerrainManager();
private:
	CCircuitAI* circuit;

public:
	int GetTerrainWidth() const { return terrainData->terrainWidth; }
	int GetTerrainHeight() const { return terrainData->terrainHeight; }

public:
	void AddBlocker(CCircuitDef* cdef, const springai::AIFloat3& pos, int facing);
	void RemoveBlocker(CCircuitDef* cdef, const springai::AIFloat3& pos, int facing);
	void ResetBuildFrame();
	// TODO: Use IsInBounds test and Bound operation only if mask or search offsets (endr) are out of bounds
	// TODO: Based on map complexity use BFS or circle to calculate build offset
	// TODO: Consider abstract task position (any area with builder) and task for certain unit-pos-area
	springai::AIFloat3 FindBuildSite(CCircuitDef* cdef,
									 const springai::AIFloat3& pos,
									 float searchRadius,
									 int facing);
	springai::AIFloat3 FindBuildSite(CCircuitDef* cdef,
									 const springai::AIFloat3& pos,
									 float searchRadius,
									 int facing,
									 TerrainPredicate& predicate);

	const SBlockingMap& GetBlockingMap();

private:
	int markFrame;
	struct SStructure {
		CCircuitUnit::Id unitId;
		CCircuitDef* cdef;
		springai::AIFloat3 pos;
		int facing;
	};
	std::deque<SStructure> markedAllies;  // sorted by insertion
	void MarkAllyBuildings();

	struct SSearchOffset {
		int dx, dy;
		int qdist;  // dx*dx + dy*dy
	};
	using SearchOffsets = std::vector<SSearchOffset>;
	struct SSearchOffsetLow {
		SearchOffsets ofs;
		int dx, dy;
		int qdist;  // dx*dx + dy*dy
	};
	using SearchOffsetsLow = std::vector<SSearchOffsetLow>;
	static const SearchOffsets& GetSearchOffsetTable(int radius);
	static const SearchOffsetsLow& GetSearchOffsetTableLow(int radius);
	springai::AIFloat3 FindBuildSiteLow(CCircuitDef* cdef,
										const springai::AIFloat3& pos,
										float searchRadius,
										int facing,
										TerrainPredicate& predicate);
	springai::AIFloat3 FindBuildSiteByMask(CCircuitDef* cdef,
										   const springai::AIFloat3& pos,
										   float searchRadius,
										   int facing,
										   IBlockMask* mask,
										   TerrainPredicate& predicate);
	// NOTE: Low-resolution build site is 40-80% faster on fail and 20-50% faster on success (with large objects). But has lower precision.
	springai::AIFloat3 FindBuildSiteByMaskLow(CCircuitDef* cdef,
											  const springai::AIFloat3& pos,
											  float searchRadius,
											  int facing,
											  IBlockMask* mask,
											  TerrainPredicate& predicate);

	SBlockingMap blockingMap;
	std::unordered_map<CCircuitDef::Id, IBlockMask*> blockInfos;  // owner
	void MarkBlockerByMask(const SStructure& building, bool block, IBlockMask* mask);
	void MarkBlocker(const SStructure& building, bool block);

public:
	int GetConvertStoP() const { return terrainData->convertStoP; }
	void CorrectPosition(springai::AIFloat3& position) { terrainData->CorrectPosition(position); }
	STerrainMapArea* GetCurrentMapArea(CCircuitDef* cdef, const springai::AIFloat3& position);
	int GetSectorIndex(const springai::AIFloat3& position) const { return terrainData->GetSectorIndex(position); }
	bool CanMoveToPos(STerrainMapArea* area, const springai::AIFloat3& destination);
	springai::AIFloat3 GetBuildPosition(CCircuitDef* cdef, const springai::AIFloat3& position);
private:
	std::vector<STerrainMapAreaSector>& GetSectorList(STerrainMapArea* sourceArea = nullptr);
	STerrainMapAreaSector* GetClosestSector(STerrainMapArea* sourceArea, const int destinationSIndex);
	STerrainMapSector* GetClosestSector(STerrainMapImmobileType* sourceIT, const int destinationSIndex);
	STerrainMapAreaSector* GetAlternativeSector(STerrainMapArea* sourceArea, const int sourceSIndex, STerrainMapMobileType* destinationMT);
	STerrainMapSector* GetAlternativeSector(STerrainMapArea* destinationArea, const int sourceSIndex, STerrainMapImmobileType* destinationIT); // can return 0
	const STerrainMapSector& GetSector(int sIndex) const { return areaData->sector[sIndex]; }
public:
	const std::vector<STerrainMapMobileType>& GetMobileTypes() const {
		return areaData->mobileType;
	}
	STerrainMapMobileType* GetMobileType(CCircuitDef::Id unitDefId) const {
		return GetMobileTypeById(terrainData->udMobileType[unitDefId]);
	}
	STerrainMapMobileType::Id GetMobileTypeId(CCircuitDef::Id unitDefId) const {
		return terrainData->udMobileType[unitDefId];
	}
	STerrainMapMobileType* GetMobileTypeById(STerrainMapMobileType::Id id) const {
		return (id < 0) ? nullptr : &areaData->mobileType[id];
	}
	const std::vector<STerrainMapImmobileType>& GetImmobileTypes() const {
		return areaData->immobileType;
	}
	STerrainMapImmobileType* GetImmobileType(CCircuitDef::Id unitDefId) const {
		return GetImmobileTypeById(terrainData->udImmobileType[unitDefId]);
	}
	STerrainMapImmobileType::Id GetImmobileTypeId(CCircuitDef::Id unitDefId) const {
		return terrainData->udMobileType[unitDefId];
	}
	STerrainMapImmobileType* GetImmobileTypeById(STerrainMapImmobileType::Id id) const {
		return (id < 0) ? nullptr : &areaData->immobileType[id];
	}

	// position must be valid
	bool CanBeBuiltAt(CCircuitDef* cdef, const springai::AIFloat3& position, const float& range = .0);  // NOTE: returns false if the area was too small to be recorded
	bool CanBuildAt(CCircuitUnit* unit, const springai::AIFloat3& destination);
	bool CanMobileBuildAt(STerrainMapArea* area, CCircuitDef* builderDef, const springai::AIFloat3& destination);

	float GetPercentLand() const { return areaData->percentLand; }
	bool IsWaterSector(const springai::AIFloat3& position) const {
		return areaData->sector[GetSectorIndex(position)].isWater;
	}

	void UpdateAreaUsers();
	void DidUpdateAreaUsers() { terrainData->DidUpdateAreaUsers(); }
private:
	SAreaData* areaData;

//public:
//	void ClusterizeTerrain();
//	const std::vector<springai::AIFloat3>& GetDefencePoints() const;
//	const std::vector<springai::AIFloat3>& GetDefencePerimeter() const;
private:
	CTerrainData* terrainData;

#ifdef DEBUG_VIS
private:
	int dbgTextureId;
	uint32_t sdlWindowId;
	float* dbgMap;
	void UpdateVis();
public:
	void ToggleVis();
#endif
};

} // namespace circuit

#endif // SRC_CIRCUIT_TERRAIN_TERRAINMANAGER_H_
