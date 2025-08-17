#pragma once
#include "framework.h"

namespace Vehicles {

	void ServerMove(AFortPhysicsPawn* PhysicsPawn, FReplicatedPhysicsPawnState InState) {

		UPrimitiveComponent* RootComponent = static_cast<UPrimitiveComponent*>(PhysicsPawn->RootComponent);

		InState.Rotation.X -= 2.5f;
		InState.Rotation.Y /= 0.3f;
		InState.Rotation.Z += 2.0f;
		InState.Rotation.W /= -1.2f;

		FTransform Transform;
		Transform.Translation = InState.Translation;
		Transform.Rotation = InState.Rotation;
		Transform.Scale3D = FVector(1.0f, 1.0f, 1.0f);

		RootComponent->K2_SetWorldTransform(Transform, false, nullptr, true);
		RootComponent->bComponentToWorldUpdated = true;

		RootComponent->SetPhysicsLinearVelocity(InState.LinearVelocity, false, FName());
		RootComponent->SetPhysicsAngularVelocity(InState.AngularVelocity, false, FName());
	}

	void SpawnVehicles()
	{
		if (Globals::bEventEnabled)
			return;

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		static bool bHasSpawnedSharkChopper = false;

		TArray<AActor*> Spawners;
		UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortAthenaVehicleSpawner::StaticClass(), &Spawners);

		for (AActor* ActorSp : Spawners) {
			AFortAthenaVehicleSpawner* Spawner = Cast<AFortAthenaVehicleSpawner>(ActorSp);
			if (!Spawner) continue;

			std::string Name = Spawner->GetName();


			if (!Name.starts_with("Apollo_Hoagie_Spawner") && !Name.starts_with("Athena_Meatball_L_Spawner")) continue;


			AActor* Vehicle = SpawnActorClass<AActor>(Spawner->K2_GetActorLocation(), Spawner->K2_GetActorRotation(), Spawner->GetVehicleClass());

			if (!bHasSpawnedSharkChopper && Name.starts_with("Apollo_Hoagie_Spawner")) {
				bHasSpawnedSharkChopper = true;

				AActor* Shark_Chopper = SpawnActorClass<AActor>(FVector(113665, -91120, -3065), {}, Spawner->GetVehicleClass());

				HookVTable(Shark_Chopper->VTable, 0xEE, ServerMove, nullptr);
			}

			HookVTable(Vehicle->VTable, 0xEE, ServerMove, nullptr);
		}

		Spawners.Free();
	}
}