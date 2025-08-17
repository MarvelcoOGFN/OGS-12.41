#include "framework.h"
#include "GameMode.h"
#include "Abilities.h"
#include "PC.h"
#include "Inventory.h"
#include "Building.h"
#include "Looting.h"
#include "Quests.h"
#include "Misc.h"
#include "Net.h"
#include "Tick.h"
#include "Bots.h"
#include "Creative.h"
#include "PE.h"

void InitConsole() {
    AllocConsole();
    FILE* fptr;
    freopen_s(&fptr, "CONOUT$", "w+", stdout);
    SetConsoleTitleA("OGS 12.41 | Starting...");
    Log("Welcome to MOGS, Made with love by ObsessedTech!");
}

void LoadWorld() {
    Log("Loading World!");
    if (!Globals::bCreativeEnabled && !Globals::bSTWEnabled) {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"open Apollo_Terrain", nullptr);
    }
    else if (Globals::bCreativeEnabled) {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"open Creative_NoApollo_Terrain", nullptr);
    }
    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
}

void Hook() {
    GameMode::Hook();
    PC::Hook();
    Abilities::Hook();
    Inventory::Hook();
    Building::Hook();
    Looting::Hook();
    Quests::Hook();

    Misc::Hook();
    Net::Hook();
    Tick::Hook();

    Bots::Hook();
    Creative::Hook();

    PE::Hook();

    MH_EnableHook(MH_ALL_HOOKS);
}

static void WaitForLogin() {
    Log("Waiting for login!");

    FName Frontend = UKismetStringLibrary::Conv_StringToName(L"Frontend");
    FName MatchState = UKismetStringLibrary::Conv_StringToName(L"InProgress");

    while (true) {
        UWorld* CurrentWorld = ((UWorld*)UWorld::GetWorld());
        if (CurrentWorld) {
            if (CurrentWorld->Name == Frontend) {
                auto GameMode = (AGameMode*)CurrentWorld->AuthorityGameMode;
                if (GameMode->GetMatchState() == MatchState) {
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 1));
}

DWORD Main(LPVOID) {
    InitConsole();
    MH_Initialize();
    Log("MinHook Initialised!");

    while (UEngine::GetEngine() == 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    WaitForLogin();

    Hook();

    *(bool*)(ImageBase + 0x804B659) = false; //GIsClient
    *(bool*)(ImageBase + 0x804B65A) = true; //GIsServer

    while (UWorld::GetWorld() == nullptr)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    while (UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Num() == 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    Sleep(1000);
    LoadWorld();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, Main, 0, 0, 0);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
