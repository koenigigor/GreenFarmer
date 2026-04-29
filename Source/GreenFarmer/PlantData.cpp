// Fill out your copyright notice in the Description page of Project Settings.


#include "PlantData.h"

float UPlantData::GetWaterLevelMultiplier(EWaterLevel Level) const
{
	return WaterLevelMultipliers[uint8(Level)];
}
float UPlantData::GetLightLevelMultiplier(ELightLevel Level) const
{
	return LightLevelMultipliers[uint8(Level)];
}
