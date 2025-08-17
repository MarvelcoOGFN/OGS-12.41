#pragma once
#include "framework.h"
#include "Globals.h"
#include "GameMode.h"
#include "Quests.h"
#include "Bots.h"

namespace AccoladeTickingService {
	float NextPlaytimeXPDrop = 0.f;

	// Survival Accolades
	bool AccoladeId_026_Survival_Default_Bronze = false;
	bool AccoladeId_027_Survival_Default_Silver = false;
	bool AccoladeId_028_Survival_Default_Gold = false;
	//arena
	bool Placement25 = false;
	bool Placement5 = false;

	// first player to land accolade
	bool AccoladeId_018_First_Landing = false;

	void Tick(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState) {
		float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());

		if (NextPlaytimeXPDrop == 0.f) {
			NextPlaytimeXPDrop = CurrentTime + 120.f;
		}

		if (CurrentTime >= NextPlaytimeXPDrop) { // Broken or smth idk
			NextPlaytimeXPDrop = CurrentTime + 120.f; // 2 Minute interval
			Log("Playtime XP!");
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_054_Playtime.AccoladeId_054_Playtime"));
			}
		}

		int32 AliveCount = GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num();

		if (AliveCount == 50 && !AccoladeId_026_Survival_Default_Bronze)
		{
			AccoladeId_026_Survival_Default_Bronze = true;
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_026_Survival_Default_Bronze.AccoladeId_026_Survival_Default_Bronze"));

			}
		}


		if (AliveCount == 25 && !AccoladeId_027_Survival_Default_Silver)
		{
			AccoladeId_027_Survival_Default_Silver = true;
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_027_Survival_Default_Silver.AccoladeId_027_Survival_Default_Silver"));

			}
		}
		if (AliveCount == 10 && !AccoladeId_028_Survival_Default_Gold)
		{
			AccoladeId_028_Survival_Default_Gold = true;
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_028_Survival_Default_Gold.AccoladeId_028_Survival_Default_Gold"));
			}
		}

		if (Globals::Arena)
		{


		    if (AliveCount == 25 && !Placement25)
			{
				Placement25 = true;
				for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
				{
					GameMode->AlivePlayers[i]->ClientReportTournamentPlacementPointsScored(25, 60);

				}
			}

			else if (AliveCount == 5 && !Placement5)
			{
				Placement5 = true;
				for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
				{
					GameMode->AlivePlayers[i]->ClientReportTournamentPlacementPointsScored(5, 30);

				}
			}

		}

		for (AFortPlayerControllerAthena* Player : GameMode->AlivePlayers) { 
			// KillStreak Accolades!
			if (CurrentTime <= Player->LastKillTimeWindow && Player->KillsInKillTimeWindow != Player->LastRecordedKillsInKillTimeWindow) {
				if (Player->KillsInKillTimeWindow == 2) {
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_002_DoubleKill.AccoladeId_002_DoubleKill"));
				}
				else if (Player->KillsInKillTimeWindow == 3) {
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_003_TrippleKill.AccoladeId_003_TrippleKill"));
				}
				else if (Player->KillsInKillTimeWindow == 4) {
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_004_QuadraKill.AccoladeId_004_QuadraKill"));
				}
				else if (Player->KillsInKillTimeWindow == 5) {
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_005_PentaKill.AccoladeId_005_PentaKill"));
				}
				else if (Player->KillsInKillTimeWindow == 6) {
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_006_MonsterKill.AccoladeId_006_MonsterKill"));
				}

				Player->LastRecordedKillsInKillTimeWindow = Player->KillsInKillTimeWindow;
			}

			// Maybe proper?
			if (!AccoladeId_018_First_Landing && !Player->IsInAircraft()) {
				FVector Vel = Player->Pawn->GetVelocity();
				float Speed = Vel.Z;
				if (Speed == 0.f) {
					AccoladeId_018_First_Landing = true;
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_018_First_Landing.AccoladeId_018_First_Landing"));

					// Eh give discover landmark and poi accolade cuz idk how to track it either (for ones idk how to track i will put them in the proper places when i figure it out)
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_032_DiscoverLandmark.AccoladeId_032_DiscoverLandmark"));
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_033_DiscoverPOI.AccoladeId_033_DiscoverPOI"));
				}
			}
		}
	}
}

namespace Tick {
	void (*ServerReplicateActors)(void*) = decltype(ServerReplicateActors)(ImageBase + 0x1023F60);

	inline void (*TickFlushOG)(UNetDriver*, float);
	void TickFlush(UNetDriver* Driver, float DeltaTime)
	{
		if (!Driver)
			return;

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		if (!Driver->ReplicationDriver)
		{
			Log("ReplicationDriver Doesent Exist!");
		}

		ServerReplicateActors(Driver->ReplicationDriver);

		if (GameState->GamePhase == EAthenaGamePhase::Warmup
			&& (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num()) >= Globals::MinPlayersForEarlyStart
			&& GameState->WarmupCountdownEndTime > UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 10.f) {

			auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			auto DR = 10.f;

			GameState->WarmupCountdownEndTime = TS + DR;
			GameMode->WarmupCountdownDuration = DR;
			GameState->WarmupCountdownStartTime = TS;
			GameMode->WarmupEarlyCountdownDuration = DR;
		}

		if (Globals::bBotsEnabled && !Globals::bEventEnabled) {
			static bool InitialisedPlayerStarts = false;
			if (!InitialisedPlayerStarts)
			{
				UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);
				InitialisedPlayerStarts = true;
			}

			if (((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->GamePhase == EAthenaGamePhase::Warmup &&
				GameMode->AlivePlayers.Num() > 0
				&& (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num()) < GameMode->GameSession->MaxPlayers
				&& GameMode->AliveBots.Num() < Globals::MaxBotsToSpawn
				&& GameState->WarmupCountdownEndTime > UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()))
			{
				if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.05f))
				{
					AActor* SpawnLocator = PlayerStarts[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, PlayerStarts.Num() - 1)];

					if (SpawnLocator)
					{
						PlayerBots::SpawnPlayerBots(SpawnLocator);
					}
				}
			}
		}

		if (GameState->WarmupCountdownEndTime - UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) <= 0 && GameState->GamePhase == EAthenaGamePhase::Warmup)
		{
			GameMode::StartAircraftPhase(GameMode, 0);
		}

		if (GameState->GamePhase > EAthenaGamePhase::Warmup && !Globals::bCreativeEnabled) {
			AccoladeTickingService::Tick(GameMode, GameState);
		}

		if (Globals::bBossesEnabled && !Globals::bEventEnabled && GameState->GamePhase > EAthenaGamePhase::Warmup)
		{
			Bosses::TickBots(DeltaTime);
		}

		if (Globals::bBotsEnabled && !Globals::bEventEnabled) {
			PlayerBots::Tick();
		}

		return TickFlushOG(Driver, DeltaTime);
	}

	inline float GetMaxTickRate()
	{
		return 30.f;
	}

	void Hook() {
		MH_CreateHook((LPVOID)(ImageBase + 0x42C3ED0), TickFlush, (LPVOID*)&TickFlushOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x4576310), GetMaxTickRate, nullptr);

		Log("Tick Hooked!");
	}
}