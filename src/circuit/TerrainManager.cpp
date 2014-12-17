/*
 * TerrainManager.cpp
 *
 *  Created on: Dec 6, 2014
 *      Author: rlcevg
 */

#include "CircuitAI.h"
#include "CircuitUnit.h"
#include "MetalManager.h"
#include "TerrainManager.h"
#include "BlockRectangle.h"
#include "BlockCircle.h"
#include "Scheduler.h"
#include "MetalManager.h"
#include "utils.h"

#include "Map.h"
#include "Unit.h"
#include "UnitDef.h"
#include "OOAICallback.h"
#include "WeaponDef.h"
#include "Pathing.h"
#include "MoveData.h"

namespace circuit {

using namespace springai;

CTerrainManager::CTerrainManager(CCircuitAI* circuit) :
		IModule(circuit),
		cacheBuildFrame(0)
{
	Map* map = circuit->GetMap();
	int mapWidth = map->GetWidth();
	int mapHeight = map->GetHeight();
	terrainWidth = mapWidth * SQUARE_SIZE;
	terrainHeight = mapHeight * SQUARE_SIZE;

	blockingMap.columns = mapWidth / 2;  // build-step = 2 little green squares
	blockingMap.rows = mapHeight / 2;
	SBlockingMap::BlockCell cell = {0};
	blockingMap.grid.resize(blockingMap.columns * blockingMap.rows, cell);
	blockingMap.columnsLow = mapWidth / (GRID_RATIO_LOW * 2);
	blockingMap.rowsLow = mapHeight / (GRID_RATIO_LOW * 2);
	SBlockingMap::BlockCellLow cellLow = {0};
	blockingMap.gridLow.resize(blockingMap.columnsLow * blockingMap.rowsLow, cellLow);

	const CMetalData::Metals& spots = circuit->GetMetalManager()->GetSpots();
	UnitDef* def = circuit->GetUnitDefByName("cormex");
	int size = std::max(def->GetXSize(), def->GetZSize()) / 2;
	int& xsize = size, &zsize = size;
	for (auto& spot : spots) {
		const int x1 = int(spot.position.x / (SQUARE_SIZE << 1)) - (xsize >> 1), x2 = x1 + xsize;
		const int z1 = int(spot.position.z / (SQUARE_SIZE << 1)) - (zsize >> 1), z2 = z1 + zsize;
		for (int z = z1; z < z2; z++) {
			for (int x = x1; x < x2; x++) {
				blockingMap.MarkBlocker(x, z, SBlockingMap::StructType::MEX);
			}
		}
	}

	/*
	 * building handlers
	 */
	auto buildingCreatedHandler = [this](CCircuitUnit* unit) {
		AddBlocker(unit);
	};
	auto buildingDestroyedHandler = [this](CCircuitUnit* unit) {
		RemoveBlocker(unit);
	};

	CCircuitAI::UnitDefs& defs = circuit->GetUnitDefs();
	for (auto& kv : defs) {
		UnitDef* def = kv.second;
		int unitDefId = def->GetUnitDefId();
		if (def->GetSpeed() == 0) {
			createdHandler[unitDefId] = buildingCreatedHandler;
			destroyedHandler[unitDefId] = buildingDestroyedHandler;
		}
	}
	// Forbid from removing mex blocker
	int unitDefId = def->GetUnitDefId();
	createdHandler.erase(unitDefId);
	destroyedHandler.erase(unitDefId);

	/*
	 * building masks
	 */
	WeaponDef* wpDef;
	int2 offset;
	int2 bsize;
	int2 ssize;
	int radius;
	int ignoreMask;
	def = circuit->GetUnitDefByName("factorycloak");
	ssize = int2(def->GetXSize() / 2, def->GetZSize() / 2);
	bsize = ssize + int2(6, 4);
	// offset in South facing
	offset = int2(0, 4);
	blockInfos[def] = new CBlockRectangle(offset, bsize, ssize, SBlockingMap::StructType::FACTORY);

	def = circuit->GetUnitDefByName("armsolar");
	ssize = int2(def->GetXSize() / 2, def->GetZSize() / 2);
	bsize = ssize;
	offset = int2(0, 0);
	ignoreMask = static_cast<int>(SBlockingMap::StructMask::MEX) | static_cast<int>(SBlockingMap::StructMask::DEF_LOW) | static_cast<int>(SBlockingMap::StructMask::PYLON) | static_cast<int>(SBlockingMap::StructMask::NANO);
	blockInfos[def] = new CBlockRectangle(offset, bsize, ssize, SBlockingMap::StructType::ENERGY, ignoreMask);

	def = circuit->GetUnitDefByName("armfus");
	wpDef = def->GetDeathExplosion();
	radius = wpDef->GetAreaOfEffect() / (SQUARE_SIZE * 2);
	delete wpDef;
	ssize = int2(def->GetXSize() / 2, def->GetZSize() / 2);
	offset = int2(0, 0);
	ignoreMask = static_cast<int>(SBlockingMap::StructMask::MEX) | static_cast<int>(SBlockingMap::StructMask::DEF_LOW) | static_cast<int>(SBlockingMap::StructMask::PYLON) | static_cast<int>(SBlockingMap::StructMask::NANO);
	blockInfos[def] = new CBlockCircle(offset, radius, ssize, SBlockingMap::StructType::ENERGY, ignoreMask);

	def = circuit->GetUnitDefByName("cafus");
	wpDef = def->GetDeathExplosion();
	radius = wpDef->GetAreaOfEffect() / (SQUARE_SIZE * 2);
	delete wpDef;
	ssize = int2(def->GetXSize() / 2, def->GetZSize() / 2);
	offset = int2(0, 0);
	ignoreMask = static_cast<int>(SBlockingMap::StructMask::MEX) | static_cast<int>(SBlockingMap::StructMask::DEF_LOW) | static_cast<int>(SBlockingMap::StructMask::PYLON) | static_cast<int>(SBlockingMap::StructMask::NANO);
	blockInfos[def] = new CBlockCircle(offset, radius, ssize, SBlockingMap::StructType::ENERGY, ignoreMask);

	def = circuit->GetUnitDefByName("armestor");
	ssize = int2(def->GetXSize() / 2, def->GetZSize() / 2);
	bsize = ssize;
	offset = int2(0, 0);
	ignoreMask = static_cast<int>(SBlockingMap::StructMask::ENERGY) | static_cast<int>(SBlockingMap::StructMask::NANO);
	blockInfos[def] = new CBlockRectangle(offset, bsize, ssize, SBlockingMap::StructType::PYLON, ignoreMask);

	def = circuit->GetUnitDefByName("cormex");
	ssize = int2(def->GetXSize() / 2, def->GetZSize() / 2);
	bsize = ssize;
	offset = int2(0, 0);
	ignoreMask = static_cast<int>(SBlockingMap::StructMask::ALL);
	blockInfos[def] = new CBlockRectangle(offset, bsize, ssize, SBlockingMap::StructType::MEX, ignoreMask);

	def = circuit->GetUnitDefByName("corrl");
	ssize = int2(def->GetXSize() / 2, def->GetZSize() / 2);
	bsize = ssize;
	offset = int2(0, 0);
	ignoreMask = static_cast<int>(SBlockingMap::StructMask::ENERGY) | static_cast<int>(SBlockingMap::StructMask::NANO);
	blockInfos[def] = new CBlockRectangle(offset, bsize, ssize, SBlockingMap::StructType::DEF_LOW, ignoreMask);

	def = circuit->GetUnitDefByName("armnanotc");
	radius = 160 / (SQUARE_SIZE * 2);  // 160 - Zero-K const
	ssize = int2(def->GetXSize() / 2, def->GetZSize() / 2);
	offset = int2(0, 0);
	ignoreMask = static_cast<int>(SBlockingMap::StructMask::MEX) | static_cast<int>(SBlockingMap::StructMask::DEF_LOW) | static_cast<int>(SBlockingMap::StructMask::ENERGY) | static_cast<int>(SBlockingMap::StructMask::PYLON);
	blockInfos[def] = new CBlockCircle(offset, radius, ssize, SBlockingMap::StructType::NANO, ignoreMask);
}

CTerrainManager::~CTerrainManager()
{
	PRINT_DEBUG("Execute: %s\n", __PRETTY_FUNCTION__);
	for (auto& kv : blockInfos) {
		delete kv.second;
	}
}

int CTerrainManager::UnitCreated(CCircuitUnit* unit, CCircuitUnit* builder)
{
	auto search = createdHandler.find(unit->GetDef()->GetUnitDefId());
	if (search != createdHandler.end()) {
		search->second(unit);
	}

	return 0; //signaling: OK
}

int CTerrainManager::UnitDestroyed(CCircuitUnit* unit, CCircuitUnit* attacker)
{
	auto search = destroyedHandler.find(unit->GetDef()->GetUnitDefId());
	if (search != destroyedHandler.end()) {
		search->second(unit);
	}

	return 0; //signaling: OK
}

int CTerrainManager::GetTerrainWidth()
{
	return terrainWidth;
}

int CTerrainManager::GetTerrainHeight()
{
	return terrainHeight;
}

AIFloat3 CTerrainManager::FindBuildSite(UnitDef* unitDef, const AIFloat3& pos, float searchRadius, int facing)
{
	// Mark ally buildings
	int lastFrame = circuit->GetLastFrame();
	if (cacheBuildFrame + FRAMES_PER_SEC < lastFrame) {
		MarkAllyBuildings();
		cacheBuildFrame = lastFrame;
	}

	auto search = blockInfos.find(unitDef);
	if (search != blockInfos.end()) {
		return FindBuildSiteByMask(unitDef, pos, searchRadius, facing, search->second);
	}

	if (searchRadius > SQUARE_SIZE * 2 * 100) {
		return FindBuildSiteLow(unitDef, pos, searchRadius, facing);
	}

	/*
	 * Default FindBuildSite
	 */
	const int xsize = (((facing & 1) == 0) ? unitDef->GetXSize() : unitDef->GetZSize()) / 2;
	const int zsize = (((facing & 1) == 1) ? unitDef->GetXSize() : unitDef->GetZSize()) / 2;

	auto isOpenSite = [this](const int2& s1, const int2& s2) {
		const int notIgnore = static_cast<int>(SBlockingMap::StructMask::ALL);
		for (int x = s1.x; x < s2.x; x++) {
			for (int z = s1.y; z < s2.y; z++) {
				if (blockingMap.IsBlocked(x, z, notIgnore)) {
					return false;
				}
			}
		}
		return true;
	};

	const int endr = (int)(searchRadius / (SQUARE_SIZE * 2));
	const std::vector<SearchOffset>& ofs = GetSearchOffsetTable(endr);

	const int cornerX1 = int(pos.x / (SQUARE_SIZE * 2)) - (xsize / 2);
	const int cornerZ1 = int(pos.z / (SQUARE_SIZE * 2)) - (zsize / 2);

	AIFloat3 probePos(ZeroVector);
	Map* map = circuit->GetMap();

	for (int so = 0; so < endr * endr * 4; so++) {
		int2 s1(cornerX1 + ofs[so].dx, cornerZ1 + ofs[so].dy);
		int2 s2(    s1.x + xsize,          s1.y + zsize);
		if (!blockingMap.IsInBounds(s1, s2) || !isOpenSite(s1, s2)) {
			continue;
		}

		probePos.x = (s1.x + s2.x) * SQUARE_SIZE;
		probePos.z = (s1.y + s2.y) * SQUARE_SIZE;
		if (map->IsPossibleToBuildAt(unitDef, probePos, facing)) {
			probePos.y = map->GetElevationAt(probePos.x, probePos.z);
			return probePos;
		}
	}

	return -RgtVector;
}

void CTerrainManager::MarkAllyBuildings()
{
//	const std::vector<CCircuitUnit*>& allies = circuit->GetAllyUnits();
//
//	std::vector<Unit*> units = circuit->GetCallback()->GetFriendlyUnits();
//	std::set<AllyBuilding, cmp_building> newUnits;
//	for (auto u : units) {
//		if (u->GetMaxSpeed() <= 0) {
//			int unitId = u->GetUnitId();
//			if (markedUnits.find(unitId) == markedUnits.end()) {
//				UnitDef* def = u->GetDef();
//				AllyBuilding building = {unitId, circuit->GetUnitDefById(def->GetUnitDefId()), u->GetPos()};
//				delete def;
//				newUnits.insert(building);
//				MarkBlocker(, true);
//			}
//		}
//	}
//	utils::free_clear(units);
}

const CTerrainManager::SearchOffsets& CTerrainManager::GetSearchOffsetTable(int radius)
{
	static std::vector<SearchOffset> searchOffsets;
	unsigned int size = radius * radius * 4;
	if (size > searchOffsets.size()) {
		searchOffsets.resize(size);

		for (int y = 0; y < radius * 2; y++) {
			for (int x = 0; x < radius * 2; x++) {
				SearchOffset& i = searchOffsets[y * radius * 2 + x];

				i.dx = x - radius;
				i.dy = y - radius;
				i.qdist = i.dx * i.dx + i.dy * i.dy;
			}
		}

		auto searchOffsetComparator = [](const SearchOffset& a, const SearchOffset& b) {
			return a.qdist < b.qdist;
		};
		std::sort(searchOffsets.begin(), searchOffsets.end(), searchOffsetComparator);
	}

	return searchOffsets;
}

const CTerrainManager::SearchOffsetsLow& CTerrainManager::GetSearchOffsetTableLow(int radius)
{
	static SearchOffsetsLow searchOffsetsLow;
	int radiusLow = radius / GRID_RATIO_LOW;
	unsigned int sizeLow = radiusLow * radiusLow * 4;
	if (sizeLow > searchOffsetsLow.size()) {
		searchOffsetsLow.resize(sizeLow);

		SearchOffsets searchOffsets;
		searchOffsets.resize(radius * radius * 4);
		for (int y = 0; y < radius * 2; y++) {
			for (int x = 0; x < radius * 2; x++) {
				SearchOffset& i = searchOffsets[y * radius * 2 + x];
				i.dx = x - radius;
				i.dy = y - radius;
				i.qdist = i.dx * i.dx + i.dy * i.dy;
			}
		}

		auto searchOffsetComparator = [](const SearchOffset& a, const SearchOffset& b) {
			return a.qdist < b.qdist;
		};
		for (int yl = 0; yl < radiusLow * 2; yl++) {
			for (int xl = 0; xl < radiusLow * 2; xl++) {
				SearchOffsetLow& il = searchOffsetsLow[yl * radiusLow * 2 + xl];
				il.dx = xl - radiusLow;
				il.dy = yl - radiusLow;
				il.qdist = il.dx * il.dx + il.dy * il.dy;

				il.ofs.reserve(GRID_RATIO_LOW * GRID_RATIO_LOW);
				const int xi = xl * GRID_RATIO_LOW;
				for (int y = yl * GRID_RATIO_LOW; y < (yl + 1) * GRID_RATIO_LOW; y++) {
					for (int x = xi; x < xi + GRID_RATIO_LOW; x++) {
						il.ofs.push_back(searchOffsets[y * radius * 2 + x]);
					}
				}

				std::sort(il.ofs.begin(), il.ofs.end(), searchOffsetComparator);
			}
		}

		auto searchOffsetLowComparator = [](const SearchOffsetLow& a, const SearchOffsetLow& b) {
			return a.qdist < b.qdist;
		};
		std::sort(searchOffsetsLow.begin(), searchOffsetsLow.end(), searchOffsetLowComparator);
	}

	return searchOffsetsLow;
}

AIFloat3 CTerrainManager::FindBuildSiteLow(UnitDef* unitDef, const AIFloat3& pos, float searchRadius, int facing)
{
	const int xsize = (((facing & 1) == 0) ? unitDef->GetXSize() : unitDef->GetZSize()) / 2;
	const int zsize = (((facing & 1) == 1) ? unitDef->GetXSize() : unitDef->GetZSize()) / 2;

	auto isOpenSite = [this](const int2& s1, const int2& s2) {
		const int notIgnore = static_cast<int>(SBlockingMap::StructMask::ALL);
		for (int x = s1.x; x < s2.x; x++) {
			for (int z = s1.y; z < s2.y; z++) {
				if (blockingMap.IsBlocked(x, z, notIgnore)) {
					return false;
				}
			}
		}
		return true;
	};

	const int endr = (int)(searchRadius / (SQUARE_SIZE * 2));
	const SearchOffsetsLow& ofsLow = GetSearchOffsetTableLow(endr);
	const int endrLow = endr / GRID_RATIO_LOW;

	const int centerX = int(pos.x / (SQUARE_SIZE * 2 * GRID_RATIO_LOW));
	const int centerZ = int(pos.z / (SQUARE_SIZE * 2 * GRID_RATIO_LOW));

	const int cornerX1 = int(pos.x / (SQUARE_SIZE * 2)) - (xsize / 2);
	const int cornerZ1 = int(pos.z / (SQUARE_SIZE * 2)) - (zsize / 2);

	const int notIgnore = static_cast<int>(SBlockingMap::StructMask::ALL);

	AIFloat3 probePos(ZeroVector);
	Map* map = circuit->GetMap();

	for (int soLow = 0; soLow < endrLow * endrLow * 4; soLow++) {
		int xlow = centerX + ofsLow[soLow].dx;
		int zlow = centerZ + ofsLow[soLow].dy;
		if (!blockingMap.IsInBoundsLow(xlow, zlow) || blockingMap.IsBlockedLow(xlow, zlow, notIgnore)) {
			continue;
		}

		const SearchOffsets& ofs = ofsLow[soLow].ofs;
		for (int so = 0; so < GRID_RATIO_LOW * GRID_RATIO_LOW; so++) {
			int2 s1(cornerX1 + ofs[so].dx, cornerZ1 + ofs[so].dy);
			int2 s2(    s1.x + xsize,          s1.y + zsize);
			if (!blockingMap.IsInBounds(s1, s2) || !isOpenSite(s1, s2)) {
				continue;
			}

			probePos.x = (s1.x + s2.x) * SQUARE_SIZE;
			probePos.z = (s1.y + s2.y) * SQUARE_SIZE;
			if (map->IsPossibleToBuildAt(unitDef, probePos, facing)) {
				probePos.y = map->GetElevationAt(probePos.x, probePos.z);
				return probePos;
			}
		}
	}

	return -RgtVector;
}

AIFloat3 CTerrainManager::FindBuildSiteByMask(UnitDef* unitDef, const AIFloat3& pos, float searchRadius, int facing, IBlockMask* mask)
{
	int xmsize = mask->GetXSize();
	int zmsize = mask->GetZSize();
	if ((searchRadius > GRID_RATIO_LOW * 2 * 100) || (xmsize * zmsize > GRID_RATIO_LOW * GRID_RATIO_LOW * 9)) {
		return FindBuildSiteByMaskLow(unitDef, pos, searchRadius, facing, mask);
	}

	int xssize, zssize;
	switch (facing) {
		default:
		case UNIT_FACING_SOUTH:
		case UNIT_FACING_NORTH: {
			xssize = unitDef->GetXSize() / 2;
			zssize = unitDef->GetZSize() / 2;
			break;
		}
		case UNIT_FACING_EAST:
		case UNIT_FACING_WEST: {
			xmsize = mask->GetZSize();
			zmsize = mask->GetXSize();
			xssize = unitDef->GetZSize() / 2;
			zssize = unitDef->GetXSize() / 2;
			break;
		}
	}

#define DECLARE_TEST(testName, facingType)													\
	auto testName = [this, mask, notIgnore, structMask](const int2& m1, const int2& m2) {	\
		for (int x = m1.x, xm = 0; x < m2.x; x++, xm++) {									\
			for (int z = m1.y, zm = 0; z < m2.y; z++, zm++) {								\
				switch (mask->facingType(xm, zm)) {											\
					case IBlockMask::BlockType::BLOCKED: {									\
						if (blockingMap.IsStruct(x, z, structMask)) {						\
							return false;													\
						}																	\
						break;																\
					}																		\
					case IBlockMask::BlockType::STRUCT: {									\
						if (blockingMap.IsBlocked(x, z, notIgnore)) {						\
							return false;													\
						}																	\
						break;																\
					}																		\
				}																			\
			}																				\
		}																					\
		return true;																		\
	};

	const int endr = (int)(searchRadius / (SQUARE_SIZE * 2));
	const SearchOffsets& ofs = GetSearchOffsetTable(endr);

	int2 structCorner;
	structCorner.x = int(pos.x / (SQUARE_SIZE * 2)) - (xssize / 2);
	structCorner.y = int(pos.z / (SQUARE_SIZE * 2)) - (zssize / 2);

	const int2& offset = mask->GetStructOffset(facing);
	int2 maskCorner = structCorner - offset;

	const int notIgnore = ~mask->GetIgnoreMask();
	SBlockingMap::StructMask structMask = SBlockingMap::GetStructMask(mask->GetStructType());

	AIFloat3 probePos(ZeroVector);
	Map* map = circuit->GetMap();

#define DO_TEST(testName)																	\
	for (int so = 0; so < endr * endr * 4; so++) {											\
		int2 s1(structCorner.x + ofs[so].dx, structCorner.y + ofs[so].dy);					\
		int2 s2(          s1.x + xssize,               s1.y + zssize);						\
		if (!blockingMap.IsInBounds(s1, s2)) {												\
			continue;																		\
		}																					\
																							\
		int2 m1(maskCorner.x + ofs[so].dx, maskCorner.y + ofs[so].dy);						\
		int2 m2(        m1.x + xmsize,             m1.y + zmsize);							\
		blockingMap.Bound(m1, m2);															\
		if (!testName(m1, m2)) {															\
			continue;																		\
		}																					\
																							\
		probePos.x = (s1.x + s2.x) * SQUARE_SIZE;											\
		probePos.z = (s1.y + s2.y) * SQUARE_SIZE;											\
		if (map->IsPossibleToBuildAt(unitDef, probePos, facing)) {							\
			probePos.y = map->GetElevationAt(probePos.x, probePos.z);						\
			return probePos;																\
		}																					\
	}

	switch (facing) {
		default:
		case UNIT_FACING_SOUTH: {
			DECLARE_TEST(isOpenSouth, GetTypeSouth);
			DO_TEST(isOpenSouth);
			break;
		}
		case UNIT_FACING_EAST: {
			DECLARE_TEST(isOpenEast, GetTypeEast);
			DO_TEST(isOpenEast);
			break;
		}
		case UNIT_FACING_NORTH: {
			DECLARE_TEST(isOpenNorth, GetTypeNorth);
			DO_TEST(isOpenNorth);
			break;
		}
		case UNIT_FACING_WEST: {
			DECLARE_TEST(isOpenWest, GetTypeWest);
			DO_TEST(isOpenWest);
			break;
		}
	}

	return -RgtVector;
}

AIFloat3 CTerrainManager::FindBuildSiteByMaskLow(UnitDef* unitDef, const AIFloat3& pos, float searchRadius, int facing, IBlockMask* mask)
{
	int xmsize, zmsize, xssize, zssize;
	switch (facing) {
		default:
		case UNIT_FACING_SOUTH:
		case UNIT_FACING_NORTH: {
			xmsize = mask->GetXSize();
			zmsize = mask->GetZSize();
			xssize = unitDef->GetXSize() / 2;
			zssize = unitDef->GetZSize() / 2;
			break;
		}
		case UNIT_FACING_EAST:
		case UNIT_FACING_WEST: {
			xmsize = mask->GetZSize();
			zmsize = mask->GetXSize();
			xssize = unitDef->GetZSize() / 2;
			zssize = unitDef->GetXSize() / 2;
			break;
		}
	}

#define DECLARE_TEST_LOW(testName, facingType)																	\
	auto testName = [this, mask, notIgnore, structMask](const int2& m1, const int2& m2) {						\
		for (int x = m1.x, xm = 0; x < m2.x; x++, xm++) {														\
			for (int z = m1.y, zm = 0; z < m2.y; z++, zm++) {													\
				switch (mask->facingType(xm, zm)) {																\
					case IBlockMask::BlockType::BLOCKED: {														\
						if (blockingMap.IsStruct(x, z, structMask)) {											\
							return false;																		\
						}																						\
						break;																					\
					}																							\
					case IBlockMask::BlockType::STRUCT: {														\
						if (blockingMap.IsBlocked(x, z, notIgnore)) {											\
							return false;																		\
						}																						\
						break;																					\
					}																							\
				}																								\
			}																									\
		}																										\
		return true;																							\
	};

	const int endr = (int)(searchRadius / (SQUARE_SIZE * 2));
	const SearchOffsetsLow& ofsLow = GetSearchOffsetTableLow(endr);
	const int endrLow = endr / GRID_RATIO_LOW;

	int2 structCorner;
	structCorner.x = int(pos.x / (SQUARE_SIZE * 2)) - (xssize / 2);
	structCorner.y = int(pos.z / (SQUARE_SIZE * 2)) - (zssize / 2);

	const int2& offset = mask->GetStructOffset(facing);
	int2 maskCorner = structCorner - offset;

	int2 structCenter;
	structCenter.x = int(pos.x / (SQUARE_SIZE * 2 * GRID_RATIO_LOW));
	structCenter.y = int(pos.x / (SQUARE_SIZE * 2 * GRID_RATIO_LOW));

	const int notIgnore = ~mask->GetIgnoreMask();
	SBlockingMap::StructMask structMask = SBlockingMap::GetStructMask(mask->GetStructType());

	AIFloat3 probePos(ZeroVector);
	Map* map = circuit->GetMap();

#define DO_TEST_LOW(testName)																					\
	for (int soLow = 0; soLow < endrLow * endrLow * 4; soLow++) {												\
		int2 low(structCenter.x + ofsLow[soLow].dx, structCenter.y + ofsLow[soLow].dy);							\
		if (!blockingMap.IsInBoundsLow(low.x, low.y) || blockingMap.IsBlockedLow(low.x, low.y, notIgnore)) {	\
			continue;																							\
		}																										\
																												\
		const SearchOffsets& ofs = ofsLow[soLow].ofs;															\
		for (int so = 0; so < GRID_RATIO_LOW * GRID_RATIO_LOW; so++) {											\
			int2 s1(structCorner.x + ofs[so].dx, structCorner.y + ofs[so].dy);									\
			int2 s2(          s1.x + xssize,               s1.y + zssize);										\
			if (!blockingMap.IsInBounds(s1, s2)) {																\
				continue;																						\
			}																									\
																												\
			int2 m1(maskCorner.x + ofs[so].dx, maskCorner.y + ofs[so].dy);										\
			int2 m2(        m1.x + xmsize,             m1.y + zmsize);											\
			blockingMap.Bound(m1, m2);																			\
			if (!testName(m1, m2)) {																			\
				continue;																						\
			}																									\
																												\
			probePos.x = (s1.x + s2.x) * SQUARE_SIZE;															\
			probePos.z = (s1.y + s2.y) * SQUARE_SIZE;															\
			if (map->IsPossibleToBuildAt(unitDef, probePos, facing)) {											\
				probePos.y = map->GetElevationAt(probePos.x, probePos.z);										\
				return probePos;																				\
			}																									\
		}																										\
	}

	switch (facing) {
		default:
		case UNIT_FACING_SOUTH: {
			DECLARE_TEST_LOW(isOpenSouth, GetTypeSouth);
			DO_TEST_LOW(isOpenSouth);
			break;
		}
		case UNIT_FACING_EAST: {
			DECLARE_TEST_LOW(isOpenEast, GetTypeEast);
			DO_TEST_LOW(isOpenEast);
			break;
		}
		case UNIT_FACING_NORTH: {
			DECLARE_TEST_LOW(isOpenNorth, GetTypeNorth);
			DO_TEST_LOW(isOpenNorth);
			break;
		}
		case UNIT_FACING_WEST: {
			DECLARE_TEST_LOW(isOpenWest, GetTypeWest);
			DO_TEST_LOW(isOpenWest);
			break;
		}
	}

	return -RgtVector;
}

void CTerrainManager::AddBlocker(CCircuitUnit* unit)
{
	MarkBlocker(unit, true);
}

void CTerrainManager::RemoveBlocker(CCircuitUnit* unit)
{
	MarkBlocker(unit, false);
}

void CTerrainManager::MarkBlockerByMask(CCircuitUnit* unit, bool block, IBlockMask* mask)
{
	Unit* u = unit->GetUnit();
	UnitDef* unitDef = unit->GetDef();
	int facing = u->GetBuildingFacing();
	const AIFloat3& pos = u->GetPos();

	int xmsize, zmsize, xssize, zssize;
	switch (facing) {
		default:
		case UNIT_FACING_SOUTH:
		case UNIT_FACING_NORTH: {
			xmsize = mask->GetXSize();
			zmsize = mask->GetZSize();
			xssize = unitDef->GetXSize() / 2;
			zssize = unitDef->GetZSize() / 2;
			break;
		}
		case UNIT_FACING_EAST:
		case UNIT_FACING_WEST: {
			xmsize = mask->GetZSize();
			zmsize = mask->GetXSize();
			xssize = unitDef->GetZSize() / 2;
			zssize = unitDef->GetXSize() / 2;
			break;
		}
	}

#define DECLARE_MARKER(typeName, blockerOp, structOp)					\
	for (int x = m1.x, xm = 0; x < m2.x; x++, xm++) {					\
		for (int z = m1.y, zm = 0; z < m2.y; z++, zm++) {				\
			switch (mask->typeName(xm, zm)) {							\
				case IBlockMask::BlockType::BLOCKED: {					\
					blockingMap.blockerOp(x, z, structType);			\
					break;												\
				}														\
				case IBlockMask::BlockType::STRUCT: {					\
					blockingMap.structOp(x, z, structType, notIgnore);	\
					break;												\
				}														\
			}															\
		}																\
	}

	int2 corner;
	corner.x = int(pos.x / (SQUARE_SIZE * 2)) - (xssize / 2);
	corner.y = int(pos.z / (SQUARE_SIZE * 2)) - (zssize / 2);

	int2 m1 = corner - mask->GetStructOffset(facing);
	int2 m2(m1.x + xmsize, m1.y + zmsize);
	blockingMap.Bound(m1, m2);

	const int notIgnore = ~mask->GetIgnoreMask();
	SBlockingMap::StructType structType = mask->GetStructType();

	Map* map = circuit->GetMap();

#define DO_MARK(facingType)												\
	if (block) {														\
		DECLARE_MARKER(facingType, AddBlocker, AddStruct);				\
	} else {															\
		DECLARE_MARKER(facingType, RemoveBlocker, RemoveStruct);		\
	}

	switch (facing) {
		default:
		case UNIT_FACING_SOUTH: {
			DO_MARK(GetTypeSouth);
			break;
		}
		case UNIT_FACING_EAST: {
			DO_MARK(GetTypeEast);
			break;
		}
		case UNIT_FACING_NORTH: {
			DO_MARK(GetTypeNorth);
			break;
		}
		case UNIT_FACING_WEST: {
			DO_MARK(GetTypeWest);
			break;
		}
	}
}

void CTerrainManager::MarkBlocker(CCircuitUnit* unit, bool block)
{
	UnitDef* unitDef = unit->GetDef();
	auto search = blockInfos.find(unitDef);
	if (search != blockInfos.end()) {
		MarkBlockerByMask(unit, block, search->second);
		return;
	}

	/*
	 * Default marker
	 */
	Unit* u = unit->GetUnit();
	int facing = u->GetBuildingFacing();
	const AIFloat3& pos = u->GetPos();

	const int xsize = (((facing & 1) == 0) ? unitDef->GetXSize() : unitDef->GetZSize()) / 2;
	const int zsize = (((facing & 1) == 1) ? unitDef->GetXSize() : unitDef->GetZSize()) / 2;

	const int x1 = int(pos.x / (SQUARE_SIZE * 2)) - (xsize / 2), x2 = x1 + xsize;
	const int z1 = int(pos.z / (SQUARE_SIZE * 2)) - (zsize / 2), z2 = z1 + zsize;

	const SBlockingMap::StructType structType = SBlockingMap::StructType::UNKNOWN;
	const int notIgnore = static_cast<int>(SBlockingMap::StructMask::ALL);

	if (block) {
		for (int z = z1; z < z2; z++) {
			for (int x = x1; x < x2; x++) {
				blockingMap.AddStruct(x, z, structType, notIgnore);
			}
		}
	} else {
		// NOTE: This can mess up things if unit is inside factory :/
		// SOLUTION: Do not mark movable units
		for (int z = z1; z < z2; z++) {
			for (int x = x1; x < x2; x++) {
				blockingMap.RemoveStruct(x, z, structType, notIgnore);
			}
		}
	}
}

void CTerrainManager::ClusterizeTerrain()
{
	PRINT_DEBUG("Execute: %s\n", __PRETTY_FUNCTION__);
	terrainData.SetClusterizing(true);

	// step 1: Create waypoints
	Pathing* pathing = circuit->GetPathing();
	Map* map = circuit->GetMap();
	const CMetalData::Metals& spots = circuit->GetMetalManager()->GetSpots();
	std::vector<AIFloat3> wayPoints;
	for (auto& spot : spots) {
		AIFloat3 start = spot.position;
		for (auto& s : spots) {
			if (spot.position == s.position) {
				continue;
			}
			AIFloat3 end = s.position;
			int pathId = pathing->InitPath(start, end, 4, .0f);
			AIFloat3 lastPoint, point(start);
			int j = 0;
			do {
				lastPoint = point;
				point = pathing->GetNextWaypoint(pathId);
				if (point.x <= 0 || point.z <= 0) {
					break;
				}
				if (j++ % 32 == 0) {
					wayPoints.push_back(point);
				}
			} while ((lastPoint != point) && (point.x > 0 && point.z > 0));
			pathing->FreePath(pathId);
		}
	}

	// step 2: Create path traversability map
	// @see path traversability map rts/
	int widthX = circuit->GetMap()->GetWidth();
	int heightZ = circuit->GetMap()->GetHeight();
	int widthSX = widthX / 2;
	MoveData* moveDef = circuit->GetUnitDefByName("armcom1")->GetMoveData();
	float maxSlope = moveDef->GetMaxSlope();
	float depth = moveDef->GetDepth();
	float slopeMod = moveDef->GetSlopeMod();
	const std::vector<float>& heightMap = circuit->GetMap()->GetHeightMap();
	const std::vector<float>& slopeMap = circuit->GetMap()->GetSlopeMap();

	std::vector<float> traversMap(widthX * heightZ);

	auto posSpeedMod = [](float maxSlope, float depth, float slopeMod, float depthMod, float height, float slope) {
		float speedMod = 0.0f;

		// slope too steep or square too deep?
		if (slope > maxSlope)
			return speedMod;
		if (-height > depth)
			return speedMod;

		// slope-mod
		speedMod = 1.0f / (1.0f + slope * slopeMod);
		// FIXME: Read waterDamageCost from mapInfo
//			speedMod *= (height < 0.0f) ? waterDamageCost : 1.0f;
		speedMod *= depthMod;

		return speedMod;
	};

	for (int hz = 0; hz < heightZ; ++hz) {
		for (int hx = 0; hx < widthX; ++hx) {
			const int sx = hx / 2;  // hx >> 1;
			const int sz = hz / 2;  // hz >> 1;
//				const bool losSqr = losHandler->InLos(sqx, sqy, gu->myAllyTeam);

			float scale = 1.0f;

			// TODO: First implement blocking map
//				if (los || losSqr) {
//					if (CMoveMath::IsBlocked(*md, sqx,     sqy    , NULL) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
//					if (CMoveMath::IsBlocked(*md, sqx + 1, sqy    , NULL) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
//					if (CMoveMath::IsBlocked(*md, sqx,     sqy + 1, NULL) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
//					if (CMoveMath::IsBlocked(*md, sqx + 1, sqy + 1, NULL) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
//				}

			int idx = hz * widthX + hx;
			float height = heightMap[idx];
			float slope = slopeMap[sz * widthSX + sx];
			float depthMod = moveDef->GetDepthMod(height);
			traversMap[idx] = posSpeedMod(maxSlope, depth, slopeMod, depthMod, height, slope);
			// FIXME: blocking map first
//				const SColor& smc = GetSpeedModColor(sm * scale);
		}
	}
	delete moveDef;

	// step 3: Filter key waypoints
	printf("points size: %i\n", wayPoints.size());
	auto iter = wayPoints.begin();
	while (iter != wayPoints.end()) {
		bool isKey = false;
		if ((iter->z / SQUARE_SIZE - 10 >= 0 && iter->z / SQUARE_SIZE + 10 < heightZ) && (iter->x / SQUARE_SIZE - 10 >= 0 && iter->x / SQUARE_SIZE + 10 < widthX)) {
			int idx = (int)(iter->z / SQUARE_SIZE) * widthX + (int)(iter->x / SQUARE_SIZE);
			if (traversMap[idx] > 0.8) {
				for (int hz = -10; hz <= 10; ++hz) {
					for (int hx = -10; hx <= 10; ++hx) {
						idx = (int)(iter->z / SQUARE_SIZE - hz) * widthX + iter->x / SQUARE_SIZE;
						if (traversMap[idx] < 0.8) {
							isKey = true;
							break;
						}
					}
					if (isKey) {
						break;
					}
				}
			}
		}
		if (!isKey) {
			iter = wayPoints.erase(iter);
		} else {
			++iter;
		}
	}

	// step 4: Clusterize key waypoints
	float maxDistance = circuit->GetUnitDefByName("cordoom")->GetMaxWeaponRange() * 2;
	maxDistance *= maxDistance;
	circuit->GetScheduler()->RunParallelTask(std::make_shared<CGameTask>(&CTerrainData::Clusterize, &terrainData, wayPoints, maxDistance, circuit));
}

const std::vector<springai::AIFloat3>& CTerrainManager::GetDefencePoints() const
{
	return terrainData.GetDefencePoints();
}

const std::vector<springai::AIFloat3>& CTerrainManager::GetDefencePerimeter() const
{
	return terrainData.GetDefencePerimeter();
}

} // namespace circuit