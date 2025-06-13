// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleGridActor.generated.h"


template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept
{
	return static_cast<typename std::underlying_type<E>::type>(e);
}


#pragma region -- FGridCell Structs --

const float DEFAULT_GRID_CELL_MOVE_COST = 0.5f;

UENUM(BlueprintType)
enum class EGridOccupy : uint8
{
	None,
	Enemy,
	Friendly,
	Count			UMETA(Hidden)
};


USTRUCT(BlueprintType)
struct FGridCell
{
	GENERATED_BODY()

	friend class ABattleGridActor;


public:
	void SetOccupancyStatus(EGridOccupy eOccupy)
	{
		OccupancyStatus = eOccupy;
	}

	void SetOccupancyNone()
	{
		OccupancyStatus = EGridOccupy::None;
	}

	// Grid Cell 에 배치 가능 여부
	bool IsPlaceable() const
	{
		return (whetherObstacle == false) && (OccupancyStatus == EGridOccupy::None);
	}

	// Grid Cell 에 배치된 Battle Unit 여부와 상관없이 통과 가능. 해당 특성 보유해야만 가능.
	bool IsPassable() const
	{
		return whetherObstacle == false;
	}

	// Grid Cell 에 배치된 Battle Unit 이 아군인 경우 통과 가능. 해당 특성 보유해야만 가능.
	bool IsFriendlyPassable() const
	{
		return (whetherObstacle == false) && (OccupancyStatus != EGridOccupy::Enemy);		// None, Friendly
	}

	// Grid Cell 에 배치된 Battle Unit 이 적군인 경우 통과 가능. 해당 특성 보유해야만 가능.
	bool IsEnemyPassable() const
	{
		return (whetherObstacle == false) && (OccupancyStatus != EGridOccupy::Friendly);	// None, Enemy
	}

	float GetMoveCost() const
	{
		return moveCost;
	}


protected:
	void SetGridCellObstacle(bool bObstacle)
	{
		whetherObstacle = bObstacle;
	}

	void SetGridCellMoveCost(float fMoveCost)
	{
		moveCost = fMoveCost;
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EGridOccupy OccupancyStatus;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool whetherObstacle = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float moveCost = DEFAULT_GRID_CELL_MOVE_COST;
};

#pragma endregion


#pragma region -- EGridDirection Enum --

UENUM(BlueprintType)
enum class EGridDirection : uint8
{
	Left,
	Top,
	Right,
	Bottom,
	Count			UMETA(Hidden)
};

// @see https://benui.ca/unreal/iterate-over-enum-tenumrange/
ENUM_RANGE_BY_COUNT(EGridDirection, EGridDirection::Count);

const int32 LEFT_DIRECTION = to_underlying(EGridDirection::Left);
const int32 TOP_DIRECTION = to_underlying(EGridDirection::Top);
const int32 RIGHT_DIRECTION = to_underlying(EGridDirection::Right);
const int32 BOTTOM_DIRECTION = to_underlying(EGridDirection::Bottom);

const int32 DIRECTION_TYPE_COUNT = to_underlying(EGridDirection::Count);

#pragma endregion


UCLASS(BlueprintType, Blueprintable)
class MERCENARY_API ABattleGridActor : public AActor
{
	GENERATED_BODY()
	

public:	
	ABattleGridActor();
	//ABattleGridActor(const FObjectInitializer& ObjectInitializer);


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "BattleGrid")
	void UpdateGridCells();

	UFUNCTION(BlueprintCallable, Category = "BattleGrid")
	void UpdateGridObstacles(const TArray<int32>& ObstacleGridCellIds);

	// Setup Grid Mesh & Material Parameters
	UFUNCTION(BlueprintImplementableEvent, Category = "BattleGrid")
	void SetupGridMesh();

	// Reflecting Grid BlockActor to Grid Cells
	UFUNCTION(BlueprintImplementableEvent, Category = "BattleGrid")
	void UpdateGridBlocker();

	UFUNCTION(BlueprintCallable, Category = "BattleGrid")
	TArray<int32> GetGridCellIdsOfBlocker(const FVector& WorldPos, const FVector2D& RightNormal, const FVector2D& BlockSize) const;


public:
	UFUNCTION(BlueprintCallable, Category = "BattleGrid")
	TArray<int32> FindPathOnBattleGridAStar(const int32 StartCellId, const int32 GoalCellId, const int32 BattleSlotSize, const TSet<int32>& MovableCellSet, bool& PathExists);

	// 지정된 위치의 좌상단에 해당하는 BattleSlot 의 CoreId 반환
	UFUNCTION(BlueprintPure, Category = "BattleGrid")
	int32 GetGridSlotIdByWorldLocation(const FVector& WorldLocation) const;

	// Return the index of a grid cell via world coordinates. Grid Cell Index : 0 Based.
	// Z in WorldLocation is not used.
	UFUNCTION(BlueprintPure, Category = "BattleGrid")
	int32 GetGridCellIdByWorldLocation(const FVector& WorldLocation) const;

	// Returns the world location of the corresponding grid Corner.
	// Used to draw border lines (In World). Grid Corner Id is 0 Based.
	UFUNCTION(BlueprintPure, Category = "BattleGrid")
	FVector GetGridCornerWorldLocation(const int32 GridCornerId) const;

	// Returns the world location of the corresponding grid cell.
	UFUNCTION(BlueprintPure, Category = "BattleGrid")
	FVector GetGridCellWorldLocation(const int32 GridCellId) const;

	// Returns the 2D center Location of the corresponding grid cell.
	// The coordinates are zero-based On Grid
	UFUNCTION(BlueprintPure, Category = "BattleGrid")
	FVector2D GetGridBasedLocation2D(const int32 GridCellId) const;

	// Returns the 2D coordinates(Row, Col) of the corresponding grid cell.
	// Most Left Top Coordinates is 0, 0, Next is 0, 1
	UFUNCTION(BlueprintPure, Category = "BattleGrid")
	FIntPoint GetGridRowColumn(const int32 GridCellId) const;

	// Gets the index of the grid cell in the specified direction from the current grid cell.
	// If the direction is invalid or the grid cell ID is invalid, it returns -1.
	UFUNCTION(BlueprintPure, Category = "BattleGrid")
	int32 GetArroundGridCellId(const int32 currGridCellId, const EGridDirection eDirection) const;

	bool IsValidGridCellId(const int32 GridCellId) const
	{
		return GridCells.IsValidIndex(GridCellId);
	}

	// Move Cost of Grid Cell
	float GetGridCellMoveCost(const int32 GridCellId) const
	{
		if (GridCells.IsValidIndex(GridCellId))
		{
			return GridCells[GridCellId].GetMoveCost();
		}

		return DEFAULT_GRID_CELL_MOVE_COST;
	}

	// Move Cost of Grid Slot
	float GetGridSlotMoveCost(const int32 SlotCoreGridCellId, const uint8 BattleSlotSquareSize) const;

	// Whether a BattleUnit can be placed in that grid cell
	bool IsBattleUnitPlaceable(const int32 SlotCoreGridCellId, const uint8 BattleSlotSquareSize) const;

	int32 GetGridCellTotalSize() const
	{
		return GridCellTotalSize;
	}

	// Row : X, Column : Y
	FIntPoint GetGridCellSize() const
	{
		return GridCellSize;
	}


protected:
	//// For Debug
	//UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	//TObjectPtr<UBillboardComponent> BillboardComp;

	//UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	//TObjectPtr<UStaticMeshComponent> GridMeshComp;

	// Battle Grid 의 X, Y Cell 개수.
	// X: Row (Portrait), Y: Column (Landscape)
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	FIntPoint GridCellSize = FIntPoint(36, 36);

	// Battle Grid 1개의 Centimeter 단위 크기
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	float GridCellLength = 50.f;

	UPROPERTY(BlueprintReadOnly)
	TArray<FGridCell> GridCells;

	UPROPERTY(BlueprintReadOnly)
	int32 GridCellTotalSize;

	UPROPERTY(BlueprintReadOnly)
	int32 GridCornerTotalSize;

	// Battle Grid 전체 크기의 반. Centimeter 단위
	UPROPERTY(BlueprintReadOnly)
	FVector2D BattleGridHalfSize;
};

