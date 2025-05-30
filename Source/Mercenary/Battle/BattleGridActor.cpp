// Fill out your copyright notice in the Description page of Project Settings.


#include "Mercenary/Battle/BattleGridActor.h"

// Engine
#include "Components/BillboardComponent.h"

// Project
#include "Mercenary/Logging.h"


// Sets default values
ABattleGridActor::ABattleGridActor()
//ABattleGridActor::ABattleGridActor(const FObjectInitializer& ObjectInitializer)
//	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
/*
	// Create Default SubObjects
	{
		RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

		BillboardComp = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard For Debug"));
		BillboardComp->SetupAttachment(RootComponent);

		GridMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visible Grid Mesh"));
		GridMeshComp->SetupAttachment(RootComponent);

		GridMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
/**/
}

// Called when the game starts or when spawned
void ABattleGridActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABattleGridActor::UpdateGridCells()
{
	// #todo : 차후에 Map 크기에 따라 Min, Max 변경 필요.
	int32 ClampedX = FMath::Clamp(GridCellSize.X, 30, 120);
	int32 ClampedY = FMath::Clamp(GridCellSize.Y, 30, 120);

	// Init Grid Cells
	{
		GridCellSize = FIntPoint(ClampedX, ClampedY);
		GridCellTotalSize = ClampedX * ClampedY;
		GridCornerTotalSize = (ClampedX + 1) * (ClampedY + 1);

		GridCells.Init(FGridCell(), GridCellTotalSize);
	}

	// Set Battle Grid Half Size
	FVector2D BattleGridCellSize = FVector2D(ClampedX, ClampedY);
	BattleGridHalfSize = BattleGridCellSize * (GridCellLength * 0.5f);

	//
	SetupGridMesh();

	// #todo : 레벨에 배치된 모든 ABattleGridBlockActor 에게서 Obstacle 정보를 얻어와 GridCells 에 반영하는 코드 추가해야 함.
	//  BP_Grid : UpdateGridPassablity 함수 참고
	UpdateGridBlocker();
}

void ABattleGridActor::UpdateGridObstacles(const TArray<int32>& ObstacleGridCellIds)
{
	int32 ObstacleGridCellCount = ObstacleGridCellIds.Num();

	for (int32 i = 0; i < ObstacleGridCellCount; ++i)
	{
		int32 CellId = ObstacleGridCellIds[i];
		if(GridCells.IsValidIndex(CellId))
		{
			GridCells[CellId].SetGridCellObstacle(true);

		}
		else
		{
			TRACE(Warning, "Invalid Obstacle Grid Cell Id : %d", CellId);
		}
	}
}

FVector ABattleGridActor::GetGridCornerWorldLocation(const int32 GridCornerId) const
{
	if (GridCornerId < 0 || GridCornerTotalSize <= GridCornerId)
	{
		TRACE(Warning, "Invalid Grid Corner ID : %d", GridCornerId);
		return FVector::ZeroVector;
	}

	/**
	 * Grid Cell, Corner Schema
	 * 각 Grid Cell 가운데는 Grid Cell 의 인덱스, 각 모서리는 Grid Corner 의 인덱스
	 * 
	 *  ex) 2행 3열 일 경우 
	 *		이때 BattleGridActor 의 위치는 보통 코너 인덱스 6 과 7 의 정 가운데. (전체 Grid 의 정 가운데)
	 * 
	 *  0-------1-------2-------3
	 *  |		|		|		|
	 *  |   0	|	1	|	2	|
	 *  |		|		|		|
	 *  4-------5-------6-------7
	 *  |		|		|		|
	 *  |   3	|	4	|	5	|
	 *  |		|		|		|
	 *  8-------9-------10------11
	 */

	int32 CornerColumnSize = GridCellSize.Y + 1;

	int32 CornerRow = GridCornerId / CornerColumnSize;
	int32 CornerCol = GridCornerId % CornerColumnSize;

	FVector2D CornerLoc = FVector2D(CornerRow, CornerCol) * GridCellLength - BattleGridHalfSize;
	FVector ConerWorldLocation = FVector(CornerLoc.X, CornerLoc.Y, 0.f) + GetActorLocation();

	return ConerWorldLocation;
}

// BP_Grid::GetCellWorldCoordinates
// 해당 인덱스의 월드 로케이션 반환
FVector ABattleGridActor::GetGridWorldLocationByGridcellID(const int32 GridCellId) const
{
	if (GridCellId < 0 || !GridCells.IsValidIndex(GridCellId))
	{
		TRACE(Warning, "Invalid Grid Cell Id : %d", GridCellId);
		return FVector::ZeroVector;
	}

	int32 ColumnSize = GridCellSize.Y;
	float halfGridCellLength = GridCellLength * 0.5f;

	// zero-based Row, Column
	int32 Row = GridCellId / ColumnSize;
	int32 Column = GridCellId % ColumnSize;

	float locX = Row * GridCellLength + halfGridCellLength - BattleGridHalfSize.X;
	float locY = Column * GridCellLength + halfGridCellLength - BattleGridHalfSize.Y;

	FVector GridCellLoc(locX, locY, 0.f);
	FVector GridCellWorldLocation = GridCellLoc + GetActorLocation();

	return GridCellWorldLocation;
}

// BP_Grid::GetCellNumberByWorldLocation
// 월드 로케이션에 해당하는 그리드 셀 인덱스 반환
int32 ABattleGridActor::GetGridCellIdByWorldLocation(const FVector& WorldLocation) const
{
	// Find GridCell Start World 2D(X,Y) Location
	FVector gridWorldLoc = GetActorLocation();
	FVector2D gridActorWorld2DLoc = FVector2D(gridWorldLoc.X, gridWorldLoc.Y);
	FVector2D gridStartWorld2DLoc = FVector2D(gridActorWorld2DLoc.X + BattleGridHalfSize.X, gridActorWorld2DLoc.Y - BattleGridHalfSize.Y);

	// GridCell 의 WorldLocation 을 gridStartWorld2DLoc 를 Base 하는 Local 2D 좌표로 변형
	FVector2D grid2DWorldLoc = FVector2D(WorldLocation.X, WorldLocation.Y);
	FVector2D grid2DStartBasedLoc = grid2DWorldLoc - gridStartWorld2DLoc;
	FVector2D grid2DLoc = FVector2D(grid2DStartBasedLoc.X * -1.f, grid2DStartBasedLoc.Y);	// X 축 진행 방향 Up 에서 Bottom 으로...

	if (grid2DLoc.X < 0.f ||							// Top border
		grid2DLoc.X > BattleGridHalfSize.X * 2.f ||		// Bottom Border
		grid2DLoc.Y < 0.f ||							// Left Border
		grid2DLoc.Y > BattleGridHalfSize.Y * 2.f)		// Right Border
	{
		TRACE(Warning, "Invalid World Location : %s", *grid2DWorldLoc.ToString());
		return -1;
	}

	int32 Row = FMath::TruncToInt(grid2DLoc.X / GridCellLength);
	int32 Column = FMath::TruncToInt(grid2DLoc.Y / GridCellLength);
	int32 retGridCellId = Row * GridCellSize.Y + Column;

	return retGridCellId;
}

// BP_Grid::GetCellLocalCoordinates
// 해당 인덱스의 그리드 상의 우하단 진행 방향 2D 좌표 반환
FVector2D ABattleGridActor::GetGrid2DLocByGridCellId(const int32 GridCellId) const
{
	if (GridCellId < 0 || !GridCells.IsValidIndex(GridCellId))
	{
		TRACE(Warning, "Invalid Grid Cell Id : %d", GridCellId);
		return FVector2D::ZeroVector;
	}

	int32 ColumnSize = GridCellSize.Y;
	float halfGridCellLength = GridCellLength * 0.5f;

	/**
	 *	0 Based Row, Column
	 *	----------------> y축 증가 방향
	 * |0, 0 | 0, 1 |
	 * |1, 0 | 1, 1 |
	 * |
	 * V
	 * x 축 증가 방향
	 * 
	 * Grid 의 좌상단을 0, 0 으로 하는 2D 좌표 체계.
	 * 월드 좌표로 직접 변환은 비추. 
	 *  > 변환 하기 위해선 BattleGridActor 의 위치에 대한 상대적인 좌표에 X 축을 뒤집어야 하기에...
	 */

	int32 Row = GridCellId / ColumnSize;
	int32 Column = GridCellId % ColumnSize;

	float coordinateX = Row * GridCellLength + halfGridCellLength;
	float coordinateY = Column * GridCellLength + halfGridCellLength;

	FVector2D GridCellCoordinates(coordinateX, coordinateY);
	
	return GridCellCoordinates;
}

// 해당 인덱스의 그리드 상의 행열 반환
FIntPoint ABattleGridActor::GetGridCoordinateByGridCellId(const int32 GridCellId) const
{
	if (GridCellId < 0 || !GridCells.IsValidIndex(GridCellId))
	{
		TRACE(Warning, "Invalid Grid Cell Id : %d", GridCellId);
		return FIntPoint::ZeroValue;
	}

	int32 ColumnSize = GridCellSize.Y;
	
	// zero-based Row, Column
	int32 Row = GridCellId / ColumnSize;
	int32 Column = GridCellId % ColumnSize;
	
	FIntPoint GridCellCoordinates(Row, Column);
	return GridCellCoordinates;
}

int32 ABattleGridActor::GetArroundGridCellId(const int32 currGridCellId, const EGridDirection eDirection) const
{
	if(currGridCellId < 0)
	{
		TRACE(Warning, "Invalid Grid Cell Id : %d", currGridCellId);
		return -1;
	}

	/**
	 *	GridCellSize.X -> Portrait (Rows)
	 *	GridCellSize.Y -> Landscape (Columns)
	 */
	int32 ArroundGridCellId;

	switch (eDirection)
	{
		case EGridDirection::Left:
		{
			ArroundGridCellId = ((currGridCellId % GridCellSize.Y) == 0) ? -1 : currGridCellId - 1;
			break;
		}
		case EGridDirection::Top:
		{
			ArroundGridCellId = (currGridCellId < GridCellSize.Y) ? -1 : currGridCellId - GridCellSize.Y;
			break;
		}
		case EGridDirection::Right:
		{
			ArroundGridCellId = ((currGridCellId % GridCellSize.Y) + 1 == GridCellSize.Y) ? -1 : currGridCellId + 1;
			break;
		}
		case EGridDirection::Bottom:
		{
			ArroundGridCellId = (currGridCellId < (GridCellTotalSize - GridCellSize.Y)) ? currGridCellId + GridCellSize.Y : -1;
			break;
		}
	}

	return ArroundGridCellId;
}

float ABattleGridActor::GetGridSlotMoveCost(const int32 SlotCoreGridCellId, const uint8 BattleSlotSquareSize) const
{
	if (BattleSlotSquareSize == 0)
	{
		return DEFAULT_GRID_CELL_MOVE_COST;
	}

	float AverageMoveCost = 0.f;

	for (int32 i = 0; i < BattleSlotSquareSize; ++i)
	{
		for (int32 j = 0; j < BattleSlotSquareSize; ++j)
		{
			int32 GridCellId = SlotCoreGridCellId + (i * GridCellSize.Y) + j;
			AverageMoveCost += GetGridCellMoveCost(GridCellId);
		}
	}

	AverageMoveCost /= (BattleSlotSquareSize * BattleSlotSquareSize);

	return AverageMoveCost;
}

bool ABattleGridActor::IsBattleUnitPlaceable(const int32 SlotCoreGridCellId, const uint8 BattleSlotSquareSize) const
{
	if (BattleSlotSquareSize == 0)
	{
		TRACE(Error, "Invalid Battle Slot Size : %d", BattleSlotSquareSize);
		return false;
	}

	// SlotCoreGridCellId 을 기준으로 하는 Slot 의 우측이 맵의 우측 안에 있는지 체크
	{
		// 이값 보다 크면 맵의 우측 영역 벗어남.
		int32 MaxColumnBySlot = GridCellSize.Y - (BattleSlotSquareSize - 1);
		
		if (SlotCoreGridCellId % GridCellSize.Y > MaxColumnBySlot)
		{
			TRACE(Warning, "Out Of Right Side Of Map Area - Slot Core Grid Cell Id : %d", SlotCoreGridCellId);
			return false;
		}
	}

	// SlotCoreGridCellId 을 기준으로 하는 Slot 의 하단이 맵의 하단 안에 있는지 체크
	{
		// 이값 보다 크면 맵의 하단 영역 벗어남.
		int32 MaxRightBottomGridCellIdBySlot = GridCellTotalSize - (BattleSlotSquareSize - 1) * GridCellSize.Y;
		
		if (SlotCoreGridCellId > MaxRightBottomGridCellIdBySlot)
		{
			TRACE(Warning, "Out Of Bottom Side Of Map Area - Slot Core Grid Cell Id : %d", SlotCoreGridCellId);
			return false;
		}
	}

	//
	int32 currGridCellId;

	for (int i = 0; i < BattleSlotSquareSize; ++i)
	{
		for (int j = 0; j < BattleSlotSquareSize; ++j)
		{
			currGridCellId = SlotCoreGridCellId + (i * GridCellSize.Y) + j;

			if (GridCells[currGridCellId].IsPlaceable() == false)
			{
				return false;
			}
		}
	}

	return true;
}

