#include "levels/drlg_sotw1.h"

#include "engine/load_file.hpp"
#include "engine/point.hpp"
#include "engine/random.hpp"
#include "engine/rectangle.hpp"
#include "levels/gendung.h"
#include "player.h"
#include "quests.h"
#include "utils/bitset2d.hpp"

namespace devilution {

namespace {

/** Marks where walls may not be added to the level */
Bitset2d<DMAXX, DMAXY> Chamber;

/** Miniset: stairs up on a corner wall. */
const Miniset STAIRSUP {
	{ 5, 3 },
	{
	    { 19, 1, 7, 7, 7 },
	    { 19, 1, 7, 7, 7 },
	    { 19, 1, 7, 7, 7 },
	},
	{
	    { 0, 16, 0, 0, 0 },
	    { 0, 15, 0, 0, 0 },
	    { 0, 5, 0, 0, 0 },
	}
};
/** Miniset: stairs down. */
const Miniset STAIRSDOWN {
	{ 3, 4 },
	{
	    { 7, 7, 7 },
	    { 7, 7, 7 },
	    { 7, 7, 7 },
	    { 7, 7, 7 },
	},
	{
	    { 0, 0, 0 },
	    { 27, 28, 0 },
	    { 26, 25, 0 },
	    { 0, 0, 0 },
	}
};

enum Tile : uint8_t {
	// clang-format off
	VWall          =  1,
	HWall          =  2,
	Corner         =  3,
	DWall          =  4,
	VWallEnd       =  5,
	HWallEnd       =  6,
	Floor          =  7,
	Pillar         =  8,
	VDoor          =  9,
	HDoor          = 10,
	VFence         = 11,
	HFence         = 12,
	HWallVFence    = 13,
	HFenceVWall    = 14,
	EntranceStairs = 15,
	DirtHwall      = 17,
	DirtVwall      = 18,
	Dirt           = 19,
	HDirtCorner    = 20,
	DirtHwallEnd   = 21,
	DirtVwallEnd   = 22,
	HCorner        = 29,
	VCorner        = 30,
	Floor2         =  7,
	Floor3         =  7,
	// clang-format on
};

void FillFloor()
{
	for (int j = 0; j < DMAXY; j++) {
		for (int i = 0; i < DMAXX; i++) {
			if (dungeon[i][j] != Floor || Protected.test(i, j))
				continue;

			int rv = GenerateRnd(3);
			if (rv == 1)
				dungeon[i][j] = Floor2;
			else if (rv == 2)
				dungeon[i][j] = Floor3;
		}
	}
}

void InitDungeonPieces()
{
	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++) {
			int8_t pc;
			if (dPiece[i][j] == 9) {
				pc = 1;
			} else if (dPiece[i][j] == 6) {
				pc = 2;
			} else {
				continue;
			}
			dSpecial[i][j] = pc;
		}
	}
}

void InitDungeonFlags()
{
	memset(dungeon, Dirt, sizeof(dungeon));
	Protected.reset();
	Chamber.reset();
}

void MapRoom(Rectangle room)
{
	for (int y = 0; y < room.size.height; y++) {
		for (int x = 0; x < room.size.width; x++) {
			DungeonMask.set(room.position.x + x, room.position.y + y);
		}
	}
}

bool CheckRoom(Rectangle room)
{
	for (int j = 0; j < room.size.height; j++) {
		for (int i = 0; i < room.size.width; i++) {
			if (i + room.position.x < 0 || i + room.position.x >= DMAXX || j + room.position.y < 0 || j + room.position.y >= DMAXY) {
				return false;
			}
			if (DungeonMask.test(i + room.position.x, j + room.position.y)) {
				return false;
			}
		}
	}

	return true;
}

void GenerateRoom(Rectangle area, bool verticalLayout)
{
	bool rotate = FlipCoin(4);
	verticalLayout = (!verticalLayout && rotate) || (verticalLayout && !rotate);

	bool placeRoom1;
	Rectangle room1;

	for (int num = 0; num < 20; num++) {
		const int32_t randomWidth = (GenerateRnd(5) + 2) & ~1;
		const int32_t randomHeight = (GenerateRnd(5) + 2) & ~1;
		room1.size = { randomWidth, randomHeight };
		room1.position = area.position;
		if (verticalLayout) {
			room1.position += Displacement { -room1.size.width, area.size.height / 2 - room1.size.height / 2 };
			placeRoom1 = CheckRoom({ room1.position + Displacement { -1, -1 }, { room1.size.height + 1, room1.size.width + 2 } });
		} else {
			room1.position += Displacement { area.size.width / 2 - room1.size.width / 2, -room1.size.height };
			placeRoom1 = CheckRoom({ room1.position + Displacement { -1, -1 }, { room1.size.width + 2, room1.size.height + 1 } });
		}
		if (placeRoom1)
			break;
	}

	if (placeRoom1)
		MapRoom(room1);

	bool placeRoom2;
	Rectangle room2 = room1;
	if (verticalLayout) {
		room2.position.x = area.position.x + area.size.width;
		placeRoom2 = CheckRoom({ room2.position + Displacement { 0, -1 }, { room2.size.width + 1, room2.size.height + 2 } });
	} else {
		room2.position.y = area.position.y + area.size.height;
		placeRoom2 = CheckRoom({ room2.position + Displacement { -1, 0 }, { room2.size.width + 2, room2.size.height + 1 } });
	}

	if (placeRoom2)
		MapRoom(room2);
	if (placeRoom1)
		GenerateRoom(room1, !verticalLayout);
	if (placeRoom2)
		GenerateRoom(room2, !verticalLayout);
}

/**
 * @brief Generate a boolean dungoen room layout
 */
void FirstRoom()
{
	DungeonMask.reset();

	Rectangle chamber {
		{
		    GenerateRnd(28) + 1,
		    GenerateRnd(28) + 1,
		},
		{
		    GenerateRnd(7) + 3,
		    GenerateRnd(7) + 3,
		}
	};

	GenerateRoom(chamber, FlipCoin());
}

/**
 * @brief Find the number of mega tiles used by layout
 */
inline size_t FindArea()
{
	return DungeonMask.count();
}

void MakeDmt()
{
	for (int j = 0; j < DMAXY - 1; j++) {
		for (int i = 0; i < DMAXX - 1; i++) {
			if (DungeonMask.test(i, j))
				dungeon[i][j] = Floor;
			else if (!DungeonMask.test(i + 1, j + 1) && DungeonMask.test(i, j + 1) && DungeonMask.test(i + 1, j))
				dungeon[i][j] = Floor; // Remove diagonal corners
			else if (DungeonMask.test(i + 1, j + 1) && DungeonMask.test(i, j + 1) && DungeonMask.test(i + 1, j))
				dungeon[i][j] = VCorner;
			else if (DungeonMask.test(i, j + 1))
				dungeon[i][j] = HWall;
			else if (DungeonMask.test(i + 1, j))
				dungeon[i][j] = VWall;
			else if (DungeonMask.test(i + 1, j + 1))
				dungeon[i][j] = DWall;
			else
				dungeon[i][j] = Dirt;
		}
	}
}

int HorizontalWallOk(Point position)
{
	int length;
	for (length = 1; dungeon[position.x + length][position.y] == Floor; length++) {
		if (dungeon[position.x + length][position.y - 1] != Floor || dungeon[position.x + length][position.y + 1] != Floor || Protected.test(position.x + length, position.y) || Chamber.test(position.x + length, position.y))
			break;
	}

	if (length == 1)
		return -1;

	auto tileId = static_cast<Tile>(dungeon[position.x + length][position.y]);

	if (!IsAnyOf(tileId, Corner, DWall, VWallEnd, HWallEnd, VCorner, HCorner, DirtHwall, DirtVwall, HDirtCorner, DirtHwallEnd, DirtVwallEnd))
		return -1;

	return length;
}

int VerticalWallOk(Point position)
{
	int length;
	for (length = 1; dungeon[position.x][position.y + length] == Floor; length++) {
		if (dungeon[position.x - 1][position.y + length] != Floor || dungeon[position.x + 1][position.y + length] != Floor || Protected.test(position.x, position.y + length) || Chamber.test(position.x, position.y + length))
			break;
	}

	if (length == 1)
		return -1;

	auto tileId = static_cast<Tile>(dungeon[position.x][position.y + length]);

	if (!IsAnyOf(tileId, Corner, DWall, VWallEnd, HWallEnd, VCorner, HCorner, DirtHwall, DirtVwall, HDirtCorner, DirtHwallEnd, DirtVwallEnd))
		return -1;

	return length;
}

void HorizontalWall(Point position, Tile start, int maxX)
{
	Tile wallTile = HWall;
	Tile doorTile = HDoor;

	if (FlipCoin(3)) { // Add Fence
		wallTile = HFence;
		if (start == HWall)
			start = HFence;
		else if (start == DWall)
			start = HFenceVWall;
	}

	dungeon[position.x][position.y] = start;

	for (int x = 1; x < maxX; x++) {
		dungeon[position.x + x][position.y] = wallTile;
	}

	int x = GenerateRnd(maxX - 1) + 1;

	dungeon[position.x + x][position.y] = doorTile;
	if (doorTile == HDoor) {
		Protected.set(position.x + x, position.y);
	}
}

void VerticalWall(Point position, Tile start, int maxY)
{
	Tile wallTile = VWall;
	Tile doorTile = VDoor;

	if (FlipCoin(3)) { // Add Fence
		wallTile = VFence;
		if (start == VWall)
			start = VFence;
		else if (start == DWall)
			start = HWallVFence;
	}

	dungeon[position.x][position.y] = start;

	for (int y = 1; y < maxY; y++) {
		dungeon[position.x][position.y + y] = wallTile;
	}

	int y = GenerateRnd(maxY - 1) + 1;

	dungeon[position.x][position.y + y] = doorTile;
	if (doorTile == VDoor) {
		Protected.set(position.x, position.y + y);
	}
}

void AddWall()
{
	for (int j = 0; j < DMAXY; j++) {
		for (int i = 0; i < DMAXX; i++) {
			if (Protected.test(i, j) || Chamber.test(i, j))
				continue;

			if (dungeon[i][j] == Corner) {
				AdvanceRndSeed();
				int maxX = HorizontalWallOk({ i, j });
				if (maxX != -1) {
					HorizontalWall({ i, j }, HWall, maxX);
				}
			}
			if (dungeon[i][j] == Corner) {
				AdvanceRndSeed();
				int maxY = VerticalWallOk({ i, j });
				if (maxY != -1) {
					VerticalWall({ i, j }, VWall, maxY);
				}
			}
			if (dungeon[i][j] == VWallEnd) {
				AdvanceRndSeed();
				int maxX = HorizontalWallOk({ i, j });
				if (maxX != -1) {
					HorizontalWall({ i, j }, DWall, maxX);
				}
			}
			if (dungeon[i][j] == HWallEnd) {
				AdvanceRndSeed();
				int maxY = VerticalWallOk({ i, j });
				if (maxY != -1) {
					VerticalWall({ i, j }, DWall, maxY);
				}
			}
			if (dungeon[i][j] == HWall) {
				AdvanceRndSeed();
				int maxX = HorizontalWallOk({ i, j });
				if (maxX != -1) {
					HorizontalWall({ i, j }, HWall, maxX);
				}
			}
			if (dungeon[i][j] == VWall) {
				AdvanceRndSeed();
				int maxY = VerticalWallOk({ i, j });
				if (maxY != -1) {
					VerticalWall({ i, j }, VWall, maxY);
				}
			}
		}
	}
}

void FixTilesPatterns()
{
	// BUGFIX: Bounds checks are required in all loop bodies.
	// See https://github.com/diasurgical/devilutionX/pull/401

	for (int j = 0; j < DMAXY; j++) {
		for (int i = 0; i < DMAXX; i++) {
			if (i + 1 < DMAXX) {
				if (dungeon[i][j] == HWall && dungeon[i + 1][j] == Dirt)
					dungeon[i + 1][j] = DirtHwallEnd;
				if (dungeon[i][j] == Floor && dungeon[i + 1][j] == Dirt)
					dungeon[i + 1][j] = DirtHwall;
				if (dungeon[i][j] == Floor && dungeon[i + 1][j] == HWall)
					dungeon[i + 1][j] = HWallEnd;
				if (dungeon[i][j] == VWallEnd && dungeon[i + 1][j] == Dirt)
					dungeon[i + 1][j] = DirtVwallEnd;
			}
			if (j + 1 < DMAXY) {
				if (dungeon[i][j] == VWall && dungeon[i][j + 1] == Dirt)
					dungeon[i][j + 1] = DirtVwallEnd;
				if (dungeon[i][j] == Floor && dungeon[i][j + 1] == VWall)
					dungeon[i][j + 1] = VWallEnd;
				if (dungeon[i][j] == Floor && dungeon[i][j + 1] == Dirt)
					dungeon[i][j + 1] = DirtVwall;
			}
		}
	}

	for (int j = 0; j < DMAXY; j++) {
		for (int i = 0; i < DMAXX; i++) {
			if (i + 1 < DMAXX) {
				if (dungeon[i][j] == Floor && dungeon[i + 1][j] == DirtVwall)
					dungeon[i + 1][j] = HDirtCorner;
				if (dungeon[i][j] == HWallEnd && dungeon[i + 1][j] == Dirt)
					dungeon[i + 1][j] = DirtHwallEnd;
				if (dungeon[i][j] == Floor && dungeon[i + 1][j] == DirtVwallEnd)
					dungeon[i + 1][j] = HDirtCorner;
				if (dungeon[i][j] == HWall && dungeon[i + 1][j] == DirtVwall)
					dungeon[i + 1][j] = HDirtCorner;
				if (dungeon[i][j] == DirtVwall && dungeon[i + 1][j] == VWall)
					dungeon[i + 1][j] = VWallEnd;
				if (dungeon[i][j] == HWallEnd && dungeon[i + 1][j] == DirtVwall)
					dungeon[i + 1][j] = HDirtCorner;
				if (dungeon[i][j] == HWall && dungeon[i + 1][j] == VWall)
					dungeon[i + 1][j] = VWallEnd;
				if (dungeon[i][j] == Corner && dungeon[i + 1][j] == Dirt)
					dungeon[i + 1][j] = DirtVwallEnd;
				if (dungeon[i][j] == HDirtCorner && dungeon[i + 1][j] == VWall)
					dungeon[i + 1][j] = VWallEnd;
				if (dungeon[i][j] == HWallEnd && dungeon[i + 1][j] == VWall)
					dungeon[i + 1][j] = VWallEnd;
				if (dungeon[i][j] == HWallEnd && dungeon[i + 1][j] == DirtVwallEnd)
					dungeon[i + 1][j] = HDirtCorner;
				if (dungeon[i][j] == DWall && dungeon[i + 1][j] == VCorner)
					dungeon[i + 1][j] = HCorner;
				if (dungeon[i][j] == HWallEnd && dungeon[i + 1][j] == Floor)
					dungeon[i + 1][j] = HCorner;
				if (dungeon[i][j] == HWall && dungeon[i + 1][j] == DirtVwallEnd)
					dungeon[i + 1][j] = HDirtCorner;
				if (dungeon[i][j] == HWall && dungeon[i + 1][j] == Floor)
					dungeon[i + 1][j] = HCorner;
			}
			if (i > 0) {
				if (dungeon[i][j] == DirtHwallEnd && dungeon[i - 1][j] == Dirt)
					dungeon[i - 1][j] = DirtVwall;
				if (dungeon[i][j] == DirtVwall && dungeon[i - 1][j] == DirtHwallEnd)
					dungeon[i - 1][j] = HDirtCorner;
				if (dungeon[i][j] == VWallEnd && dungeon[i - 1][j] == Dirt)
					dungeon[i - 1][j] = DirtVwallEnd;
				if (dungeon[i][j] == VWallEnd && dungeon[i - 1][j] == DirtHwallEnd)
					dungeon[i - 1][j] = HDirtCorner;
			}
			if (j + 1 < DMAXY) {
				if (dungeon[i][j] == VWall && dungeon[i][j + 1] == HWall)
					dungeon[i][j + 1] = HWallEnd;
				if (dungeon[i][j] == VWallEnd && dungeon[i][j + 1] == DirtHwall)
					dungeon[i][j + 1] = HDirtCorner;
				if (dungeon[i][j] == DirtHwall && dungeon[i][j + 1] == HWall)
					dungeon[i][j + 1] = HWallEnd;
				if (dungeon[i][j] == VWallEnd && dungeon[i][j + 1] == HWall)
					dungeon[i][j + 1] = HWallEnd;
				if (dungeon[i][j] == HDirtCorner && dungeon[i][j + 1] == HWall)
					dungeon[i][j + 1] = HWallEnd;
				if (dungeon[i][j] == VWallEnd && dungeon[i][j + 1] == Dirt)
					dungeon[i][j + 1] = DirtVwallEnd;
				if (dungeon[i][j] == VWallEnd && dungeon[i][j + 1] == Floor)
					dungeon[i][j + 1] = VCorner;
				if (dungeon[i][j] == VWall && dungeon[i][j + 1] == Floor)
					dungeon[i][j + 1] = VCorner;
				if (dungeon[i][j] == Floor && dungeon[i][j + 1] == VCorner)
					dungeon[i][j + 1] = HCorner;
			}
			if (j > 0) {
				if (dungeon[i][j] == VWallEnd && dungeon[i][j - 1] == Dirt)
					dungeon[i][j - 1] = HWallEnd;
				if (dungeon[i][j] == VWallEnd && dungeon[i][j - 1] == Dirt)
					dungeon[i][j - 1] = DirtVwallEnd;
				if (dungeon[i][j] == HWallEnd && dungeon[i][j - 1] == DirtVwallEnd)
					dungeon[i][j - 1] = HDirtCorner;
				if (dungeon[i][j] == DirtHwall && dungeon[i][j - 1] == DirtVwallEnd)
					dungeon[i][j - 1] = HDirtCorner;
			}
		}
	}

	for (int j = 0; j < DMAXY; j++) {
		for (int i = 0; i < DMAXX; i++) {
			if (j + 1 < DMAXY && dungeon[i][j] == DWall && dungeon[i][j + 1] == HWall)
				dungeon[i][j + 1] = HWallEnd;
			if (i + 1 < DMAXX && dungeon[i][j] == HWall && dungeon[i + 1][j] == DirtVwall)
				dungeon[i + 1][j] = HDirtCorner;
		}
	}
}

void FixTransparency()
{
	int yy = 16;
	for (int j = 0; j < DMAXY; j++) {
		int xx = 16;
		for (int i = 0; i < DMAXX; i++) {
			if (dungeon[i][j] == DirtHwallEnd && j > 0 && dungeon[i][j - 1] == DirtHwall) {
				dTransVal[xx + 1][yy] = dTransVal[xx][yy];
				dTransVal[xx + 1][yy + 1] = dTransVal[xx][yy];
			}
			if (dungeon[i][j] == DirtVwallEnd && i + 1 < DMAXY && dungeon[i + 1][j] == DirtVwall) {
				dTransVal[xx][yy + 1] = dTransVal[xx][yy];
				dTransVal[xx + 1][yy + 1] = dTransVal[xx][yy];
			}
			if (dungeon[i][j] == DirtHwall) {
				dTransVal[xx + 1][yy] = dTransVal[xx][yy];
				dTransVal[xx + 1][yy + 1] = dTransVal[xx][yy];
			}
			if (dungeon[i][j] == DirtVwall) {
				dTransVal[xx][yy + 1] = dTransVal[xx][yy];
				dTransVal[xx + 1][yy + 1] = dTransVal[xx][yy];
			}
			if (dungeon[i][j] == Dirt) {
				dTransVal[xx + 1][yy] = dTransVal[xx][yy];
				dTransVal[xx][yy + 1] = dTransVal[xx][yy];
				dTransVal[xx + 1][yy + 1] = dTransVal[xx][yy];
			}
			xx += 2;
		}
		yy += 2;
	}
}

void FixCornerTiles()
{
	for (int j = 1; j < DMAXY - 1; j++) {
		for (int i = 1; i < DMAXX - 1; i++) {
			if (!Protected.test(i, j) && dungeon[i][j] == HCorner && dungeon[i - 1][j] == Floor && dungeon[i][j - 1] == VWall) {
				dungeon[i][j] = VCorner;
				Protected.set(i, j);
			}
		}
	}
}

bool PlaceStairs(lvl_entry entry)
{
	// Place stairs up
	std::optional<Point> stairsUp = PlaceMiniSet(STAIRSUP, DMAXX * DMAXY, false);
	if (!stairsUp) {
		return false;
	} else if (entry == ENTRY_MAIN || entry == ENTRY_TWARPDN) {
		ViewPosition = stairsUp->megaToWorld() + Displacement { 3, 3 };
	}

	// Prevent sairs from being within 30 tiles radious of each other
	SetPieceRoom = WorldTileRectangle { stairsUp->megaToWorld() + Displacement { 2, 3 }, 30 };

	// Place stairs down
	std::optional<Point> stairsDown = PlaceMiniSet(STAIRSDOWN, DMAXX * DMAXY, false);
	if (!stairsDown) {
		return false;
	} else if (entry == ENTRY_PREV) {
		ViewPosition = stairsDown->megaToWorld() + Displacement { 2, 2 };
	}

	SetPieceRoom = { { 0, 0 }, { 0, 0 } };

	return true;
}

void GenerateLevel(lvl_entry entry)
{
	size_t minarea = 650;

	while (true) {
		DRLG_InitTrans();

		do {
			FirstRoom();

		} while (FindArea() < minarea);

		InitDungeonFlags();
		MakeDmt();
		FixTilesPatterns();
		AddWall();
		FloodTransparencyValues(7);
		if (PlaceStairs(entry))
			break;
	}

	FreeQuestSetPieces();

	for (int j = 0; j < DMAXY; j++) {
		for (int i = 0; i < DMAXX; i++) {
			if (dungeon[i][j] == EntranceStairs) {
				int xx = 2 * i + 16; /* todo: fix loop */
				int yy = 2 * j + 16;
				DRLG_CopyTrans(xx, yy + 1, xx, yy);
				DRLG_CopyTrans(xx + 1, yy + 1, xx + 1, yy);
			}
		}
	}

	FixTransparency();
	FixCornerTiles();

	FillFloor();

	memcpy(pdungeon, dungeon, sizeof(pdungeon));

	// DRLG_CheckQuests(SetPiece.position);
}

void Pass3()
{
	DRLG_LPass3(Dirt - 1);

	InitDungeonPieces();
}

} // namespace

void CreateSOTW1Dungeon(uint32_t rseed, lvl_entry entry)
{
	SetRndSeed(rseed);

	GenerateLevel(entry);

	Pass3();
}

void LoadPreSOTW1Dungeon(const char *path)
{
	memset(dungeon, Dirt, sizeof(dungeon));

	auto dunData = LoadFileInMem<uint16_t>(path);
	PlaceDunTiles(dunData.get(), { 0, 0 }, Floor);

	FillFloor();

	memcpy(pdungeon, dungeon, sizeof(pdungeon));
}

void LoadSOTW1Dungeon(const char *path, Point spawn)
{
	LoadDungeonBase(path, spawn, Floor, Dirt);

	FillFloor();

	Pass3();

	AddLotusObjects(0, 0, MAXDUNX, MAXDUNY);
}

} // namespace devilution
