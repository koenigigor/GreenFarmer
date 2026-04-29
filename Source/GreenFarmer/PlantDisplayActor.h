// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlantDisplayActor.generated.h"

/** Display actor, represent plant view, not contain any core logic. */
UCLASS(Abstract)
class GREENFARMER_API APlantDisplayActor : public AActor
{
	GENERATED_BODY()

public:
	APlantDisplayActor();

	// Blueprint override for nice view destroy
	UFUNCTION(BlueprintNativeEvent)
	void DestroyPlant();

	UFUNCTION(BlueprintImplementableEvent)
	void SetProgress(float Percent);
	
	int32 CellID = INDEX_NONE;
};
