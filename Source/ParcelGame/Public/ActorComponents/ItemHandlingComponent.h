#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemHandlingComponent.generated.h"

class AHandleableItem;
class AGridSurface;
class UStaticMeshComponent;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EInteractResult : uint8
{
    NoTarget,
    NotInteractable,
    HandsFull,
    NoItemHeld,
    InvalidPlacement
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemPickedUp, AHandleableItem*, Item, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemPlaced, AHandleableItem*, Item, AGridSurface*, Grid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemDropped, AHandleableItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractFailed, EInteractResult, Reason);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UItemHandlingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UItemHandlingComponent();

    UPROPERTY(EditAnywhere, Category="Handling") int32 MaxSlots = 3;
    UPROPERTY(EditAnywhere, Category="Handling") float InteractDistance = 400.f;
    UPROPERTY(EditAnywhere, Category="Handling") float DropDistance = 120.f;
    UPROPERTY(EditAnywhere, Category="Handling") UMaterialInterface* PreviewMaterial = nullptr;
    UPROPERTY(EditAnywhere, Category="Handling") bool bShowDebug = true;

    // Raw actor hit by the trace this frame (grid, item, or null).
    UPROPERTY(BlueprintReadOnly, Category="Handling") AActor* HitActor = nullptr;

    // Actor currently receiving LookAt. May be an item when the trace reached the grid box.
    UPROPERTY(BlueprintReadOnly, Category="Handling") AActor* LookedAtActor = nullptr;

    UPROPERTY(BlueprintAssignable, Category="Handling") FOnItemPickedUp OnItemPickedUp;
    UPROPERTY(BlueprintAssignable, Category="Handling") FOnItemPlaced OnItemPlaced;
    UPROPERTY(BlueprintAssignable, Category="Handling") FOnItemDropped OnItemDropped;
    UPROPERTY(BlueprintAssignable, Category="Handling") FOnInteractFailed OnInteractFailed;

    // Single interact entry point. Calls Interact on the looked-at actor.
    UFUNCTION(BlueprintCallable, Category="Handling") bool Interact();

    // Called by AHandleableItem::Interact.
    UFUNCTION(BlueprintCallable, Category="Handling") bool PickUpItem(AHandleableItem* Item);

    // Called by AGridSurface::Interact.
    UFUNCTION(BlueprintCallable, Category="Handling") bool PlaceActiveItem(AGridSurface* Grid);

    UFUNCTION(BlueprintCallable, Category="Handling") void DropActive();
    UFUNCTION(BlueprintCallable, Category="Handling") void Rotate(float AxisValue);
    UFUNCTION(BlueprintCallable, Category="Handling") void SelectSlot(int32 Index);

    UFUNCTION(BlueprintPure, Category="Handling") AHandleableItem* GetActiveItem() const;
    UFUNCTION(BlueprintPure, Category="Handling") AHandleableItem* GetSlotItem(int32 Index) const;
    UFUNCTION(BlueprintPure, Category="Handling") int32 GetActiveSlotIndex() const { return ActiveSlot; }
    UFUNCTION(BlueprintPure, Category="Handling") bool HasFreeSlot() const;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float DT, ELevelTick TT, FActorComponentTickFunction* F) override;

private:
    bool GetView(FVector& Loc, FRotator& Rot) const;
    FTransform GetItemTransformOnGrid(AHandleableItem* Item, AGridSurface* Grid, FIntPoint Origin, FIntPoint Size) const;
    void HidePreview();
    void SelectNextFilled();
    void SetLookedAtActor(AActor* NewActor);
    void Fail(EInteractResult Reason);

    UPROPERTY() TArray<AHandleableItem*> Slots;
    int32 ActiveSlot = 0;

    UPROPERTY() UStaticMeshComponent* Preview = nullptr;
    TWeakObjectPtr<AHandleableItem> PreviewItem;

    TWeakObjectPtr<AGridSurface> TargetGrid;
    FIntPoint TargetOrigin;
    FIntPoint TargetSize;
    bool bTargetValid = false;
};