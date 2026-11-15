#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractComponent.generated.h"

// Add this component to your player character. It traces from the camera on a
// timer and calls StartLookAt / StopLookAt on actors implementing IInteractInterface.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// How far ahead of the camera the trace reaches (Unreal units).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	float TraceDistance = 300.f;

	// Seconds between traces. 0.05 = 20 times per second.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	float TraceInterval = 0.05f;
	
	// The interactable actor currently being looked at, or null if none.
	// Callable from Blueprint as "Get Hit Actor" on the component.
	UFUNCTION(BlueprintPure, Category = "Interact")
	AActor* GetHitActor() const { return StoredActor.Get(); }

protected:
	// Starts the looping trace timer.
	virtual void BeginPlay() override;

	// Stops the timer and sends a final Stop to the current actor, if any.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Runs every TraceInterval seconds. Does the trace and handles Start/Stop.
	void InteractionTrace();

	// Handle for the looping timer, so it can be cleared in EndPlay.
	FTimerHandle TraceTimer;

	// The actor we're currently "looking at" (your old Hit Actor variable).
	// Weak pointer so it becomes null safely if the actor is destroyed.
	TWeakObjectPtr<AActor> StoredActor;
};