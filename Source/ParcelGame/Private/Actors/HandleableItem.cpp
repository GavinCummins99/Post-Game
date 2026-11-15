#include "Actors/HandleableItem.h"
#include "ActorComponents/ItemHandlingComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AHandleableItem::AHandleableItem()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// Items block the grid trace so they can be looked at and never get a preview drawn over them.
	Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
}

void AHandleableItem::SetHeld()
{
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorHiddenInGame(true);
}

void AHandleableItem::SetInWorld(bool bSimulatePhysics)
{
	SetActorHiddenInGame(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(bSimulatePhysics);
}

void AHandleableItem::SetPlaced()
{
	SetInWorld(false);
}

FVector AHandleableItem::GetPivotOffset() const
{
	const UStaticMesh* SM = Mesh->GetStaticMesh();
	if (!SM) return FVector::ZeroVector;

	const FBoxSphereBounds B = SM->GetBounds();
	const FVector S = Mesh->GetComponentScale();
	return FVector(-B.Origin.X, -B.Origin.Y, -(B.Origin.Z - B.BoxExtent.Z)) * S;
}

void AHandleableItem::LookAt_Implementation(AActor* Looker)
{
}

void AHandleableItem::StopLookAt_Implementation(AActor* Looker)
{
}

void AHandleableItem::Interact_Implementation(AActor* Interactor)
{
	if (!Interactor) return;

	if (UItemHandlingComponent* Handling = Interactor->FindComponentByClass<UItemHandlingComponent>())
		Handling->PickUpItem(this);
}