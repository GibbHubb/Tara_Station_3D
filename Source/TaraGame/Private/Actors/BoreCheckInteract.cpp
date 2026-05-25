#include "Actors/BoreCheckInteract.h"
#include "Bridge/TaraSimSubsystem.h"
#include "Station.h"
#include "Entities/Bore.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

ABoreCheckInteract::ABoreCheckInteract()
{
	PromptVerb = NSLOCTEXT("Tara", "BoreCheckPromptVerb", "Check bore");
}

bool ABoreCheckInteract::IsInteractAvailable_Implementation() const
{
	if (BoreId.IsEmpty()) return false;

	UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
	UTaraSimSubsystem* Sub = GI ? GI->GetSubsystem<UTaraSimSubsystem>() : nullptr;
	if (!Sub || !Sub->GetStation()) return false;

	// Walk the bores; show prompt iff this one's condition is below threshold.
	for (const FBore& B : Sub->GetStation()->GetBores())
	{
		if (B.Id == BoreId)
		{
			return B.Condition < ConditionThreshold;
		}
	}
	return false;
}

bool ABoreCheckInteract::OnInteract_Implementation(APawn* /*InstigatorPawn*/)
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
	UTaraSimSubsystem* Sub = GI ? GI->GetSubsystem<UTaraSimSubsystem>() : nullptr;
	if (!Sub) return false;

	const bool bFixed = Sub->CheckBore(BoreId);
	UE_LOG(LogTemp, Display, TEXT("[BoreCheckInteract] %s: %s"),
		*BoreId, bFixed ? TEXT("repaired") : TEXT("couldn't fix"));

	// Refresh prompt — if we fixed it, the prompt should hide.
	RefreshPrompt();
	return bFixed;
}
