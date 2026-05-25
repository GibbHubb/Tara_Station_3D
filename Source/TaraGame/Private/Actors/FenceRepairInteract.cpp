#include "Actors/FenceRepairInteract.h"
#include "Bridge/TaraSimSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

AFenceRepairInteract::AFenceRepairInteract()
{
	PromptVerb = NSLOCTEXT("Tara", "FenceRepairPromptVerb", "Repair fence");
}

bool AFenceRepairInteract::IsInteractAvailable_Implementation() const
{
	if (FenceId.IsEmpty()) return false;

	UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
	UTaraSimSubsystem* Sub = GI ? GI->GetSubsystem<UTaraSimSubsystem>() : nullptr;
	if (!Sub) return false;

	return Sub->GetFenceIntegrity(FenceId) < IntegrityThreshold;
}

bool AFenceRepairInteract::OnInteract_Implementation(APawn* /*InstigatorPawn*/)
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
	UTaraSimSubsystem* Sub = GI ? GI->GetSubsystem<UTaraSimSubsystem>() : nullptr;
	if (!Sub) return false;

	const bool bRepaired = Sub->RepairFence(FenceId);
	UE_LOG(LogTemp, Display, TEXT("[FenceRepairInteract] %s: %s"),
		*FenceId, bRepaired ? TEXT("repaired") : TEXT("repair failed"));

	RefreshPrompt();
	return bRepaired;
}
