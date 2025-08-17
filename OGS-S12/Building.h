#pragma once
#include "framework.h"
#include "Inventory.h"
#include "Quests.h"

namespace Building {
	inline void ServerCreateBuildingActor(AFortPlayerControllerAthena* PC, FCreateBuildingActorData CreateBuildingData)
	{
		if (!PC || PC->IsInAircraft())
			return;

		UClass* BuildingClass = PC->BroadcastRemoteClientInfo->RemoteBuildableClass.Get();
		char a7;
		TArray<AActor*> BuildingsToRemove;
		if (!CantBuild(UWorld::GetWorld(), BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, CreateBuildingData.bMirrored, &BuildingsToRemove, &a7))
		{
			auto ResDef = UFortKismetLibrary::GetDefaultObj()->K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->DefaultObject)->ResourceType);
			Inventory::RemoveItem(PC, ResDef, 10);

			ABuildingSMActor* NewBuilding = SpawnActor<ABuildingSMActor>(CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, PC, BuildingClass);

			NewBuilding->bPlayerPlaced = true;
			NewBuilding->InitializeKismetSpawnedBuildingActor(NewBuilding, PC, true);
			NewBuilding->TeamIndex = ((AFortPlayerStateAthena*)PC->PlayerState)->TeamIndex;
			NewBuilding->Team = EFortTeam(NewBuilding->TeamIndex);

			for (size_t i = 0; i < BuildingsToRemove.Num(); i++)
			{
				BuildingsToRemove[i]->K2_DestroyActor();
			}
			UKismetArrayLibrary::Array_Clear((TArray<int32>&)BuildingsToRemove);
		}
	}

	inline void ServerBeginEditingBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit)
	{
		if (!BuildingActorToEdit || !BuildingActorToEdit->bPlayerPlaced || !PC->MyFortPawn)
			return;

		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_Awake);
		BuildingActorToEdit->EditingPlayer = PlayerState;
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			auto Item = PC->WorldInventory->Inventory.ItemInstances[i];
			if (Item->GetItemDefinitionBP()->IsA(UFortEditToolItemDefinition::StaticClass()))
			{
				PC->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Item->GetItemDefinitionBP(), Item->GetItemGuid());
				break;
			}
		}
		if (!PC->MyFortPawn->CurrentWeapon || !PC->MyFortPawn->CurrentWeapon->WeaponData || !PC->MyFortPawn->CurrentWeapon->IsA(AFortWeap_EditingTool::StaticClass()))
			return;

		AFortWeap_EditingTool* EditTool = (AFortWeap_EditingTool*)PC->MyFortPawn->CurrentWeapon;
		EditTool->EditActor = BuildingActorToEdit;
		EditTool->OnRep_EditActor();
	}

	inline void ServerEndEditingBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit)
	{
		if (!BuildingActorToEdit || !PC->MyFortPawn || BuildingActorToEdit->bDestroyed == 1 || BuildingActorToEdit->EditingPlayer != PC->PlayerState)
			return;
		BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_DormantAll);
		BuildingActorToEdit->EditingPlayer = nullptr;
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			auto Item = PC->WorldInventory->Inventory.ItemInstances[i];
			if (Item->GetItemDefinitionBP()->IsA(UFortEditToolItemDefinition::StaticClass()))
			{
				PC->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Item->GetItemDefinitionBP(), Item->GetItemGuid());
				break;
			}
		}
		if (!PC->MyFortPawn->CurrentWeapon || !PC->MyFortPawn->CurrentWeapon->WeaponData || !PC->MyFortPawn->CurrentWeapon->IsA(AFortWeap_EditingTool::StaticClass()))
			return;

		AFortWeap_EditingTool* EditTool = (AFortWeap_EditingTool*)PC->MyFortPawn->CurrentWeapon;
		EditTool->EditActor = nullptr;
		EditTool->OnRep_EditActor();
	}

	inline void ServerEditBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored)
	{
		if (!BuildingActorToEdit || BuildingActorToEdit->EditingPlayer != PC->PlayerState || !NewBuildingClass.Get() || BuildingActorToEdit->bDestroyed == 1)
			return;

		BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_DormantAll);
		BuildingActorToEdit->EditingPlayer = nullptr;
		ABuildingSMActor* NewActor = ReplaceBuildingActor(BuildingActorToEdit, 1, NewBuildingClass.Get(), 0, RotationIterations, bMirrored, PC);
		if (NewActor)
			NewActor->bPlayerPlaced = true;
	}

	void ServerRepairBuildingActor(AFortPlayerController* PC, ABuildingSMActor* BuildingActorToRepair)
	{
		auto FortKismet = (UFortKismetLibrary*)UFortKismetLibrary::StaticClass()->DefaultObject;
		if (!BuildingActorToRepair)
			return;

		if (BuildingActorToRepair->EditingPlayer)
		{
			return;
		}

		float BuildingHealthPercent = BuildingActorToRepair->GetHealthPercent();

		float BuildingCost = 10;
		float RepairCostMultiplier = 0.75;

		float BuildingHealthPercentLost = 1.0f - BuildingHealthPercent;
		float RepairCostUnrounded = (BuildingCost * BuildingHealthPercentLost) * RepairCostMultiplier;
		float RepairCost = std::floor(RepairCostUnrounded > 0 ? RepairCostUnrounded < 1 ? 1 : RepairCostUnrounded : 0);

		if (RepairCost < 0)
			return;

		auto ResDef = FortKismet->K2_GetResourceItemDefinition(BuildingActorToRepair->ResourceType);

		if (!ResDef)
			return;

		if (!PC->bBuildFree)
		{
			Inventory::RemoveItem(PC, ResDef, (int)RepairCost);
		}

		BuildingActorToRepair->RepairBuilding(PC, (int)RepairCost);
	}

	inline __int64 (*OnDamageServerOG)(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, FHitResult HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle EffectContext);
	inline __int64 OnDamageServer(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, FHitResult HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle EffectContext)
	{
		if (!Actor || !InstigatedBy || !InstigatedBy->IsA(AFortPlayerControllerAthena::StaticClass()) || !DamageCauser->IsA(AFortWeapon::StaticClass()) || !((AFortWeapon*)DamageCauser)->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) || Actor->bPlayerPlaced)
			return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

		auto Def = UFortKismetLibrary::K2_GetResourceItemDefinition(Actor->ResourceType);
		static std::map<AFortPlayerControllerAthena*, int> NumWeakSpots{};

		if (Def)
		{
			auto& BuildingResourceAmountOverride = Actor->BuildingResourceAmountOverride;
			if (!BuildingResourceAmountOverride.CurveTable) {
				return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
			}
			auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

			FString CurveTableAssetPath = UKismetStringLibrary::Conv_NameToString(GameState->CurrentPlaylistInfo.BasePlaylist->ResourceRates.ObjectID.AssetPathName);
			static auto CurveTable = StaticLoadObject<UCurveTable>(CurveTableAssetPath.ToString());
			if (!CurveTable)
				CurveTable = StaticLoadObject<UCurveTable>("/Game/Athena/Balance/DataTables/AthenaResourceRates.AthenaResourceRates");

			float Average = 1;
			EEvaluateCurveTableResult OutCurveTable;
			UDataTableFunctionLibrary::EvaluateCurveTableRow(CurveTable, BuildingResourceAmountOverride.RowName, 0.f, &OutCurveTable, &Average, FString());
			float FinalResourceCount = round(Average / (Actor->GetMaxHealth() / Damage));

			if (FinalResourceCount > 0)
			{
				if (Damage == 100.f && GameState->GamePhase != EAthenaGamePhase::Warmup)
				{
					UFortAccoladeItemDefinition* AccoladeDef = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_066_WeakSpotsInARow.AccoladeId_066_WeakSpotsInARow");

					NumWeakSpots[InstigatedBy]++;
					if (NumWeakSpots[InstigatedBy] == 2) {
						Quests::GiveAccolade(InstigatedBy, AccoladeDef);
						NumWeakSpots[InstigatedBy] = 0;
					}
				}

				InstigatedBy->ClientReportDamagedResourceBuilding(Actor, Actor->ResourceType, FinalResourceCount, false, Damage == 100.f);
				Inventory::GiveItemStack(InstigatedBy, Def, FinalResourceCount, 0);
			}
		}

		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	}

	void Hook() {
		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x230, ServerCreateBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x237, ServerBeginEditingBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x235, ServerEndEditingBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x232, ServerEditBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x22C, ServerRepairBuildingActor, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x2683F80), OnDamageServer, (LPVOID*)&OnDamageServerOG);

		Log("Hooked Building!");
	}
}