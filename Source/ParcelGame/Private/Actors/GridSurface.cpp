#include "Public/Actors/GridSurface.h"
#include "ActorComponents/ItemHandlingComponent.h"
#include "Components/BoxComponent.h"

AGridSurface::AGridSurface()
{
    Root = CreateDefaultSubobject<USceneComponent>("Root");
    SetRootComponent(Root);

    SurfaceBox = CreateDefaultSubobject<UBoxComponent>("SurfaceBox");
    SurfaceBox->SetupAttachment(Root);
    SurfaceBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SurfaceBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    SurfaceBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block); // GridSurface
}

void AGridSurface::OnConstruction(const FTransform& T)
{
    Super::OnConstruction(T);
    const float W = GridSize.X * CellSize;
    const float H = GridSize.Y * CellSize;
    // Root = grid corner (cell 0,0). Box covers the grid, thin in Z, centred on Z=0.
    SurfaceBox->SetBoxExtent(FVector(W * 0.5f, H * 0.5f, 1.f));
    SurfaceBox->SetRelativeLocation(FVector(W * 0.5f, H * 0.5f, 0.f));
}

FIntPoint AGridSurface::GetRotatedSize(FIntPoint F, int32 Step)
{
    return (Step % 2 != 0) ? FIntPoint(F.Y, F.X) : F;
}

FIntPoint AGridSurface::WorldToOrigin(const FVector& World, FIntPoint Size) const
{
    const FVector Local = GetActorTransform().InverseTransformPosition(World);
    FIntPoint O(FMath::RoundToInt(Local.X / CellSize - Size.X * 0.5f),
                FMath::RoundToInt(Local.Y / CellSize - Size.Y * 0.5f));
    O.X = FMath::Clamp(O.X, 0, FMath::Max(0, GridSize.X - Size.X));
    O.Y = FMath::Clamp(O.Y, 0, FMath::Max(0, GridSize.Y - Size.Y));
    return O;
}

bool AGridSurface::CanPlace(FIntPoint Origin, FIntPoint Size) const
{
    for (int32 X = 0; X < Size.X; ++X)
        for (int32 Y = 0; Y < Size.Y; ++Y)
        {
            const FIntPoint C = Origin + FIntPoint(X, Y);
            if (!IsInBounds(C)) return false;
            if (const TWeakObjectPtr<AActor>* A = Occupied.Find(C)) { if (A->IsValid()) return false; }
        }
    return true;
}

FTransform AGridSurface::GetPlacementTransform(FIntPoint Origin, FIntPoint Size, int32 Step) const
{
    const FVector LocalCenter((Origin.X + Size.X * 0.5f) * CellSize,
                              (Origin.Y + Size.Y * 0.5f) * CellSize, 0.f);
    const FTransform G = GetActorTransform();
    return FTransform(G.GetRotation() * FRotator(0.f, Step * 90.f, 0.f).Quaternion(),
                      G.TransformPosition(LocalCenter));
}

void AGridSurface::Occupy(FIntPoint Origin, FIntPoint Size, AActor* Item)
{
    for (int32 X = 0; X < Size.X; ++X)
        for (int32 Y = 0; Y < Size.Y; ++Y)
            Occupied.Add(Origin + FIntPoint(X, Y), Item);
}

void AGridSurface::FreeActor(AActor* Item)
{
    for (auto It = Occupied.CreateIterator(); It; ++It)
        if (It.Value() == Item) It.RemoveCurrent();
}

AActor* AGridSurface::GetOccupantAt(const FVector& World) const
{
    const FVector Local = GetActorTransform().InverseTransformPosition(World);
    const FIntPoint Cell(FMath::FloorToInt(Local.X / CellSize), FMath::FloorToInt(Local.Y / CellSize));
    if (const TWeakObjectPtr<AActor>* A = Occupied.Find(Cell))
        return A->Get();
    return nullptr;
}

void AGridSurface::LookAt_Implementation(AActor* Looker)
{
}

void AGridSurface::StopLookAt_Implementation(AActor* Looker)
{
}

void AGridSurface::Interact_Implementation(AActor* Interactor)
{
    if (!Interactor) return;

    if (UItemHandlingComponent* Handling = Interactor->FindComponentByClass<UItemHandlingComponent>())
        Handling->PlaceActiveItem(this);
}