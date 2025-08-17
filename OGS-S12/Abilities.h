#pragma once
#include "framework.h"

namespace Abilities {
	inline void InitAbilitiesForPlayer(AFortPlayerController* PC)
	{
		auto PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		static auto AbilitySet = StaticLoadObject<UFortAbilitySet>("/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer");

		if (PlayerState && AbilitySet)
		{
			for (size_t i = 0; i < AbilitySet->GameplayAbilities.Num(); i++)
			{
				FGameplayAbilitySpec Spec{};
				AbilitySpecConstructor(&Spec, (UGameplayAbility*)AbilitySet->GameplayAbilities[i].Get()->DefaultObject, 1, -1, nullptr);
				GiveAbility(PlayerState->AbilitySystemComponent, &Spec.Handle, Spec);
			}
		}
	}

	inline FGameplayAbilitySpec* FindAbilitySpecFromHandle(UFortAbilitySystemComponentAthena* ASC, FGameplayAbilitySpecHandle& Handle)
	{
		for (size_t i = 0; i < ASC->ActivatableAbilities.Items.Num(); i++)
		{
			if (ASC->ActivatableAbilities.Items[i].Handle.Handle == Handle.Handle)
				return &ASC->ActivatableAbilities.Items[i];
		}
		return nullptr;
	}


	FGameplayAbilitySpec* FindAbilitySpec(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle) {

		for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->ActivatableAbilities.Items) {
			if (Spec.Handle.Handle == Handle.Handle) {
				if (!Spec.PendingRemove) {
					return &Spec;
				}
			}
		}

		return nullptr;
	}

	void InternalServerTryActivateAbility(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, const struct FPredictionKey& InPredictionKey, FGameplayEventData* TriggerEventData)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpec(AbilitySystemComponent, Handle);
		if (!Spec) {
			return AbilitySystemComponent->ClientActivateAbilityFailed(Handle, InPredictionKey.Current);
		}
		UGameplayAbility* AbilityToActivate = Spec->Ability;

		if (!AbilityToActivate)
		{
			return AbilitySystemComponent->ClientActivateAbilityFailed(Handle, InPredictionKey.Current);
		}

		UGameplayAbility* InstancedAbility = nullptr;
		Spec->InputPressed = true;

		if (InternalTryActivateAbility(AbilitySystemComponent, Handle, InPredictionKey, &InstancedAbility, nullptr, TriggerEventData)) {}
		else {

			AbilitySystemComponent->ClientActivateAbilityFailed(Handle, InPredictionKey.Current);
			Spec->InputPressed = false;

			AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(*Spec);
		}
	}

	void Hook()
	{
		HookVTable(UAbilitySystemComponent::GetDefaultObj(), 0xFA, InternalServerTryActivateAbility, nullptr);
		HookVTable(UFortAbilitySystemComponent::GetDefaultObj(), 0xFA, InternalServerTryActivateAbility, nullptr);
		HookVTable(UFortAbilitySystemComponentAthena::GetDefaultObj(), 0xFA, InternalServerTryActivateAbility, nullptr);

		Log("Abilities Hooked!");
	}
}