#pragma once
#include "framework.h"

struct LocRot {
	FVector Location;
	FRotator Rotation;
};

TArray<LocRot> CameraSpawners;
TArray<AActor*> CameraActors;

namespace Net {
	enum class ENetMode
	{
		Standalone,
		DedicatedServer,
		ListenServer,
		Client,

		MAX,
	};

	ENetMode (*WorldGetNetModeOG)(UWorld*);
	ENetMode WorldGetNetMode(UWorld* a1)
	{
		std::string Name = a1->GetName();
		/*if (Name != "Apollo_Terrain" && Name != "Frontend") {
			Log(Name);
		}*/

		// Makes generators work for some reason
		/*if (Name.contains("Apollo_Terrain")) {
			return ENetMode::ListenServer;
		}*/

		ENetMode OriginalNetMode = WorldGetNetModeOG(a1);
		//Log("WorldNetMode: " + std::to_string(static_cast<int>(OriginalNetMode)));
		return ENetMode::DedicatedServer;
	}

	ENetMode (*AActorGetNetModeOG)(AActor*);
	ENetMode AActorGetNetMode(AActor* a1)
	{
		std::string Name = a1->GetName();

		/*if (Name.contains("Sentry_Alarm")) {
			TArray<AActor*> SecurityCameras;
			auto CamClass = a1->Class;
			Log(CamClass->GetFullName());
			if (CamClass) {
				TArray<AActor*> FoundCameras;
				auto* Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
				Statics->GetAllActorsOfClass(UWorld::GetWorld(), CamClass, &FoundCameras);

				if (FoundCameras.Num() > CameraActors.Num()) {
					CameraSpawners.Clear();
					CameraActors = FoundCameras;

					for (auto* Cam : CameraActors) {
						if (!Cam) continue;

						LocRot locRot;
						locRot.Location = Cam->K2_GetActorLocation();
						locRot.Rotation = Cam->K2_GetActorRotation();
						Log("Cam Location:");
						Log("X: " + std::to_string(locRot.Location.X));
						Log("Y: " + std::to_string(locRot.Location.Y));
						Log("Z: " + std::to_string(locRot.Location.Z));
						Log("Cam Rotation:");
						Log("Pitch: " + std::to_string(locRot.Rotation.Pitch));
						Log("Yaw: " + std::to_string(locRot.Rotation.Yaw));
						Log("Roll: " + std::to_string(locRot.Rotation.Roll));

						CameraSpawners.Add(locRot);
					}

					Log("Updated Cams with " + std::to_string(CameraSpawners.Num()) + " entries.");
				}
			}
			else {
				Log("Cam class is null!");
			}
		}*/

		//Log(Name);

		ENetMode OriginalNetMode = AActorGetNetModeOG(a1);
		//Log("AActorNetMode: " + std::to_string(static_cast<int>(OriginalNetMode)));
		return ENetMode::DedicatedServer;
	}

	void Hook() {
		MH_CreateHook((LPVOID)(ImageBase + 0x45C9D90), WorldGetNetMode, (LPVOID*)&WorldGetNetModeOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x3EB6780), AActorGetNetMode, (LPVOID*)&AActorGetNetModeOG);

		Log("Hooked Net!");
	}
}