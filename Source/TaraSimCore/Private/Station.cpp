#include "Station.h"

FStation::FStation()
{
	SeedDefaults();

	// Systems — construct in dependency order. Water + Economy FIRST so
	// Condition can read water access + active supplements on its tick.
	// Muster comes AFTER Economy because cancel-refund hits cash via
	// EconomySystem.AddCash; Condition still last.
	WaterSys = MakeUnique<FWaterSystem>(Bus, *this);
	WaterSys->RecomputeAccess();
	EconomySys = MakeUnique<FEconomySystem>(Bus, *this);
	MusterSys = MakeUnique<FMusteringSystem>(*this, Bus);
	EventSys = MakeUnique<FEventSystem>(*this, Bus);
	BreedingSys = MakeUnique<FBreedingSystem>(*this, Bus);
	WildlifeSys = MakeUnique<FWildlifeSystem>(*this, Bus);
	WildlifeSys->InitDefaults();
	ProgressionSys = MakeUnique<FProgressionSystem>(*this, Bus);
	SensorSys = MakeUnique<FSensorSystem>(*this, Bus);
	ConditionSys = MakeUnique<FConditionSystem>(Bus, *this);
}

void FStation::SeedDefaults()
{
	// Default M1 starter — three paddocks, one 8-head cohort in Home.
	FPaddockConfig HomeCfg;
	HomeCfg.Id = TEXT("A");
	HomeCfg.Name = TEXT("Home Paddock");
	HomeCfg.AreaHa = 200.0f;
	HomeCfg.StartingGrassKgPerHa = 2500.0f;
	Paddocks.Add(FPaddock(HomeCfg));

	FPaddockConfig RiverCfg;
	RiverCfg.Id = TEXT("B");
	RiverCfg.Name = TEXT("River Paddock");
	RiverCfg.AreaHa = 180.0f;
	RiverCfg.StartingGrassKgPerHa = 1800.0f;
	Paddocks.Add(FPaddock(RiverCfg));

	FPaddockConfig BackCfg;
	BackCfg.Id = TEXT("C");
	BackCfg.Name = TEXT("Back Paddock");
	BackCfg.AreaHa = 250.0f;
	BackCfg.StartingGrassKgPerHa = 2900.0f;
	Paddocks.Add(FPaddock(BackCfg));

	FCattleCohortConfig CohortCfg;
	CohortCfg.BirthYear = 0;
	CohortCfg.Count = 8;
	Herd = FHerd(FCattleCohort(CohortCfg), TEXT("A"));

	// M2 seed — adjacency A-B-C linear chain.
	Adjacency.Add(TEXT("A"), { TEXT("B") });
	Adjacency.Add(TEXT("B"), { TEXT("A"), TEXT("C") });
	Adjacency.Add(TEXT("C"), { TEXT("B") });

	// M2 seed — 2 bores: bore-1 serves Home only (single-paddock topology);
	// bore-2 serves River + Back (shared 2-paddock topology). Player meets
	// both topologies on day one — when bore-2 fails, both paddocks lose
	// water at the same time, teaching the design lesson.
	FBoreConfig Bore1Cfg;
	Bore1Cfg.Id = TEXT("bore-1");
	Bore1Cfg.ServesPaddockIds.Add(TEXT("A"));
	Bore1Cfg.Condition = 88.0f;
	Bores.Add(FBore(Bore1Cfg));

	FBoreConfig Bore2Cfg;
	Bore2Cfg.Id = TEXT("bore-2");
	Bore2Cfg.ServesPaddockIds.Add(TEXT("B"));
	Bore2Cfg.ServesPaddockIds.Add(TEXT("C"));
	Bore2Cfg.Condition = 82.0f;
	Bores.Add(FBore(Bore2Cfg));

	// M4a seed — the 6 vehicle types (foot + horse always owned; the rest
	// purchasable). 0 hired hands to start (the ringer works alone). Two
	// roads + two fences linking the A↔B↔C adjacency chain.
	for (EVehicleType T : FVehicle::AllTypes())
	{
		Vehicles.Add(FVehicle(T));
	}

	// Hands roster — three potential hires available at $80/day. The 2D
	// project's default seed mirrors this; player hires from the homestead.
	Hands.Add(FHand({ TEXT("hand-1"), TEXT("Wazza"), 60, 80, false }));
	Hands.Add(FHand({ TEXT("hand-2"), TEXT("Bek"),   70, 95, false }));
	Hands.Add(FHand({ TEXT("hand-3"), TEXT("Macca"), 45, 70, false }));

	// Roads on each adjacency edge.
	Roads.Add(FRoad({ TEXT("road-AB"), TEXT("A"), TEXT("B"), 60.0f }));
	Roads.Add(FRoad({ TEXT("road-BC"), TEXT("B"), TEXT("C"), 55.0f }));

	// Fences on each adjacency edge — start at 75 integrity so decay puts
	// the player into "needs fixing" within a year of in-game time.
	Fences.Add(FFence({ TEXT("fence-AB"), TEXT("A"), TEXT("B"), 75.0f }));
	Fences.Add(FFence({ TEXT("fence-BC"), TEXT("B"), TEXT("C"), 75.0f }));

	// M8 seed — the work-machine fleet (7 types, all unowned at start).
	for (EWorkMachineType T : FWorkMachine::AllTypes())
	{
		WorkMachines.Add(FWorkMachine(T));
	}
}

void FStation::TickDay()
{
	{
		FDayStartedPayload P;
		P.Day = Clock.DayOfYear;
		P.Year = Clock.Year;
		Bus.DayStarted.Emit(P);
	}

	Clock.TickDay();

	// Grass simulation reads the event-system's grass-growth multiplier
	// (drought cuts it ~70% at peak). Compute once per tick.
	const float GrowthMul = EventSys ? EventSys->GrassGrowthMultiplier() : 1.0f;
	for (FPaddock& P : Paddocks)
	{
		const int32 HerdHere = Herd.HerdInPaddock(P.Id);
		P.SimulateDay(Clock.GetSeason(), HerdHere, GrowthMul);
	}

	// Order: Water → Economy → Muster → Infrastructure → Events → Breeding →
	// Condition. Events runs late so its drought-severity write to Prices is
	// fresh for Economy's NEXT tick; Breeding before Condition so newly-born
	// calves can take their first condition drift. Condition always last.
	WaterSys->TickDay();
	EconomySys->TickDay(Clock.DayOfYear, Clock.Year);
	MusterSys->TickDay();
	TickInfrastructure();
	EventSys->TickDay();
	BreedingSys->TickDay();
	WildlifeSys->TickDay();
	ProgressionSys->TickDay();
	SensorSys->TickDay();
	ConditionSys->TickDay();
	Player.DaysOnStation += 1;

	{
		FDayEndedPayload P;
		P.Day = Clock.DayOfYear;
		P.Year = Clock.Year;
		Bus.DayEnded.Emit(P);
	}
}

const FPaddock* FStation::PaddockById(const FString& Id) const
{
	for (const FPaddock& P : Paddocks)
	{
		if (P.Id == Id) return &P;
	}
	return nullptr;
}

FPaddock* FStation::PaddockById(const FString& Id)
{
	for (FPaddock& P : Paddocks)
	{
		if (P.Id == Id) return &P;
	}
	return nullptr;
}

EWaterAccess FStation::GetWaterAccess(const FString& PaddockId) const
{
	return WaterSys->GetAccess(PaddockId);
}

FStation::FCheckBoreResult FStation::CheckBore(const FString& BoreId)
{
	const auto R = WaterSys->CheckBore(BoreId, Clock.DayOfYear);
	FCheckBoreResult Out;
	Out.bFixed = R.bFixed;
	Out.DaysSpent = R.DaysSpent;
	return Out;
}

const FVehicle* FStation::VehicleByType(EVehicleType Type) const
{
	for (const FVehicle& V : Vehicles)
	{
		if (V.Type == Type) return &V;
	}
	return nullptr;
}

FVehicle* FStation::VehicleByType(EVehicleType Type)
{
	for (FVehicle& V : Vehicles)
	{
		if (V.Type == Type) return &V;
	}
	return nullptr;
}

const FFence* FStation::FenceBetween(const FString& A, const FString& B) const
{
	for (const FFence& F : Fences)
	{
		if (F.Connects(A, B)) return &F;
	}
	return nullptr;
}

const FRoad* FStation::RoadBetween(const FString& A, const FString& B) const
{
	for (const FRoad& R : Roads)
	{
		if (R.Connects(A, B)) return &R;
	}
	return nullptr;
}

bool FStation::StartMuster(
	const FString& ToPaddockId,
	EVehicleType VehicleType,
	const TArray<FString>& HandIds,
	int32 PlannedDays,
	int32 FuelCost,
	int32 BreakawayPending)
{
	if (!Herd.StartMuster(ToPaddockId, PlannedDays, (int32)VehicleType, HandIds, FuelCost, BreakawayPending))
	{
		return false;
	}

	// Debit fuel cost up-front (refunded on cancel via MusteringSystem).
	if (FuelCost > 0)
	{
		EconomySys->AddCash(-FuelCost, TEXT("muster fuel"));
	}

	FMusterStartedPayload Payload;
	Payload.FromPaddockId = Herd.CurrentPaddockId;
	Payload.ToPaddockId = ToPaddockId;
	Payload.VehicleType = (int32)VehicleType;
	Payload.HandIds = HandIds;
	Payload.PlannedDays = PlannedDays;
	Payload.FuelCost = FuelCost;
	Payload.BreakawayPending = BreakawayPending;
	Bus.MusterStarted.Emit(Payload);
	return true;
}

bool FStation::BuyVehicle(EVehicleType Type)
{
	FVehicle* V = VehicleByType(Type);
	if (!V) return false;
	if (V->bOwned) return false;

	const int32 Cost = V->Stats.PurchasePrice;
	if (Cost <= 0) { V->bOwned = true; return true; }
	if (Player.Cash < Cost) return false;

	EconomySys->AddCash(-Cost, TEXT("vehicle purchase"));
	V->bOwned = true;

	FVehiclePurchasedPayload Payload;
	Payload.VehicleType = (int32)Type;
	Payload.Cost = Cost;
	Bus.VehiclePurchased.Emit(Payload);
	return true;
}

bool FStation::HireHand(const FString& HandId)
{
	for (FHand& H : Hands)
	{
		if (H.Id == HandId)
		{
			if (H.bHired) return false;
			H.bHired = true;
			FHandHiredPayload P; P.HandId = HandId;
			Bus.HandHired.Emit(P);
			return true;
		}
	}
	return false;
}

bool FStation::FireHand(const FString& HandId)
{
	for (FHand& H : Hands)
	{
		if (H.Id == HandId)
		{
			if (!H.bHired) return false;
			H.bHired = false;
			FHandFiredPayload P; P.HandId = HandId;
			Bus.HandFired.Emit(P);
			return true;
		}
	}
	return false;
}

bool FStation::RepairFence(const FString& FenceId)
{
	for (FFence& F : Fences)
	{
		if (F.Id == FenceId)
		{
			F.Integrity = 100.0f;
			FFenceRepairedPayload P; P.FenceId = FenceId;
			Bus.FenceRepaired.Emit(P);
			return true;
		}
	}
	return false;
}

void FStation::ResolveBadWeatherDecision(const FString& Choice)
{
	if (EventSys) EventSys->ResolveBadWeatherDecision(Choice);
}

bool FStation::BuyProperty()
{
	return ProgressionSys ? ProgressionSys->BuyProperty() : false;
}

const FWorkMachine* FStation::WorkMachineByType(EWorkMachineType Type) const
{
	for (const FWorkMachine& M : WorkMachines)
	{
		if (M.Type == Type) return &M;
	}
	return nullptr;
}

FWorkMachine* FStation::WorkMachineByType(EWorkMachineType Type)
{
	for (FWorkMachine& M : WorkMachines)
	{
		if (M.Type == Type) return &M;
	}
	return nullptr;
}

bool FStation::BuyWorkMachine(EWorkMachineType Type)
{
	FWorkMachine* M = WorkMachineByType(Type);
	if (!M) return false;
	if (M->bOwned) return false;
	const int32 Cost = M->Stats.PurchasePrice;
	if (Cost <= 0) { M->bOwned = true; return true; }
	if (Player.Cash < Cost) return false;

	EconomySys->AddCash(-Cost, TEXT("work-machine purchase"));
	M->bOwned = true;

	FWorkMachinePurchasedPayload P;
	P.Type = (int32)Type;
	P.Cost = Cost;
	Bus.WorkMachinePurchased.Emit(P);
	return true;
}

bool FStation::InstallSensor(ESensorKind Kind, const FString& LocationId)
{
	if (!SensorSys) return false;
	const FSensorProfile Profile = GetSensorProfile(Kind);
	if (Player.Cash < Profile.PurchasePrice) return false;
	EconomySys->AddCash(-Profile.PurchasePrice, TEXT("sensor purchase"));
	return SensorSys->InstallSensor(Kind, LocationId);
}

bool FStation::SwapSensorBattery(const FString& SensorId)
{
	return SensorSys ? SensorSys->SwapBattery(SensorId) : false;
}

bool FStation::GradeRoad(const FString& RoadId)
{
	for (FRoad& R : Roads)
	{
		if (R.Id == RoadId)
		{
			R.GradeQuality = FRoad::ClampGrade(R.GradeQuality + 50.0f);
			FRoadGradedPayload P; P.RoadId = RoadId;
			Bus.RoadGraded.Emit(P);
			return true;
		}
	}
	return false;
}

bool FStation::WeanCohort(int32 CalfCohortBirthYear)
{
	// Find the calf cohort + flip Unweaned → Weaner.
	FCattleCohort* Calf = nullptr;
	for (FCattleCohort& C : Herd.Cohorts)
	{
		if (C.BirthYear == CalfCohortBirthYear && C.State == ECohortState::Unweaned)
		{
			Calf = &C;
			break;
		}
	}
	if (!Calf) return false;

	Calf->State = ECohortState::Weaner;
	Calf->MusterTrainedness = 0.0f;   // schooling starts now

	// Find the lactating dam cohort (any breeder/heifer with bLactating == true).
	// Our aggregate-cohort model doesn't track per-individual dam-of relations;
	// the dam IS the lactating breeder cohort.
	int32 DamBirthYear = -1;
	for (FCattleCohort& C : Herd.Cohorts)
	{
		if (C.bLactating &&
			(C.State == ECohortState::Breeder || C.State == ECohortState::Heifer))
		{
			C.bLactating = false;
			DamBirthYear = C.BirthYear;
			break;
		}
	}

	FCohortWeanedPayload P;
	P.CalfCohortBirthYear = CalfCohortBirthYear;
	P.DamCohortBirthYear = DamBirthYear;
	Bus.CohortWeaned.Emit(P);
	return true;
}

int32 FStation::SplitCohort(int32 SourceCohortBirthYear, const TArray<FCohortSplitRequest>& Splits)
{
	if (Splits.Num() == 0) return 0;

	// Find the source cohort.
	int32 SrcIdx = -1;
	for (int32 I = 0; I < Herd.Cohorts.Num(); I++)
	{
		if (Herd.Cohorts[I].BirthYear == SourceCohortBirthYear)
		{
			SrcIdx = I;
			break;
		}
	}
	if (SrcIdx == -1) return 0;

	// Snapshot the source by value (appending new cohorts may invalidate any
	// reference into the TArray on growth).
	const FCattleCohort Source = Herd.Cohorts[SrcIdx];
	const int32 SourceCount = Source.Count;
	if (SourceCount <= 0) return 0;

	int32 RequestedTotal = 0;
	for (const FCohortSplitRequest& R : Splits) RequestedTotal += FMath::Max(0, R.Count);
	if (RequestedTotal <= 0) return 0;

	// Allocate counts proportionally to the requested split sizes, total
	// clamped to SourceCount. Any rounding leftover lands on the LAST entry
	// so we never lose head.
	TArray<int32> ActualCounts;
	ActualCounts.Reserve(Splits.Num());
	int32 AllocatedTotal = 0;
	for (int32 I = 0; I < Splits.Num(); I++)
	{
		const float Fraction = (RequestedTotal > 0)
			? (float)FMath::Max(0, Splits[I].Count) / (float)RequestedTotal
			: 0.0f;
		int32 Alloc = (I == Splits.Num() - 1)
			? (SourceCount - AllocatedTotal)
			: FMath::FloorToInt(SourceCount * Fraction);
		Alloc = FMath::Max(0, Alloc);
		ActualCounts.Add(Alloc);
		AllocatedTotal += Alloc;
	}

	// Remove the source cohort. Append the new sub-cohorts.
	Herd.Cohorts.RemoveAt(SrcIdx);

	int32 Created = 0;
	for (int32 I = 0; I < Splits.Num(); I++)
	{
		if (ActualCounts[I] <= 0) continue;
		FCattleCohortConfig Cfg;
		Cfg.BirthYear = Source.BirthYear;
		Cfg.Count = ActualCounts[I];
		Cfg.SexRatio = Source.SexRatio;
		Cfg.ConditionMean = Source.ConditionMean;
		Cfg.ConditionVariance = Source.ConditionVariance;
		Cfg.State = Source.State;
		Cfg.BehaviourProfile = Source.BehaviourProfile;
		Cfg.MusterTrainedness = Source.MusterTrainedness;
		Cfg.bLactating = Source.bLactating;
		Cfg.BrandedDay = Source.BrandedDay;
		Cfg.bHorned = Source.bHorned;
		Cfg.BreederStage = Splits[I].Stage;   // the field that diverges per split
		Herd.Cohorts.Add(FCattleCohort(Cfg));
		Created++;
	}

	FCohortSplitPayload P;
	P.SourceCohortBirthYear = SourceCohortBirthYear;
	P.NumSplits = Created;
	Bus.CohortSplit.Emit(P);
	return Created;
}

void FStation::TickInfrastructure()
{
	// Daily decay constants — mirror 2D defaults.
	const float FenceDecayPerDay = 0.10f;   // ~1% over 10 days
	const float RoadDecayPerDay = 0.05f;    // slower; graders restore in M8

	// Decay all fences; emit FenceBroken if integrity hits 0 this tick.
	for (FFence& F : Fences)
	{
		const float Prev = F.Integrity;
		F.Integrity = FFence::ClampIntegrity(F.Integrity - FenceDecayPerDay);
		if (Prev > 0.0f && F.Integrity <= 0.0f)
		{
			FFenceBrokenPayload P;
			P.FenceId = F.Id;
			P.PaddockA = F.PaddockA;
			P.PaddockB = F.PaddockB;
			Bus.FenceBroken.Emit(P);
		}
	}

	// Decay all roads (no event — quality is read at muster-plan time).
	for (FRoad& R : Roads)
	{
		R.GradeQuality = FRoad::ClampGrade(R.GradeQuality - RoadDecayPerDay);
	}

	// Broken-fence drift: if the herd's paddock has any fence at <=0 integrity
	// AND the herd is NOT in transit, a small number of head drift to the
	// adjacent paddock. Mirrors 2D Fence.ts drift behaviour.
	if (Herd.IsInTransit()) return;

	const FString HerdPaddock = Herd.CurrentPaddockId;
	for (const FFence& F : Fences)
	{
		if (F.Integrity > 0.0f) continue;
		if (F.PaddockA != HerdPaddock && F.PaddockB != HerdPaddock) continue;
		const FString OtherPaddock = (F.PaddockA == HerdPaddock) ? F.PaddockB : F.PaddockA;

		// Drift count scales with herd size; cap at 5 head/day per broken fence.
		const int32 DriftCount = FMath::Clamp(Herd.GetSize() / 20, 1, 5);
		const int32 Drifted = Herd.ApplyBreakaway(DriftCount);
		if (Drifted > 0)
		{
			FCattleDriftedPayload P;
			P.FromPaddockId = HerdPaddock;
			P.ToPaddockId = OtherPaddock;
			P.Count = Drifted;
			Bus.CattleDrifted.Emit(P);
		}
		break;  // one drift event per tick is plenty
	}
}

FString FStation::SerializeJson() const
{
	FString PaddocksJson = TEXT("[");
	for (int32 I = 0; I < Paddocks.Num(); I++)
	{
		if (I > 0) PaddocksJson += TEXT(",");
		PaddocksJson += Paddocks[I].SerializeJson();
	}
	PaddocksJson += TEXT("]");

	FString BoresJson = TEXT("[");
	for (int32 I = 0; I < Bores.Num(); I++)
	{
		if (I > 0) BoresJson += TEXT(",");
		BoresJson += Bores[I].SerializeJson();
	}
	BoresJson += TEXT("]");

	// Adjacency: {"A":["B"],"B":["A","C"],"C":["B"]}
	FString AdjJson = TEXT("{");
	bool bFirstKey = true;
	for (const auto& Pair : Adjacency)
	{
		if (!bFirstKey) AdjJson += TEXT(",");
		bFirstKey = false;
		AdjJson += FString::Printf(TEXT("\"%s\":["), *Pair.Key);
		for (int32 I = 0; I < Pair.Value.Num(); I++)
		{
			if (I > 0) AdjJson += TEXT(",");
			AdjJson += FString::Printf(TEXT("\"%s\""), *Pair.Value[I]);
		}
		AdjJson += TEXT("]");
	}
	AdjJson += TEXT("}");

	// M4a — hands, vehicles, fences, roads arrays.
	auto SerializeArray = [](auto& Container) -> FString
	{
		FString S = TEXT("[");
		bool bFirst = true;
		for (const auto& Item : Container)
		{
			if (!bFirst) S += TEXT(",");
			bFirst = false;
			S += Item.SerializeJson();
		}
		S += TEXT("]");
		return S;
	};
	const FString HandsJson = SerializeArray(Hands);
	const FString VehiclesJson = SerializeArray(Vehicles);
	const FString FencesJson = SerializeArray(Fences);
	const FString RoadsJson = SerializeArray(Roads);

	const FString EventsJson = EventSys ? EventSys->SerializeJson() : TEXT("{}");
	const FString BreedingJson = BreedingSys ? BreedingSys->SerializeJson() : TEXT("{}");
	const FString WildlifeJson = WildlifeSys ? WildlifeSys->SerializeJson() : TEXT("{}");
	const FString ProgressionJson = ProgressionSys ? ProgressionSys->SerializeJson() : TEXT("{}");
	const FString SensorJson = SensorSys ? SensorSys->SerializeJson() : TEXT("{}");
	const FString WorkMachinesJson = SerializeArray(WorkMachines);

	return FString::Printf(
		TEXT("{\"schemaVersion\":\"%s\",\"clock\":%s,\"paddocks\":%s,\"herd\":%s,\"adjacency\":%s,\"bores\":%s,\"player\":%s,\"prices\":%s,\"economy\":%s,\"hands\":%s,\"vehicles\":%s,\"fences\":%s,\"roads\":%s,\"events\":%s,\"breeding\":%s,\"wildlife\":%s,\"progression\":%s,\"workMachines\":%s,\"sensors\":%s}"),
		TARA_SIM_SAVE_SCHEMA_VERSION,
		*Clock.SerializeJson(),
		*PaddocksJson,
		*Herd.SerializeJson(),
		*AdjJson,
		*BoresJson,
		*Player.SerializeJson(),
		*Prices.SerializeJson(),
		*EconomySys->SerializeJson(),
		*HandsJson,
		*VehiclesJson,
		*FencesJson,
		*RoadsJson,
		*EventsJson,
		*BreedingJson,
		*WildlifeJson,
		*ProgressionJson,
		*WorkMachinesJson,
		*SensorJson);
}

namespace
{
	// Helper — pull the JSON object substring starting at the first '{' after `Token`.
	FString ExtractObject(const FString& Json, const FString& Token)
	{
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return FString();
		int32 Start = Idx + Token.Len();
		while (Start < Json.Len() && Json[Start] != '{') Start++;
		if (Start >= Json.Len()) return FString();
		int32 Depth = 0;
		int32 End = Start;
		for (; End < Json.Len(); End++)
		{
			if (Json[End] == '{') Depth++;
			else if (Json[End] == '}')
			{
				Depth--;
				if (Depth == 0) { End++; break; }
			}
		}
		return Json.Mid(Start, End - Start);
	}
}

TUniquePtr<FStation> FStation::FromJson(const FString& Json)
{
	// Schema check first.
	const FString SchemaToken = TEXT("\"schemaVersion\":\"");
	const int32 SchemaIdx = Json.Find(SchemaToken);
	if (SchemaIdx == INDEX_NONE) return nullptr;
	const int32 SchemaStart = SchemaIdx + SchemaToken.Len();
	const int32 SchemaEnd = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, SchemaStart);
	if (SchemaEnd == INDEX_NONE) return nullptr;
	const FString SchemaVersion = Json.Mid(SchemaStart, SchemaEnd - SchemaStart);
	if (SchemaVersion != TARA_SIM_SAVE_SCHEMA_VERSION)
	{
		// Schema mismatch — discard. Subsystem starts a fresh game.
		return nullptr;
	}

	auto Station = MakeUnique<FStation>();
	// Wipe defaults — we're about to overwrite them from the snapshot.
	Station->Paddocks.Empty();
	Station->Bores.Empty();
	Station->Adjacency.Empty();

	// Clock.
	const FString ClockJson = ExtractObject(Json, TEXT("\"clock\":"));
	if (!ClockJson.IsEmpty()) Station->Clock = FSeasonClock::FromJson(ClockJson);

	// Herd.
	const FString HerdJson = ExtractObject(Json, TEXT("\"herd\":"));
	if (!HerdJson.IsEmpty()) Station->Herd = FHerd::FromJson(HerdJson);

	// Paddocks array.
	{
		const FString PdkToken = TEXT("\"paddocks\":[");
		const int32 PdkIdx = Json.Find(PdkToken);
		if (PdkIdx != INDEX_NONE)
		{
			int32 Depth = 0;
			int32 ObjStart = INDEX_NONE;
			for (int32 P = PdkIdx + PdkToken.Len(); P < Json.Len(); P++)
			{
				const TCHAR Ch = Json[P];
				if (Ch == '{')
				{
					if (Depth == 0) ObjStart = P;
					Depth++;
				}
				else if (Ch == '}')
				{
					Depth--;
					if (Depth == 0 && ObjStart != INDEX_NONE)
					{
						Station->Paddocks.Add(FPaddock::FromJson(Json.Mid(ObjStart, P - ObjStart + 1)));
						ObjStart = INDEX_NONE;
					}
				}
				else if (Ch == ']' && Depth == 0)
				{
					break;
				}
			}
		}
	}

	// Bores array.
	{
		const FString BorToken = TEXT("\"bores\":[");
		const int32 BorIdx = Json.Find(BorToken);
		if (BorIdx != INDEX_NONE)
		{
			int32 Depth = 0;
			int32 ObjStart = INDEX_NONE;
			for (int32 P = BorIdx + BorToken.Len(); P < Json.Len(); P++)
			{
				const TCHAR Ch = Json[P];
				if (Ch == '{')
				{
					if (Depth == 0) ObjStart = P;
					Depth++;
				}
				else if (Ch == '}')
				{
					Depth--;
					if (Depth == 0 && ObjStart != INDEX_NONE)
					{
						Station->Bores.Add(FBore::FromJson(Json.Mid(ObjStart, P - ObjStart + 1)));
						ObjStart = INDEX_NONE;
					}
				}
				else if (Ch == ']' && Depth == 0)
				{
					break;
				}
			}
		}
	}

	// Adjacency: {"A":["B"],"B":["A","C"],"C":["B"]}
	{
		const FString AdjToken = TEXT("\"adjacency\":{");
		const int32 AdjIdx = Json.Find(AdjToken);
		if (AdjIdx != INDEX_NONE)
		{
			int32 P = AdjIdx + AdjToken.Len();
			while (P < Json.Len())
			{
				if (Json[P] == '}') break;
				if (Json[P] != '"') { P++; continue; }

				const int32 KeyStart = P + 1;
				const int32 KeyEnd = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, KeyStart);
				if (KeyEnd == INDEX_NONE) break;
				const FString Key = Json.Mid(KeyStart, KeyEnd - KeyStart);
				P = KeyEnd + 1;
				while (P < Json.Len() && Json[P] != '[') P++;
				if (P >= Json.Len()) break;
				P++;
				TArray<FString> Values;
				while (P < Json.Len() && Json[P] != ']')
				{
					if (Json[P] == '"')
					{
						const int32 VS = P + 1;
						const int32 VE = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, VS);
						if (VE == INDEX_NONE) { P = Json.Len(); break; }
						Values.Add(Json.Mid(VS, VE - VS));
						P = VE + 1;
					}
					else
					{
						P++;
					}
				}
				Station->Adjacency.Add(Key, Values);
				P++;
			}
		}
	}

	// Player + Prices + Economy.
	{
		const FString PlayerJson = ExtractObject(Json, TEXT("\"player\":"));
		if (!PlayerJson.IsEmpty()) Station->Player = FPlayer::FromJson(PlayerJson);
	}
	{
		const FString PricesJson = ExtractObject(Json, TEXT("\"prices\":"));
		if (!PricesJson.IsEmpty()) Station->Prices = FPriceTable::FromJson(PricesJson);
	}
	{
		const FString EconJson = ExtractObject(Json, TEXT("\"economy\":"));
		if (!EconJson.IsEmpty() && Station->EconomySys) Station->EconomySys->LoadFromJson(EconJson);
	}

	// M4a — hands, vehicles, fences, roads arrays.
	// Generic object-array extractor mirroring the paddocks/bores loops above.
	auto ParseObjectArray = [&Json](const FString& ArrayToken, auto OnObject)
	{
		const int32 ArrIdx = Json.Find(ArrayToken);
		if (ArrIdx == INDEX_NONE) return;
		int32 Depth = 0;
		int32 ObjStart = INDEX_NONE;
		for (int32 P = ArrIdx + ArrayToken.Len(); P < Json.Len(); P++)
		{
			const TCHAR Ch = Json[P];
			if (Ch == '{')
			{
				if (Depth == 0) ObjStart = P;
				Depth++;
			}
			else if (Ch == '}')
			{
				Depth--;
				if (Depth == 0 && ObjStart != INDEX_NONE)
				{
					OnObject(Json.Mid(ObjStart, P - ObjStart + 1));
					ObjStart = INDEX_NONE;
				}
			}
			else if (Ch == ']' && Depth == 0)
			{
				break;
			}
		}
	};

	Station->Hands.Empty();
	ParseObjectArray(TEXT("\"hands\":["), [&](const FString& Obj)
	{
		Station->Hands.Add(FHand::FromJson(Obj));
	});

	Station->Vehicles.Empty();
	ParseObjectArray(TEXT("\"vehicles\":["), [&](const FString& Obj)
	{
		Station->Vehicles.Add(FVehicle::FromJson(Obj));
	});

	Station->Fences.Empty();
	ParseObjectArray(TEXT("\"fences\":["), [&](const FString& Obj)
	{
		Station->Fences.Add(FFence::FromJson(Obj));
	});

	Station->Roads.Empty();
	ParseObjectArray(TEXT("\"roads\":["), [&](const FString& Obj)
	{
		Station->Roads.Add(FRoad::FromJson(Obj));
	});

	// M5 — events + breeding sub-objects.
	{
		const FString EventsJson = ExtractObject(Json, TEXT("\"events\":"));
		if (!EventsJson.IsEmpty() && Station->EventSys) Station->EventSys->LoadFromJson(EventsJson);
	}
	{
		const FString BreedingJson = ExtractObject(Json, TEXT("\"breeding\":"));
		if (!BreedingJson.IsEmpty() && Station->BreedingSys) Station->BreedingSys->LoadFromJson(BreedingJson);
	}
	// M6 — wildlife sub-object.
	{
		const FString WildlifeJson = ExtractObject(Json, TEXT("\"wildlife\":"));
		if (!WildlifeJson.IsEmpty() && Station->WildlifeSys) Station->WildlifeSys->LoadFromJson(WildlifeJson);
	}
	// M7 — progression sub-object.
	{
		const FString ProgressionJson = ExtractObject(Json, TEXT("\"progression\":"));
		if (!ProgressionJson.IsEmpty() && Station->ProgressionSys) Station->ProgressionSys->LoadFromJson(ProgressionJson);
	}

	// M8 — work machines array + sensors sub-object.
	Station->WorkMachines.Empty();
	ParseObjectArray(TEXT("\"workMachines\":["), [&](const FString& Obj)
	{
		Station->WorkMachines.Add(FWorkMachine::FromJson(Obj));
	});
	{
		const FString SensorJson = ExtractObject(Json, TEXT("\"sensors\":"));
		if (!SensorJson.IsEmpty() && Station->SensorSys) Station->SensorSys->LoadFromJson(SensorJson);
	}

	// Reset water access cache for the restored topology.
	if (Station->WaterSys) Station->WaterSys->RecomputeAccess();

	return Station;
}
