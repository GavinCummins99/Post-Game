#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractInterface : public UInterface
{
	GENERATED_BODY()
};

class IInteractInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interact")
	void LookAt(AActor* Looker);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interact")
	void StopLookAt(AActor* Looker);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interact")
	void Interact(AActor* Interactor);
};