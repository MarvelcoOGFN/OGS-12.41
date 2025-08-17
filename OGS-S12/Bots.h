#pragma once
#include "framework.h"
#include "Looting.h"

#include "Bosses.h"
#include "PlayerBots.h"

#include "BehaviourTree_System.h"
#include "BT_MANG2.h"

namespace Bots {
	void StartTree(UBehaviorTreeComponent* BTComp, UBehaviorTree* BTAsset, EBTExecutionMode::Type Mode = EBTExecutionMode::Looped)
	{
		if (!BTComp)
		{
			Log("StartTree: Unable to run NULL BTComp");
			return;
		}

		BTComp->DefaultBehaviorTreeAsset = BTAsset;

		BTComp->StartLogic();
		return;
	}

	// I have suspicion this is stripped so i will recreate it
	bool RunBehaviorTree(AFortAthenaAIBotController* PC, UBehaviorTree* BTAsset)
	{
		// @todo: find BrainComponent and see if it's BehaviorTreeComponent
		// Also check if BTAsset requires BlackBoardComponent, and if so 
		// check if BB type is accepted by BTAsset.
		// Spawn BehaviorTreeComponent if none present. 
		// Spawn BlackBoardComponent if none present, but fail if one is present but is not of compatible class
		if (BTAsset == NULL)
		{
			Log("RunBehaviorTree: Unable to run NULL behavior tree");
			return false;
		}

		bool bSuccess = true;

		// see if need a blackboard component at all
		UBlackboardComponent* BlackboardComp = PC->Blackboard;
		if (BTAsset->BlackboardAsset && PC->Blackboard == nullptr)
		{
			bSuccess = PC->UseBlackboard(BTAsset->BlackboardAsset, &BlackboardComp);
		}

		if (bSuccess)
		{
			UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(PC->BrainComponent);
			if (BTComp == NULL)
			{
				Log("RunBehaviorTree: spawning BehaviorTreeComponent..");

				
				BTComp = (UBehaviorTreeComponent*)((UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject)->SpawnObject(UBehaviorTreeComponent::StaticClass(), PC);
				RegisterComponentWithWorld(BTComp, UWorld::GetWorld());
			}

			// make sure BrainComponent points at the newly created BT component
			PC->BrainComponent = BTComp;

			//BTComp->StartTree(*BTAsset, EBTExecutionMode::Looped);
			if (PC->RunBehaviorTree(BTAsset)) {
				Log("BehaviourTree Ran Successfully!");
			}
			else {
				Log("RunBehaviorTree returned false, manually running...");
				StartTree(BTComp, BTAsset, EBTExecutionMode::Looped);
			}
		}

		return bSuccess;
	}

	AFortPlayerPawnAthena* (*SpawnBotOG)(UFortServerBotManagerAthena* BotManager, FVector SpawnLoc, FRotator SpawnRot, UFortAthenaAIBotCustomizationData* BotData, FFortAthenaAIBotRunTimeCustomizationData RuntimeBotData);
	AFortPlayerPawnAthena* SpawnBot(UFortServerBotManagerAthena* BotManager, FVector SpawnLoc, FRotator SpawnRot, UFortAthenaAIBotCustomizationData* BotData, FFortAthenaAIBotRunTimeCustomizationData RuntimeBotData)
	{
		if (__int64(_ReturnAddress()) - __int64(GetModuleHandleA(0)) == 0x1A4153F) {
			return SpawnBotOG(BotManager, SpawnLoc, SpawnRot, BotData, RuntimeBotData);
		}

		std::string BotName = BotData->Name.ToString();

		if (BotData->GetFullName().contains("MANG_POI_Yacht"))
		{
			BotData = StaticLoadObject<UFortAthenaAIBotCustomizationData>("/Game/Athena/AI/MANG/BotData/BotData_MANG_POI_HDP.BotData_MANG_POI_HDP");
		}

		if (BotData->CharacterCustomization->CustomizationLoadout.Character->GetName() == "CID_556_Athena_Commando_F_RebirthDefaultA")
		{
			std::string Tag = RuntimeBotData.PredefinedCosmeticSetTag.TagName.ToString();
			if (Tag == "Athena.Faction.Alter") {
				BotData->CharacterCustomization->CustomizationLoadout.Character = StaticLoadObject<UAthenaCharacterItemDefinition>("/Game/Athena/Items/Cosmetics/Characters/CID_NPC_Athena_Commando_M_HenchmanBad.CID_NPC_Athena_Commando_M_HenchmanBad");
			}
			else if (Tag == "Athena.Faction.Ego") {
				BotData->CharacterCustomization->CustomizationLoadout.Character = StaticLoadObject<UAthenaCharacterItemDefinition>("/Game/Athena/Items/Cosmetics/Characters/CID_NPC_Athena_Commando_M_HenchmanGood.CID_NPC_Athena_Commando_M_HenchmanGood");
			}
		}

		AActor* SpawnLocator = SpawnActor<ADefaultPawn>(SpawnLoc, SpawnRot);
		AFortPlayerPawnAthena* Ret = BotMutator->SpawnBot(BotData->PawnClass, SpawnLocator, SpawnLoc, SpawnRot, true);
		AFortAthenaAIBotController* PC = (AFortAthenaAIBotController*)Ret->Controller;

		PC->CosmeticLoadoutBC = BotData->CharacterCustomization->CustomizationLoadout;
		PC->CachedPatrollingComponent = (UFortAthenaNpcPatrollingComponent*)UGameplayStatics::SpawnObject(UFortAthenaNpcPatrollingComponent::StaticClass(), PC);
		for (int32 i = 0; i < BotData->CharacterCustomization->CustomizationLoadout.Character->HeroDefinition->Specializations.Num(); i++)
		{
			UFortHeroSpecialization* Spec = StaticLoadObject<UFortHeroSpecialization>(UKismetStringLibrary::Conv_NameToString(BotData->CharacterCustomization->CustomizationLoadout.Character->HeroDefinition->Specializations[i].ObjectID.AssetPathName).ToString());

			if (Spec)
			{
				for (int32 i = 0; i < Spec->CharacterParts.Num(); i++)
				{
					UCustomCharacterPart* Part = StaticLoadObject<UCustomCharacterPart>(UKismetStringLibrary::Conv_NameToString(Spec->CharacterParts[i].ObjectID.AssetPathName).ToString());
					Ret->ServerChoosePart(Part->CharacterPartType, Part);
				}
			}
		}

		Ret->CosmeticLoadout = BotData->CharacterCustomization->CustomizationLoadout;
		Ret->OnRep_CosmeticLoadout();

		SpawnLocator->K2_DestroyActor();
		DWORD CustomSquadId = RuntimeBotData.CustomSquadId;
		BYTE TrueByte = 1;
		BYTE FalseByte = 0;

		static UBehaviorTree* BehaviorTree = StaticLoadObject<UBehaviorTree>("/Game/Athena/AI/MANG/BehaviorTree/BT_MANG2.BT_MANG2");
		static UBlackboardData* Blackboard = StaticLoadObject<UBlackboardData>("/Game/Athena/AI/MANG/BehaviorTree/BB_MANG2.BB_MANG2");
		if (!BehaviorTree)
		{
			Log("Behaviourtree does not exist!");
		}
		if (!Blackboard)
		{
			Log("Blackboard doesent exist!");
		}

		BotManagerSetup(__int64(BotManager), __int64(Ret), __int64(BotData->BehaviorTree), 0, &CustomSquadId, 0, __int64(BotData->StartupInventory), __int64(BotData->BotNameSettings), 0, &FalseByte, 0, &TrueByte, RuntimeBotData);

		FactionBot* bot = new FactionBot(Ret);

		bot->CID = BotData->CharacterCustomization->CustomizationLoadout.Character->GetName();

		for (int32 i = 0; i < BotData->StartupInventory->Items.Num(); i++)
		{
			bool equip = true;

			if (BotData->StartupInventory->Items[i]->GetName().starts_with("WID_Athena_FloppingRabbit") || BotData->StartupInventory->Items[i]->GetName().starts_with("WID_Boss_Adventure_GH")) {
				equip = false;
			}
			bot->GiveItem(BotData->StartupInventory->Items[i], 1, equip);
			if (auto Data = Cast<UFortWeaponItemDefinition>(BotData->StartupInventory->Items[i]))
			{
				if (Data->GetAmmoWorldItemDefinition_BP() && Data->GetAmmoWorldItemDefinition_BP() != Data)
				{
					bot->GiveItem(Data->GetAmmoWorldItemDefinition_BP(), 99999);
				}
			}
		}

		TArray<AFortAthenaPatrolPath*> PatrolPaths;

		Statics->GetAllActorsOfClass(UWorld::GetWorld(), AFortAthenaPatrolPath::StaticClass(), (TArray<AActor*>*) & PatrolPaths);

		bot->Name = BotName;
		for (int i = 0; i < PatrolPaths.Num(); i++) {
			if (PatrolPaths[i]->PatrolPoints[0]->K2_GetActorLocation() == SpawnLoc) {
				//Log("Found patrol path for bot: " + BotData->GetFullName());
				bot->PC->CachedPatrollingComponent->SetPatrolPath(PatrolPaths[i]);
				PC->CachedPatrollingComponent->SetPatrolPath(PatrolPaths[i]);
				bot->PatrolPath = PatrolPaths[i];
			}
		}

		PC->PathFollowingComponent->MyNavData = ((UAthenaNavSystem*)UWorld::GetWorld()->NavigationSystem)->MainNavData;
		PC->PathFollowingComponent->OnNavDataRegistered(PC->PathFollowingComponent->MyNavData);
		PC->PathFollowingComponent->Activate(true);
		PC->PathFollowingComponent->SetActive(true, true);
		PC->PathFollowingComponent->OnRep_IsActive();

		if (!PC->BrainComponent) {
			PC->BrainComponent = (UBrainComponent*)UGameplayStatics::SpawnObject(UBrainComponent::StaticClass(), PC);
			PC->BrainComponent->Activate(false);
			PC->BrainComponent->SetActive(true, false);
			PC->BrainComponent->OnRep_IsActive();
		}

		PC->UseBlackboard(Blackboard, &PC->Blackboard);
		PC->OnUsingBlackBoard(PC->Blackboard, Blackboard);

		PC->Blackboard->SetValueAsBool(UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_IsMovementBlocked")), false);

		PC->BehaviorTree = BehaviorTree;
		/*if (Globals::bUseLegacyAI_MANG) {
			if (RunBehaviorTree(PC, StaticLoadObject<UBehaviorTree>("/Game/Athena/AI/MANG/BehaviorTree/BT_MANG2.BT_MANG2"))) {
				Log("Hi!");
			}
			else {
				Log("Bye!");
			}
			PC->BlueprintOnBehaviorTreeStarted();
		}*/

		static auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
		static auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
		PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::Warmup);
		PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Warmup);

		PC->Blackboard->SetValueAsBool(UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_IsMovementBlocked")), false);
		PC->Blackboard->SetValueAsEnum(UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_AvoidThreat_ExecutionStatus")), (uint8)EExecutionStatus::ExecutionAllowed);
		PC->Blackboard->SetValueAsEnum(UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Leash_ExecutionStatus")), (uint8)EExecutionStatus::ExecutionAllowed);
		PC->Blackboard->SetValueAsEnum(UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Patrolling_ExecutionStatus")), (uint8)EExecutionStatus::ExecutionAllowed);
		PC->Blackboard->SetValueAsEnum(UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_DynamicBlueprint_ExecutionStatus")), (uint8)EExecutionStatus::ExecutionAllowed);
		PC->Blackboard->SetValueAsEnum(UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_CharacterLaunched_ExecutionStatus")), (uint8)EExecutionStatus::ExecutionAllowed);

		Ret->Mesh->AnimBlueprintGeneratedClass = StaticLoadObject<UClass>("/Game/Athena/AI/MANG/AnimSet/MANG_PatrolLayerAnimBP.MANG_PatrolLayerAnimBP_C");
		Ret->OnRep_AnimBPOverride();

		PC->OnRep_Pawn();

		PC->BrainComponent->RestartLogic();

		bot->BT_MANG = BT_MANG2::ConstructBehaviorTree();

		return Ret;
	}

	inline char (*SetUpInventoryBotOG)(AActor* Pawn, __int64 a2);
	inline char __fastcall SetUpInventoryBot(AActor* Pawn, __int64 a2)
	{
		*(AActor**)(__int64(Pawn) + 1352) = SpawnActor<AFortInventory>({}, {}, Pawn);

		return SetUpInventoryBotOG(Pawn, a2);
	}

	void (*OnPossessedPawnDiedOG)(ABP_PhoebePlayerController_C* PC, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum);
	void OnPossessedPawnDied(ABP_PhoebePlayerController_C* PC, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum)
	{
		if (!PC) {
			return;
		}

		FactionBot* KilledBot = nullptr;
		for (size_t i = 0; i < FactionBots.size(); i++)
		{
			auto bot = FactionBots[i];
			if (bot && bot->PC == PC)
			{
				if (bot->Pawn->GetName().starts_with("BP_Pawn_DangerGrape_")) {
					goto nodrop;
				}
				else {
					KilledBot = bot;
				}
			}
		}

		if (KilledBot && InstigatedBy && InstigatedBy->PlayerState && !InstigatedBy->PlayerState->bIsABot) {
			std::string DisguiseName;
			AFortPlayerControllerAthena* PlayerController = (AFortPlayerControllerAthena*)InstigatedBy;
			if (PlayerController && PlayerController->WorldInventory && PlayerController->WorldInventory->Inventory.ReplicatedEntries) {
				for (size_t i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					auto& Entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries[i];
					if (Entry.ItemDefinition) {
						std::string ItemName = Entry.ItemDefinition->Name.ToString();
						if (ItemName.contains("Disguise")) {
							DisguiseName = ItemName;
						}
					}
				}
			}

			Log(KilledBot->Name);
			if (KilledBot->Name.contains("POI")) {
				Quests::GiveAccolade((AFortPlayerControllerAthena*)InstigatedBy, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_078_ElimBoss.AccoladeId_078_ElimBoss"));
			}
			else if (KilledBot->Name.contains("Ego") && DisguiseName.contains("Alter")) {
				Quests::GiveAccolade((AFortPlayerControllerAthena*)InstigatedBy, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_081_ElimGhost.AccoladeId_081_ElimGhost"));
			}
			else if (KilledBot->Name.contains("Alter") && DisguiseName.contains("Ego")) {
				Quests::GiveAccolade((AFortPlayerControllerAthena*)InstigatedBy, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_082_ElimShadow.AccoladeId_082_ElimShadow"));
			}
			else {
				Quests::GiveAccolade((AFortPlayerControllerAthena*)InstigatedBy, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_075_ElimMang.AccoladeId_075_ElimMang"));
			}
		}

		if (InstigatedBy && DamageCauser) {
			for (auto& bot : PlayerBotArray)
			{
				if (bot && bot->PC && bot->PC == PC && !bot->bIsDead)
				{
					bot->OnDied((AFortPlayerStateAthena*)InstigatedBy->PlayerState, DamageCauser, BoneName);
					break;
				}
			}
		}

		PC->PlayerBotPawn->SetMaxShield(0);
		for (int32 i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) || PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()))
				continue;
			auto Def = PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition;
			if (Def->GetName() == "AGID_Athena_Keycard_Yacht") {
				goto nodrop;
			}
			if (Def->GetName() == "WID_Boss_Tina") {
				Def = KilledBot->Weapon;
			}
			SpawnPickup(Def, PC->Inventory->Inventory.ReplicatedEntries[i].Count, PC->Inventory->Inventory.ReplicatedEntries[i].LoadedAmmo, PC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::PlayerElimination);
			int Ammo = 0;
			int AmmoC = 0;
			if (Def->GetName() == "WID_Boss_Hos_MG") {
				Ammo = 60;
				AmmoC = 60;
			}
			else if (Def->GetName().starts_with("WID_Assault_LMG_Athena_")) {
				Ammo = 45;
				AmmoC = 45;
			}
			if (Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
			{
				UFortWeaponItemDefinition* Def2 = (UFortWeaponItemDefinition*)Def;
				SpawnPickup(Def2->GetAmmoWorldItemDefinition_BP(), AmmoC != 0 ? AmmoC : Looting::GetClipSize(Def2), Ammo != 0 ? Ammo : Looting::GetClipSize(Def2), PC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::PlayerElimination);
			}
		}

	nodrop:
		for (size_t i = 0; i < FactionBots.size(); i++)
		{
			auto bot = FactionBots[i];
			if (bot && bot->PC == PC)
			{
				FactionBots.erase(FactionBots.begin() + i);
				break;
			}
		}

		return OnPossessedPawnDiedOG(PC, DamagedActor, Damage, InstigatedBy, DamageCauser, HitLocation, HitComp, BoneName, Momentum);
	}

	wchar_t* (*OnPerceptionSensedOG)(ABP_PhoebePlayerController_C* PC, AActor* SourceActor, FAIStimulus& Stimulus);
	wchar_t* OnPerceptionSensed(ABP_PhoebePlayerController_C* PC, AActor* SourceActor, FAIStimulus& Stimulus)
	{
		if (SourceActor->IsA(AFortPlayerPawnAthena::StaticClass()))
		{
			std::string DisguiseName;
			AFortPlayerController* PlayerController = Cast<AFortPlayerController>(SourceActor->GetOwner());
			if (PlayerController && PlayerController->WorldInventory && PlayerController->WorldInventory->Inventory.ReplicatedEntries) {
				for (size_t i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					auto& Entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries[i];
					if (Entry.ItemDefinition) {
						std::string ItemName = Entry.ItemDefinition->Name.ToString();
						if (ItemName.contains("Disguise")) {
							DisguiseName = ItemName;
						}
					}
				}
			}

			for (auto bot : FactionBots)
			{
				if (bot->PC == PC)
				{
					if (!DisguiseName.empty()) {
						Log(bot->Name);
						if (bot->Name.contains("Ego") && DisguiseName.contains("Ego")) {
							return nullptr;
						}
						else if (bot->Name.contains("Alter") && DisguiseName.contains("Alter")) {
							return nullptr;
						}
					}
					bot->OnPerceptionSensed(SourceActor, Stimulus);
				}
			}
		}

		return OnPerceptionSensedOG(PC, SourceActor, Stimulus);
	}

	void (*OnActorBumpOG)(UPathFollowingComponent* Comp, AActor* SelfActor, AActor* OtherActor, const FVector& NormalImpulse, FHitResult& Hit);
	void OnActorBump(UFortAthenaAIBotPathFollowingComponent* Comp, AActor* SelfActor, AActor* OtherActor, const FVector& NormalImpulse, FHitResult& Hit)
	{
		//Log("OnActorBump Called!");
		for (auto& bot : FactionBots)
		{
			if (bot->PC == Comp->BotController)
			{
				bot->bWasReportedStuck = true;

				if (!bot->Pawn->bIsCrouched && Math->RandomBoolWithWeight(0.1f)) {
					bot->Pawn->Crouch(false);
				}
				if (bot->Pawn->bIsCrouched && (bot->tick_counter % 30) == 0) {
					bot->Pawn->UnCrouch(false);
				}

				if (Math->RandomBoolWithWeight(0.025f)) {
					bot->Pawn->UnCrouch(false);
					bot->Pawn->Jump();
				}

				bot->Pawn->EquipWeaponDefinition(bot->Pickaxe, bot->PickaxeGuid);

				bot->Pawn->PawnStartFire(0);

				if (UKismetMathLibrary::GetDefaultObj()->RandomBool()) {
					bot->Pawn->AddMovementInput((bot->Pawn->GetActorRightVector() * -1.0f), 1.5f, true);
				}
				else {
					bot->Pawn->AddMovementInput(bot->Pawn->GetActorRightVector(), 1.5f, true);
				}

				bot->Pawn->PawnStopFire(0);
				break;
			}
		}

		for (auto& bot : PlayerBotArray)
		{
			if (bot->PC == Comp->BotController)
			{
				bot->SetStuck(OtherActor, Hit);
				break;
			}
		}

		return OnActorBumpOG(Comp, SelfActor, OtherActor, NormalImpulse, Hit);
	}

	// Pathfinding
	inline void (*InitializeForWorldOG)(UNavigationSystemV1* NavSystem, UWorld* World, EFNavigationSystemRunMode Mode);
	void InitializeForWorld(UNavigationSystemV1* NavSystem, UWorld* World, EFNavigationSystemRunMode Mode)
	{
		Log("InitializeForWorld Called!");
		auto AthenaNavSystem = (UAthenaNavSystem*)NavSystem;
		AthenaNavSystem->bAutoCreateNavigationData = true;
		return InitializeForWorldOG(NavSystem, World, Mode);
	}

	void Hook() {
		MH_CreateHook((LPVOID)(ImageBase + 0x19E9B10), SpawnBot, (LPVOID*)&SpawnBotOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x1637F50), SetUpInventoryBot, (LPVOID*)&SetUpInventoryBotOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x163C760), OnPossessedPawnDied, (LPVOID*)&OnPossessedPawnDiedOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x163C300), OnPerceptionSensed, (LPVOID*)&OnPerceptionSensedOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x49932C0), OnActorBump, (LPVOID*)&OnActorBumpOG);

		HookVTable(UAthenaNavSystem::GetDefaultObj(), 0x53, InitializeForWorld, (LPVOID*)&InitializeForWorldOG);

		Log("Hooked Bots!");
	}
}