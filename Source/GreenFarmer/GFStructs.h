// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GFStructs.generated.h"

UENUM(BlueprintType)
enum class EWaterLevel : uint8
{
	None,
	Dry,
	Wet,
	Liquid,
	Num UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ELightLevel : uint8
{
	None,
	Dark,
	Twilight,
	Brights,
	Num UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ECellState : uint8
{
	None,
	Idle,
	Blocked,
	Ready
};


/** */
UCLASS(Abstract)
class GREENFARMER_API UGFStructs : public UObject
{
	GENERATED_BODY()


};
