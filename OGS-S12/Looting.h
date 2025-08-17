#pragma once
#include "framework.h"
#include "Globals.h"
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <string>
#include <cstdlib>
#include <ctime> 

// Credits: MGS

namespace Looting {
    void SpawnLlamas()
    {
        int LlamasSpawned = 0;
        auto LlamasToSpawn = (rand() % 3) + 3;
        Log(std::string("Spawned ") + std::to_string(LlamasToSpawn) + " Llamas");

        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

        auto MapInfo = GameState->MapInfo;

        for (int i = 0; i < LlamasToSpawn; i++)
        {
            int Radius = 100000;
            FVector Location = PickSupplyDropLocation(MapInfo, FVector(1, 1, 10000), (float)Radius);

            FRotator RandomYawRotator{};
            RandomYawRotator.Yaw = (float)rand() * 0.010986663f;

            FTransform InitialSpawnTransform{};
            InitialSpawnTransform.Translation = Location;
            InitialSpawnTransform.Rotation = FRotToQuat(RandomYawRotator);
            InitialSpawnTransform.Scale3D = FVector(1, 1, 1);

            auto LlamaStart = SpawnActor<AFortAthenaSupplyDrop>(Location, RandomYawRotator, nullptr, MapInfo->LlamaClass.Get());

            if (!LlamaStart)
                continue;

            auto GroundLocation = LlamaStart->FindGroundLocationAt(InitialSpawnTransform.Translation);

            LlamaStart->K2_DestroyActor();

            auto Llama = SpawnActor<AFortAthenaSupplyDrop>(GroundLocation, RandomYawRotator, nullptr, MapInfo->LlamaClass.Get());

            Llama->bCanBeDamaged = false;

            if (!Llama)
                continue;
            LlamasSpawned++;
        }
    }

    static FFortLootTierData* GetLootTierData(std::vector<FFortLootTierData*>& LootTierData)
    {
        float TotalWeight = 0;

        for (auto Item : LootTierData)
            TotalWeight += Item->Weight;

        float RandomNumber = UKismetMathLibrary::RandomFloatInRange(0, TotalWeight);

        FFortLootTierData* SelectedItem = nullptr;


        for (auto Item : LootTierData)
        {

            if (RandomNumber <= Item->Weight)
            {
                SelectedItem = Item;
                break;
            }

            RandomNumber -= Item->Weight;
        }

        if (!SelectedItem)
            return GetLootTierData(LootTierData);

        return SelectedItem;
    }

    static FFortLootPackageData* GetLootPackage(std::vector<FFortLootPackageData*>& LootPackages)
    {
        float TotalWeight = 0;

        for (auto Item : LootPackages)
            TotalWeight += Item->Weight;

        float RandomNumber = UKismetMathLibrary::RandomFloatInRange(0, TotalWeight);

        FFortLootPackageData* SelectedItem = nullptr;

        for (auto Item : LootPackages)
        {
            if (RandomNumber <= Item->Weight)
            {
                SelectedItem = Item;
                break;
            }

            RandomNumber -= Item->Weight;
        }

        if (!SelectedItem)
            return GetLootPackage(LootPackages);

        return SelectedItem;
    }

    int GetClipSize(UFortItemDefinition* ItemDef) {
        if (auto RangedDef = Cast<UFortWeaponRangedItemDefinition>(ItemDef)) {
            auto DataTable = RangedDef->WeaponStatHandle.DataTable;
            auto RowName = RangedDef->WeaponStatHandle.RowName;

            if (DataTable && RowName.ComparisonIndex) {
                auto& RowMap = *(TMap<FName, FFortRangedWeaponStats*>*)(__int64(DataTable) + 0x30);

                for (auto& Pair : RowMap) {
                    FName CurrentRowName = Pair.Key();
                    FFortRangedWeaponStats* PackageData = Pair.Value();

                    if (CurrentRowName == RowName && PackageData) {
                        return PackageData->ClipSize;
                    }
                }
            }
        }

        return 0;
    }

    std::vector<FFortItemEntry> PickLootDrops(FName TierGroupName, int recursive = 0)
    {

        std::vector<FFortItemEntry> LootDrops;

        if (recursive >= 5)
        {
            return LootDrops;
        }

        auto TierGroupFName = TierGroupName;

        static std::vector<UDataTable*> LTDTables;
        static std::vector<UDataTable*> LPTables;

        static bool First = false;

        if (!First)
        {
            First = true;

            auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

            UDataTable* MainLTD = StaticLoadObject<UDataTable>(UKismetStringLibrary::Conv_NameToString(GameState->CurrentPlaylistInfo.BasePlaylist->LootTierData.ObjectID.AssetPathName).ToString());;
            UDataTable* MainLP = StaticLoadObject<UDataTable>(UKismetStringLibrary::Conv_NameToString(GameState->CurrentPlaylistInfo.BasePlaylist->LootPackages.ObjectID.AssetPathName).ToString());

            if (!MainLTD)
                MainLTD = UObject::FindObject<UDataTable>("AthenaLootTierData_Client");

            if (!MainLP)
                MainLP = UObject::FindObject<UDataTable>("AthenaLootPackages_Client");

            LTDTables.push_back(MainLTD);

            LPTables.push_back(MainLP);
        }

        std::vector<FFortLootTierData*> TierGroupLTDs;

        for (int p = 0; p < LTDTables.size(); p++)
        {
            auto LTD = LTDTables[p];
            auto& LTDRowMap = LTD->RowMap;

            auto LTDRowMapNum = LTDRowMap.Elements.Num();


            for (int i = 0; i < LTDRowMapNum; i++)
            {
                auto& CurrentLTD = LTDRowMap.Elements[i];
                auto TierData = (FFortLootTierData*)CurrentLTD.Value();


                if (TierGroupName == TierData->TierGroup && TierData->Weight != 0)
                {
                    TierGroupLTDs.push_back(TierData);
                }
            }
        }

        FFortLootTierData* ChosenRowLootTierData = GetLootTierData(TierGroupLTDs);

        if (ChosenRowLootTierData->NumLootPackageDrops <= 0)
            return PickLootDrops(TierGroupName, ++recursive);

        if (ChosenRowLootTierData->LootPackageCategoryMinArray.Num() != ChosenRowLootTierData->LootPackageCategoryWeightArray.Num() ||
            ChosenRowLootTierData->LootPackageCategoryMinArray.Num() != ChosenRowLootTierData->LootPackageCategoryMaxArray.Num())
            return PickLootDrops(TierGroupName, ++recursive);

        int MinimumLootDrops = 0;
        float NumLootPackageDrops = std::floor(ChosenRowLootTierData->NumLootPackageDrops);

        if (ChosenRowLootTierData->LootPackageCategoryMinArray.Num())
        {
            for (int i = 0; i < ChosenRowLootTierData->LootPackageCategoryMinArray.Num(); i++)
            {
                if (ChosenRowLootTierData->LootPackageCategoryMinArray[i] > 0)
                {
                    MinimumLootDrops += ChosenRowLootTierData->LootPackageCategoryMinArray[i];
                }
            }
        }

        int SumLootPackageCategoryWeightArray = 0;

        for (int i = 0; i < ChosenRowLootTierData->LootPackageCategoryWeightArray.Num(); i++)
        {
            auto CategoryWeight = ChosenRowLootTierData->LootPackageCategoryWeightArray[i];

            if (CategoryWeight > 0)
            {
                auto CategoryMaxArray = ChosenRowLootTierData->LootPackageCategoryMaxArray[i];

                if (CategoryMaxArray < 0)
                {
                    SumLootPackageCategoryWeightArray += CategoryWeight;
                }
            }
        }

        int SumLootPackageCategoryMinArray = 0;

        for (int i = 0; i < ChosenRowLootTierData->LootPackageCategoryMinArray.Num(); i++)
        {
            auto CategoryWeight = ChosenRowLootTierData->LootPackageCategoryMinArray[i];

            if (CategoryWeight > 0)
            {
                auto CategoryMaxArray = ChosenRowLootTierData->LootPackageCategoryMaxArray[i];

                if (CategoryMaxArray < 0)
                {
                    SumLootPackageCategoryMinArray += CategoryWeight;
                }
            }
        }

        if (SumLootPackageCategoryWeightArray > SumLootPackageCategoryMinArray)
            return PickLootDrops(TierGroupName, ++recursive);

        std::vector<FFortLootPackageData*> TierGroupLPs;

        for (int p = 0; p < LPTables.size(); p++)
        {
            auto LP = LPTables[p];
            auto& LPRowMap = LP->RowMap;

            for (int i = 0; i < LPRowMap.Elements.Num(); i++)
            {
                auto& CurrentLP = LPRowMap.Elements[i];
                auto LootPackage = (FFortLootPackageData*)CurrentLP.Value();

                if (!LootPackage)
                    continue;

                if (LootPackage->LootPackageID == ChosenRowLootTierData->LootPackage && LootPackage->Weight != 0)
                {
                    TierGroupLPs.push_back(LootPackage);
                }
            }
        }

        auto ChosenLootPackageName = ChosenRowLootTierData->LootPackage.ToString();


        bool bIsWorldList = ChosenLootPackageName.contains("WorldList");


        LootDrops.reserve(NumLootPackageDrops);

        for (float i = 0; i < NumLootPackageDrops; i++)
        {
            if (i >= TierGroupLPs.size())
                break;

            auto TierGroupLP = TierGroupLPs.at(i);
            auto TierGroupLPStr = TierGroupLP->LootPackageCall.ToString();

            if (TierGroupLPStr.contains(".Empty"))
            {
                NumLootPackageDrops++;
                continue;
            }

            std::vector<FFortLootPackageData*> lootPackageCalls;

            if (bIsWorldList)
            {
                for (int j = 0; j < TierGroupLPs.size(); j++)
                {
                    auto& CurrentLP = TierGroupLPs.at(j);

                    if (CurrentLP->Weight != 0)
                        lootPackageCalls.push_back(CurrentLP);
                }
            }
            else
            {
                for (int p = 0; p < LPTables.size(); p++)
                {
                    auto LPRowMap = LPTables[p]->RowMap;

                    for (int j = 0; j < LPRowMap.Elements.Num(); j++)
                    {
                        auto& CurrentLP = LPRowMap.Elements[j];

                        auto LootPackage = (FFortLootPackageData*)CurrentLP.Value();

                        if (LootPackage->LootPackageID.ToString() == TierGroupLPStr && LootPackage->Weight != 0)
                        {
                            lootPackageCalls.push_back(LootPackage);
                        }
                    }
                }
            }

            if (lootPackageCalls.size() == 0)
            {
                NumLootPackageDrops++;
                continue;
            }


            FFortLootPackageData* LootPackageCall = GetLootPackage(lootPackageCalls);

            if (!LootPackageCall)
                continue;

            auto ItemDef = LootPackageCall->ItemDefinition.Get();

            if (!ItemDef)
            {
                NumLootPackageDrops++;
                continue;
            }

            FFortItemEntry LootDropEntry{};

            LootDropEntry.ItemDefinition = ItemDef;
            LootDropEntry.LoadedAmmo = GetClipSize(Cast<UFortWeaponItemDefinition>(ItemDef));
            LootDropEntry.Count = LootPackageCall->Count;

            LootDrops.push_back(LootDropEntry);
        }

        return LootDrops;
    }

    char SpawnLoot(ABuildingContainer* BuildingContainer)
    {
        std::string ClassName = BuildingContainer->Class->GetName();

        auto SearchLootTierGroup = BuildingContainer->SearchLootTierGroup;
        EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset;

        EFortPickupSourceTypeFlag PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Container;

        static auto Loot_Treasure = UKismetStringLibrary::Conv_StringToName(L"Loot_Treasure");
        static auto Loot_Ammo = UKismetStringLibrary::Conv_StringToName(L"Loot_Ammo");
        static auto Loot_AthenaFloorLoot = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaFloorLoot");
        static auto Loot_AthenaFloorLoot_Warmup = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaFloorLoot_Warmup");

        if (SearchLootTierGroup == Loot_AthenaFloorLoot || SearchLootTierGroup == Loot_AthenaFloorLoot_Warmup)
        {
            PickupSourceTypeFlags = EFortPickupSourceTypeFlag::FloorLoot;
        }

        if (!Globals::LateGame || !Globals::Arsenal)
        {
            BuildingContainer->bAlreadySearched = true;
            BuildingContainer->SearchBounceData.SearchAnimationCount++;
            BuildingContainer->OnRep_bAlreadySearched();
        }
        else if (Globals::LateGame && SearchLootTierGroup == Loot_AthenaFloorLoot_Warmup || Globals::Arsenal && SearchLootTierGroup == Loot_AthenaFloorLoot_Warmup)
        {
            BuildingContainer->bAlreadySearched = true;
            BuildingContainer->SearchBounceData.SearchAnimationCount++;
            BuildingContainer->OnRep_bAlreadySearched();

            auto LootDrops = PickLootDrops(SearchLootTierGroup);

            auto CorrectLocation = BuildingContainer->K2_GetActorLocation() + (BuildingContainer->GetActorForwardVector() * BuildingContainer->LootSpawnLocation_Athena.X) + (BuildingContainer->GetActorRightVector() * BuildingContainer->LootSpawnLocation_Athena.Y) + (BuildingContainer->GetActorUpVector() * BuildingContainer->LootSpawnLocation_Athena.Z);

            for (auto& LootDrop : LootDrops)
            {
                if (LootDrop.Count > 0)
                {
                    SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, PickupSourceTypeFlags, SpawnSource);

                    if (SearchLootTierGroup == Loot_AthenaFloorLoot || SearchLootTierGroup == Loot_AthenaFloorLoot_Warmup)
                    {
                        if (LootDrop.ItemDefinition->GetName() == "WID_Athena_HappyGhost")
                        {
                            return 0;
                        }

                        UFortAmmoItemDefinition* AmmoDef = (UFortAmmoItemDefinition*)((UFortWeaponRangedItemDefinition*)LootDrop.ItemDefinition)->GetAmmoWorldItemDefinition_BP();

                        if (AmmoDef && LootDrop.ItemDefinition != AmmoDef && AmmoDef->DropCount > 0)
                        {
                            SpawnPickup(AmmoDef, AmmoDef->DropCount, 0, CorrectLocation, PickupSourceTypeFlags, SpawnSource);
                        }
                    }
                }

            }
        }

        if (!Globals::LateGame || !Globals::Arsenal)
        {

            if (SearchLootTierGroup == Loot_Treasure)
            {
                EFortPickupSourceTypeFlag PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Container;

                SearchLootTierGroup = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaTreasure");
                SpawnSource = EFortPickupSpawnSource::Chest;
            }

            if (SearchLootTierGroup == Loot_Ammo)
            {
                EFortPickupSourceTypeFlag PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Container;

                SearchLootTierGroup = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaAmmoLarge");
                SpawnSource = EFortPickupSpawnSource::AmmoBox;
            }

            if (ClassName.contains("Tiered_Chest_Athena_FactionChest"))
            {
                for (int i = 0; i < 2; i++)
                {
                    auto LootDrops = PickLootDrops(SearchLootTierGroup);

                    auto CorrectLocation = BuildingContainer->K2_GetActorLocation() + (BuildingContainer->GetActorForwardVector() * BuildingContainer->LootSpawnLocation_Athena.X) + (BuildingContainer->GetActorRightVector() * BuildingContainer->LootSpawnLocation_Athena.Y) + (BuildingContainer->GetActorUpVector() * BuildingContainer->LootSpawnLocation_Athena.Z);

                    for (auto& LootDrop : LootDrops)
                    {
                        SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, PickupSourceTypeFlags, SpawnSource);

                        UFortAmmoItemDefinition* AmmoDef = (UFortAmmoItemDefinition*)((UFortWeaponRangedItemDefinition*)LootDrop.ItemDefinition)->GetAmmoWorldItemDefinition_BP();

                        if (AmmoDef && LootDrop.ItemDefinition != AmmoDef && AmmoDef->DropCount > 0)
                        {
                            SpawnPickup(AmmoDef, AmmoDef->DropCount, 0, CorrectLocation, PickupSourceTypeFlags, SpawnSource);
                        }
                    }
                }
                return true;
            }

            else if (ClassName.contains("Tiered_Chest"))
            {
                auto LootDrops = PickLootDrops(SearchLootTierGroup);

                auto CorrectLocation = BuildingContainer->K2_GetActorLocation() + (BuildingContainer->GetActorForwardVector() * BuildingContainer->LootSpawnLocation_Athena.X) + (BuildingContainer->GetActorRightVector() * BuildingContainer->LootSpawnLocation_Athena.Y) + (BuildingContainer->GetActorUpVector() * BuildingContainer->LootSpawnLocation_Athena.Z);

                for (auto& LootDrop : LootDrops)
                {
                    SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, PickupSourceTypeFlags, SpawnSource);
                }

                static auto Wood = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
                static auto Metal = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/MetalItemData.MetalItemData");
                static auto Stone = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/StoneItemData.StoneItemData");

                UFortItemDefinition* Mats = (rand() % 40 > 20) ? ((rand() % 20 > 10) ? Wood : Stone) : Metal;

                SpawnPickup(Mats, 30, 0, CorrectLocation, PickupSourceTypeFlags, SpawnSource);

                return true;
            }

            else
            {
                auto LootDrops = PickLootDrops(SearchLootTierGroup);

                auto CorrectLocation = BuildingContainer->K2_GetActorLocation() + (BuildingContainer->GetActorForwardVector() * BuildingContainer->LootSpawnLocation_Athena.X) + (BuildingContainer->GetActorRightVector() * BuildingContainer->LootSpawnLocation_Athena.Y) + (BuildingContainer->GetActorUpVector() * BuildingContainer->LootSpawnLocation_Athena.Z);

                for (auto& LootDrop : LootDrops)
                {
                    if (LootDrop.Count > 0)
                    {
                        SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, PickupSourceTypeFlags, SpawnSource);

                        if (SearchLootTierGroup == Loot_AthenaFloorLoot || SearchLootTierGroup == Loot_AthenaFloorLoot_Warmup)
                        {
                            if (LootDrop.ItemDefinition->GetName() == "WID_Athena_HappyGhost")
                            {
                                return 0;
                            }

                            UFortAmmoItemDefinition* AmmoDef = (UFortAmmoItemDefinition*)((UFortWeaponRangedItemDefinition*)LootDrop.ItemDefinition)->GetAmmoWorldItemDefinition_BP();

                            if (AmmoDef && LootDrop.ItemDefinition != AmmoDef && AmmoDef->DropCount > 0)
                            {
                                SpawnPickup(AmmoDef, AmmoDef->DropCount, 0, CorrectLocation, PickupSourceTypeFlags, SpawnSource);
                            }
                        }
                    }

                }
            }
        }

        return true;
    }

    void SpawnFloorLoot()
    {
        auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

        TArray<AActor*> FloorLootSpawners;
        UClass* SpawnerClass = StaticLoadObject<UClass>("/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C");
        Statics->GetAllActorsOfClass(UWorld::GetWorld(), SpawnerClass, &FloorLootSpawners);

        for (size_t i = 0; i < FloorLootSpawners.Num(); i++)
        {
            FloorLootSpawners[i]->K2_DestroyActor();
        }

        FloorLootSpawners.Free();

        SpawnerClass = StaticLoadObject<UClass>("/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C");
        Statics->GetAllActorsOfClass(UWorld::GetWorld(), SpawnerClass, &FloorLootSpawners);

        for (size_t i = 0; i < FloorLootSpawners.Num(); i++)
        {
            FloorLootSpawners[i]->K2_DestroyActor();
        }

        FloorLootSpawners.Free();
    }

    void Hook() {
        MH_CreateHook((LPVOID)(ImageBase + 0x1B46D00), SpawnLoot, nullptr);

        Log("Looting Hooked!");
    }
}