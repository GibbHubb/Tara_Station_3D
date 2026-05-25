#include "Actors/RoadGradeInteract.h"
#include "Bridge/TaraSimSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

ARoadGradeInteract::ARoadGradeInteract()
{
	PromptVerb = NSLOCTEXT("Tara", "RoadGradePromptVerb", "Grade road");
}

bool ARoadGradeInteract::IsInteractAvailable_Implementation() const
{
	if (RoadId.IsEmpty()) return false;

	UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
	UTaraSimSubsystem* Sub = GI ? GI->GetSubsystem<UTaraSimSubsystem>() : nullptr;
	if (!Sub) return false;

	return Sub->GetRoadGradeQuality(RoadId) < GradeThreshold;
}

bool ARoadGradeInteract::OnInteract_Implementation(APawn* /*InstigatorPawn*/)
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
	UTaraSimSubsystem* Sub = GI ? GI->GetSubsystem<UTaraSimSubsystem>() : nullptr;
	if (!Sub) return false;

	const bool bGraded = Sub->GradeRoad(RoadId);
	UE_LOG(LogTemp, Display, TEXT("[RoadGradeInteract] %s: %s"),
		*RoadId, bGraded ? TEXT("graded") : TEXT("grading failed"));

	RefreshPrompt();
	return bGraded;
}
