#pragma once
#include "framework.h"
#include "Inventory.h"
#include "Looting.h"

#include "BehaviourTree_System.h"

auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
auto Gamemode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

enum class EBossesStrafeType {
	StrafeLeft,
	StrafeRight
};

struct BT_MANG_Context : BTContext
{
	class FactionBot* bot;
};

std::vector<class FactionBot*> FactionBots{};
class FactionBot
{
public:
	// The behaviortree for the new ai system
	BehaviorTree* BT_MANG = nullptr;

	// The context that should be sent to the behaviortree
	BT_MANG_Context Context = {};

	// The playercontroller of the bot
	ABP_PhoebePlayerController_C* PC;

	// The Pawn of the bot
	AFortPlayerPawnAthena* Pawn;

	// Are we ticking the bot?
	bool bTickEnabled = false;

	// So we can track the current tick that the bot is doing
	uint64_t tick_counter = 0;

	// Is the bot stressed, will be determined every tick.
	bool bIsStressed = false;

	// The current target that the bot is focusing on
	AActor* CurrentTarget = nullptr;

	// The current alertlevel of the bot, should have different behaviour for each one
	EAlertLevel AlertLevel = EAlertLevel::Unaware;

	// The bots should only have one weapon, so why not make it easier for us to switch back to it if we switch to pickaxe
	UFortWeaponItemDefinition* Weapon = nullptr;
	FGuid WeaponGuid;

	// So we can easily switch to the pickaxe when we need to
	FGuid PickaxeGuid;
	UFortWeaponMeleeItemDefinition* Pickaxe = nullptr;

	// The cid (pretty self explanatory)
	std::string CID = "";

	// The actual name of the bots class
	std::string Name = "";

	// The patrol path that the henchmen/bot is assigned to
	AFortAthenaPatrolPath* PatrolPath = nullptr;

	// The location of the current patrol point
	FVector CurrentPatrolPointLoc;

	// The current patrol point the bot is on
	int CurrentPatrolPoint = 0;

	// the amount of patrol points the bot patrol path has
	int MaxPatrolPoints = 0;

	// Are we patrolling?
	bool bIsPatrolling = false;

	// patrol points decremental
	bool bIsPatrollingBack = false;

	// bot is paused for some time for next patrol
	bool bIsWaitingForNextPatrol = false;

	// Time for the bot to go to the next patrol point
	float TimeWaitingForNextPatrol = 0.f;

	// Time for the bot to go back to being unaware
	float TimeUntilUnaware = 0.f;

	// Is the bot currently strafing (combat technique used by the bots)
	bool bIsCurrentlyStrafing = false;

	// The strafe type used by the bot, determines what direction
	EBossesStrafeType StrafeType = EBossesStrafeType::StrafeLeft;

	// When should the current strafe end?
	float StrafeEndTime = 0.0f;

	// the bot this bot will revive when downed, or the other way round
	FactionBot* CurrentAssignedDBNOBot;

	bool bIsReviving = false;

	float ReviveTime = 0.f;

	bool bWasReportedStuck = false;

public:
	FactionBot(AFortPlayerPawnAthena* Pawn)
	{
		this->Pawn = Pawn;
		PC = (ABP_PhoebePlayerController_C*)Pawn->Controller;

		Context.Controller = PC;
		Context.Pawn = Pawn;
		Context.bot = this;

		FactionBots.push_back(this);
	}

public:
	// We shouldnt need to use this but we have it anyway
	FGuid GetItem(UFortItemDefinition* Def)
	{
		for (int32 i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
				return PC->Inventory->Inventory.ReplicatedEntries[i].ItemGuid;
		}
		return FGuid();
	}

	// only really should be called when initialisint the bot
	void GiveItem(UFortItemDefinition* ODef, int Count = 1, bool equip = true)
	{
		UFortItemDefinition* Def = ODef;
		if (Def->GetName() == "AGID_Boss_Tina") {
			Def = StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/Boss/WID_Boss_Tina.WID_Boss_Tina");
		}
		UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);
		Item->OwnerInventory = PC->Inventory;

		if (Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
		{
			Item->ItemEntry.LoadedAmmo = Looting::GetClipSize((UFortWeaponItemDefinition*)Def);
		}

		PC->Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->Inventory->Inventory.ItemInstances.Add(Item);
		PC->Inventory->Inventory.MarkItemDirty(Item->ItemEntry);
		PC->Inventory->HandleInventoryLocalUpdate();
		if (Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()) && equip)
		{
			this->Weapon = (UFortWeaponItemDefinition*)ODef;
			this->WeaponGuid = GetItem(Def);
			Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Def, GetItem(Def));
		}
		if (Def->GetName() == "WID_Harvest_Pickaxe_Athena_C_T01") {
			this->Pickaxe = (UFortWeaponMeleeItemDefinition*)Def;
			this->PickaxeGuid = GetItem(Def);
		}
	}

	// set the currenttarget so that we can have the bot react appropriately to the target
	virtual void OnPerceptionSensed(AActor* SourceActor, FAIStimulus& Stimulus)
	{
		if (Stimulus.bSuccessfullySensed == 1 && PC->LineOfSightTo(SourceActor, FVector(), true) && Pawn->GetDistanceTo(SourceActor) < 5000)
		{
			CurrentTarget = SourceActor;
		}
	}
};

namespace BossesBTService_Patrolling {
	auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;

	inline FVector GetPatrolLocation(FactionBot* bot) {
		bot->MaxPatrolPoints = (bot->PatrolPath->PatrolPoints.Num() - 1);
		FVector Loc{};
		if (!bot || !bot->PatrolPath || bot->PatrolPath->PatrolPoints.Num() == 0)
			return Loc;

		if (bot->MaxPatrolPoints < 1)
		{
			Log("Not enough patrol points for " + bot->PC->GetFullName() + ", cancelling patrol.");
			bot->PatrolPath = nullptr;
			return Loc;
		}

		FVector PatrolPointLoc = bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]->K2_GetActorLocation();
		if (PatrolPointLoc.IsZero())
		{
			Log("No Patrol Point Location For Index: " + std::to_string(bot->CurrentPatrolPoint));
		}
		else
		{
			auto BotPos = bot->Pawn->K2_GetActorLocation();
			auto TestRot = Math->FindLookAtRotation(BotPos, PatrolPointLoc);

			bot->PC->SetControlRotation(TestRot);
			bot->PC->K2_SetActorRotation(TestRot, true);
			Loc = PatrolPointLoc;
		}

		Log("Started Patrolling " + std::to_string(bot->MaxPatrolPoints) + " Patrol Points for bot: " + bot->Name);
		bot->CurrentPatrolPointLoc = Loc;
		bot->bIsPatrolling = true;

		return Loc;
	}

	inline FVector DetermineNextPatrolPointLoc(FactionBot* bot) {
		FVector Loc{};
		if (!bot || !bot->PatrolPath || bot->PatrolPath->PatrolPoints.Num() == 0)
			return Loc;

		if (!bot->bIsPatrollingBack) {
			if ((bot->CurrentPatrolPoint + 1) >= bot->MaxPatrolPoints) {
				bot->bIsPatrollingBack = true;
			}
			bot->CurrentPatrolPoint = bot->CurrentPatrolPoint + 1;
			if (!bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]) {
				bot->bIsPatrollingBack = true;
			}
			else {
				FVector PatrolPointLoc = bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]->K2_GetActorLocation();
				if (PatrolPointLoc.IsZero())
				{
					Log("No Patrol Point Location For Index: " + std::to_string(bot->CurrentPatrolPoint));
				}
				else
				{
					auto BotPos = bot->Pawn->K2_GetActorLocation();
					auto TestRot = Math->FindLookAtRotation(BotPos, PatrolPointLoc);

					bot->PC->SetControlRotation(TestRot);
					bot->PC->K2_SetActorRotation(TestRot, true);
					Loc = PatrolPointLoc;
				}
			}
		}
		else {
			if ((bot->CurrentPatrolPoint - 1) <= 0) {
				bot->bIsPatrollingBack = false;
			}
			bot->CurrentPatrolPoint = bot->CurrentPatrolPoint - 1;
			if (!bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]) {
				bot->bIsPatrollingBack = false;
			}
			else {
				FVector PatrolPointLoc = bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]->K2_GetActorLocation();
				if (PatrolPointLoc.IsZero())
				{
					Log("No Patrol Point Location For Index: " + std::to_string(bot->CurrentPatrolPoint));
				}
				else
				{
					auto BotPos = bot->Pawn->K2_GetActorLocation();
					auto TestRot = Math->FindLookAtRotation(BotPos, PatrolPointLoc);

					bot->PC->SetControlRotation(TestRot);
					bot->PC->K2_SetActorRotation(TestRot, true);
					Loc = PatrolPointLoc;
				}
			}
		}

		bot->PC->Blackboard->SetValueAsVector(UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Patrolling_Destination")), Loc);
		return Loc;
	}
}

class BossesBTService_AIEvaluator {
public:
	// When stressed the bot will handle combat situations with players or playerbots differently
	bool IsStressed(AFortPlayerPawnAthena* Pawn, ABP_PhoebePlayerController_C* PC) {
		// If the bots shield is below 0 and they are threatened then they are stressed
		if (Pawn->GetShield() <= 0 && PC->CurrentAlertLevel == EAlertLevel::Threatened) {
			return true;
		}
		return false;
	}

public:
	void Tick(FactionBot* bot) {
		FVector BotPos = bot->Pawn->K2_GetActorLocation();

		bot->bIsStressed = IsStressed(bot->Pawn, bot->PC);

		if (bot->bIsReviving) {
			bot->PC->ReviveTarget = bot->CurrentAssignedDBNOBot->Pawn;
			if (Statics->GetTimeSeconds(UWorld::GetWorld()) >= bot->ReviveTime) {
				auto PlayerState = (AFortPlayerStateAthena*)bot->CurrentAssignedDBNOBot->PC->PlayerState;
				auto AbilitySystemComp = (UFortAbilitySystemComponentAthena*)PlayerState->AbilitySystemComponent;

				FGameplayEventData Data{};
				Data.EventTag = bot->CurrentAssignedDBNOBot->Pawn->EventReviveTag;
				Data.ContextHandle = PlayerState->AbilitySystemComponent->MakeEffectContext();
				Data.Instigator = bot->PC;
				Data.Target = bot->CurrentAssignedDBNOBot->Pawn;
				Data.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(bot->CurrentAssignedDBNOBot->Pawn);
				Data.TargetTags = bot->CurrentAssignedDBNOBot->Pawn->GameplayTags;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(bot->CurrentAssignedDBNOBot->Pawn, bot->CurrentAssignedDBNOBot->Pawn->EventReviveTag, Data);

				for (auto& Ability : AbilitySystemComp->ActivatableAbilities.Items)
				{
					const std::string AbilityName = Ability.Ability->Class->GetName();

					if (AbilityName.contains("GAB_AthenaDBNO") || AbilityName.contains("GAB_AthenaDBNORevive"))
					{
						AbilitySystemComp->ServerCancelAbility(Ability.Handle, Ability.ActivationInfo);
						AbilitySystemComp->ServerEndAbility(Ability.Handle, Ability.ActivationInfo, Ability.ActivationInfo.PredictionKeyWhenActivated);
						AbilitySystemComp->ClientCancelAbility(Ability.Handle, Ability.ActivationInfo);
						AbilitySystemComp->ClientEndAbility(Ability.Handle, Ability.ActivationInfo);
					}
				}

				bot->CurrentAssignedDBNOBot->Pawn->bIsDBNO = false;
				bot->Pawn->bPlayedDying = false;
				bot->Pawn->bIsDying = false;
				bot->Pawn->DBNORevivalStacking = 0;
				bot->CurrentAssignedDBNOBot->Pawn->OnRep_IsDBNO();
				bot->CurrentAssignedDBNOBot->Pawn->SetHealth(30);
				PlayerState->DeathInfo = {};
				PlayerState->OnRep_DeathInfo();

				bot->CurrentAssignedDBNOBot->Pawn->EquipWeaponDefinition(bot->Weapon, bot->WeaponGuid);
			}
		}

		if (!bot->bIsStressed && bot->PatrolPath && bot->CurrentPatrolPointLoc.IsZero() && !bot->bIsPatrolling) {
			BossesBTService_Patrolling::GetPatrolLocation(bot);
		}

		if (bot->bIsPatrolling && bot->bIsWaitingForNextPatrol && Statics->GetTimeSeconds(UWorld::GetWorld()) >= bot->TimeWaitingForNextPatrol) {
			auto BotPos = bot->Pawn->K2_GetActorLocation();
			auto PatrolRot = Math->FindLookAtRotation(BotPos, bot->CurrentPatrolPointLoc);
			
			bot->PC->SetControlRotation(PatrolRot);
			bot->PC->K2_SetActorRotation(PatrolRot, true);

			bot->bIsWaitingForNextPatrol = false;
		}

		if (bot->PC->CurrentAlertLevel != EAlertLevel::Threatened && bot->PC->CurrentAlertLevel != EAlertLevel::Alerted
			&& bot->PatrolPath && bot->bIsPatrolling && !bot->bIsWaitingForNextPatrol && !bot->CurrentPatrolPointLoc.IsZero()) {
			float Distance = Math->Vector_Distance(BotPos, bot->CurrentPatrolPointLoc);

			if (Distance < 175.0f) {
				bot->bIsWaitingForNextPatrol = true;
				bot->TimeWaitingForNextPatrol = Statics->GetTimeSeconds(UWorld::GetWorld()) + Math->RandomFloatInRange(1.5f, 3.0f);
				bot->CurrentPatrolPointLoc = BossesBTService_Patrolling::DetermineNextPatrolPointLoc(bot);
			}
		}

		if (bot->tick_counter % 30 == 0) {
			float ClosestDistance = FLT_MAX;
			FactionBot* ClosestBot = nullptr;

			for (int i = 0; i < FactionBots.size(); i++) {
				if (((AFortPlayerStateAthena*)bot->PC->PlayerState)->PlayerTeam == ((AFortPlayerStateAthena*)FactionBots[i]->PC->PlayerState)->PlayerTeam) {
					if (bot->Pawn->bIsDBNO) {
						if (FactionBots[i]->Pawn->bIsDBNO)
							continue;
					}
					if (FactionBots[i]->Pawn == bot->Pawn)
						continue;

					float Dist = Math->Vector_Distance(bot->Pawn->K2_GetActorLocation(), FactionBots[i]->Pawn->K2_GetActorLocation());

					if (Dist < ClosestDistance) {
						ClosestDistance = Dist;
						ClosestBot = FactionBots[i];
					}
				}
			}

			if (ClosestBot) {
				bot->CurrentAssignedDBNOBot = ClosestBot;
			}
		}
	}
};

namespace Bosses {
	inline void TickBots(float DeltaTime)
	{
		auto block = [](FactionBot* bot, std::function<void(FactionBot* bot)> const& SetUnaware, bool Alerted, bool Threatened, bool LKP) {
			BossesBTService_AIEvaluator Evaluator;
			Evaluator.Tick(bot);
			//return;

			if (bot->CurrentAssignedDBNOBot && bot->CurrentAssignedDBNOBot->Pawn && bot->CurrentAssignedDBNOBot->Pawn->bIsDBNO && !Threatened) {
				FVector BotPos = bot->Pawn->K2_GetActorLocation();
				float Distance = Math->Vector_Distance(BotPos, bot->CurrentAssignedDBNOBot->Pawn->K2_GetActorLocation());
				auto TestRot = Math->FindLookAtRotation(BotPos, bot->CurrentAssignedDBNOBot->Pawn->K2_GetActorLocation());

				bot->PC->SetControlRotation(TestRot);
				bot->PC->K2_SetActorRotation(TestRot, true);

				if (Distance < 200.0f) {
					if (!bot->bIsReviving) {
						bot->ReviveTime = Statics->GetTimeSeconds(UWorld::GetWorld()) + 9.9;
						bot->bIsReviving = true;
					}
					return;
				}
				else {
					if (bot->bIsReviving) {
						bot->bIsReviving = false;
					}
					//bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
					bot->PC->MoveToActor(bot->CurrentAssignedDBNOBot->PC, 100, true, false, true, nullptr, true);
				}

				bot->tick_counter++;
				return;
			}

			if (bot->bIsReviving) {
				bot->bIsReviving = false;
			}

			if (bot->Pawn->bIsDBNO && bot->CurrentAssignedDBNOBot) {
				FVector BotPos = bot->Pawn->K2_GetActorLocation();
				float Distance = Math->Vector_Distance(BotPos, bot->CurrentAssignedDBNOBot->Pawn->K2_GetActorLocation());
				auto TestRot = Math->FindLookAtRotation(BotPos, bot->CurrentAssignedDBNOBot->Pawn->K2_GetActorLocation());

				bot->PC->SetControlRotation(TestRot);
				bot->PC->K2_SetActorRotation(TestRot, true);

				if (Distance < 100.0f) {}
				else {
					//bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
					bot->PC->MoveToActor(bot->CurrentAssignedDBNOBot->PC, 100, true, false, true, nullptr, true);
				}

				bot->tick_counter++;
				return;
			}

			if ((bot->tick_counter % 30 == 0) && bot->bWasReportedStuck) {
				bot->Pawn->EquipWeaponDefinition(bot->Weapon, bot->WeaponGuid);
				bot->bWasReportedStuck = false;

				if (bot->Pawn->bIsCrouched) {
					bot->Pawn->UnCrouch(false);
				}
			}

			if (!Threatened && !Alerted && !LKP && bot->PatrolPath && bot->bIsPatrolling && !bot->bIsWaitingForNextPatrol && !bot->CurrentPatrolPointLoc.IsZero()) {
				bot->PC->Blackboard->SetValueAsBool(UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Patrolling_ShouldMove")), true);
				bot->PC->MoveToLocation(bot->CurrentPatrolPointLoc, 100.f, false, false, false, true, nullptr, true);
			}

			if (!Threatened && !Alerted && !LKP) {
				if (bot->CurrentAssignedDBNOBot && bot->CurrentAssignedDBNOBot->bIsStressed) {
					bot->PC->MoveToActor(bot->CurrentAssignedDBNOBot->PC, Math->RandomFloatInRange(400, 1000), true, false, true, nullptr, true);
				}
			}

			/*if (Alerted || Threatened || LKP) {
				if (bot->Pawn->CurrentWeapon && bot->Pawn->CurrentWeapon->ItemEntryGuid != bot->WeaponGuid) {
					bot->Pawn->EquipWeaponDefinition(bot->Weapon, bot->WeaponGuid);
				}
			}*/

			if (Alerted) {
				if (bot->CurrentTarget) {
					auto BotPos = bot->Pawn->K2_GetActorLocation();
					auto TargetPos = bot->CurrentTarget->K2_GetActorLocation();
					float Distance = bot->Pawn->GetDistanceTo(bot->CurrentTarget);

					bot->Pawn->PawnStopFire(0);

					auto Rot = Math->FindLookAtRotation(BotPos, TargetPos);

					if (Math->RandomBoolWithWeight(0.25f)) {
						bot->PC->SetControlRotation(Rot);
						bot->PC->K2_SetActorRotation(Rot, true);
					}

					bot->PC->MoveToLocation(TargetPos, 25.f, true, false, false, true, nullptr, true);

					if (Math->RandomBoolWithWeight(0.25f)) {
						float RandomXmod = Math->RandomFloatInRange(-360000, 360000);
						float RandomYmod = Math->RandomFloatInRange(-360000, 360000);
						//float RandomZmod = Math->RandomFloatInRange(-360000, 360000);

						FVector TargetPosMod{ TargetPos.X + RandomXmod, TargetPos.Y + RandomYmod, TargetPos.Z };

						Rot = Math->FindLookAtRotation(BotPos, TargetPosMod);

						bot->PC->SetControlRotation(Rot);
						bot->PC->K2_SetActorRotation(Rot, true);
					}
				}
			}
			else if (Threatened) {
				if (bot->CurrentTarget) {
					auto BotPos = bot->Pawn->K2_GetActorLocation();
					auto TargetPos = bot->CurrentTarget->K2_GetActorLocation();
					float Distance = bot->Pawn->GetDistanceTo(bot->CurrentTarget);

					if (!bot->Pawn->bIsCrouched && Math->RandomBoolWithWeight(0.01f)) {
						bot->Pawn->Crouch(false);
					}
					if (bot->Pawn->bIsCrouched && (bot->tick_counter % 30) == 0) {
						bot->Pawn->UnCrouch(false);
					}

					if (Math->RandomBoolWithWeight(0.001f)) {
						bot->Pawn->UnCrouch(false);
						bot->Pawn->Jump();
					}

					if (!bot->bIsStressed) {
						bot->PC->MoveToActor(bot->CurrentTarget, Math->RandomFloatInRange(400, 1000), true, false, true, nullptr, true);
					}
					else {
						bot->Pawn->AddMovementInput((bot->Pawn->GetActorForwardVector() * -1.0f), 1.2f, true);
					}

					if (true) { // wouldve had a tickcounter condition here but no
						float RandomXmod = Math->RandomFloatInRange(-180, 180);
						float RandomYmod = Math->RandomFloatInRange(-180, 180);
						float RandomZmod = Math->RandomFloatInRange(-180, 180);

						FVector TargetPosMod{ TargetPos.X + RandomXmod, TargetPos.Y + RandomYmod, TargetPos.Z + RandomZmod };

						FRotator Rot = Math->FindLookAtRotation(BotPos, TargetPosMod);

						bot->PC->SetControlRotation(Rot);
						bot->PC->K2_SetActorRotation(Rot, true);

						//bot->PC->K2_SetFocalPoint(TargetPosMod); doesent fix the issue with them not aiming up or down
					}

					if (!bot->bIsCurrentlyStrafing)
					{
						if (UKismetMathLibrary::RandomBoolWithWeight(0.05))
						{
							bot->bIsCurrentlyStrafing = true;
							if (UKismetMathLibrary::RandomBool()) {
								bot->StrafeType = EBossesStrafeType::StrafeLeft;
							}
							else {
								bot->StrafeType = EBossesStrafeType::StrafeRight;
							}
							bot->StrafeEndTime = Statics->GetTimeSeconds(UWorld::GetWorld()) + Math->RandomFloatInRange(2.0f, 4.0f);
						}
					}
					else
					{
						if (Statics->GetTimeSeconds(UWorld::GetWorld()) < bot->StrafeEndTime)
						{
							if (bot->StrafeType == EBossesStrafeType::StrafeLeft) {
								bot->Pawn->AddMovementInput((bot->Pawn->GetActorRightVector() * -1.0f), 1.2f, true);
							}
							else {
								bot->Pawn->AddMovementInput(bot->Pawn->GetActorRightVector(), 1.2f, true);
							}
						}
						else
						{
							bot->bIsCurrentlyStrafing = false;
						}
					}

					if (bot->PC->LineOfSightTo(bot->CurrentTarget, BotPos, true)
						&& Math->RandomBoolWithWeight(0.5f)) {
						bool HasMinigun = bot->Pawn->CurrentWeapon->Name.ToString().contains("Minigun");
						if (!HasMinigun && (bot->tick_counter % 2 != 0)) {
							bot->Pawn->PawnStopFire(0);
						}
						else if (HasMinigun && (bot->tick_counter % 150 != 0)) {
							bot->Pawn->PawnStopFire(0);
						}
						else {
							bot->Pawn->PawnStartFire(0);
						}
					}
					else {
						bot->Pawn->PawnStopFire(0);
					}
				}
			}
			else if (LKP) {
				if (bot->CurrentTarget) {
					auto BotPos = bot->Pawn->K2_GetActorLocation();
					auto TargetPos = bot->CurrentTarget->K2_GetActorLocation();
					float Distance = bot->Pawn->GetDistanceTo(bot->CurrentTarget);

					auto Rot = Math->FindLookAtRotation(BotPos, TargetPos);

					if (Math->RandomBoolWithWeight(0.25f)) {
						bot->PC->SetControlRotation(Rot);
						bot->PC->K2_SetActorRotation(Rot, true);
					}

					bot->PC->MoveToActor(bot->CurrentTarget, 150, true, false, true, nullptr, true);

					if (Math->RandomBoolWithWeight(0.25f)) {
						float RandomXmod = Math->RandomFloatInRange(-360000, 360000);
						float RandomYmod = Math->RandomFloatInRange(-360000, 360000);
						//float RandomZmod = Math->RandomFloatInRange(-360000, 360000);

						FVector TargetPosMod{ TargetPos.X + RandomXmod, TargetPos.Y + RandomYmod, TargetPos.Z };

						Rot = Math->FindLookAtRotation(BotPos, TargetPosMod);

						bot->PC->SetControlRotation(Rot);
						bot->PC->K2_SetActorRotation(Rot, true);
					}
				}
			}
			else {
				SetUnaware(bot);
			}

			bot->tick_counter++;
		};
		auto SetUnaware = [](FactionBot* bot) {
			bot->PC->CurrentAlertLevel = EAlertLevel::Unaware;
			bot->PC->OnAlertLevelChanged(bot->AlertLevel, EAlertLevel::Unaware);
			bot->Pawn->PawnStopFire(0);
			if (bot->PatrolPath) {
				bot->bIsPatrolling = true;
			}
		};
		for (auto bot : FactionBots)
		{
			if (Globals::bUseLegacyAI_MANG) {
				auto Alerted = bot->PC->CurrentAlertLevel == EAlertLevel::Alerted;
				auto Threatened = bot->PC->CurrentAlertLevel == EAlertLevel::Threatened;
				auto LKP = bot->PC->CurrentAlertLevel == EAlertLevel::LKP;
				block(bot, SetUnaware, Alerted, Threatened, LKP);
			}
			else {
				if (bot->BT_MANG) {
					bot->BT_MANG->Tick(bot->Context);
				}
			}
		}
	}
}