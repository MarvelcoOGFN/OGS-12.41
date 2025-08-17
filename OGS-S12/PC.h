#pragma once
#include "framework.h"
#include "Inventory.h"
#include "Looting.h"
#include "Vehicles.h"
#include "Quests.h"
#include "PlayerBots.h"

#include "PhantomBoothSpawner.h"
#include "Misc.h"
#include "GameMode.h"

namespace PC {
	// The accolades i need to do (there is way too many accolades so these are just the main ones i want completed)
	// TODO: Teamscore Accolades (029, 030, 031)
	// TODO: Damage Accolades (too many to list)
	// TODO: Upgrade Bench Accolades (064)
	// TODO: First Upgrade (071, 072)
	// TODO: First Fished Item (071)
	// TODO: Shakedown (076, 077)
	// TODO: IDK WHAT THESE ONES ARE (073, 074)

	void (*ServerReadyToStartMatchOG)(AFortPlayerControllerAthena* PC);
	void ServerReadyToStartMatch(AFortPlayerControllerAthena* PC)
	{
		if (!PC) {
			return;
		}

		UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		if (!Globals::bEventEnabled)
			GameState->MapInfo->SupplyDropInfoList[0]->SupplyDropClass = StaticLoadObject<UClass>("/Game/Athena/SupplyDrops/AthenaSupplyDrop_Donut.AthenaSupplyDrop_Donut_C");

		GameState->DefaultBattleBus = StaticLoadObject<UAthenaBattleBusItemDefinition>("/Game/Athena/Items/Cosmetics/BattleBuses/BBID_DonutBus.BBID_DonutBus");

		for (size_t i = 0; i < GameMode->BattleBusCosmetics.Num(); i++)
		{
			GameMode->BattleBusCosmetics[i] = GameState->DefaultBattleBus;
		}

		static bool setupWorld = false;
		if (!Globals::bEventEnabled && !setupWorld) {
			setupWorld = true;
			Looting::SpawnFloorLoot();

			Looting::SpawnLlamas();

			Vehicles::SpawnVehicles();

			if (true) {
				PhantomBoothSpawner::SpawnBooths();
			}

			TArray<AActor*> VendingMachinesArray;

			Statics->GetAllActorsOfClass(UWorld::GetWorld(), UObject::FindObject<UClass>("BlueprintGeneratedClass B_Athena_VendingMachine.B_Athena_VendingMachine_C"), &VendingMachinesArray);

			for (size_t i = 0; i < VendingMachinesArray.Num(); i++)
			{
				VendingMachinesArray[i]->K2_DestroyActor();
			}

			VendingMachinesArray.Free();
		}

		if (Globals::Arena)
		{
			GameState->EventTournamentRound = EEventTournamentRound::Arena;
			GameState->OnRep_EventTournamentRound();
		}

		return ServerReadyToStartMatchOG(PC);
	}

	void (*ServerAcknowledgePossessionOG)(AFortPlayerControllerAthena* PC, APawn* Pawn);
	inline void ServerAcknowledgePossession(AFortPlayerControllerAthena* PC, APawn* Pawn)
	{
		Log("ServerAck Called!");
		PC->AcknowledgedPawn = Pawn;

		return ServerAcknowledgePossessionOG(PC, Pawn);
	}

	inline void (*ServerLoadingScreenDroppedOG)(AFortPlayerControllerAthena* PC);
	inline void ServerLoadingScreenDropped(AFortPlayerControllerAthena* PC)
	{
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto Pawn = (AFortPlayerPawn*)PC->Pawn;

		if (PC && Globals::bCreativeEnabled)
		{
			Log("test");
			auto State = (AFortPlayerStateAthena*)PC->PlayerState;
			if (GameState->IsTeleportToCreativeHubAllowed(State))
			{
				auto PlayerStartTEST = GameMode->FindPlayerStart(PC, L"");

				AActor* StartSpot = nullptr;
				if (PlayerStartTEST)
				{
					StartSpot = PlayerStartTEST;
				}
				else
				{
					TArray<AActor*> StartSpotsC;
					Statics->GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartCreative::StaticClass(), &StartSpotsC);

					StartSpot = StartSpotsC[rand() % StartSpotsC.Num()];

					StartSpotsC.Free();
				}

				if (StartSpot)
				{
					Log("StartSpot!");
					PC->Pawn->K2_TeleportTo(StartSpot->K2_GetActorLocation(), StartSpot->K2_GetActorRotation());
				}
				else {
					Log("No StartSpot!");
				}
			}
			else {
				Log("Not allowed!");
			}
		}

		return ServerLoadingScreenDroppedOG(PC);
	}

	inline void ServerAttemptAircraftJump(UFortControllerComponent_Aircraft* Comp, FRotator Rotation)
	{
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		auto PC = (AFortPlayerControllerAthena*)Comp->GetOwner();
		UWorld::GetWorld()->AuthorityGameMode->RestartPlayer(PC);

		if (PC->MyFortPawn)
		{
			PC->ClientSetRotation(Rotation, true);
			PC->MyFortPawn->BeginSkydiving(true);
			PC->MyFortPawn->SetHealth(100);
			PC->MyFortPawn->SetShield(0);
		}

		GameState->OnRep_SafeZoneIndicator();
		GameState->OnRep_SafeZonePhase();
	}

	//this is deff skunked but it works as of now
	inline void (*ClientOnPawnDiedOG)(AFortPlayerControllerAthena*, FFortPlayerDeathReport);
	inline void ClientOnPawnDied(AFortPlayerControllerAthena* DeadPC, FFortPlayerDeathReport DeathReport)
	{

		Log("Pawn died");

		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortPlayerStateAthena* DeadState = (AFortPlayerStateAthena*)DeadPC->PlayerState;
		AFortPlayerPawnAthena* KillerPawn = (AFortPlayerPawnAthena*)DeathReport.KillerPawn;
		AFortPlayerStateAthena* KillerState = (AFortPlayerStateAthena*)DeathReport.KillerPlayerState;
		auto PlayerState = Cast<AFortPlayerStateAthena>(DeadPC->PlayerState);

		if (DeadPC && !bFirstEliminated) {
			bFirstEliminated = true;
			// Broken
			//Quests::GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeID_First_Death.AccoladeID_First_Death"));
		}

		static bool Won = false;

		if (!GameState->IsRespawningAllowed(DeadState))
		{
			if (DeadPC && DeadPC->WorldInventory)
			{
				for (size_t i = 0; i < DeadPC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					if (((UFortWorldItemDefinition*)DeadPC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped)
					{
						SpawnPickup(DeadPC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemDefinition, DeadPC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.Count, DeadPC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.LoadedAmmo, DeadPC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, DeadPC->MyFortPawn);
					}
				}
			}
		}

		if (!Won && DeadPC && DeadState)
		{
			if (KillerState && KillerState != DeadState)
			{
				KillerState->KillScore++;

				if (!KillerState->bIsABot)
				{
					for (size_t i = 0; i < KillerState->PlayerTeam->TeamMembers.Num(); i++)
					{
						((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->TeamKillScore++;
						((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->OnRep_TeamKillScore();
					}

					KillerState->ClientReportKill(DeadState);
					KillerState->ClientReportTeamKill(KillerState->KillScore);
					KillerState->OnRep_Kills();

				}

				DeadState->PawnDeathLocation = DeadPC->Pawn->K2_GetActorLocation();
				FDeathInfo& DeathInfo = DeadState->DeathInfo;

				if (!KillerState->bIsABot && (AFortPlayerControllerAthena*)KillerState->Owner)
				{
					AFortPlayerControllerAthena* KillerPC = (AFortPlayerControllerAthena*)KillerState->GetOwner();

					if (!bFirstElimTriggered) {
						Quests::GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_017_First_Elimination.AccoladeId_017_First_Elimination"));
						bFirstElimTriggered = true;
					}

					Quests::GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_012_Elimination.AccoladeId_012_Elimination"));
					Quests::GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, GetDefFromEvent(EAccoladeEvent::Kill, KillerState->KillScore));

					// giving assist accolade cuz idfk how to track assists
					Quests::GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_013_Assist.AccoladeId_013_Assist"));

					AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
					float Distance = DeathInfo.Distance / 100.0f;

					if (Distance >= 100.0f && Distance < 150.0f)
					{
						Quests::GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_DistanceShot.AccoladeId_DistanceShot")); // 100-149m accolade
					}
					else if (Distance >= 150.0f && Distance < 200.0f)
					{
						Quests::GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_051_LongShot.AccoladeId_051_LongShot")); // 150-199m accolade
					}
					else if (Distance >= 200.0f && Distance < 250.0f)
					{
						Quests::GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_052_LudicrousShot.AccoladeId_052_LudicrousShot")); // 200-249m accolade
					}
					else if (Distance >= 250.0f)
					{
						Quests::GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_053_ImpossibleShot.AccoladeId_053_ImpossibleShot")); // 250+m accolade
					}

					if (KillerPC) {
						float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());

						if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) > KillerPC->LastKillTimeWindow) {
							KillerPC->KillsInKillTimeWindow = 0;
							KillerPC->LastRecordedKillsInKillTimeWindow = 0;
						}
						KillerPC->LastKillTimeWindow = CurrentTime + 10.f;
						KillerPC->KillsInKillTimeWindow++;
					}
				}

				bool IsSolo = PlayerState->PlayerTeam->TeamMembers.Num() <= 1;
				bool AllDead = IsSolo;

				if (!IsSolo)
				{
					for (auto Member : PlayerState->PlayerTeam->TeamMembers)
					{
						AFortPlayerControllerAthena* MemberPC = (AFortPlayerControllerAthena*)Member;
						if (Member != DeadPC && MemberPC->bMarkedAlive && !((AFortPlayerPawnAthena*)MemberPC->Pawn)->bIsDBNO)
						{
							AllDead = false;
							break;
						}
					}
				}

				if (!GameMode->bDBNOEnabled || AllDead)
				{
					Log("Dead!");
					DeathInfo.bDBNO = false;
					DeathInfo.bInitialized = true;
					DeathInfo.DeathLocation = DeadPC->Pawn->K2_GetActorLocation();
					DeathInfo.DeathTags = DeathReport.Tags;
					DeathInfo.Downer = KillerState;
					DeathInfo.Distance = (KillerPawn ? KillerPawn->GetDistanceTo(DeadPC->Pawn) : ((AFortPlayerPawnAthena*)DeadPC->Pawn)->LastFallDistance);
					PlayerState->DeathInfo.FinisherOrDowner = DeathReport.KillerPlayerState ? DeathReport.KillerPlayerState : DeadPC->PlayerState;
					DeathInfo.DeathCause = DeadState->ToDeathCause(DeathInfo.DeathTags, DeathInfo.bDBNO);
					DeadState->OnRep_DeathInfo();
					RemoveFromAlivePlayers(GameMode, DeadPC, PlayerState, KillerPawn, DeathReport.KillerWeapon, (uint8)PlayerState->DeathInfo.DeathCause, 0);
					DeadPC->bMarkedAlive = false;
				}
				else if (GameMode->bDBNOEnabled && !AllDead)
				{
					Log("Downed!");
					DeathInfo.bDBNO = DeadPC->MyFortPawn->bWasDBNOOnDeath;
					DeathInfo.bInitialized = true;
					DeathInfo.DeathLocation = DeadPC->Pawn->K2_GetActorLocation();
					DeathInfo.DeathTags = DeathReport.Tags;
					DeathInfo.Downer = KillerState;
					DeathInfo.Distance = (KillerPawn ? KillerPawn->GetDistanceTo(DeadPC->Pawn) : ((AFortPlayerPawnAthena*)DeadPC->Pawn)->LastFallDistance);
					PlayerState->DeathInfo.FinisherOrDowner = DeathReport.KillerPlayerState ? DeathReport.KillerPlayerState : DeadPC->PlayerState;
					DeathInfo.DeathCause = DeadState->ToDeathCause(DeathInfo.DeathTags, DeathInfo.bDBNO);
					DeadState->OnRep_DeathInfo();
				}
			}

			if (GameMode->bDBNOEnabled)
			{
				for (int32 i = 0; i < GameState->Teams.Num(); i++)
				{
					AFortTeamInfo* TeamInfo = GameState->Teams[i];
					if (!TeamInfo) continue;

					if (TeamInfo->Team != PlayerState->TeamIndex)
						continue;

					FDeathInfo& DeathInfo = DeadState->DeathInfo;

					bool bIsTeamAlive = false;
					for (int32 j = 0; j < TeamInfo->TeamMembers.Num(); j++)
					{
						AFortPlayerControllerAthena* TeamMember = Cast<AFortPlayerControllerAthena>(TeamInfo->TeamMembers[j]);
						if (!TeamMember || (TeamMember == DeadPC)) continue;

						AFortPlayerPawn* TeamMemberPlayerPawn = Cast<AFortPlayerPawn>(TeamMember->MyFortPawn);
						if (!TeamMemberPlayerPawn || TeamMemberPlayerPawn->bIsDying) continue;

						bIsTeamAlive = true;
						break;
					}

					if (true)
					{
						for (int32 j = 0; j < TeamInfo->TeamMembers.Num(); j++)
						{
							AFortPlayerControllerAthena* TeamMember = Cast<AFortPlayerControllerAthena>(TeamInfo->TeamMembers[j]);
							if (!TeamMember) continue;


							FAthenaRewardResult Result;
							UFortPlayerControllerAthenaXPComponent* XPComponent = TeamMember->XPComponent;
							Result.TotalBookXpGained = XPComponent->TotalXpEarned;
							Result.TotalSeasonXpGained = XPComponent->TotalXpEarned;

							DeadPC->ClientSendEndBattleRoyaleMatchForPlayer(true, Result);

							FAthenaMatchStats Stats;
							FAthenaMatchTeamStats TeamStats;

							if (DeadState && !bIsTeamAlive)
							{
								for (int32 j = 0; j < TeamInfo->TeamMembers.Num(); j++) {
									AFortPlayerControllerAthena* TeamMember = Cast<AFortPlayerControllerAthena>(TeamInfo->TeamMembers[j]);
									if (!TeamMember) continue;

									AFortPlayerStateAthena* MemberState = (AFortPlayerStateAthena*)TeamMember->PlayerState;

									MemberState->Place = GameMode->AliveBots.Num() + GameMode->AlivePlayers.Num();
									MemberState->OnRep_Place();
								}
							}

							for (size_t i = 0; i < 20; i++)
							{
								Stats.Stats[i] = 0;
							}

							Stats.Stats[3] = DeadState->KillScore;

							TeamStats.Place = DeadState->Place;
							TeamStats.TotalPlayers = GameState->TotalPlayers;

							DeadPC->ClientSendMatchStatsForPlayer(Stats);
							DeadPC->ClientSendTeamStatsForPlayer(TeamStats);
							FDeathInfo& DeathInfo = DeadState->DeathInfo;
						}
					}
					break;
				}
			}
			else {
	
				FAthenaRewardResult Result;
				UFortPlayerControllerAthenaXPComponent* XPComponent = DeadPC->XPComponent;
				Result.TotalBookXpGained = XPComponent->TotalXpEarned;
				Result.TotalSeasonXpGained = XPComponent->TotalXpEarned;

				DeadPC->ClientSendEndBattleRoyaleMatchForPlayer(true, Result);

				FAthenaMatchStats Stats;
				FAthenaMatchTeamStats TeamStats;

				if (DeadState)
				{
					DeadState->Place = GameMode->AliveBots.Num() + GameMode->AlivePlayers.Num();
					DeadState->OnRep_Place();
				}

				for (size_t i = 0; i < 20; i++)
				{
					Stats.Stats[i] = 0;
				}

				Stats.Stats[3] = DeadState->KillScore;

				TeamStats.Place = DeadState->Place;
				TeamStats.TotalPlayers = GameState->TotalPlayers;

				DeadPC->ClientSendMatchStatsForPlayer(Stats);
				DeadPC->ClientSendTeamStatsForPlayer(TeamStats);
				FDeathInfo& DeathInfo = DeadState->DeathInfo;
			}

			if (KillerState)
			{
				if (KillerState->Place == 1)
				{
					if (DeathReport.KillerWeapon)
					{
						((AFortPlayerControllerAthena*)KillerState->Owner)->PlayWinEffects(KillerPawn, DeathReport.KillerWeapon, PlayerState->DeathInfo.DeathCause, false);
						((AFortPlayerControllerAthena*)KillerState->Owner)->ClientNotifyWon(KillerPawn, DeathReport.KillerWeapon, PlayerState->DeathInfo.DeathCause);
					}

					FAthenaRewardResult Result;
					AFortPlayerControllerAthena* KillerPC = (AFortPlayerControllerAthena*)KillerState->GetOwner();
					KillerPC->ClientSendEndBattleRoyaleMatchForPlayer(true, Result);

					FAthenaMatchStats Stats;
					FAthenaMatchTeamStats TeamStats;

					for (size_t i = 0; i < 20; i++)
					{
						Stats.Stats[i] = 0;
					}

					Stats.Stats[3] = KillerState->KillScore;

					TeamStats.Place = 1;
					TeamStats.TotalPlayers = GameState->TotalPlayers;

					KillerPC->ClientSendMatchStatsForPlayer(Stats);
					KillerPC->ClientSendTeamStatsForPlayer(TeamStats);

					GameState->WinningPlayerState = KillerState;
					GameState->WinningTeam = KillerState->TeamIndex;
					GameState->OnRep_WinningPlayerState();
					GameState->OnRep_WinningTeam();

					Quests::GiveAccolade(KillerPC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_001_Victory.AccoladeId_001_Victory"));
					KillerPC->ClientSendMatchStatsForPlayer(Stats);
					KillerPC->ClientSendTeamStatsForPlayer(TeamStats);

					GameState->WinningPlayerState = KillerState;
					GameState->WinningTeam = KillerState->TeamIndex;
					GameState->OnRep_WinningPlayerState();
					GameState->OnRep_WinningTeam();

					KillerPC->ClientReportTournamentPlacementPointsScored(1, 60);
				}
			}
			
		}

		return ClientOnPawnDiedOG(DeadPC, DeathReport);
	}

	void RebootingDelegate(ABuildingGameplayActorSpawnMachine* RebootVan)
	{
		if (!RebootVan->ResurrectLocation || RebootVan->PlayerIdsForResurrection.Num() <= 0)
			return;
		auto PC = GetPCFromId(RebootVan->PlayerIdsForResurrection[0]);
		if (!PC)
			return;

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		FTransform Transform{};
		Transform.Translation = RebootVan->K2_GetActorLocation() + FVector(0, 0, 2000);
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		APlayerPawn_Athena_C* NewPawn = (APlayerPawn_Athena_C*)GameMode->SpawnDefaultPawnAtTransform(PC, Transform);
		if (!PlayerState)
			return;
		TWeakObjectPtr<AFortPlayerStart> WeakPlayerStart{};
		WeakPlayerStart.ObjectIndex = RebootVan->ResurrectLocation->Index;
		WeakPlayerStart.ObjectSerialNumber = UObject::GObjects->GetSerialByIdx(WeakPlayerStart.ObjectIndex);
		PC->ResurrectionComponent->ResurrectionLocation = WeakPlayerStart;
		PC->RespawnPlayerAfterDeath(false);
		PC->ClientClearDeathNotification();
		UWorld::GetWorld()->AuthorityGameMode->RestartPlayer(PC);
		AFortPlayerPawnAthena* Pawn = Cast<AFortPlayerPawnAthena>(PC->Pawn);
		if (!Pawn)
		{
			printf("Failed to spawn!\n");
			RebootVan->OnResurrectionCompleted();
			return;
		}

		Pawn->SetHealth(100);
		Pawn->SetMaxHealth(100);
		Pawn->SetMaxShield(100);
		Pawn->SetShield(0);

		static UFunction* OnPlayerPawnResurrected = RebootVan->Class->GetFunction(RebootVan->Class->GetName(), "OnPlayerPawnResurrected");
		RebootVan->ProcessEvent(OnPlayerPawnResurrected, &Pawn);

		AFortPlayerControllerAthena* Instigator = RebootVan->InstigatorPC.Get();
		if (Instigator && Instigator->PlayerState) {
			((AFortPlayerStateAthena*)Instigator->PlayerState)->RebootCounter++;
			((AFortPlayerStateAthena*)Instigator->PlayerState)->OnRep_RebootCounter();
			for (size_t i = 0; i < ((AFortPlayerStateAthena*)Instigator->PlayerState)->Spectators.SpectatorArray.Num(); i++)
			{
				auto& Spectator = ((AFortPlayerStateAthena*)Instigator->PlayerState)->Spectators.SpectatorArray[i];
				if (Spectator.PlayerState == PlayerState)
				{
					((AFortPlayerStateAthena*)Instigator->PlayerState)->Spectators.SpectatorArray.Remove(i);
					break;
				}
			}
			((AFortPlayerStateAthena*)Instigator->PlayerState)->Spectators.MarkArrayDirty();
		}
		PlayerState->Spectators.SpectatorArray.Free();
		PlayerState->Spectators.MarkArrayDirty();
		PlayerState->SpectatingTarget = nullptr;
		PlayerState->OnRep_SpectatingTarget();

		static FName Loot_AthenaSCM = UKismetStringLibrary::Conv_StringToName(TEXT("Loot_AthenaSCM"));
		static auto LootDrops = Looting::PickLootDrops(Loot_AthenaSCM, Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState)->WorldLevel);
		for (auto& Drop : LootDrops)
		{
			Inventory::GiveItem(PC, Drop.ItemDefinition, Drop.Count, Drop.LoadedAmmo);
		}

		AddToAlivePlayers(GameMode, PC);
		RebootVan->PlayerIdsForResurrection.Remove(0);
		if (RebootVan->PlayerIdsForResurrection.Num() <= 0)
		{
			static UFunction* ResurrectComplete = StaticLoadObject<UFunction>("/Game/Athena/Items/EnvironmentalItems/SCMachine/BGA_Athena_SCMachine.BGA_Athena_SCMachine_C.OnPlayerPawnResurrected");
			RebootVan->OnResurrectionCompleted();
			RebootVan->OnPlayerPawnResurrected(NewPawn);
			RebootVan->ProcessEvent(ResurrectComplete, &NewPawn);
		}
	}

	inline void (*ServerAttemptInteractOG)(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalData, EInteractionBeingAttempted InteractionBeingAttempted);
	inline void ServerAttemptInteract(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalData, EInteractionBeingAttempted InteractionBeingAttempted)
	{
		if (!ReceivingActor || !Comp) {
			return;
		}

		ServerAttemptInteractOG(Comp, ReceivingActor, InteractComponent, InteractType, OptionalData, InteractionBeingAttempted);

		Log("ServerAttemptInteract: " + ReceivingActor->GetName());

		AFortPlayerControllerAthena* PC = Cast<AFortPlayerControllerAthena>(Comp->GetOwner());
		if (!PC) {
			return ServerAttemptInteractOG(Comp, ReceivingActor, InteractComponent, InteractType, OptionalData, InteractionBeingAttempted);
		}

		static UClass* AthenaQuestBGAClass = StaticLoadObject<UClass>("/Game/Athena/Items/QuestInteractablesV2/Parents/AthenaQuest_BGA.AthenaQuest_BGA_C");
		static std::map<AFortPlayerControllerAthena*, int> ChestsSearched{};

		if (ReceivingActor->IsA(AFortAthenaSupplyDrop::StaticClass()))
		{
			if (ReceivingActor->GetName().starts_with("AthenaSupplyDrop_Llama_C_"))
			{
				static auto Drop = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaLlama");

				auto LootDrops = Looting::PickLootDrops(Drop);

				auto CorrectLocation = ReceivingActor->K2_GetActorLocation();

				for (auto& LootDrop : LootDrops)
				{
					SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, EFortPickupSourceTypeFlag::Container, EFortPickupSpawnSource::SupplyDrop);
				}

				Quests::GiveAccolade(PC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_020_SearchLlama.AccoladeId_020_SearchLlama"));
			}
			else
			{
				static auto Drop = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaSupplyDrop");

				auto LootDrops = Looting::PickLootDrops(Drop);

				auto CorrectLocation = ReceivingActor->K2_GetActorLocation();

				for (auto& LootDrop : LootDrops)
				{
					SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, EFortPickupSourceTypeFlag::Container, EFortPickupSpawnSource::SupplyDrop);
				}

				Quests::GiveAccolade(PC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_019_SearchSupplyDrop.AccoladeId_019_SearchSupplyDrop"));

				if (!bFirstSupplyDropSearched) {
					bFirstSupplyDropSearched = true;
					Quests::GiveAccolade(PC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_070_FirstSearchedSupplyDrop.AccoladeId_070_FirstSearchedSupplyDrop"));
				}
			}

			ChestsSearched[PC]++;
			Quests::GiveAccolade(PC, GetDefFromEvent(EAccoladeEvent::Search, ChestsSearched[PC], ReceivingActor));
		}
		else if (PC->MyFortPawn && PC->MyFortPawn->IsInVehicle())
		{
			auto Vehicle = PC->MyFortPawn->GetVehicle();

			if (Vehicle)
			{
				auto SeatIdx = PC->MyFortPawn->GetVehicleSeatIndex();
				auto WeaponComp = Vehicle->GetSeatWeaponComponent(SeatIdx);
				if (WeaponComp)
				{
					Inventory::GiveItem(PC, WeaponComp->WeaponSeatDefinitions[SeatIdx].VehicleWeapon, 1, 9999);
					for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
					{
						if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition == WeaponComp->WeaponSeatDefinitions[SeatIdx].VehicleWeapon)
						{
							PC->SwappingItemDefinition = PC->MyFortPawn->CurrentWeapon->WeaponData;
							PC->ServerExecuteInventoryItem(PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid);
							break;
						}
					}
				}
			}
		}
		/*else if (ReceivingActor->IsA(AthenaQuestBGAClass))
		{
			ReceivingActor->ProcessEvent(ReceivingActor->Class->GetFunction("AthenaQuest_BGA_C", "BindToQuestManagerForQuestUpdate"), &PC);
			TArray<UFortQuestItemDefinition*>& QuestsRequiredOnProfile = *(TArray<UFortQuestItemDefinition*>*)(__int64(ReceivingActor) + 0x850);
			FName& Primary_BackendName = *(FName*)(__int64(ReceivingActor) + 0x860);

			Quests::ProgressQuest(PC, QuestsRequiredOnProfile[0], Primary_BackendName);
		}*/ // Super Buggy Sometimes
		else if (ReceivingActor->Class->GetName().contains("Ammo") && !Globals::LateGame) {
			Quests::GiveAccolade(PC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_011_SearchAmmoBox.AccoladeId_011_SearchAmmoBox"));
		}
		else if (ReceivingActor->Class->GetName().contains("FactionChest") && !Globals::LateGame) {
			ChestsSearched[PC]++;
			Quests::GiveAccolade(PC, GetDefFromEvent(EAccoladeEvent::Search, ChestsSearched[PC], ReceivingActor));
			Quests::GiveAccolade(PC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_080_OpenFactionChest.AccoladeId_080_OpenFactionChest"));

			if (!bFirstChestSearched) {
				bFirstChestSearched = true;
				Quests::GiveAccolade(PC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_069_FirstSearchedChest.AccoladeId_069_FirstSearchedChest"));
			}
		}
		else if (ReceivingActor->Class->GetName().contains("Tiered_") && !Globals::LateGame)
		{
			ChestsSearched[PC]++;
			Quests::GiveAccolade(PC, GetDefFromEvent(EAccoladeEvent::Search, ChestsSearched[PC], ReceivingActor));
			Quests::GiveAccolade(PC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_007_SearchChests.AccoladeId_007_SearchChests"));

			if (!bFirstChestSearched) {
				bFirstChestSearched = true;
				Quests::GiveAccolade(PC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_069_FirstSearchedChest.AccoladeId_069_FirstSearchedChest"));
			}
		}
		else if (ReceivingActor->GetName().contains("Wumba"))
		{
			{
				auto UpgradeBench = (AB_Athena_Wumba_C*)ReceivingActor;
				auto WoodCostCurve = UpgradeBench->WoodCostCurve;
				auto HorizontalEnabled = UpgradeBench->HorizontalEnabled;

				static auto WumbaDataTable = StaticLoadObject<UDataTable>("/Game/Items/Datatables/AthenaWumbaData.AthenaWumbaData");
				static auto LootPackagesRowMap = WumbaDataTable->RowMap;

				auto Pawn = PC->MyFortPawn;
				auto CurrentHeldWeapon = Pawn->CurrentWeapon;
				auto CurrentHeldWeaponDef = CurrentHeldWeapon->WeaponData;

				FWeaponUpgradeItemRow* FoundRow = nullptr;

				static auto WoodItemData = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
				static auto StoneItemData = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
				static auto MetalItemData = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/MetalItemData.MetalItemData");

				auto WoodInstance = Inventory::FindItemInstance(PC->WorldInventory, WoodItemData);
				auto StoneInstance = Inventory::FindItemInstance(PC->WorldInventory, StoneItemData);
				auto MetalInstance = Inventory::FindItemInstance(PC->WorldInventory, MetalItemData);

				int WoodCount = WoodInstance ? WoodInstance->ItemEntry.Count : 0;
				int StoneCount = StoneInstance ? StoneInstance->ItemEntry.Count : 0;
				int MetalCount = MetalInstance ? MetalInstance->ItemEntry.Count : 0;

				for (int i = 0; i < LootPackagesRowMap.Elements.Num(); i++) {
					auto& Pair = LootPackagesRowMap.Elements[i];

					auto RowFName = Pair.First;
					if (!RowFName.ComparisonIndex)
						continue;

					auto Row = (FWeaponUpgradeItemRow*)Pair.Second;

					if (InteractionBeingAttempted == EInteractionBeingAttempted::SecondInteraction) {
						if (Row->CurrentWeaponDef == CurrentHeldWeaponDef &&
							Row->Direction == EFortWeaponUpgradeDirection::Horizontal) {
							FoundRow = Row;
							break;
						}
					}
					else {
						if (Row->CurrentWeaponDef == CurrentHeldWeaponDef) {
							FoundRow = Row;
							break;
						}
					}
				}

				if (!FoundRow)
					return;

				auto NewDefinition = FoundRow->UpgradedWeaponDef;

				int WoodCost = static_cast<int>(FoundRow->WoodCost) * 50;
				int StoneCost = static_cast<int>(FoundRow->BrickCost) * 50;
				int MetalCost = static_cast<int>(FoundRow->MetalCost) * 50;

				if (!PC->bInfiniteAmmo) {
					if (FoundRow->Direction == EFortWeaponUpgradeDirection::Vertical) {
						int brick = max(0, int(FoundRow->BrickCost) - 8);
						int metal = max(0, int(FoundRow->MetalCost) - 4);

						Inventory::RemoveItem(PC, WoodInstance->ItemEntry.ItemDefinition, WoodCost);
						Inventory::RemoveItem(PC, StoneInstance->ItemEntry.ItemDefinition, brick * 50);
						Inventory::RemoveItem(PC, MetalInstance->ItemEntry.ItemDefinition, metal * 50);
					}
					else {
						Inventory::RemoveItem(PC, WoodInstance->ItemEntry.ItemDefinition, 20);
						Inventory::RemoveItem(PC, StoneInstance->ItemEntry.ItemDefinition, 20);
						Inventory::RemoveItem(PC, MetalInstance->ItemEntry.ItemDefinition, 20);
					}
				}

				Inventory::RemoveItem(PC, CurrentHeldWeapon->ItemEntryGuid, 1);
				Inventory::GiveItem(PC, NewDefinition, 1, CurrentHeldWeapon->GetMagazineAmmoCount());
			}

		}
		else if (ReceivingActor->IsA(ABGA_Athena_Keycard_Lock_Parent_C::StaticClass()))
		{
			// Commented because K2_RemoveItemFromPlayer should remove
			/*UFortItemDefinition* Def = *(UFortItemDefinition**)(__int64(ReceivingActor) + 0x8F0);// this took long to find
			if (Inventory::FindItemEntry(PC, Def)) {
				Inventory::RemoveItem(PC, Def, 1);
			}*/
			Quests::GiveAccolade(PC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_079_OpenVault.AccoladeId_079_OpenVault"));

		}
	}

	//taken from ero
	int64_t(*ApplyCostOG)(UGameplayAbility* arg1, int32_t arg2, void* arg3, void* arg4);
	int64_t ApplyCost(UFortGameplayAbility* arg1, int32_t arg2, void* arg3, void* arg4)
	{
		auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
		if (arg1->GetName().starts_with("GA_Athena_AppleSun_Passive_C_")) {
			auto Def = StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/AppleSun/WID_Athena_AppleSun.WID_Athena_AppleSun");
			auto ASC = arg1->GetActivatingAbilityComponent();
			AFortPlayerStateAthena* PS = (AFortPlayerStateAthena*)ASC->GetOwner();
			auto Pawn = PS->GetCurrentPawn();
			AFortPlayerController* PC = nullptr;
			PC = (AFortPlayerController*)Pawn->GetOwner();

			if (!PC->bInfiniteAmmo) {
				Inventory::RemoveItem(PC, Def, 1);
			}
		}

		return ApplyCostOG(arg1, arg2, arg3, arg4);
	}

	void (*OnCapsuleBeginOverlapOG)(AFortPlayerPawn* Pawn, UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, FHitResult SweepResult);
	void OnCapsuleBeginOverlap(AFortPlayerPawn* Pawn, UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, FHitResult SweepResult)
	{
		if (OtherActor->IsA(AFortPickup::StaticClass()))
		{
			AFortPickup* Pickup = (AFortPickup*)OtherActor;

			if (Pickup->PawnWhoDroppedPickup == Pawn)
				return OnCapsuleBeginOverlapOG(Pawn, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

			UFortWorldItemDefinition* Def = (UFortWorldItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition;

			if (!Def) {
				return OnCapsuleBeginOverlapOG(Pawn, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
			}

			auto PC = (AFortPlayerControllerAthena*)Pawn->GetOwner();
			FFortItemEntry* FoundEntry = nullptr;
			auto HighestCount = 0;

			if (PC->IsA(ABP_PhoebePlayerController_C::StaticClass())) return OnCapsuleBeginOverlapOG(Pawn, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
			for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
			{
				FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

				if (Entry.ItemDefinition == Def && (Entry.Count <= Inventory::GetMaxStackSize(Def)))
				{
					FoundEntry = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
					if (Entry.Count > HighestCount)
						HighestCount = Entry.Count;
				}
			}
			if (Def->IsA(UFortAmmoItemDefinition::StaticClass()) || Def->IsA(UFortResourceItemDefinition::StaticClass()) || Def->IsA(UFortTrapItemDefinition::StaticClass())) {
				if (FoundEntry && HighestCount < Inventory::GetMaxStackSize(Def)) {
					Pawn->ServerHandlePickup(Pickup, 0.40f, FVector(), true);
				}
				else if (!FoundEntry) {
					Pawn->ServerHandlePickup(Pickup, 0.40f, FVector(), true);
				}
			}
			else if (FoundEntry)
			{
				if (FoundEntry->Count < Inventory::GetMaxStackSize(Def)) {
					Pawn->ServerHandlePickup(Pickup, 0.40f, FVector(), true);
				}
			}
		}

		return OnCapsuleBeginOverlapOG(Pawn, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	}

	//taken from ero
	__int64 (*OnExplodedOG)(AB_Prj_Athena_Consumable_Thrown_C* Consumable, TArray<class AActor*>& HitActors, TArray<struct FHitResult>& HitResults) = decltype(OnExplodedOG)(__int64(GetModuleHandleA(0)) + 0x3EA2740);
	__int64 OnExploded(AB_Prj_Athena_Consumable_Thrown_C* Consumable, TArray<class AActor*>& HitActors, TArray<struct FHitResult>& HitResults)
	{
		if (!Consumable)
			return OnExplodedOG(Consumable, HitActors, HitResults);

		if (Consumable->GetName() == "B_Prj_Lotus_Mustache_C") {
			auto PC = Consumable->GetOwnerPlayerController();
			auto Pawn = PC->MyFortPawn;
			UFortItemDefinition* Deff = StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/Bandage/Athena_Bandage.Athena_Bandage");
			SpawnPickup(Deff, 1, 0, Consumable->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset, Pawn);
		}
		else if (Consumable->GetName() == "B_Prj_Athena_Bucket_Old_C" || Consumable->GetName() == "B_Prj_Athena_Bucket_Nice_C") {
			auto PC = Consumable->GetOwnerPlayerController();
			auto Pawn = PC->MyFortPawn;
			auto Def = *(UFortItemDefinition**)(__int64(Consumable) + 0x888);
			if (!Def)
				return OnExplodedOG(Consumable, HitActors, HitResults);

			AFortPickupAthena* NewPickup = Actors<AFortPickupAthena>(AFortPickupAthena::StaticClass(), Consumable->K2_GetActorLocation());
			NewPickup->bRandomRotation = true;
			NewPickup->PrimaryPickupItemEntry.ItemDefinition = Def;
			NewPickup->PrimaryPickupItemEntry.Count = 1;
			NewPickup->PrimaryPickupItemEntry.LoadedAmmo = 1;
			NewPickup->OnRep_PrimaryPickupItemEntry();
			NewPickup->PawnWhoDroppedPickup = Pawn;
			Pawn->ServerHandlePickup(NewPickup, 0.40f, FVector(), false);
		}
		if (!Consumable->GetName().starts_with("B_Prj_Athena_Consumable_Thrown_")) {
			return OnExplodedOG(Consumable, HitActors, HitResults);
		}
		auto Def = *(UFortItemDefinition**)(__int64(Consumable) + 0x888);
		if (!Def)
			return OnExplodedOG(Consumable, HitActors, HitResults);
		SpawnPickup(Def, 1, 0, Consumable->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset);
		Consumable->K2_DestroyActor();
		return OnExplodedOG(Consumable, HitActors, HitResults);
	}

	void ServerPlayEmoteItem(AFortPlayerControllerAthena* PC, UFortMontageItemDefinitionBase* EmoteAsset, float EmoteRandomNumber) {
		Log("ServerPlayEmoteItem Called!");

		if (!PC || !EmoteAsset)
			return;

		if (GameState->GamePhase == EAthenaGamePhase::Aircraft) {
			return;
		}

		UClass* DanceAbility = StaticLoadObject<UClass>("/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");
		UClass* SprayAbility = StaticLoadObject<UClass>("/Game/Abilities/Sprays/GAB_Spray_Generic.GAB_Spray_Generic_C");

		if (!DanceAbility || !SprayAbility)
			return;

		auto EmoteDef = (UAthenaDanceItemDefinition*)EmoteAsset;
		if (!EmoteDef)
			return;

		PC->MyFortPawn->bMovingEmote = EmoteDef->bMovingEmote;
		PC->MyFortPawn->EmoteWalkSpeed = EmoteDef->WalkForwardSpeed;
		PC->MyFortPawn->bMovingEmoteForwardOnly = EmoteDef->bMoveForwardOnly;
		PC->MyFortPawn->EmoteWalkSpeed = EmoteDef->WalkForwardSpeed;

		FGameplayAbilitySpec Spec{};
		UGameplayAbility* Ability = nullptr;

		if (EmoteAsset->IsA(UAthenaSprayItemDefinition::StaticClass()))
		{
			Ability = (UGameplayAbility*)SprayAbility->DefaultObject;
		}

		if (Ability == nullptr) {
			Ability = (UGameplayAbility*)DanceAbility->DefaultObject;
		}

		AbilitySpecConstructor(&Spec, Ability, 1, -1, EmoteDef);
		GiveAbilityAndActivateOnce(((AFortPlayerStateAthena*)PC->PlayerState)->AbilitySystemComponent, &Spec.Handle, Spec);
	}

	void ServerPlaySquadQuickChatMessage(AFortPlayerControllerAthena* PC, FAthenaQuickChatActiveEntry ChatEntry, __int64) {
		Log("ServerPlaySquadQuickChatMessage Called!");

		static ETeamMemberState MemberStates[10] = {
			ETeamMemberState::ChatBubble,
			ETeamMemberState::EnemySpotted,
			ETeamMemberState::NeedMaterials,
			ETeamMemberState::NeedBandages,
			ETeamMemberState::NeedShields,
			ETeamMemberState::NeedAmmoHeavy,
			ETeamMemberState::NeedAmmoLight,
			ETeamMemberState::FIRST_CHAT_MESSAGE,
			ETeamMemberState::NeedAmmoMedium,
			ETeamMemberState::NeedAmmoShells
		};

		auto PlayerState = reinterpret_cast<AFortPlayerStateAthena*>(PC->PlayerState);

		PlayerState->ReplicatedTeamMemberState = MemberStates[ChatEntry.Index];

		PlayerState->OnRep_ReplicatedTeamMemberState();

		static auto EmojiComm = StaticFindObject<UAthenaEmojiItemDefinition>(L"/Game/Athena/Items/Cosmetics/Dances/Emoji/Emoji_Comm.Emoji_Comm");

		if (EmojiComm)
		{
			PC->ServerPlayEmoteItem(EmojiComm, 0);
		}
	}

	void (*MovingEmoteStoppedOG)(AFortPawn* Pawn);
	void MovingEmoteStopped(AFortPawn* Pawn)
	{
		if (!Pawn)
			return;

		Pawn->bMovingEmote = false;
		Pawn->bMovingEmoteFollowingOnly = false;

		return MovingEmoteStoppedOG(Pawn);
	}

	static inline void(*OrginalServerSetInAircraft)(AFortPlayerStateAthena* PlayerState, bool bNewInAircraft);
	void ServerSetInAircraft(AFortPlayerStateAthena* PlayerState, bool bNewInAircraft)
	{

		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)PlayerState->Owner;

		if (PC && PC->WorldInventory)
		{
			for (int i = PC->WorldInventory->Inventory.ReplicatedEntries.Num() - 1; i >= 0; i--)
			{
				if (((UFortWorldItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped && !GivenLootPlayers.Contains(PC) && Globals::LateGame)
				{
					int Count = PC->WorldInventory->Inventory.ReplicatedEntries[i].Count;
					Inventory::RemoveItem(PC, PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Count);
				}
				else if (((UFortWorldItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped && !Globals::LateGame)
				{
					int Count = PC->WorldInventory->Inventory.ReplicatedEntries[i].Count;
					Inventory::RemoveItem(PC, PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Count);
				}
			}
		}

		if (Globals::bEventEnabled)
		{
			AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

			AActor* BattleBus = GameState->GetAircraft(0);
			auto Aircraft = GameState->GetAircraft(0);
			FVector Loc = FVector(62590, -75501, 13982);
			Aircraft->FlightInfo.FlightStartLocation = (FVector_NetQuantize100)Loc;
			Aircraft->FlightInfo.FlightSpeed = 1000;
			Aircraft->FlightInfo.TimeTillDropStart = 1;
			Aircraft->DropStartTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 1;
			GameState->bAircraftIsLocked = false;
		}

		if (Globals::LateGame)
		{
			auto GameState = (AFortGameStateAthena*)UEngine::GetEngine()->GameViewport->World->GameState;
			auto GameMode = (AFortGameModeAthena*)UEngine::GetEngine()->GameViewport->World->AuthorityGameMode;
			FVector BattleBusLocation = GameMode->SafeZoneLocations[4];
			BattleBusLocation.Z += 15000;
			auto Aircraft = GameState->GetAircraft(0);

			float Speed = 1000;
			float Distance = Speed * 1.5f; // Speed x time
			FVector Direction = FVector(1, 0, 0);
			Direction.Normalize();

			FVector StartLocation = BattleBusLocation;
			FVector EndLocation = StartLocation + (Direction * Distance);

			if (Aircraft)
			{
				Aircraft->FlightInfo.FlightSpeed = Speed;
				Aircraft->FlightInfo.FlightStartLocation = FVector_NetQuantize100(StartLocation);
				Aircraft->FlightInfo.TimeTillFlightEnd = 1.5f;
				Aircraft->ExitLocation = EndLocation;
				GameState->bAircraftIsLocked = false;
			}

			if (!GivenLootPlayers.Contains(PC))
			{
				Inventory::GiveLoadout(PC);
				GivenLootPlayers.Add(PC);
			}

			std::thread(Misc::LateGameAircraftThread, BattleBusLocation).detach();
		}



		return OrginalServerSetInAircraft(PlayerState, bNewInAircraft);
	}

	void ServerReviveFromDBNO(AFortPlayerPawnAthena* Pawn, AFortPlayerControllerAthena* Instigator)
	{
		float ServerTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

		if (!Pawn || !Instigator)
			return;

		// TODO: RevivePlayer accolades (022, 023, 024, 025)
		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;
		if (!PC || !PC->PlayerState)
			return;
		auto PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		auto AbilitySystemComp = (UFortAbilitySystemComponentAthena*)PlayerState->AbilitySystemComponent;

		FGameplayEventData Data{};
		Data.EventTag = Pawn->EventReviveTag;
		Data.ContextHandle = PlayerState->AbilitySystemComponent->MakeEffectContext();
		Data.Instigator = Instigator;
		Data.Target = Pawn;
		Data.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Pawn);
		Data.TargetTags = Pawn->GameplayTags;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, Pawn->EventReviveTag, Data);

		for (auto& Ability : AbilitySystemComp->ActivatableAbilities.Items)
		{
			if (Ability.Ability->Class == UGAB_AthenaDBNO_C::StaticClass())
			{
				AbilitySystemComp->ServerCancelAbility(Ability.Handle, Ability.ActivationInfo);
				AbilitySystemComp->ServerEndAbility(Ability.Handle, Ability.ActivationInfo, Ability.ActivationInfo.PredictionKeyWhenActivated);
				AbilitySystemComp->ClientCancelAbility(Ability.Handle, Ability.ActivationInfo);
				AbilitySystemComp->ClientEndAbility(Ability.Handle, Ability.ActivationInfo);
				break;
			}
		}

		Pawn->bIsDBNO = false;
		PC->bMarkedAlive = true;
		Pawn->bPlayedDying = false;
		Pawn->bIsDying = false;
		Pawn->DBNORevivalStacking = 0;
		Pawn->OnRep_IsDBNO();
		Pawn->SetHealth(30);
		PlayerState->DeathInfo = {};
		PlayerState->OnRep_DeathInfo();

		PC->ClientOnPawnRevived(Instigator);
	}

	inline void (*ServerAttemptExitVehicleOG)(AFortPlayerController* PC);
	inline void ServerAttemptExitVehicle(AFortPlayerControllerZone* PC)
	{
		if (!PC)
			return;

		auto Pawn = (AFortPlayerPawn*)PC->Pawn;

		ServerAttemptExitVehicleOG(PC);

		if (!Pawn->CurrentWeapon || !Pawn->CurrentWeapon->IsA(AFortWeaponRangedForVehicle::StaticClass()))
			return;

		Inventory::RemoveItem((AFortPlayerController*)Pawn->Controller, Pawn->CurrentWeapon->GetInventoryGUID(), 1);

		UFortWorldItemDefinition* SwappingItemDef = ((AFortPlayerControllerAthena*)PC)->SwappingItemDefinition;
		if (!SwappingItemDef)
			return;

		FFortItemEntry* SwappingItemEntry = Inventory::FindItemEntry(PC, SwappingItemDef);
		if (!SwappingItemEntry)
			return;

		PC->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)SwappingItemDef, SwappingItemEntry->ItemGuid);
	}

	void ServerClientIsReadyToRespawn(AFortPlayerControllerAthena* PC)
	{
		auto PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;

		auto GameMode = (AFortGameModeAthena*)UEngine::GetEngine()->GameViewport->World->AuthorityGameMode;

		if (PlayerState->RespawnData.bRespawnDataAvailable && PlayerState->RespawnData.bServerIsReady)
		{
			PlayerState->RespawnData.bClientIsReady = true;

			FTransform Transform{};
			Transform.Translation = PlayerState->RespawnData.RespawnLocation;
			Transform.Scale3D = FVector{ 1,1,1 };
			auto Pawn = (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(PC, Transform);
			PC->Possess(Pawn);
			Pawn->SetMaxHealth(100);
			Pawn->SetHealth(100);
			Pawn->SetMaxShield(100);
			Pawn->SetShield(0);
			PC->RespawnPlayerAfterDeath(true);
		}
	}

	void ServerReturnToMainMenu(AFortPlayerControllerAthena* PC)
	{
		PC->ClientReturnToMainMenu(L"");
	}

	void ServerCheat(AFortPlayerControllerAthena* PC, FString& Msg) {
		if (Globals::bIsProdServer)
			return;

		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
		auto Gamemode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
		
		std::string Command = Msg.ToString();
		Log(Command);

		if (Command == "SpawnBot") {
			if (GameState->GamePhase > EAthenaGamePhase::Warmup) {
				PlayerBots::SpawnPlayerBots(PC->Pawn, EBotState::Landed);
			}
			Log("Spawned Bot!");
		}
		else if (Command == "SpawnFriendlyBot") {
			if (GameState->GamePhase > EAthenaGamePhase::Warmup) {
				PlayerBots::SpawnPlayerBots(PC->Pawn, EBotState::Landed, PC);
			}
			Log("Spawned A Friendly Bot!");
		}
		else if (Command.contains("SpawnAmountBots ")) {
			std::vector<std::string> args = TextManipUtils::SplitWhitespace(Command);
			int AmountToSpawn = std::stoi(args[1]);
			for (int i = 0; i < AmountToSpawn; i++) {
				if (GameState->GamePhase > EAthenaGamePhase::Warmup) {
					PlayerBots::SpawnPlayerBots(PC->Pawn, EBotState::Landed);
				}
			}
		}
		else if (Command == "GodMode") {
			if (!PC->MyFortPawn->bIsInvulnerable) {
				PC->MyFortPawn->bIsInvulnerable = true;
			}
			else {
				PC->MyFortPawn->bIsInvulnerable = false;
			}
		}
		else if (Command == "DumpLoc") {
			FVector Loc = PC->Pawn->K2_GetActorLocation();
			Log("X: " + std::to_string(Loc.X));
			Log("Y: " + std::to_string(Loc.Y));
			Log("Z: " + std::to_string(Loc.Z));
		}
		else if (Command.contains("Teleport ")) {
			std::vector<std::string> args = TextManipUtils::SplitWhitespace(Command);
			FVector TeleportLoc = FVector();

			TeleportLoc.X = std::stoi(args[1]);
			TeleportLoc.Y = std::stoi(args[2]);
			TeleportLoc.Z = std::stoi(args[3]);

			PC->Pawn->K2_TeleportTo(TeleportLoc, PC->Pawn->K2_GetActorRotation());
			Log("Teleported: X: " + args[1] + " Y: " + args[2] + " Z: " + args[3]);
		}
		else if (Command == "StartEvent")
		{
			UFunction* StartEventFunc = GameMode::Event::JerkyLoader->Class->GetFunction("BP_Jerky_Loader_C", "startevent");

			float ToStart = 0.f;
			GameMode::Event::JerkyLoader->ProcessEvent(StartEventFunc, &ToStart);
		}
		else if (Command == "startaircraft")
		{
			UKismetSystemLibrary::GetDefaultObj()->ExecuteConsoleCommand(UWorld::GetWorld(), TEXT("startaircraft"), nullptr);
		}
		else if (Command == "pausesafezone")
		{
			UKismetSystemLibrary::GetDefaultObj()->ExecuteConsoleCommand(UWorld::GetWorld(), TEXT("pausesafezone"), nullptr);
		}
	}

	void Hook() {
		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x269, ServerReadyToStartMatch, (LPVOID*)&ServerReadyToStartMatchOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x10D, ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x26B, ServerLoadingScreenDropped, (LPVOID*)&ServerLoadingScreenDroppedOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x4F0, ServerClientIsReadyToRespawn, nullptr);

		HookVTable(UFortControllerComponent_Aircraft::GetDefaultObj(), 0x89, ServerAttemptAircraftJump, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x29B5C80), ClientOnPawnDied, (LPVOID*)&ClientOnPawnDiedOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x1C8EA00), ServerAttemptInteract, (LPVOID*)&ServerAttemptInteractOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x1583360), ApplyCost, (LPVOID*)&ApplyCostOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x22A08C0), OnCapsuleBeginOverlap, (LPVOID*)&OnCapsuleBeginOverlapOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x1C7, ServerPlayEmoteItem, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x197f6d0), ServerPlaySquadQuickChatMessage, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x1b6c232), RebootingDelegate, nullptr);

		HookVTable(AFortPlayerStateAthena::StaticClass()->DefaultObject, 0xFF, ServerSetInAircraft, (PVOID*)&OrginalServerSetInAircraft);

		HookVTable(APlayerPawn_Athena_C::StaticClass()->DefaultObject, 0x1D2, ServerReviveFromDBNO, nullptr);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x265, ServerReturnToMainMenu, nullptr);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x1C5, ServerCheat, nullptr);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x41E, ServerAttemptExitVehicle, (PVOID*)&ServerAttemptExitVehicleOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x6853B0), MovingEmoteStopped, (LPVOID*)&MovingEmoteStoppedOG);

		//MH_CreateHook((LPVOID)(ImageBase + 0xF703E0), ServerCheat, nullptr);

		for (size_t i = 0; i < UObject::GObjects->Num(); i++)
		{
			UObject* Obj = UObject::GObjects->GetByIndex(i);
			if (Obj && Obj->IsA(AB_Prj_Athena_Consumable_Thrown_C::StaticClass()))
			{
				HookVTable(Obj->Class->DefaultObject, 0x44, OnExploded, nullptr);
			}
		}

		Log("PC Hooked!");
	}
}