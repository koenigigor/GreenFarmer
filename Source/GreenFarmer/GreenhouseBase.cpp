// Fill out your copyright notice in the Description page of Project Settings.


#include "GreenhouseBase.h"

#include "JsonObjectConverter.h"
#include "PlantData.h"
#include "PlantDisplayActor.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogGreenhouse, All, All);

AGreenhouseBase::AGreenhouseBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.f;
}

void AGreenhouseBase::BeginPlay()
{
	Super::BeginPlay();

	// Init default cells
	Cells.AddDefaulted(Rows * Columns);
	
	// Load from save (json version)
	{
		const FString UniqueName = GetPathName(GetOuter());
		UE_LOG(LogGreenhouse, Display, TEXT("Load Greenhouse [%s]"), *UniqueName)

		// Load local save
		TArray<uint8> ByteArray;
		if (!UGameplayStatics::LoadDataFromSlot(ByteArray, UniqueName, 0))
		{
			return;
		}

		// Reconstruct json
		FUTF8ToTCHAR Converter((const ANSICHAR*)ByteArray.GetData(), ByteArray.Num());
		FString JsonString(Converter.Length(), Converter.Get());

		TSharedPtr<FJsonObject> Json;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
		if (!FJsonSerializer::Deserialize(JsonReader, Json) || !Json.IsValid())
		{
			UE_LOG(LogGreenhouse, Warning, TEXT("Unable to parse. json=[%s]"), *JsonString);
			return;
		}

		// Restore data
		WaterLevel = (EWaterLevel)Json->GetIntegerField(TEXT("WaterLevel"));
		LightLevel = (ELightLevel)Json->GetIntegerField(TEXT("LightLevel"));
		TArray<TSharedPtr<FJsonValue>> ValArray = Json->GetArrayField(TEXT("Cells"));
		for (int32 i = 0; i != ValArray.Num(); ++i)
		{
			check(Cells.IsValidIndex(i));
			
			TSharedPtr<FJsonObject> CellJson = ValArray[i]->AsObject();
			FJsonObjectConverter::JsonObjectToUStruct(CellJson.ToSharedRef(), &Cells[i]);
		}

		// Spawn display actors
		for (int32 CellID = 0; CellID != Cells.Num(); ++CellID)
		{
			FGreenhouseCell& Cell = Cells[CellID];
			if (!Cell.Plant) continue;

			UClass* PlantActorClass = Cell.Plant->PlantActor.LoadSynchronous();
			if (!PlantActorClass)
			{
				UE_LOG(LogGreenhouse, Error, TEXT("No DisplyActor in plant [%s]"), *Cell.Plant->GetName())
				continue;
			}

			const FTransform CellTransform = GetCellTransform(CellID);

			Cell.DisplayActor = GetWorld()->SpawnActorDeferred<APlantDisplayActor>(PlantActorClass, CellTransform, this);
			Cell.DisplayActor->CellID = CellID;
			Cell.DisplayActor->FinishSpawning(CellTransform);
		}
	}
}

void AGreenhouseBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Serialize greenhouse (json version)
	{
		const FString UniqueName = GetPathName(GetOuter());
		UE_LOG(LogGreenhouse, Display, TEXT("Save Greenhouse [%s]"), *UniqueName)

		// Make save json
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetNumberField("WaterLevel", (uint8)WaterLevel);
		Json->SetNumberField("LightLevel", (uint8)LightLevel);

		TArray<TSharedPtr<FJsonValue>> JsonCells;
		for (const FGreenhouseCell& Cell : Cells)
		{
			TSharedRef<FJsonObject> CellJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FGreenhouseCell::StaticStruct(), &Cell, CellJson);
			
			JsonCells.Add_GetRef(MakeShareable(new FJsonValueObject(CellJson)));
		}
		Json->SetArrayField("Cells", JsonCells);

		// Convert to string for local save
		FString JsonString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
		FJsonSerializer::Serialize(Json, Writer);

		//UE_LOG(LogGreenhouse, Display, TEXT("Save: %s"), *JsonString)
		
		// Save
		FTCHARToUTF8 Converter(*JsonString);
		TArray<uint8> ByteArray;
		ByteArray.Append((uint8*)Converter.Get(), Converter.Length());
		UGameplayStatics::SaveDataToSlot(ByteArray, UniqueName, 0);
	}
}
void AGreenhouseBase::Destroyed()
{
	Super::Destroyed();

	for (const FGreenhouseCell& Cell : Cells)
	{
		Cell.DisplayActor->Destroy();
	}
}

void AGreenhouseBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DrawDebugBox(GetWorld(), GetActorLocation() + FVector(-(CellSize * 0.5) + CellSize * Columns * 0.5, -(CellSize * 0.5) + CellSize * Rows * 0.5, 0.0),
		FVector(CellSize * Columns * 0.5, CellSize * Rows * 0.5, 30.0), FColor::Orange, false, DeltaTime, 0, 1);

	TRACE_CPUPROFILER_EVENT_SCOPE(Greenhouse_TickCells);
	/** Note: If you came here to optimize
	* Here are a couple of thoughts.
	* 1. Replace DisplayАctor with HISM driven way with material params
	* 2. Breack update per ticks
	* 3. For large plant volumes (2000+),
	*	it makes sense to cache plants by type (cache latency) and quickly collect statistics.	*/
	
	// Simple lambda for nice view
	auto ChangeStateWithNotify = [&](int32 Index, FGreenhouseCell& Cell, ECellState NewState)
	{
		if (Cell.State != NewState)
		{
			const ECellState OldState = Cell.State;
			Cell.State = NewState;
			HandleCellStateChange(Index, OldState, ECellState::Blocked);
		}
	};
	
	// Tick cells
	for (int32 i = 0; i != Cells.Num(); ++i)
	{
		FGreenhouseCell& Cell = Cells[i];

		if (!Cell.Plant)
		{
			// empty cell
			ChangeStateWithNotify(i, Cell, ECellState::None);
			continue;
		}
		if (Cell.State == ECellState::Ready)
		{
			// Already ready, nothing to do
			continue;
		}
		
		// Multipliers based on water/light level
		const float EnvironmentMultiplier = Cell.Plant->GetWaterLevelMultiplier(WaterLevel) * Cell.Plant->GetLightLevelMultiplier(LightLevel);
		if (FMath::IsNearlyZero(EnvironmentMultiplier))
		{
			// Blocked
			UE_LOG(LogGreenhouse, Verbose, TEXT("[%d] Tick plant: [%.2f] [%s] (skip wrong environment)"),
				i, Cell.GrowthTime/Cell.Plant->GrowthDuration, *Cell.Plant->GetName())
			ChangeStateWithNotify(i, Cell, ECellState::Blocked);
			continue;
		}

		if (Cell.State == ECellState::Ready)
		{
			continue;
		}
		
		// Update plant state
		Cell.GrowthTime = FMath::Min(Cell.Plant->GrowthDuration, Cell.GrowthTime + DeltaTime * EnvironmentMultiplier);
		ECellState NewState = Cell.GrowthTime >= Cell.Plant->GrowthDuration ? ECellState::Ready : ECellState::Idle;
		ChangeStateWithNotify(i, Cell, NewState);

		UE_LOG(LogGreenhouse, Verbose, TEXT("[%d] Tick plant: [%.2f] [%s] (env multiplier: %f)"),
			i, Cell.GrowthTime/Cell.Plant->GrowthDuration, *Cell.Plant->GetName(), EnvironmentMultiplier)

		// Update meshes based on duration
		if (Cell.DisplayActor)
		{
			Cell.DisplayActor->SetProgress(Cell.GrowthTime/Cell.Plant->GrowthDuration);
		}
	}

	// Consume water, energy etc...
}

void AGreenhouseBase::AddPlant(int32 CellID, UPlantData* Plant)
{
	if (!Plant)
	{
		return;
	}
	if (!Cells.IsValidIndex(CellID))
	{
		// Cell not exist
		return;
	}

	FGreenhouseCell& Cell = Cells[CellID];
	if (Cell.Plant)
	{
		// Cell already occupied
		UE_LOG(LogGreenhouse, Log, TEXT("Try add [%s] but cell already occupied with [%s]"),
			*Plant->GetName(), *Cell.Plant->GetName())
		return;
	}

	UE_LOG(LogGreenhouse, Log, TEXT("Add plant, loading plant mesh"))
	UClass* PlantActorClass = Plant->PlantActor.LoadSynchronous();
	if (!PlantActorClass)
	{
		UE_LOG(LogGreenhouse, Error, TEXT("No DisplyActor in plant [%s]"), *Plant->GetName())
		return;
	}
	// TODO async load with block cell?

	const FTransform CellTransform = GetCellTransform(CellID);
	
	Cell = FGreenhouseCell();
	Cell.Plant = Plant;
	Cell.State = ECellState::Idle;
	Cell.DisplayActor = GetWorld()->SpawnActorDeferred<APlantDisplayActor>(PlantActorClass, CellTransform, this);
	Cell.DisplayActor->CellID = CellID;
	Cell.DisplayActor->FinishSpawning(CellTransform);

	HandleCellStateChange(CellID, ECellState::None, ECellState::Idle);
}
void AGreenhouseBase::GatherPlant(int32 CellID)
{
	FGreenhouseCell& Cell = Cells[CellID];
	if (!Cell.Plant)
	{
		// Empty nothing to gather
		return;
	}
	if (Cell.State != ECellState::Ready)
	{
		// Can't gather, plant not ready
		return;
	}

	// TODO add inventory
	
	// Clear cell
	if (Cell.DisplayActor) Cell.DisplayActor->DestroyPlant();
	Cell = FGreenhouseCell();
	HandleCellStateChange(CellID, ECellState::Ready, ECellState::None);
}
void AGreenhouseBase::GatherAll()
{
	for (int32 i = 0; i != Cells.Num(); ++i)
	{
		if (Cells[i].State == ECellState::Ready)
		{
			GatherPlant(i);
		}
	}
}

void AGreenhouseBase::SetWaterLevel(EWaterLevel InWaterLevel)
{
	WaterLevel = InWaterLevel;
	UE_LOG(LogGreenhouse, Verbose, TEXT("Set WaterLevel: %s"), *StaticEnum<EWaterLevel>()->GetNameStringByValue(uint8(WaterLevel)))

	OnUpdateEnvironment.Broadcast();
}
void AGreenhouseBase::SetLightLevel(ELightLevel InLightLevel)
{
	LightLevel = InLightLevel;
	UE_LOG(LogGreenhouse, Verbose, TEXT("Set LightLevel: %s"), *StaticEnum<ELightLevel>()->GetNameStringByValue(uint8(LightLevel)))

	OnUpdateEnvironment.Broadcast();
}

FTransform AGreenhouseBase::GetCellTransform(int32 CellID) const
{
	// TODO Select points manual for custom shaped rows (like _-_)

	const double CellX = CellSize * (CellID / Rows);
	const double CellY = CellSize * (CellID % Rows);

	return GetActorTransform() + FTransform(FVector(CellX, CellY, 0.0));
}

void AGreenhouseBase::HandleCellStateChange(int32 CellID, ECellState OldState, ECellState NewState)
{
	OnUpdateCell.Broadcast(CellID);
	
	K2_HandleCellStateChange(CellID, OldState, NewState);
}

void AGreenhouseBase::GetPlantStats(TMap<UPlantData*, FGreenhousePlantStat>& OutPlantStat, int32& OutFreeCellCount)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Greenhouse_GetPlantStats);
	OutPlantStat.Empty();
	OutFreeCellCount = Cells.Num();

	for (const FGreenhouseCell& Cell : Cells)
	{
		if (Cell.State == ECellState::None)
		{
			continue;
		}

		--OutFreeCellCount;
		
		FGreenhousePlantStat& PlantStat = OutPlantStat.FindOrAdd(Cell.Plant, FGreenhousePlantStat{Cell.Plant});
		++PlantStat.Amount;
		if (Cell.State == ECellState::Ready)
		{
			++PlantStat.AmountReady;
		}
	}
}
