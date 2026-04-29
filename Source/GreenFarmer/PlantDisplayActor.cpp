// Fill out your copyright notice in the Description page of Project Settings.


#include "PlantDisplayActor.h"


APlantDisplayActor::APlantDisplayActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void APlantDisplayActor::DestroyPlant_Implementation()
{
	Destroy();
}

