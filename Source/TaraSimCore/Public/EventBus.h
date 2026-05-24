#pragma once

#include "CoreMinimal.h"
#include "EventPayloads.h"

// TEvent<T> — a typed pub/sub channel for one payload type.
// Pure C++; uses TFunction (from Core) so it has no UE delegate dependency.
//
// Mirrors src/sim/EventBus.ts from the 2D project, but takes advantage of C++
// templates: instead of a string-keyed lookup ("CattleDied" → Handler<...>),
// each event is its own typed channel that you access by member name on
// FEventBus. The compiler enforces payload correctness.

template<typename TPayload>
class TEvent
{
public:
	using Handler = TFunction<void(const TPayload&)>;

	int32 Subscribe(Handler InHandler)
	{
		int32 Id = NextId++;
		Handlers.Add(Id, MoveTemp(InHandler));
		return Id;
	}

	void Unsubscribe(int32 SubscriptionId)
	{
		Handlers.Remove(SubscriptionId);
	}

	void Emit(const TPayload& Payload) const
	{
		// Copy keys to a temp array so handlers may unsubscribe themselves
		// without invalidating the iteration.
		TArray<int32> Keys;
		Handlers.GetKeys(Keys);
		for (int32 Key : Keys)
		{
			if (const Handler* HandlerPtr = Handlers.Find(Key))
			{
				(*HandlerPtr)(Payload);
			}
		}
	}

	void Clear()
	{
		Handlers.Empty();
	}

private:
	TMap<int32, Handler> Handlers;
	int32 NextId = 1;
};

// FEventBus is just a bundle of TEvent<T> members — one per event type.
// To add a new event:
//   1) Add the payload struct to EventPayloads.h.
//   2) Add `TEvent<F...Payload> EventName;` here.
//   3) Use `Bus.EventName.Subscribe(...)` / `Bus.EventName.Emit(...)`.
//
// The bridge (UTaraSimSubsystem in TaraGame) subscribes to whichever of these
// the engine layer wants to react to, and re-emits via UE delegates so Blueprints
// / Widgets can consume them too. The sim itself never knows about the bridge.

class TARASIMCORE_API FEventBus
{
public:
	// M1 events.
	TEvent<FDayStartedPayload> DayStarted;
	TEvent<FDayEndedPayload> DayEnded;
	TEvent<FHerdConditionChangedPayload> HerdConditionChanged;
	TEvent<FCattleDiedPayload> CattleDied;

	// M2 events.
	TEvent<FWaterAccessLostPayload> WaterAccessLost;
	TEvent<FWaterAccessRestoredPayload> WaterAccessRestored;
	TEvent<FBoreFailedPayload> BoreFailed;
	TEvent<FBoreRepairedPayload> BoreRepaired;

	// Future milestones extend this list.
};
