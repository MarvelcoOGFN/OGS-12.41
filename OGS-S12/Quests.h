#pragma once
#include "framework.h"

namespace Quests {
	void GiveAccolade(AFortPlayerControllerAthena* PC, UFortAccoladeItemDefinition* Def, UFortQuestItemDefinition* QuestDef = nullptr)
	{
		if (!PC || !Def) return;

		FAthenaAccolades Accolade{};
		Accolade.AccoladeDef = Def;
		Accolade.Count = 1;
		std::string DefName = Def->GetName();
		Accolade.TemplateId = std::wstring(DefName.begin(), DefName.end()).c_str();

		auto ID = UKismetSystemLibrary::GetDefaultObj()->GetPrimaryAssetIdFromObject(Def);

		FXPEventInfo EventInfo{};
		EventInfo.Accolade = ID;
		EventInfo.EventName = Def->Name;
		EventInfo.EventXpValue = Def->GetAccoladeXpValue();
		EventInfo.Priority = Def->Priority;
		if (QuestDef) {
			EventInfo.QuestDef = QuestDef;
		}
		EventInfo.SimulatedText = Def->GetShortDescription();
		EventInfo.RestedValuePortion = EventInfo.EventXpValue;
		EventInfo.RestedXPRemaining = EventInfo.EventXpValue;
		EventInfo.SeasonBoostValuePortion = 20;
		EventInfo.TotalXpEarnedInMatch = EventInfo.EventXpValue + PC->XPComponent->TotalXpEarned;

		PC->XPComponent->MedalBonusXP += 1250;
		PC->XPComponent->MatchXp += EventInfo.EventXpValue;
		PC->XPComponent->TotalXpEarned += EventInfo.EventXpValue + 1250;

		PC->XPComponent->PlayerAccolades.Add(Accolade);
		PC->XPComponent->MedalsEarned.Add(Def);

		PC->XPComponent->ClientMedalsRecived(PC->XPComponent->PlayerAccolades);
		PC->XPComponent->OnXPEvent(EventInfo);
	}

	void ProgressQuest(AFortPlayerControllerAthena* PC, UFortQuestItemDefinition* QuestDef, FName BackendName)
	{
		PC->GetQuestManager(ESubGame::Athena)->SelfCompletedUpdatedQuest(PC, QuestDef, BackendName, 1, 1, nullptr, true, false);
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		for (size_t i = 0; i < PlayerState->PlayerTeam->TeamMembers.Num(); i++)
		{
			auto pc = (AFortPlayerControllerAthena*)PlayerState->PlayerTeam->TeamMembers[i];
			if (pc && pc != PC)
			{
				pc->GetQuestManager(ESubGame::Athena)->SelfCompletedUpdatedQuest(PC, QuestDef, BackendName, 1, 1, PlayerState, true, false);
			}
		}
		auto QuestItem = PC->GetQuestManager(ESubGame::Athena)->GetQuestWithDefinition(QuestDef);

		int32 XPCount = 20;

		if (auto RewardsTable = QuestDef->RewardsTable)
		{
			static auto Name = FName(L"Default");
			auto DefaultRow = RewardsTable->Search([](FName& RName, uint8* Row) { return RName == Name; });
			XPCount = (*(FFortQuestRewardTableRow**)DefaultRow)->Quantity;
		}

		FXPEventEntry Entry{};
		Entry.EventXpValue = XPCount;
		Entry.QuestDef = QuestDef;
		Entry.Time = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
		PC->XPComponent->ChallengeXp += Entry.EventXpValue;
		PC->XPComponent->TotalXpEarned += Entry.EventXpValue;
		Entry.TotalXpEarnedInMatch = PC->XPComponent->TotalXpEarned;
		Entry.SimulatedXpEvent = QuestDef->GetSingleLineDescription();
		PC->XPComponent->RestXP += Entry.EventXpValue;
		PC->XPComponent->InMatchProfileVer++;
		PC->XPComponent->OnInMatchProfileUpdate(PC->XPComponent->InMatchProfileVer);
		PC->XPComponent->OnProfileUpdated();
		PC->XPComponent->OnXpUpdated(PC->XPComponent->CombatXp, PC->XPComponent->SurvivalXp, PC->XPComponent->MedalBonusXP, PC->XPComponent->ChallengeXp, PC->XPComponent->MatchXp, PC->XPComponent->TotalXpEarned);
		PC->XPComponent->WaitingQuestXp.Add(Entry);

		//std::cout << PC->XPComponent->WaitingQuestXp.Num() << endl;
		PC->XPComponent->HighPrioXPEvent(Entry);
	}

	bool ContainsTag(FGameplayTagContainer Container, FName Tag)
	{
		for (auto tag : Container.GameplayTags)
		{
			if (tag.TagName.ComparisonIndex == Tag.ComparisonIndex)
				return true;
		}
		return false;
	}

	void SendStatEvent(UFortQuestManager* ManagerComp, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, int32 Count, EFortQuestObjectiveStatEvent StatEvent)
	{
		if (!ManagerComp || !TargetObject) {
			Log("ManagerComp or TargetObject is nullptr!");
		}

		if (TargetTags.GameplayTags.IsValid())
		{
			Log("Valid");
			for (int i = 0; i < TargetTags.GameplayTags.Num(); i++)
			{
				Log("TargetTags GameplayTags: " + TargetTags.GameplayTags[i].TagName.ToString());
			}
		}

		if (TargetTags.ParentTags.IsValid())
		{
			Log("Valid");
			for (int i = 0; i < TargetTags.ParentTags.Num(); i++)
			{
				Log("TargetTags ParentTags: " + TargetTags.ParentTags[i].TagName.ToString());
			}
		}

		/*if (AdditionalSourceTags.GameplayTags.IsValid())
		{
			Log("Valid");
			for (int i = 0; i < AdditionalSourceTags.GameplayTags.Num(); i++)
			{
				Log("AdditionalSourceTags GamePlayTags: " + AdditionalSourceTags.GameplayTags[i].TagName.ToString());
			}
		}

		if (AdditionalSourceTags.ParentTags.IsValid())
		{
			Log("Valid");
			for (int i = 0; i < AdditionalSourceTags.ParentTags.Num(); i++)
			{
				Log("AdditionalSourceTags ParentTags: " + AdditionalSourceTags.ParentTags[i].TagName.ToString());
			}
		}*/

		auto PC = (AFortPlayerControllerAthena*)ManagerComp->GetPlayerControllerBP();
		if (!PC) {
			Log("PC doesent exist!");
			return;
		}
		else {
			Log("PC does exist!");
		}

		TArray<UFortQuestItem*> CurrentQuests;
		ManagerComp->GetCurrentQuests(&CurrentQuests);

		if (CurrentQuests.IsValid())
		{
			for (int i = 0; i < CurrentQuests.Num(); i++)
			{
				UFortQuestItem* CurrentQuest = CurrentQuests[i];
				if (!CurrentQuest)
					continue;

				UFortQuestItemDefinition* QuestDefinition = CurrentQuest->GetQuestDefinitionBP();
				if (!QuestDefinition)
					continue;

				for (int i = 0; i < QuestDefinition->Objectives.Num(); i++)
				{
					FFortMcpQuestObjectiveInfo ObjectiveInfo = QuestDefinition->Objectives[i];
					FDataTableRowHandle ObjectStatHandle = ObjectiveInfo.ObjectiveStatHandle;
					UDataTable* DataTable = ObjectStatHandle.DataTable;
					if (!DataTable)
						continue;

					auto& RowMap = DataTable->RowMap;
					FFortQuestObjectiveStatTableRow* Row = nullptr;

					if (RowMap.Num() <= 0)
						continue;

					for (int i = 0; i < RowMap.Num(); ++i)
					{
						auto& Pair = RowMap[i];
						if (Pair.Key() == ObjectStatHandle.RowName)
						{
							Row = reinterpret_cast<FFortQuestObjectiveStatTableRow*>(Pair.Second);
							if (Row->TargetTagContainer.GameplayTags.IsValid() && Row->TargetTagContainer.GameplayTags.IsValidIndex(0))
							{
								if (Row->TargetTagContainer.GameplayTags[0].TagName == TargetTags.GameplayTags[0].TagName)
								{
									for (int i = 0; i < CurrentQuest->Objectives.Num(); i++)
									{
										UFortQuestObjectiveInfo* Objective = CurrentQuest->Objectives[i];
										if (!Objective)
											continue;

										FName BackendName = Objective->BackendName;

										Objective->AchievedCount += Count;
										Objective->bActive = true;
										Objective->DisplayDynamicQuestUpdate();
									}
									Log("Wow");
									ManagerComp->ForceTriggerQuestsUpdated();
									return ProgressQuest(PC, QuestDefinition, ObjectiveInfo.BackendName);
								}
								else {
									/*for (int i = 0; i < Row->TargetTagContainer.GameplayTags.Num(); i++) {
										Log(std::to_string(i));
										Log(Row->TargetTagContainer.GameplayTags[i].TagName.ToString());
									}*/
								}
							}
							//break;
						}
					}

					/*if (Row)
					{
						if (Row->TargetTagContainer.GameplayTags.IsValid() && Row->TargetTagContainer.GameplayTags.IsValidIndex(0))
						{
							if (Row->TargetTagContainer.GameplayTags[0].TagName == TargetTags.GameplayTags[0].TagName)
							{
								for (int i = 0; i < CurrentQuest->Objectives.Num(); i++)
								{
									UFortQuestObjectiveInfo* Objective = CurrentQuest->Objectives[i];
									if (!Objective)
										continue;

									FName BackendName = Objective->BackendName;

									Objective->AchievedCount += Count;
									Objective->bActive = true;
									Objective->DisplayDynamicQuestUpdate();
								}
								Log("Wow");
								ManagerComp->ForceTriggerQuestsUpdated();
								return ProgressQuest(PC, QuestDefinition, ObjectiveInfo.BackendName);
							}
							else {
								Log("No");
								Log(Row->TargetTagContainer.GameplayTags[0].TagName.ToString());
							}
						}
						else {
							Log("ffs");
						}
					}
					else {
						Log("No Row!");
					}*/
				}
			}
		}

		/*for (auto Quest : ManagerComp->CurrentQuests)
		{
			auto QuestDef = Quest->GetQuestDefinitionBP();
			if (!QuestDef) {
				Log("no QuestDef");
				continue;
			}

			if (ManagerComp->HasCompletedQuest(QuestDef))
				continue;

			auto PrereqQuest = QuestDef->GetPrerequisiteQuest();

			if (PrereqQuest && !ManagerComp->HasCompletedQuest(PrereqQuest))
				continue;

			if (QuestDef->PrerequisiteObjective.DataTable && QuestDef->PrerequisiteObjective.RowName.ComparisonIndex && !ManagerComp->HasCompletedObjective(QuestDef, QuestDef->PrerequisiteObjective))
				return;

			for (FFortMcpQuestObjectiveInfo& Objective : QuestDef->Objectives) {
				if (ManagerComp->HasCompletedObjectiveWithName(QuestDef, Objective.BackendName))
					continue;

				bool bFoundCorrectQuest = false;

				FDataTableRowHandle ObjectiveStatHandle = Objective.ObjectiveStatHandle;
				UDataTable* DataTable = ObjectiveStatHandle.DataTable;
				if (!DataTable)
					continue;

				auto& RowMap = DataTable->RowMap;
				FFortQuestObjectiveStatTableRow* Row = nullptr;

				if (RowMap.Num() <= 0)
					continue;

				for (int i = 0; i < RowMap.Num(); ++i)
				{
					auto& Pair = RowMap[i];
					if (Pair.Key().ToString() == ObjectiveStatHandle.RowName.ToString())
					{
						Log("Wow!");
						bFoundCorrectQuest = true;
						break;
					}
				}

				if (bFoundCorrectQuest) {
					return ProgressQuest(PC, QuestDef, Objective.BackendName);
				}
			}
		}*/
	}

	static inline void (*SendComplexCustomStatEventOG)(UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, int32 Count);
	void SendComplexCustomStatEvent(UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, int32 Count)
	{
		Log("SendComplexCustomStateEvent Called!");
		if (!TargetObject) {
			Log("No TargetObject!");
		}
		UFortQuestManager* ManagerComp = Cast<UFortQuestManager>(TargetObject);
		if (!ManagerComp) {
			Log("No ManagerComp!");
			return;
		}

		SendStatEvent(ManagerComp, TargetObject, AdditionalSourceTags, TargetTags, Count, EFortQuestObjectiveStatEvent::ComplexCustom);
	}

	/*static inline void (*SendComplexCustomStatEventOG)(UFortQuestManager* QuestManager, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count);
	static void SendComplexCustomStatEvent(UFortQuestManager* QuestManager, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count) {
		Log("SendComplexCustomStateEvent Called!");

		if (!QuestManager || !Count)
			return;

		if (__int64(_ReturnAddress()) == ImageBase + 0x2A009AC) {
			SendStatEvent(QuestManager, TargetObject, AdditionalSourceTags, TargetTags, QuestActive, QuestCompleted, Count, EFortQuestObjectiveStatEvent::ComplexCustom);
		}
		else
		{
			SendStatEvent(QuestManager, TargetObject, AdditionalSourceTags, TargetTags, nullptr, nullptr, 1, EFortQuestObjectiveStatEvent::ComplexCustom);
		}

		return SendComplexCustomStatEventOG(QuestManager, TargetObject, AdditionalSourceTags, TargetTags, QuestActive, QuestCompleted, Count);
	}*/

	static bool GetQuestContext(UFortQuestManager* QuestManager, EFortQuestObjectiveStatEvent SE) {
		Log("test!");
		Log(QuestManager->GetPlayerControllerBP()->InteractionComponent->InteractActor->GetName());
		FGameplayTagContainer SourceTags;
		FGameplayTagContainer ContextTags;
		FGameplayTagContainer PlaylistContextTags;

		QuestManager->GetSourceAndContextTags(&SourceTags, &ContextTags);
		for (auto& Tag : SourceTags.GameplayTags)
			Log(Tag.TagName.ToString());
		return true;
	}

	void Hook() {
		//MH_CreateHook((LPVOID)(ImageBase + 0x23B1420), SendComplexCustomStatEvent, (LPVOID*)&SendComplexCustomStatEventOG);
		//MH_CreateHook((LPVOID)(ImageBase + 0x23A5180), GetQuestContext, nullptr);

		Log("Quests Hooked!");
	}
}