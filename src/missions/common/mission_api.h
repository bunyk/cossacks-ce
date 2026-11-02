// Shared declarations for the ported mission modules (src/missions/...).
//
// The original Cossacks missions are per-mission Mission.dll files that
// import the game's scenary API from dmcr.exe through their PE import
// table (contract: export OnInit + ProcessScenary; DllMain calls OnInit
// on LoadLibrary - see docs/missions.md). On Linux the game exports the
// same functions from the main executable via -rdynamic, and dlopen
// resolves them from the global scope, so the ported modules simply
// declare them extern "C" here and call them directly.
//
// Signatures are copied 1:1 from the engine (src/game/activescenary.cpp,
// DLLEXPORT definitions); names are the ones the 106 original mission
// DLLs actually import (survey over the whole GOG Missions/ tree).
// Do not resolve these with dlsym - a plain call is the port-identical
// form of the original's import-table entry.
struct GAMEOBJ;
#include <stddef.h>

// Export marker for OnInit/ProcessScenary (same shim the AI modules use).
#ifndef _WIN32
#define __declspec(x) __attribute__((visibility("default")))
#endif


// Engine integer typedefs (engine units.h) used in the signatures.
typedef unsigned char byte;
typedef unsigned short word;

// The engine's scenary object handle (activescenary.h): filled in by the
// Register* calls and passed back to the Select/Get/Send APIs.
struct GAMEOBJ
{
	word Index;
	word Serial;
	int  Type;
};

// Engine structures only passed by pointer by a few missions. Missions
// that read the full layouts copy them from the engine headers locally.
struct UnitExCaps;
struct OneUnit;


extern "C" void AddResource(byte Nat, byte ID, int Amount);
extern "C" void AllowAttack(byte Nat, byte val);
extern "C" int AskMultilineQuestion(int Nx, char* Name1, byte or1, char* Quest);
extern "C" bool AskQuestion(char* Name);
extern "C" void AttackBuildingsInZone(GAMEOBJ* ArtGroup, GAMEOBJ* Zone, byte Nat);
extern "C" void AttackEnemyInZone(GAMEOBJ* Grp, GAMEOBJ* Zone, byte EnmNation);
extern "C" void AttackZoneByArtillery(GAMEOBJ* ArtGroup, GAMEOBJ* Zone, byte Nat);
extern "C" void ChangeFriends(byte Nat, byte Flags);
extern "C" bool CheckLeaveAbility(byte Nat);
extern "C" bool CheckProduction(GAMEOBJ* Units);
extern "C" void ClearLightSpot(byte index);
extern "C" void ClearSelection(byte Nat);
extern "C" bool CreateObject0(GAMEOBJ* DstObj, GAMEOBJ* Form, GAMEOBJ* UnitType, byte NatID, GAMEOBJ* Zone, byte Direction);
extern "C" bool CreateZoneNearGroup(GAMEOBJ* DstZone, GAMEOBJ* Zone, GAMEOBJ* Grp, int R);
extern "C" bool CreateZoneNearUnit(GAMEOBJ* DstZone, GAMEOBJ* Zone, GAMEOBJ* UnitType, byte Nat, int R);
extern "C" void DisableMission(char MISSID);
extern "C" void DisableUpgrade(byte Nat, GAMEOBJ* Upg);
extern "C" void DoNotUseSelInAI(byte Nat);
extern "C" void EnableMission(char MISSID);
extern "C" void EnableUnit(byte Nat, GAMEOBJ* Type, bool State);
extern "C" void EnableUpgrade(byte Nat, GAMEOBJ* Upg);
extern "C" void FreeTimer(byte ID);
extern "C" int GetAmountOfWarriors(byte Nat);
extern "C" int GetDiff(byte NI);
extern "C" int GetGlobalTime();
extern "C" int GetKilled(GAMEOBJ* Units);
extern "C" int GetMaxInside(byte Nat);
extern "C" int GetNInside(byte Nat);
extern "C" int GetNUnits(GAMEOBJ* Units);
extern "C" int GetReadyAmount(GAMEOBJ* UnitType, byte Nation);
extern "C" int GetResource(byte Nat, byte ID);
extern "C" int GetTopDst(GAMEOBJ* Z1, GAMEOBJ* Z2);
extern "C" int GetTotalAmount0(GAMEOBJ* Units);
extern "C" int GetTotalAmount1(GAMEOBJ* UnitType, byte Nation);
extern "C" int GetTotalAmount2(GAMEOBJ* Units, GAMEOBJ* UnitType, byte Nation);
extern "C" bool GetUnitExCaps(int Index, UnitExCaps* CAPS, bool NeedOrderType);
extern "C" bool GetUnitInfo(GAMEOBJ* Units, int Index, OneUnit* Uni);
extern "C" int GetUnitsAmount0(GAMEOBJ* Zone, byte Nation);
extern "C" int GetUnitsAmount1(GAMEOBJ* Zone, GAMEOBJ* Units);
extern "C" int GetUnitsAmount2(GAMEOBJ* Zone, GAMEOBJ* UnitType, byte Nation);
extern "C" int GetUnitsByNation(GAMEOBJ* Units, byte Nat);
extern "C" bool GetZoneCoor(GAMEOBJ* Zone, int* x, int* y);
extern "C" void InitialUpgrade(char* Grp, char* Upgrade);
extern "C" int InsertUnitToGroup(GAMEOBJ* Src, GAMEOBJ* Dst, int Index);
extern "C" bool IsUpgradeDone(GAMEOBJ* Upgrade, byte Nation);
extern "C" void LooseGame();
extern "C" bool NationIsErased(byte Nat);
extern "C" bool Patrol(byte Nat, GAMEOBJ* Zone, byte Dir);
extern "C" void ProduceUnit(GAMEOBJ* Units, GAMEOBJ* UnitType, GAMEOBJ* DestGroup);
extern "C" void ProduceUnitFast(GAMEOBJ* Units, GAMEOBJ* UnitType, GAMEOBJ* DestGroup, int Speed);
extern "C" void PushAllUnitsAway(byte Nat);
extern "C" void RefreshScreen();
extern "C" void RegisterDynGroup(GAMEOBJ* Units);
extern "C" bool RegisterFormation(GAMEOBJ* GOBJ, char* Name);
extern "C" bool RegisterUnitType(GAMEOBJ* GOBJ, char* Name);
extern "C" bool RegisterUnits(GAMEOBJ* GOBJ, char* Name);
extern "C" bool RegisterUpgrade(GAMEOBJ* GOBJ, char* Name);
extern "C" void RegisterVar(void* Var, int size);
extern "C" void RegisterZone(GAMEOBJ* GOBJ, char* Name);
extern "C" void RemoveGroup(GAMEOBJ* Source, GAMEOBJ* Dest);
extern "C" void RemoveUnitFromGroup(GAMEOBJ* Src, int Index);
extern "C" void RunTimer(byte ID, int Long);
extern "C" void SaveSelectedUnits(byte NI, GAMEOBJ* Units, bool add);
extern "C" void SelAttackGroup(byte Nat, GAMEOBJ* Enemy);
extern "C" bool SelCenter(GAMEOBJ* DstZone, byte Nat, int R);
extern "C" void SelChangeNation(byte SrcNat, byte DstNat);
extern "C" bool SelCloseGates(byte Nat);
extern "C" bool SelDie(byte Nat);
extern "C" void SelErase(byte NI);
extern "C" bool SelOpenGates(byte Nat);
extern "C" bool SelSendAndKill(byte Nat, GAMEOBJ* Zone, byte Dir, byte Type);
extern "C" bool SelSendTo(byte Nat, GAMEOBJ* Zone, byte Dir, byte Type);
extern "C" void SelectTypeOfUnitsInZone(GAMEOBJ* Zone, GAMEOBJ* Type, byte Nation, bool add);
extern "C" void SelectUnits(GAMEOBJ* Units, bool Add);
extern "C" void SelectUnits1(byte Nat, GAMEOBJ* Units, bool Add);
extern "C" void SelectUnitsInZone(GAMEOBJ* Zone, byte Nation, bool add);
extern "C" void SelectUnitsType(GAMEOBJ* UnitsType, byte Nat, bool Add);
extern "C" void SendUnitsToTransport(byte Nat);
extern "C" void SetDestPoint(GAMEOBJ* Units, GAMEOBJ* Zone);
extern "C" void SetLightSpot(GAMEOBJ* Zone, int R, byte index);
extern "C" void SetPlayerName(byte Nat, char* ID);
extern "C" void SetResource(byte Nat, byte ID, int Amount);
extern "C" void SetStandGround(byte Nat, byte val);
extern "C" void SetStandartVictory();
extern "C" void SetStartPoint(GAMEOBJ* Zone);
extern "C" void SetTrigg(byte ID, byte Val);
extern "C" void SetUnitInfo(OneUnit* Uni);
extern "C" void ShowPage(char* Name);
extern "C" void ShowPageParam(char* Name, ...);
extern "C" void ShowVictory();
extern "C" void StartAI(byte Nat, char* Name, int Land, int Money, int ResOnMap, int Difficulty);
extern "C" void TakeFood(GAMEOBJ* Units);
extern "C" void TakeStone(GAMEOBJ* Units);
extern "C" void TakeWood(GAMEOBJ* Units);
extern "C" bool TimerDone(byte ID);
extern "C" bool TimerDoneFirst(byte ID);
extern "C" bool TimerIsEmpty(byte ID);
extern "C" unsigned char Trigg(byte ID);
extern "C" bool UnitsCenter(GAMEOBJ* DstZone, GAMEOBJ* Units, word R);
