
#include "InteractComponent.h"
#include "Interfaces/InteractInterface.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UInteractComponent::BeginPlay()
{
    Super::BeginPlay();

    // Calls InteractionTrace every TraceInterval seconds.
    GetWorld()->GetTimerManager().SetTimer(
        TraceTimer,
        this,
        &UInteractComponent::InteractionTrace,
        TraceInterval,
        true
    );
}

void UInteractComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Stop the timer.
    GetWorld()->GetTimerManager().ClearTimer(TraceTimer);

    // Stop looking at the previous actor.
    if (AActor* OldActor = StoredActor.Get())
    {
        IInteractInterface::Execute_StopLookAt(OldActor, GetOwner());
    }

    StoredActor.Reset();

    Super::EndPlay(EndPlayReason);
}


void UInteractComponent::InteractionTrace()
{
    // Determine what we're looking at.
    AActor* NewActor = nullptr;

    // Get the owning player controller.
    APlayerController* PlayerController = nullptr;

    if (APawn* OwningPawn = Cast<APawn>(GetOwner()))
    {
        PlayerController = Cast<APlayerController>(OwningPawn->GetController());
    }
    else
    {
        PlayerController = Cast<APlayerController>(GetOwner());
    }

    // Get the player's camera viewpoint.
    if (PlayerController)
    {
        FVector Start;
        FRotator ViewRotation;

        PlayerController->GetPlayerViewPoint(Start, ViewRotation);

        const FVector End = Start + ViewRotation.Vector() * TraceDistance;

        FHitResult Hit;
        FCollisionQueryParams Params;

        Params.AddIgnoredActor(GetOwner());

        // Perform the interaction trace.
        if (GetWorld()->LineTraceSingleByChannel(
            Hit,
            Start,
            End,
            ECC_Visibility,
            Params))
        {
            AActor* HitActor = Hit.GetActor();

            if (HitActor && HitActor->Implements<UInteractInterface>())
            {
                NewActor = HitActor;
            }
        }
    }

    // Compare with previous target.
    AActor* OldActor = StoredActor.Get();

    // Nothing changed.
    if (NewActor == OldActor)
    {
        return;
    }

    // Stop looking at previous actor.
    if (OldActor)
    {
        IInteractInterface::Execute_StopLookAt(OldActor, GetOwner());
    }

    // Store new target.
    StoredActor = NewActor;

    // Start looking at new actor.
    if (NewActor)
    {
        IInteractInterface::Execute_LookAt(NewActor, GetOwner());
    }
}

