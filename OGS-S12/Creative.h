#pragma once
#include "framework.h"

namespace Creative {
    void ServerTeleportToPlaygroundLobbyIsland(AFortPlayerController* PC)
    {
        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
        auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
        auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

        if (PC)
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
                    PC->Pawn->K2_TeleportTo(StartSpot->K2_GetActorLocation(), StartSpot->K2_GetActorRotation());
                }
            }
        }
        else {
            Log("PlayerController Doesent Exist!");
        }
    }

    void Hook() {
        //HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x4E6, ServerTeleportToPlaygroundLobbyIsland, nullptr);
        //MH_CreateHook((LPVOID)(ImageBase + 0x2CB1710), ServerTeleportToPlaygroundLobbyIsland, nullptr);

        Log("Hooked Creative!");
    }
}
