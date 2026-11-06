#include "ddini.h"
#include "resfile.h"
#include "fastdraw.h"
#include "mgraph.h"
#include "mouse.h"
#include "menu.h"
#include "mapdiscr.h"
#include "activescenary.h"
#include "fog.h"
#include "megapolis.h"

#include <assert.h>
#include "walls.h"
#include "mode.h"
#include "gsound.h"
#include "mapsprites.h"
#include "newmon.h"
#include "math.h"
#include "gp_draw.h"
#include "realwater.h"
#include "newupgrade.h"
#include "zbuffer.h"
#include "path.h"
#include "transport.h"
#include "3dbars.h"
#include "cdirsnd.h"
#include "newai.h"
#include "3dmaped.h"
#include "topograf.h"

#include "fonts.h"
#include "safety.h"
#include "3dgraph.h"
#include "nature.h"
#include "conststr.h"
// ---------------------------------------------------------------------------
// "SafeNet": area-level strategic routing for AI raids (diversions).
//
// The map is subdivided into NAreas walkable areas (the TopMap graph, see
// topograf.cpp). For each enemy nation this builds two per-area weights:
//   Danger - enemy combat strength standing in the area
//            (a share also bleeds into adjacent areas)
//   Pretty - "attractive target" value: enemy buildings worth capturing
//            (diversionists) or unfinished walls / capturables (grenadiers)
// The three GetNext*Cell functions are the entry points used by
// megapolis.cpp when a brigade must pick its next area while crossing
// hostile territory; fine-grained unit pathing to the chosen area is done
// by the usual motion code, not here.
// They return the next area id to move into, or 0xFFFF when no safe
// route exists (the caller keeps the current destination).
// ---------------------------------------------------------------------------
class NetSample {
public:
	word* Danger;   // [NAreas] enemy strength per area
	word* Pretty;   // [NAreas] target value per area
	int NZ;         // NAreas the buffers above are sized for
	int LastUpdate; // cache timestamp (never written - see CreateDiversantMap)
	void CreateDiversantMap(byte NI);
	void CreateGrenadersMap(byte NI);
	NetSample();
	~NetSample();
};
class SafeNet {
public:
	NetSample Diversant;
	NetSample Grenader;
	/*
	int LastUpdate;
	byte NI;
	int NZ;
	word* CellDanger;
	word* CellPretty;
	*/
	word FindNextCell(int F, int Start, NetSample* Net);
	word FindWayTo(int F, int Start, int Fin, NetSample* Net);
	//void CreateMaps();
	SafeNet();
	~SafeNet();

};
NetSample::NetSample() {
	NZ = 0;
	Danger = NULL;
	Pretty = NULL;
	LastUpdate = 0;
};
NetSample::~NetSample() {
	if (Danger)free(Danger);
	if (Pretty)free(Pretty);
	NZ = 0;
	Danger = NULL;
	Pretty = NULL;
};
SafeNet::SafeNet() {
};
SafeNet::~SafeNet() {
};
extern int tmtmt; // global sim tick (main.cpp), used for the 20-tick cache below

// Weight maps for a raiding party of nation NI (enemy = everything whose
// NMask bit (1<<NI) is not set).
// Danger: heavy cav 3, light cav/units with damage>5 get 2, others 1;
//         an area also gains 3/4 of each neighbor's danger, so garrisoned
//         town centers radiate a keep-out zone.
// Pretty: TC/producer 40, mill and other captureable buildings 20,
//         storehouse 0, enemy peasants 1 (pesky, but barely worth it).
// Note: the 20-tick cache never fires - LastUpdate is never assigned, so
// the maps are rebuilt on every call (as in the original).
void NetSample::CreateDiversantMap(byte NI) {
	if (tmtmt - LastUpdate < 20 && NZ == NAreas)return;
	if (NZ != NAreas) {
		Danger = (word*)realloc(Danger, 2 * NAreas);
		Pretty = (word*)realloc(Pretty, 2 * NAreas);
		NZ = NAreas;
	};
	int N2 = 2 * NAreas;
	word addDam[2048];
	memset(Danger, 0, N2);
	memset(Pretty, 0, N2);
	memset(addDam, 0, N2);
	byte Mask = 1 << NI;
	for (int i = 0; i < MAXOBJECT; i++) {
		OneObject* OB = Group[i];
		if (OB && !(OB->Sdoxlo || OB->NMask&Mask)) {
			NewMonster* NM = OB->newMons;
			byte Use = NM->Usage;
			int Pret = 0;
			int Dang = 0;
			if (NM->Capture) {
				if (NM->Building) {
					if (NM->Producer)Pret = 40;
					else {
						switch (Use) {
						case CenterID:
							Pret = 40;
							break;
						case MillID:
							Pret = 20;
							break;
						case SkladID:
							Pret = 0;
							break;
						default:
							Pret = 20;
						};
					};
				};
			}
			else {
				switch (Use) {
				case HardHorceID:
					Dang = 3;
					break;
				case FastHorseID:
					Dang = 2;
					break;
				case PeasantID:
					Pret = 1;
					break;
				default:
					if (OB->AddDamage > 5)Dang = 2;
					else Dang = 1;
				};
			};
			if (Dang || Pret) {
				int cx = OB->RealX >> 10;
				int cy = OB->RealY >> 10;
				int ofs = cx + (cy << TopSH);
				if (ofs >= 0 || ofs < TopLx*TopLy) { // || -> bounds check always passes (as in the original)
					word Top = TopRef[ofs];
					if (Top < 0xFFFE) {
						Danger[Top] += Dang;
						Pretty[Top] += Pret;
					};
				};
			};
		};
	};
	Area* AR = TopMap;
	for (int i = 0; i < NAreas; i++) {
		if (Danger[i] > 2) {
			int NL = AR->NLinks;
			for (int j = 0; j < NL; j++) {
				int id = AR->Link[j + j];
				int dam = Danger[id];
				if (dam) {
					addDam[i] += (dam >> 1) + (dam >> 2);
				};
			};
		};
		AR++;
	};
	for (int i = 0; i < NAreas; i++)Danger[i] += addDam[i];
};
// Grenadier variant: targets are enemy walls that are not yet complete or
// crewed (Pretty += 10 to the four neighboring areas - grenadiers are sent
// to bash wall-building), captureable buildings (Pret = 5, towers Pret = 20);
// ranged units and cavalry are the danger (Dang = 3, light infantry 1).
void NetSample::CreateGrenadersMap(byte NI) {
	if (tmtmt - LastUpdate < 20 && NZ == NAreas)return;
	if (NZ != NAreas) {
		Danger = (word*)realloc(Danger, 2 * NAreas);
		Pretty = (word*)realloc(Pretty, 2 * NAreas);
		NZ = NAreas;
	};
	int N2 = 2 * NAreas;
	word addDam[2048];
	memset(Danger, 0, N2);
	memset(Pretty, 0, N2);
	memset(addDam, 0, N2);
	byte Mask = 1 << NI;
	for (int i = 0; i < MAXOBJECT; i++) {
		OneObject* OB = Group[i];
		int Pret = 0;
		int Dang = 0;
		if (OB && (!OB->Sdoxlo) && !(OB->NMask&Mask)) {
			NewMonster* NM = OB->newMons;
			byte Use = NM->Usage;
			if (OB->Wall) {
				if (!(OB->Ready&&OB->SafeWall)) {
					int cx = OB->RealX >> 10;
					int cy = OB->RealY >> 10;
					if (cx > 0 && cx < TopLx - 1 && cy>0 && cy < TopLy - 1) {
						int ofs = cx + (cy << TopSH);
						word Top = TopRef[ofs + 1];
						if (Top < 0xFFFE)Pretty[Top] += 10;
						Top = TopRef[ofs - 1];
						if (Top < 0xFFFE)Pretty[Top] += 10;
						Top = TopRef[ofs - TopLx];
						if (Top < 0xFFFE)Pretty[Top] += 10;
						Top = TopRef[ofs + TopLx];
						if (Top < 0xFFFE)Pretty[Top] += 10;
					};
				};
			}
			else {
				if (Use == TowerID)Pret = 20;
				else
					if (Use == StrelokID || Use == HorseStrelokID || Use == HardHorceID || Use == FastHorseID )Dang = 3;
					else {
						if (NM->Capture) {
							Pret = 5;
						};
						if (Use == LightInfID)Dang = 1;
					};
					if (Dang || Pret) {
						int cx = OB->RealX >> 10;
						int cy = OB->RealY >> 10;
						int ofs = cx + (cy << TopSH);
						if (ofs >= 0 || ofs < TopLx*TopLy) { // || -> bounds check always passes (as in the original)
							word Top = TopRef[ofs];
							if (Top < 0xFFFE) {
								Danger[Top] += Dang;
								Pretty[Top] += Pret;
							};
						};
					};
			};
		};
	};
};
#define MAXAR 700
// Route one area step for a detachment of (estimated) strength F:
//  - areas with Danger > F/2 are impassable (CantGo = 1) and additionally
//    poison a one-area ring around themselves (CantGo = 2), so the route
//    does not hug the enemy;
//  - areas with a moderate, beatable danger (3 < Danger <= F/2) get a +20
//    attract bonus - the detachment is allowed to engage what it can
//    overpower on the way;
//  - a Dijkstra-like wave then spreads from the Start area over the whole
//    area graph and the first step of the cheapest path to any Pretty>0
//    area is returned. 0xFFFF = no reachable target.
word SafeNet::FindNextCell(int F, int Cell, NetSample* Net) {
	int NZ = Net->NZ;
	if (Cell < NZ) {
		int F0 = F >> 2;
		int F1 = F >> 1;
		int F2 = F0 + F1;
		int F3 = F + F0;
		word* CellDanger = Net->Danger;
		word* CellPretty = Net->Pretty;
		word AddPrett[2048];
		memcpy(AddPrett, CellPretty, NZ << 1);
		for (int i = 0; i < NAreas; i++) {
			word cdn = CellDanger[i];
			if (cdn <= F1&&cdn > 3) {
				AddPrett[i] += 20;
			};
		};
		if (AddPrett[Cell])return Cell;
		byte CantGo[MAXAR];
		for (int i = 0; i < NAreas; i++) {
			word dang = CellDanger[i];
			CantGo[i] = dang > F1;
		};
		Area* AR = TopMap;
		for (int i = 0; i < NAreas; i++) {
			if (CantGo[i] == 1) {
				int NL = AR->NLinks;
				for (int j = 0; j < NL; j++) {
					word id = AR->Link[j + j];
					if (!CantGo[id])CantGo[id] = 2;
				};
			};
			AR++;
		};

		//now we are ready to find a way

		word DistArr[2048]; // wave distance per area (arrays sized for NAreas <= 2048)
		word DistID[2048]; // per-area: which area we came from
		byte Checked[2048]; // 1 = closed by the wave, 2 = queued

		memset(DistArr, 0, NAreas << 1);
		memset(Checked, 0, NAreas);
		word BoundCells[200]; // wave frontier per step, max 200 areas, unchecked (as in the original)
		int NBound = 0;
		word NewBound[200];
		int NNewB = 0;

		NBound = 1;
		BoundCells[0] = Cell;
		DistArr[Cell] = 1;
		word LastPrettyID = 0xFFFF;
		do {
			NNewB = 0;
			for (int i = 0; i < NBound; i++)Checked[BoundCells[i]] = 1;
			for (int i = 0; i < NBound; i++) {
				int stp = BoundCells[i];
				Area* BA = TopMap + stp;
				int N = BA->NLinks;
				int L0 = DistArr[i]; // NOTE: indexed by frontier slot, not area id (as in the original) - path costs are approximate
				for (int j = 0; j < N; j++) {
					word id = BA->Link[j + j];
					if (!(CantGo[id] || Checked[id] == 1)) {
						int d = L0 + BA->Link[j + j + 1];
						if (DistArr[id]) {
							if (d < DistArr[id]) {
								DistID[id] = stp;
								DistArr[id] = d;
								if (AddPrett[id]) {
									LastPrettyID = id;
								};
							};
						}
						else {
							DistID[id] = stp;
							DistArr[id] = d;
							if (AddPrett[id]) {
								LastPrettyID = id;
							};
						};
						if (!Checked[id]) {
							Checked[id] = 2;
							NewBound[NNewB] = id;
							NNewB++;
						};
					};
				};
			};
			memcpy(BoundCells, NewBound, NNewB << 1);
			NBound = NNewB;
		} while (LastPrettyID == 0xFFFF && NNewB);
		if (LastPrettyID != 0xFFFF) {
			int PreCell = LastPrettyID;
			while (LastPrettyID != Cell) {
				PreCell = LastPrettyID;
				LastPrettyID = DistID[LastPrettyID];
			};
			return PreCell;
		};
		return 0xFFFF;
	};
	return 0xFFFF;
};

// Variant with a fixed destination: same "avoid stronger-than-us areas"
// wave as FindNextCell, but the search stops on reaching area Fin.
// Returns the first area on the path to Fin, 0xFFFF = no safe route.
word SafeNet::FindWayTo(int F, int Cell, int Fin, NetSample* Net) {
	//CreateMaps();
	if (Cell < Net->NZ) {
		word* CellDanger = Net->Danger;
		int F0 = F >> 2;
		int F1 = F >> 1;
		int F2 = F0 + F1;
		int F3 = F + F0;
		byte CantGo[MAXAR];
		for (int i = 0; i < NAreas; i++) {
			word dang = CellDanger[i];
			CantGo[i] = dang > F1;
		};

		//now we are ready to find a way

		word DistArr[2048]; // wave distance per area (arrays sized for NAreas <= 2048)
		word DistID[2048]; // per-area: which area we came from
		byte Checked[2048]; // 1 = closed by the wave, 2 = queued

		memset(DistArr, 0, NAreas << 1);
		memset(Checked, 0, NAreas);
		word BoundCells[200]; // wave frontier per step, max 200 areas, unchecked (as in the original)
		int NBound = 0;
		word NewBound[200];
		int NNewB = 0;

		NBound = 1;
		BoundCells[0] = Cell;
		DistArr[Cell] = 1;
		word LastPrettyID = 0xFFFF;
		do {
			NNewB = 0;
			for (int i = 0; i < NBound; i++)Checked[BoundCells[i]] = 1;
			for (int i = 0; i < NBound; i++) {
				int stp = BoundCells[i];
				Area* BA = TopMap + stp;
				int N = BA->NLinks;
				int L0 = DistArr[i]; // NOTE: indexed by frontier slot, not area id (as in the original) - path costs are approximate
				for (int j = 0; j < N; j++) {
					word id = BA->Link[j + j];
					if (!(CantGo[id] || Checked[id] == 1)) {
						int d = L0 + BA->Link[j + j + 1];
						if (DistArr[id]) {
							if (d < DistArr[id]) {
								DistID[id] = stp;
								DistArr[id] = d;
								if (id == Fin) {
									LastPrettyID = id;
								};
							};
						}
						else {
							DistID[id] = stp;
							DistArr[id] = d;
							if (id == Fin) {
								LastPrettyID = id;
							};
						};
						if (!Checked[id]) {
							Checked[id] = 2;
							NewBound[NNewB] = id;
							NNewB++;
						};
					};
				};
			};
			memcpy(BoundCells, NewBound, NNewB << 1);
			NBound = NNewB;
		} while (LastPrettyID == 0xFFFF && NNewB);
		if (LastPrettyID != 0xFFFF) {
			int PreCell = LastPrettyID;
			while (LastPrettyID != Cell) {
				PreCell = LastPrettyID;
				LastPrettyID = DistID[LastPrettyID];
			};
			return PreCell;
		};
		return 0xFFFF;
	};
	return 0xFFFF;
};
SafeNet SAFNET[8]; // one router per nation index (NI = 0..7)
// Entry points called from megapolis.cpp (brigade AI):
//   NI    - acting nation (index into SAFNET, enemy = !NMask bit)
//   F     - detachment strength estimate: megapolis passes the number of
//           brigades * 16 (diversions), or ~10 - Danger > F/2 means "do not enter"
//   Start - current area id of the detachment
//   Fin   - target area id (only GetNextSafeCell; the *Div/*Gre versions pick
//           their own target from the Pretty map)
// The return value is the next area to move into, or 0xFFFF when no safe
// route exists (the caller keeps the current position).
word GetNextSafeCell(byte NI, int F, int start, int Fin) {
	SAFNET[NI].Diversant.CreateDiversantMap(NI);
	return SAFNET[NI].FindWayTo(F, start, Fin, &SAFNET[NI].Diversant);
};
word GetNextDivCell(byte NI, int F, int Start) {
	SAFNET[NI].Diversant.CreateDiversantMap(NI);
	return SAFNET[NI].FindNextCell(F, Start, &SAFNET[NI].Diversant);
};
word GetNextGreCell(byte NI, int F, int Start) {
	SAFNET[NI].Grenader.CreateGrenadersMap(NI);
	return SAFNET[NI].FindNextCell(F, Start, &SAFNET[NI].Grenader);
};