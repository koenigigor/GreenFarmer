// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GFStructs.h"
#include "GameFramework/Actor.h"
#include "GreenhouseBase.generated.h"

class APlantDisplayActor;
class UPlantData;

USTRUCT(BlueprintType, Blueprintable)
struct FGreenhouseCell
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UPlantData* Plant = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GrowthTime = 0.f;

	// Cached plant state, can be updated on client so no need to replicate
	UPROPERTY(EditAnywhere, BlueprintReadWrite, NotReplicated)
	ECellState State = ECellState::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, NotReplicated, Transient)
	APlantDisplayActor* DisplayActor = nullptr;
};

/** Per plant stat for UI stats */
USTRUCT(BlueprintType, Blueprintable)
struct FGreenhousePlantStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UPlantData* PlantData;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Amount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AmountReady = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGreenhouseSimpleDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGreenhouseCellDelegate, int32, Cell);

UCLASS(Abstract)
class GREENFARMER_API AGreenhouseBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AGreenhouseBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void AddPlant(int32 CellID, UPlantData* Plant);

	UFUNCTION(BlueprintCallable)
	void GatherPlant(int32 CellID);

	UFUNCTION(BlueprintCallable)
	void GatherAll();

	
	UFUNCTION(BlueprintCallable)
	void SetWaterLevel(EWaterLevel WaterLevel);
	UFUNCTION(BlueprintPure)
	EWaterLevel GetWaterLevel() const { return WaterLevel; }

	UFUNCTION(BlueprintCallable)
	void SetLightLevel(ELightLevel LightLevel);
	UFUNCTION(BlueprintPure)
	ELightLevel GetLightLevel() const { return LightLevel; }

	UFUNCTION(BlueprintCallable)
	void GetPlantStats(TMap<UPlantData*, FGreenhousePlantStat>& OutPlantStat, int32& OutFreeCellCount);
	
	FTransform GetCellTransform(int32 CellID) const;

	UPROPERTY(BlueprintAssignable)
	FGreenhouseSimpleDelegate OnUpdateEnvironment;

	UPROPERTY(BlueprintAssignable)
	FGreenhouseCellDelegate OnUpdateCell;
	
protected:
	void HandleCellStateChange(int32 CellID, ECellState OldState, ECellState NewState);

	UFUNCTION(BlueprintImplementableEvent, DisplayName="HandleCellStateChange")
	void K2_HandleCellStateChange(int32 CellID, ECellState OldState, ECellState NewState);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Rows = 20;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Columns = 20;
	
	double CellSize = 50.0;

	EWaterLevel WaterLevel = EWaterLevel::None;
	ELightLevel LightLevel = ELightLevel::None;

	UPROPERTY(BlueprintReadOnly)
	TArray<FGreenhouseCell> Cells;
};
