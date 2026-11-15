#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/InteractInterface.h"
#include "GridSurface.generated.h"

class UBoxComponent;

UCLASS()
class AGridSurface : public AActor, public IInteractInterface
{
	GENERATED_BODY()
public:
	AGridSurface();

	UPROPERTY(EditAnywhere, Category="Grid") FIntPoint GridSize = FIntPoint(6, 4);
	UPROPERTY(EditAnywhere, Category="Grid") float CellSize = 10.f;

	UPROPERTY(VisibleAnywhere) USceneComponent* Root;
	UPROPERTY(VisibleAnywhere) UBoxComponent* SurfaceBox;

	TMap<FIntPoint, TWeakObjectPtr<AActor>> Occupied;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void LookAt_Implementation(AActor* Looker) override;
	virtual void StopLookAt_Implementation(AActor* Looker) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	static FIntPoint GetRotatedSize(FIntPoint Footprint, int32 Step);
	FIntPoint WorldToOrigin(const FVector& World, FIntPoint Size) const;
	bool CanPlace(FIntPoint Origin, FIntPoint Size) const;
	FTransform GetPlacementTransform(FIntPoint Origin, FIntPoint Size, int32 Step) const;
	void Occupy(FIntPoint Origin, FIntPoint Size, AActor* Item);
	void FreeActor(AActor* Item);
	bool IsInBounds(FIntPoint C) const { return C.X >= 0 && C.Y >= 0 && C.X < GridSize.X && C.Y < GridSize.Y; }
	AActor* GetOccupantAt(const FVector& World) const;
};