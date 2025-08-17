#pragma once
#include "framework.h"

namespace PhantomBoothSpawner {
	std::vector<FVector> PhantomBoothLocations = {
		// Agency Booths
		{ 3481.255127, 2394.962891, -1917.627686 },
		{ 7685, 5974, -1535 },
		{ -1168.074707, 7682.558594, -2302.662598 },

		// The Shark Booths
		{ 89218.656250, -75777.187500, -3829 },
		{ 90732.578125, -96676.492188, -3846 },
		{ 98479.625, -66667.960938, -3856 },
		{ 106736, -86832, -3452 },
		{ 108865, -85892, -2680 }, // Improper manually dumped loc

		// The Yacht Booths
		{ 112502, 114296, -3832 },
		{ 109492, 114338, -3063 },
		{ 105059, 108490.992188, -3840 },
		{ 116696, 114325, -3448 },
		
		// The Rig Booths
		{ -70218.710938, -78991.226563, -3843.156250 },
		{ -77443.039063, -88644.937500, -3064 },
		{ -82059.859375, -88710.59375, -3448 },
		{ -84055, -71100, -3858 }, // Improper manually dumped loc

		// The Grotto Booths
		{ -14165.263672, 111691.804688, -3840 },
		{ -21847, 112260, 2680 }, // Improper manually dumped loc

		// Pleasant Park Booths
		{ 55664.054688, -19288.376953, -2296 },
	};

	std::vector<FRotator> PhantomBoothRotations = {
		// Agency Booths
		{ 0, 89.999954, 0 },
		{ 0, 90.000420, 0 },
		{ 0, 0.000085, 0 },

		// The Shark Booths
		{ 0, 114.999809, 0 },
		{ 0, 155.000092, 0 },
		{ 0, 84.999954, 0 },
		{ 0, -90.000114, 0 },
		{ 0, 0, 0 }, // Improper manually tweaked rot

		// The Yacht Booths
		{ 0, -44.999969, 0 },
		{ 0, -90.000023, 0 },
		{ 0, -90.000023, 0 },
		{ 0, -90.000023, 0 },

		// The Rig Booths
		{ 0, 89.907799, 0 },
		{ 0, -19.034946, 0 },
		{ 0, 39.999893, 0 },
		{ 0, -35, 0 }, // Improper manually tweaked rot

		// The Grotto Booths
		{ 0, 179.999969, 0 },
		{ 0, 180, 0 }, // Improper manually tweaked rot

		// Pleasant Park Booths
		{ 0, -90.000053, 0 },
	};

	void SpawnBooths() {
		if (PhantomBoothLocations.size() != PhantomBoothRotations.size()) {
			Log("Sizes for spawndata dont match!");
			return;
		}

		static auto PhantomBooth = StaticLoadObject<UClass>("/Game/Athena/Items/EnvironmentalItems/HidingProps/Props/B_HidingProp_PhantomBooth.B_HidingProp_PhantomBooth_C");
		for (int i = 0; i < PhantomBoothLocations.size(); i++) {
			FVector Location = PhantomBoothLocations[i];
			FRotator Rotation = PhantomBoothRotations[i];

			if (PhantomBooth) {
				AActor* SpawnedBooth = SpawnActor<AActor>(Location, Rotation, nullptr, PhantomBooth);
				FGameplayTag FactionTag{};
				// Remember to shift the index for this if addded or removed from the std::vector
				if (i >= 12) {
					FactionTag.TagName = UKismetStringLibrary::Conv_StringToName(L"Athena.Faction.Alter");
				}
				else {
					FactionTag.TagName = UKismetStringLibrary::Conv_StringToName(L"Athena.Faction.Ego");
				}

				*(FGameplayTag*)((uintptr_t)SpawnedBooth + 0xB0) = FactionTag;

				FGameplayTag* Tag = (FGameplayTag*)((uintptr_t)SpawnedBooth + 0xB0);
				FName TagName = Tag->TagName;

				UFunction* FactionSelectedFn = SpawnedBooth->Class->GetFunction("B_HidingProp_PhantomBooth_C", "BndEvt__Athena_FactionSelection_AlterEgo_K2Node_ComponentBoundEvent_0_OnFactionSelected__DelegateSignature");

				struct {
					FGameplayTag FactionTagOut;
				} Params;
				Params.FactionTagOut = FactionTag;

				if (FactionSelectedFn)
					SpawnedBooth->ProcessEvent(FactionSelectedFn, &Params);

				//Log(TagName.ToString());
			}
			else {
				Log("PhantomBooth does not exist!");
			}
		}

		if (PhantomBooth) {
			Log("Spawned: " + std::to_string(PhantomBoothLocations.size()) + " PhantomBooths");
		}
	}
}