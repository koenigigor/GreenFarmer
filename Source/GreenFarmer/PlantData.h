// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GFStructs.h"
#include "PlantData.generated.h"

class APlantDisplayActor;

/** Plant data */
UCLASS(BlueprintType)
class GREENFARMER_API UPlantData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FText DisplayName;
	
	// Display actor, represent plant view, not contain any core logic
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSoftClassPtr<APlantDisplayActor> PlantActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(DisplayThumbnail="true", AllowedClasses="/Script/Engine.Texture,/Script/Engine.MaterialInterface,/Script/Engine.SlateTextureAtlasInterface", DisallowedClasses = "/Script/MediaAssets.MediaTexture"))
	TSoftObjectPtr<UObject> Icon;

	
	// Time to full-grown single plant
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float GrowthDuration = 5.f;

	// Percent duration multiplier
	UPROPERTY(EditDefaultsOnly, meta=(ArraySizeEnum="EWaterLevel"))
	float WaterLevelMultipliers[(uint8)EWaterLevel::Num];

	// Percent duration multiplier
	UPROPERTY(EditDefaultsOnly, meta=(ArraySizeEnum="ELightLevel"))
	float LightLevelMultipliers[(uint8)ELightLevel::Num];

	float GetWaterLevelMultiplier(EWaterLevel Level) const;
	float GetLightLevelMultiplier(ELightLevel Level) const;
};
