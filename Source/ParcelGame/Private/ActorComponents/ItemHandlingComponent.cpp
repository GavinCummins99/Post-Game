// ItemHandlingComponent.cpp
// Pick up, hold, preview, rotate, place and drop items on grids.
// ECC_GameTraceChannel1 is the "GridSurface" channel: grid boxes and items block it.
// The trace drives IInteractInterface: LookAt/StopLookAt fire as the target changes,
// and Interact() calls Interact on the looked-at actor.

#include "ActorComponents/ItemHandlingComponent.h"
#include "Actors/HandleableItem.h"
#include "Actors/GridSurface.h"
#include "Interfaces/InteractInterface.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"

UItemHandlingComponent::UItemHandlingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UItemHandlingComponent::BeginPlay()
{
    Super::BeginPlay();
    Slots.Init(nullptr, FMath::Max(1, MaxSlots));

    Preview = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("PlacementPreview"));
    Preview->SetMobility(EComponentMobility::Movable);
    Preview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Preview->SetCastShadow(false);
    Preview->RegisterComponent();
    Preview->SetVisibility(false);
}

void UItemHandlingComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    SetLookedAtActor(nullptr);
    Super::EndPlay(Reason);
}

bool UItemHandlingComponent::GetView(FVector& Loc, FRotator& Rot) const
{
    const APawn* P = Cast<APawn>(GetOwner());
    if (!P || !P->GetController()) return false;

    P->GetController()->GetPlayerViewPoint(Loc, Rot);
    return true;
}

void UItemHandlingComponent::Fail(EInteractResult Reason)
{
    if (bShowDebug && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(201, 2.f, FColor::Red,
            FString::Printf(TEXT("Interact failed: %s"), *UEnum::GetValueAsString(Reason)));
    }
    OnInteractFailed.Broadcast(Reason);
}

AHandleableItem* UItemHandlingComponent::GetActiveItem() const
{
    return Slots.IsValidIndex(ActiveSlot) ? Slots[ActiveSlot] : nullptr;
}

AHandleableItem* UItemHandlingComponent::GetSlotItem(int32 Index) const
{
    return Slots.IsValidIndex(Index) ? Slots[Index] : nullptr;
}

bool UItemHandlingComponent::HasFreeSlot() const
{
    return Slots.Contains(nullptr);
}

void UItemHandlingComponent::SelectSlot(int32 Index)
{
    if (Slots.IsValidIndex(Index)) ActiveSlot = Index;
}

// Moves the selection to the first filled slot, starting from the current one.
void UItemHandlingComponent::SelectNextFilled()
{
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        const int32 Idx = (ActiveSlot + i) % Slots.Num();
        if (Slots[Idx])
        {
            ActiveSlot = Idx;
            return;
        }
    }
}

// Safe to call every frame; the interface is only called when the target changes.
void UItemHandlingComponent::SetLookedAtActor(AActor* NewActor)
{
    if (LookedAtActor == NewActor) return;

    AActor* Old = LookedAtActor;
    LookedAtActor = NewActor;

    if (IsValid(Old) && Old->Implements<UInteractInterface>())
        IInteractInterface::Execute_StopLookAt(Old, GetOwner());

    if (IsValid(NewActor) && NewActor->Implements<UInteractInterface>())
        IInteractInterface::Execute_LookAt(NewActor, GetOwner());
}

bool UItemHandlingComponent::Interact()
{
    AActor* Target = LookedAtActor;

    if (!IsValid(Target))
    {
        Fail(EInteractResult::NoTarget);
        return false;
    }

    if (!Target->Implements<UInteractInterface>())
    {
        Fail(EInteractResult::NotInteractable);
        return false;
    }

    if (bShowDebug && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(202, 2.f, FColor::Green,
            FString::Printf(TEXT("Interact -> %s"), *Target->GetName()));
    }

    IInteractInterface::Execute_Interact(Target, GetOwner());
    return true;
}

bool UItemHandlingComponent::PickUpItem(AHandleableItem* Item)
{
    if (!IsValid(Item)) return false;

    const int32 FreeSlot = Slots.IndexOfByKey(nullptr);
    if (FreeSlot == INDEX_NONE)
    {
        Fail(EInteractResult::HandsFull);
        return false;
    }

    if (Item->CurrentGrid)
    {
        Item->CurrentGrid->FreeActor(Item);
        Item->CurrentGrid = nullptr;
    }

    Item->SetHeld();
    if (LookedAtActor == Item) SetLookedAtActor(nullptr);

    Slots[FreeSlot] = Item;
    ActiveSlot = FreeSlot;

    OnItemPickedUp.Broadcast(Item, FreeSlot);
    return true;
}

bool UItemHandlingComponent::PlaceActiveItem(AGridSurface* Grid)
{
    // Relies on the target state computed in TickComponent this frame.
    AHandleableItem* Item = GetActiveItem();
    if (!Item)
    {
        Fail(EInteractResult::NoItemHeld);
        return false;
    }

    if (!Grid || Grid != TargetGrid.Get() || !bTargetValid)
    {
        Fail(EInteractResult::InvalidPlacement);
        return false;
    }

    Item->SetActorTransform(GetItemTransformOnGrid(Item, Grid, TargetOrigin, TargetSize), false, nullptr, ETeleportType::TeleportPhysics);
    Item->SetPlaced();
    Item->CurrentGrid = Grid;
    Grid->Occupy(TargetOrigin, TargetSize, Item);

    Slots[ActiveSlot] = nullptr;
    SelectNextFilled();
    HidePreview();

    OnItemPlaced.Broadcast(Item, Grid);
    return true;
}

void UItemHandlingComponent::DropActive()
{
    AHandleableItem* Item = GetActiveItem();
    FVector L; FRotator R;
    if (!Item || !GetView(L, R)) return;

    Item->SetActorLocationAndRotation(L + R.Vector() * DropDistance, FRotator(0.f, R.Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
    Item->SetInWorld(true);

    Slots[ActiveSlot] = nullptr;
    SelectNextFilled();

    OnItemDropped.Broadcast(Item);
}

// Rotation is stored per item as a step of 0-3 (90 degrees each).
void UItemHandlingComponent::Rotate(float AxisValue)
{
    AHandleableItem* Item = GetActiveItem();
    if (!Item || FMath::IsNearlyZero(AxisValue)) return;

    Item->RotationStep = (Item->RotationStep + (AxisValue > 0.f ? 1 : 3)) % 4;
}

// Centres the item on its footprint and sits it on the surface, regardless of mesh pivot.
FTransform UItemHandlingComponent::GetItemTransformOnGrid(AHandleableItem* Item, AGridSurface* Grid, FIntPoint Origin, FIntPoint Size) const
{
    const FTransform T = Grid->GetPlacementTransform(Origin, Size, Item->RotationStep);
    const FVector Loc = T.GetLocation() + T.GetRotation().RotateVector(Item->GetPivotOffset());

    return FTransform(T.GetRotation(), Loc, Item->GetActorScale3D());
}

void UItemHandlingComponent::HidePreview()
{
    if (Preview) Preview->SetVisibility(false);
}

void UItemHandlingComponent::TickComponent(float DT, ELevelTick TT, FActorComponentTickFunction* F)
{
    Super::TickComponent(DT, TT, F);

    if (bShowDebug && GEngine)
    {
        for (int32 i = 0; i < Slots.Num(); ++i)
        {
            const FString Name = Slots[i] ? Slots[i]->GetName() : TEXT("(empty)");
            GEngine->AddOnScreenDebugMessage(100 + i, 0.f, i == ActiveSlot ? FColor::Yellow : FColor::White,
                FString::Printf(TEXT("Slot %d: %s"), i + 1, *Name));
        }

        GEngine->AddOnScreenDebugMessage(200, 0.f, FColor::Cyan,
            FString::Printf(TEXT("Hit: %s | LookedAt: %s"), *GetNameSafe(HitActor), *GetNameSafe(LookedAtActor)));
    }

    // PlaceActiveItem relies on this per-frame state.
    bTargetValid = false;
    TargetGrid = nullptr;
    HitActor = nullptr;

    AHandleableItem* Held = GetActiveItem();

    if (Held && Preview && PreviewItem.Get() != Held)
    {
        PreviewItem = Held;
        Preview->SetStaticMesh(Held->Mesh->GetStaticMesh());
        Preview->SetWorldScale3D(Held->GetActorScale3D());

        if (PreviewMaterial)
        {
            for (int32 i = 0; i < Preview->GetNumMaterials(); ++i)
                Preview->SetMaterial(i, PreviewMaterial);
        }
    }

    FVector L; FRotator R;
    if (!GetView(L, R))
    {
        HidePreview();
        SetLookedAtActor(nullptr);
        return;
    }

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());

    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, L, L + R.Vector() * InteractDistance, ECC_GameTraceChannel1, Params);

    AActor* LookTarget = nullptr;
    AGridSurface* Grid = nullptr;

    if (bHit)
    {
        HitActor = Hit.GetActor();
        LookTarget = HitActor;
        Grid = Cast<AGridSurface>(HitActor);
    }

    if (Grid)
    {
        if (Held)
        {
            TargetGrid   = Grid;
            TargetSize   = AGridSurface::GetRotatedSize(Held->Footprint, Held->RotationStep);
            TargetOrigin = Grid->WorldToOrigin(Hit.ImpactPoint, TargetSize);
            bTargetValid = Grid->CanPlace(TargetOrigin, TargetSize);
        }

        // Covers placed items shorter than the grid box, where the trace reaches the box.
        if (!bTargetValid)
        {
            if (AActor* Occupant = Grid->GetOccupantAt(Hit.ImpactPoint))
                LookTarget = Occupant;
        }
    }

    if (Held && Preview && bTargetValid)
    {
        const FTransform T = GetItemTransformOnGrid(Held, TargetGrid.Get(), TargetOrigin, TargetSize);
        Preview->SetWorldLocationAndRotation(T.GetLocation(), T.GetRotation());
        Preview->SetVisibility(true);

        if (PreviewMaterial)
            Preview->SetScalarParameterValueOnMaterials(TEXT("Valid"), 1.f);
    }
    else
    {
        HidePreview();
    }

    SetLookedAtActor(LookTarget);
}