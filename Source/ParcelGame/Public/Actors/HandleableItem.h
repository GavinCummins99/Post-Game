#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/InteractInterface.h"
#include "HandleableItem.generated.h"

class AGridSurface;

UCLASS()
class AHandleableItem : public AActor, public IInteractInterface
{
	GENERATED_BODY()
public:
	AHandleableItem();

	UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint Footprint = FIntPoint(1, 1);

	UPROPERTY() AGridSurface* CurrentGrid = nullptr;
	int32 RotationStep = 0;

	void SetHeld();
	void SetInWorld(bool bSimulatePhysics);
	void SetPlaced();
	FVector GetPivotOffset() const;

	virtual void LookAt_Implementation(AActor* Looker) override;
	virtual void StopLookAt_Implementation(AActor* Looker) override;
	virtual void Interact_Implementation(AActor* Interactor) override;
};