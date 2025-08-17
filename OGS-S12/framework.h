#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <intrin.h>
#include <sstream>
#include <array>
#include <tlhelp32.h>
#include <future>
#include <set>
#include <variant>

#include "minhook/MinHook.h"
#include "SDK/SDK.hpp"
#include "Globals.h"

#pragma comment(lib, "minhook/minhook.lib")

using namespace SDK;

static auto ImageBase = InSDKUtils::GetImageBase();

static UNetDriver* (*CreateNetDriver)(void*, void*, FName) = decltype(CreateNetDriver)(ImageBase + 0x4573990);
static bool (*InitListen)(void*, void*, FURL&, bool, FString&) = decltype(InitListen)(ImageBase + 0xD44C40);
static void (*SetWorld)(void*, void*) = decltype(SetWorld)(ImageBase + 0x42C2B20);
static bool (*InitHost)(UObject* Beacon) = decltype(InitHost)(ImageBase + 0xd446f0);
static void (*PauseBeaconRequests)(UObject* Beacon, bool bPause) = decltype(PauseBeaconRequests)(ImageBase + 0x20dfad0);

static void(*GiveAbility)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec) = decltype(GiveAbility)(ImageBase + 0x6b19e0);
static void (*AbilitySpecConstructor)(FGameplayAbilitySpec*, UGameplayAbility*, int, int, UObject*) = decltype(AbilitySpecConstructor)(ImageBase + 0x6d6dd0);
static bool (*InternalTryActivateAbility)(UAbilitySystemComponent* AbilitySystemComp, FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey InPredictionKey, UGameplayAbility** OutInstancedAbility, void* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData) = decltype(InternalTryActivateAbility)(ImageBase + 0x6b33f0);
static FGameplayAbilitySpecHandle(*GiveAbilityAndActivateOnce)(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec) = decltype(GiveAbilityAndActivateOnce)(ImageBase + 0x6b1b00);

static FVector* (*PickSupplyDropLocationOG)(AFortAthenaMapInfo* MapInfo, FVector* outLocation, __int64 Center, float Radius) = decltype(PickSupplyDropLocationOG)(ImageBase + 0x18848f0);

inline static ABuildingSMActor* (*ReplaceBuildingActor)(ABuildingSMActor* BuildingSMActor, unsigned int a2, UObject* a3, unsigned int a4, int a5, bool bMirrored, AFortPlayerControllerAthena* PC) = decltype(ReplaceBuildingActor)(ImageBase + 0x1B951B0);
static __int64 (*CantBuild)(UWorld*, UObject*, FVector, FRotator, char, void*, char*) = decltype(CantBuild)(ImageBase + 0x1E57790);

static void* (*ApplyCharacterCustomization)(void* a1, void* a2) = decltype(ApplyCharacterCustomization)(ImageBase + 0x2363ff0);

static void (*BotManagerSetup)(__int64 BotManaager, __int64 Pawn, __int64 BehaviorTree, __int64 a4, DWORD* SkillLevel, __int64 a7, __int64 StartupInventory, __int64 BotNameSettings, __int64 a10, BYTE* CanRespawnOnDeath, unsigned __int8 BitFieldDataThing, BYTE* CustomSquadId, FFortAthenaAIBotRunTimeCustomizationData InRuntimeBotData) = decltype(BotManagerSetup)(ImageBase + 0x19D93F0);

static char(__fastcall* RegisterComponentWithWorld)(UObject* Component, UObject* World) = decltype(RegisterComponentWithWorld)(ImageBase + 0x3FF94E0);

static void(*RemoveFromAlivePlayers)(AFortGameModeAthena*, AFortPlayerControllerAthena*, APlayerState*, AFortPlayerPawn*, UFortWeaponItemDefinition*, uint8_t DeathCause, char) = decltype(RemoveFromAlivePlayers)(ImageBase + 0x18ECBB0);
static void (*AddToAlivePlayers)(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* Player) = decltype(AddToAlivePlayers)(ImageBase + 0x18c35b0);

static void* (*StaticFindObjectOG)(UClass*, UObject* Package, const wchar_t* OrigInName, bool ExactClass) = decltype(StaticFindObjectOG)(ImageBase + 0x2E1C4B0);
static void* (*StaticLoadObjectOG)(UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, uint32_t LoadFlags, UObject* Sandbox, bool bAllowObjectReconciliation, void*) = decltype(StaticLoadObjectOG)(ImageBase + 0x2E1D7A0);

AFortAthenaMutator_Bots* BotMutator = nullptr;
TArray<FVector> PickedSupplyDropLocations;
TArray<APlayerController*> GivenLootPlayers;

static TArray<AActor*> PlayerStarts;

bool bFirstElimTriggered = false;
bool bFirstEliminated = false;
bool DontPlayAnimation = false;

bool bFirstChestSearched = false;
bool bFirstSupplyDropSearched = false;

// text manipulation utils
namespace TextManipUtils {
	// Found this from stack overflow :fire:
	std::vector<std::string> SplitWhitespace(std::string const& input) {
		std::istringstream buffer(input);
		std::vector<std::string> ret;

		std::copy(std::istream_iterator<std::string>(buffer),
			std::istream_iterator<std::string>(),
			std::back_inserter(ret));
		return ret;
	}
}

void Log(const std::string& msg)
{
	static bool firstCall = true;

	if (firstCall)
	{
		std::ofstream logFile("OGS_log.txt", std::ios::trunc);
		if (logFile.is_open())
		{
			logFile << "[OGS]: Log file initialized!\n";
			logFile.close();
		}
		firstCall = false;
	}

	std::ofstream logFile("OGS_log.txt", std::ios::app);
	if (logFile.is_open())
	{
		logFile << "[OGS]: " << msg << std::endl;
		logFile.close();
	}

	std::cout << "[OGS]: " << msg << std::endl;
}

void HookVTable(void* Base, int Idx, void* Detour, void** OG)
{
	DWORD oldProtection;

	void** VTable = *(void***)Base;

	if (OG)
	{
		*OG = VTable[Idx];
	}

	VirtualProtect(&VTable[Idx], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection);

	VTable[Idx] = Detour;

	VirtualProtect(&VTable[Idx], sizeof(void*), oldProtection, NULL);
}

// pasted shit from ploosh wow
template <typename T = void*>
__forceinline static void ExecHook(UFunction* func, void* detour, T& og = nullptr)
{
	if (!func)
		return;
	if (!std::is_same_v<T, void*>)
		og = (T)func->ExecFunction;

	func->ExecFunction = reinterpret_cast<UFunction::FNativeFuncPtr>(detour);
}

class FOutputDevice
{
public:
	bool bSuppressEventTag;
	bool bAutoEmitLineTerminator;
	uint8_t _Padding1[0x6];
};

class FFrame : public FOutputDevice
{
public:
	void** VTable;
	UFunction* Node;
	UObject* Object;
	uint8* Code;
	uint8* Locals;
	void* MostRecentProperty;
	uint8_t* MostRecentPropertyAddress;
	uint8_t _Padding1[0x40];
	FField* PropertyChainForCompiledIn;

public:
	void StepCompiledIn(void* const Result, bool ForceExplicitProp = false)
	{
		if (Code && !ForceExplicitProp)
		{
			((void (*)(FFrame*, UObject*, void* const))(ImageBase + 0x2E1DD00))(this, Object, Result);
		}
		else
		{
			FField* _Prop = PropertyChainForCompiledIn;
			PropertyChainForCompiledIn = _Prop->Next;
			((void (*)(FFrame*, void* const, FField*))(ImageBase + 0x2E1DD30))(this, Result, _Prop);
		}
	}

	template <typename T>
	T& StepCompiledInRef()
	{
		T TempVal{};
		MostRecentPropertyAddress = nullptr;

		if (Code)
		{
			((void (*)(FFrame*, UObject*, void* const))(ImageBase + 0x2E1DD00))(this, Object, &TempVal);
		}
		else
		{
			FField* _Prop = PropertyChainForCompiledIn;
			PropertyChainForCompiledIn = _Prop->Next;
			((void (*)(FFrame*, void* const, FField*))(ImageBase + 0x2E1DD30))(this, &TempVal, _Prop);
		}

		return MostRecentPropertyAddress ? *(T*)MostRecentPropertyAddress : TempVal;
	}
};

inline FQuat RotatorToQuat(FRotator Rotation)
{
	FQuat Quat;
	const float DEG_TO_RAD = 3.14159f / 180.0f;
	const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

	float SP = sin(Rotation.Pitch * DIVIDE_BY_2);
	float CP = cos(Rotation.Pitch * DIVIDE_BY_2);
	float SY = sin(Rotation.Yaw * DIVIDE_BY_2);
	float CY = cos(Rotation.Yaw * DIVIDE_BY_2);
	float SR = sin(Rotation.Roll * DIVIDE_BY_2);
	float CR = cos(Rotation.Roll * DIVIDE_BY_2);

	Quat.X = CR * SP * SY - SR * CP * CY;
	Quat.Y = -CR * SP * CY - SR * CP * SY;
	Quat.Z = CR * CP * SY - SR * SP * CY;
	Quat.W = CR * CP * CY + SR * SP * SY;

	return Quat;
}

template <typename T>
static inline T* StaticFindObject(std::wstring ObjectName)
{
	return (T*)StaticFindObjectOG(T::StaticClass(), nullptr, ObjectName.c_str(), false);
}

template<typename T>
inline T* Cast(UObject* Object)
{
	if (!Object || !Object->IsA(T::StaticClass()))
		return nullptr;
	return (T*)Object;
}

template<typename T = UObject>
static inline T* StaticLoadObject(const std::string& Name)
{
	auto ConvName = std::wstring(Name.begin(), Name.end());

	T* Object = StaticFindObject<T>(ConvName);

	if (!Object)
	{
		Object = (T*)StaticLoadObjectOG(T::StaticClass(), nullptr, ConvName.c_str(), nullptr, 0, nullptr, false, nullptr);
	}

	return Object;
}

template<typename T>
T* GetDefaultObject()
{
	return (T*)T::StaticClass()->DefaultObject;
}

static inline FQuat FRotToQuat(FRotator Rotation) {
	FQuat Quat;
	const float DEG_TO_RAD = 3.14159f / 180.0f;
	const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

	float SP = sin(Rotation.Pitch * DIVIDE_BY_2);
	float CP = cos(Rotation.Pitch * DIVIDE_BY_2);
	float SY = sin(Rotation.Yaw * DIVIDE_BY_2);
	float CY = cos(Rotation.Yaw * DIVIDE_BY_2);
	float SR = sin(Rotation.Roll * DIVIDE_BY_2);
	float CR = cos(Rotation.Roll * DIVIDE_BY_2);

	Quat.X = CR * SP * SY - SR * CP * CY;
	Quat.Y = -CR * SP * CY - SR * CP * SY;
	Quat.Z = CR * CP * SY - SR * SP * CY;
	Quat.W = CR * CP * CY + SR * SP * SY;

	return Quat;
}

template<typename T>
inline T* SpawnActor(FVector Loc, FRotator Rot = FRotator(), AActor* Owner = nullptr, SDK::UClass* Class = T::StaticClass(), ESpawnActorCollisionHandlingMethod Handle = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
{
	FTransform Transform{};
	Transform.Scale3D = FVector{ 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = FRotToQuat(Rot);

	return (T*)UGameplayStatics::FinishSpawningActor(UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, Handle, Owner), Transform);
}

template<typename T>
static inline T* SpawnAActor(FVector Loc = { 0,0,0 }, FRotator Rot = { 0,0,0 }, AActor* Owner = nullptr)
{
	FTransform Transform{};
	Transform.Scale3D = { 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = FRotToQuat(Rot);

	AActor* NewActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), T::StaticClass(), Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner);
	return (T*)UGameplayStatics::FinishSpawningActor(NewActor, Transform);
}

template<typename T>
static inline T* SpawnActorClass(FVector Loc = { 0,0,0 }, FRotator Rot = { 0,0,0 }, UClass* Class = nullptr, AActor* Owner = nullptr)
{
	FTransform Transform{};
	Transform.Scale3D = { 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = RotatorToQuat(Rot);

	AActor* NewActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner);
	return (T*)UGameplayStatics::FinishSpawningActor(NewActor, Transform);
}

template<typename T>
T* Actors(UClass* Class = T::StaticClass(), FVector Loc = {}, FRotator Rot = {}, AActor* Owner = nullptr)
{
	return SpawnActor<T>(Loc, Rot, Owner, Class);
}

AActor* DuplicateActor(AActor* Original)
{
	if (!Original) return nullptr;

	const FVector Location = Original->K2_GetActorLocation();
	const FRotator Rotation = Original->K2_GetActorRotation();
	const FTransform Transform = Original->GetTransform();

	auto* Class = Original->Class;
	auto* Owner = Original->GetOwner();

	auto* Duplicated = (AActor*)UGameplayStatics::FinishSpawningActor(
		UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, Owner),
		Transform
	);

	if (Duplicated)
	{
		//FName DupedTag = UKismetStringLibrary::Conv_StringToName(L"_ServerSpawned_");

		Duplicated->Name = Original->Name;
		Duplicated->Tags = Original->Tags;

		//Duplicated->Tags.Add(DupedTag);
	}

	FGameplayTag* OriginalFactionTag = (FGameplayTag*)((uintptr_t)Original + 0xB0);
	FGameplayTag* DuplicateFactionTag = (FGameplayTag*)((uintptr_t)Duplicated + 0xB0);

	if (OriginalFactionTag && DuplicateFactionTag)
	{
		//Log(OriginalFactionTag->TagName.ToString());
		*DuplicateFactionTag = *OriginalFactionTag;
	}

	return Duplicated;
}

AFortPickupAthena* SpawnPickup(UFortItemDefinition* ItemDef, int OverrideCount, int LoadedAmmo, FVector Loc, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, AFortPawn* Pawn = nullptr)
{
	auto SpawnedPickup = Actors<AFortPickupAthena>(AFortPickupAthena::StaticClass(), Loc);
	SpawnedPickup->bRandomRotation = true;

	auto& PickupEntry = SpawnedPickup->PrimaryPickupItemEntry;
	PickupEntry.ItemDefinition = ItemDef;
	PickupEntry.Count = OverrideCount;
	PickupEntry.LoadedAmmo = LoadedAmmo;
	PickupEntry.ReplicationKey++;
	SpawnedPickup->OnRep_PrimaryPickupItemEntry();
	SpawnedPickup->PawnWhoDroppedPickup = Pawn;

	SpawnedPickup->TossPickup(Loc, Pawn, -1, true, false, SourceType, Source);

	SpawnedPickup->SetReplicateMovement(true);
	SpawnedPickup->MovementComponent = (UProjectileMovementComponent*)GetDefaultObject<UGameplayStatics>()->SpawnObject(UProjectileMovementComponent::StaticClass(), SpawnedPickup);

	if (SourceType == EFortPickupSourceTypeFlag::Container)
	{
		SpawnedPickup->bTossedFromContainer = true;
		SpawnedPickup->OnRep_TossedFromContainer();
	}

	return SpawnedPickup;
}

std::map<AFortPickup*, float> PickupLifetimes;
AFortPickupAthena* SpawnStack(APlayerPawn_Athena_C* Pawn, UFortItemDefinition* Def, int Count, bool giveammo = false, int ammo = 0)
{
	auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

	FVector Loc = Pawn->K2_GetActorLocation();
	AFortPickupAthena* Pickup = Actors<AFortPickupAthena>(AFortPickupAthena::StaticClass(), Loc);
	Pickup->bReplicates = true;
	PickupLifetimes[Pickup] = Statics->GetTimeSeconds(UWorld::GetWorld());
	Pickup->PawnWhoDroppedPickup = Pawn;
	Pickup->PrimaryPickupItemEntry.Count = Count;
	Pickup->PrimaryPickupItemEntry.ItemDefinition = Def;
	if (giveammo)
	{
		Pickup->PrimaryPickupItemEntry.LoadedAmmo = ammo;
	}
	Pickup->PrimaryPickupItemEntry.ReplicationKey++;

	Pickup->OnRep_PrimaryPickupItemEntry();
	Pickup->TossPickup(Loc, Pawn, 6, true, true, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::Unset);

	Pickup->MovementComponent = (UProjectileMovementComponent*)Statics->SpawnObject(UProjectileMovementComponent::StaticClass(), Pickup);
	Pickup->MovementComponent->bReplicates = true;
	((UProjectileMovementComponent*)Pickup->MovementComponent)->SetComponentTickEnabled(true);

	return Pickup;
}

static AFortPickupAthena* SpawnPickup(FFortItemEntry ItemEntry, FVector Location, EFortPickupSourceTypeFlag PickupSource = EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset)
{
	auto Pickup = SpawnPickup(ItemEntry.ItemDefinition, ItemEntry.Count, ItemEntry.LoadedAmmo, Location, PickupSource, SpawnSource);
	return Pickup;
}

inline void ShowFoundation(ABuildingFoundation* BuildingFoundation) {
	if (!BuildingFoundation)
		return;

	BuildingFoundation->bServerStreamedInLevel = true;
	BuildingFoundation->DynamicFoundationType = EDynamicFoundationType::Static;
	BuildingFoundation->OnRep_ServerStreamedInLevel();

	BuildingFoundation->FoundationEnabledState = EDynamicFoundationEnabledState::Enabled;
	BuildingFoundation->DynamicFoundationRepData.EnabledState = EDynamicFoundationEnabledState::Enabled;
	BuildingFoundation->DynamicFoundationTransform = BuildingFoundation->GetTransform();
	BuildingFoundation->OnRep_DynamicFoundationRepData();
}

FVector PickSupplyDropLocation(SDK::AFortAthenaMapInfo* MapInfo, SDK::FVector Center, float Radius)
{
	if (!PickSupplyDropLocationOG)
		return SDK::FVector(0, 0, 0);

	const float MinDistance = 10000.0f;

	for (int i = 0; i < 20; i++)
	{
		SDK::FVector loc = FVector(0, 0, 0);
		PickSupplyDropLocationOG(MapInfo, &loc, (__int64)&Center, Radius);

		bool bTooClose = false;
		for (const auto& other : PickedSupplyDropLocations)
		{
			float dx = loc.X - other.X;
			float dy = loc.Y - other.Y;
			float dz = loc.Z - other.Z;

			float distSquared = dx * dx + dy * dy + dz * dz;

			if (distSquared < MinDistance * MinDistance)
			{
				bTooClose = true;
				break;
			}
		}

		if (!bTooClose)
		{
			PickedSupplyDropLocations.Add(loc);
			return loc;
		}
	}

	return SDK::FVector(0, 0, 0);
}

template<typename T>
inline std::vector<T*> GetAllObjectsOfClass(UClass* Class = T::StaticClass())
{
	std::vector<T*> Objects{};

	for (int i = 0; i < UObject::GObjects->Num(); ++i)
	{
		UObject* Object = UObject::GObjects->GetByIndex(i);

		if (!Object)
			continue;

		if (Object->GetFullName().contains("Default"))
			continue;

		if (Object->GetFullName().contains("Test"))
			continue;

		if (Object->IsA(Class) && !Object->IsDefaultObject())
		{
			Objects.push_back((T*)Object);
		}
	}

	return Objects;
}

int CountActorsWithName(FName TargetName, UClass* Class)
{
	TArray<AActor*> FoundActors;
	auto* Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
	Statics->GetAllActorsOfClass(UWorld::GetWorld(), Class, &FoundActors);

	int Count = 0;
	for (AActor* Actor : FoundActors)
	{
		if (Actor && Actor->GetName() == TargetName.ToString())
			Count++;
	}
	return Count;
}

AFortPlayerControllerAthena* GetPCFromId(FUniqueNetIdRepl& ID)
{
	for (auto& PlayerState : UWorld::GetWorld()->GameState->PlayerArray)
	{
		auto PlayerStateAthena = Cast<AFortPlayerStateAthena>(PlayerState);
		if (!PlayerStateAthena)
			continue;
		if (PlayerStateAthena->AreUniqueIDsIdentical(ID, PlayerState->UniqueId))
			return Cast<AFortPlayerControllerAthena>(PlayerState->Owner);
	}

	return nullptr;
}

enum class EAccoladeEvent : uint8
{
	Kill,
	Search,
	MAX
};

inline UFortAccoladeItemDefinition* GetDefFromEvent(EAccoladeEvent Event, int Count, UObject* Object = nullptr)
{
	UFortAccoladeItemDefinition* Def = nullptr;

	switch (Event)
	{
	case EAccoladeEvent::Kill:
		if (Count == 1)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_014_Elimination_Bronze.AccoladeId_014_Elimination_Bronze");
		}
		else if (Count == 4)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_015_Elimination_Silver.AccoladeId_015_Elimination_Silver");
		}
		else if (Count == 8)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_016_Elimination_Gold.AccoladeId_016_Elimination_Gold");
		}
		break;
	case EAccoladeEvent::Search:
		if (Count == 3)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_008_SearchChests_Bronze.AccoladeId_008_SearchChests_Bronze");
		}
		else if (Count == 7)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_009_SearchChests_Silver.AccoladeId_009_SearchChests_Silver");
		}
		else if (Count == 12)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_010_SearchChests_Gold.AccoladeId_010_SearchChests_Gold");
		}
		break;
	case EAccoladeEvent::MAX:
		break;
	default:
		break;
	}

	return Def;
}

namespace EBTExecutionMode
{
	enum Type
	{
		SingleRun,
		Looped,
	};
}

namespace EBTActiveNode
{
	enum Type
	{
		Composite,
		ActiveTask,
		AbortingTask,
		InactiveTask,
	};
}

namespace EBTTaskStatus
{
	enum Type
	{
		Active,
		Aborting,
		Inactive,
	};
}