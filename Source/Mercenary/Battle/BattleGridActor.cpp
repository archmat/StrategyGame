// Fill out your copyright notice in the Description page of Project Settings.


#include "Mercenary/Battle/BattleGridActor.h"

// Engine
#include "Components/BillboardComponent.h"
#include <Kismet/KismetMathLibrary.h>

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
	TRACE(Log, "Init GridCells");

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

	//
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

TArray<int32> ABattleGridActor::GetGridCellIdsOfBlocker(const FVector& WorldPos, const FVector2D& RightNormal, const FVector2D& BlockSize) const
{
	const FVector2D HorizontalNormal = RightNormal;
	const FVector2D VerticalNormal(-HorizontalNormal.Y, HorizontalNormal.X);	// HorizontalNormal 의 90도 회전

	const FVector2D HalfBlockSize = BlockSize * -0.5;
	const FVector2D LeftTopPos = FVector2D(WorldPos.X, WorldPos.Y) + (VerticalNormal * HalfBlockSize.X + HorizontalNormal * HalfBlockSize.Y);

	double VerticalRemainder;
	double HorizonRemainder;

	int32 Row = UKismetMathLibrary::FMod(BlockSize.X, GridCellLength, VerticalRemainder) + 1;
	int32 Col = UKismetMathLibrary::FMod(BlockSize.Y, GridCellLength, HorizonRemainder) + 1;

	TArray<int32> BlockerGridCellIds;
	double VerticalLength = 0;
	double HorizontalLength = 0;
	
	for (int32 i = 0; i < Row; ++i)
	{
		for (int32 j = 0; j < Col; ++j)
		{
			FVector2D CurrBlockPos = LeftTopPos + (VerticalNormal * VerticalLength) + (HorizontalNormal * HorizontalLength);
			int32 GridCellId = GetGridCellIdByWorldLocation(FVector(CurrBlockPos.X, CurrBlockPos.Y, WorldPos.Z));

			if (GridCellId < 0)
			{
				TRACE(Warning, "Invalid Blocker Grid Cell Id : %d, %s", GridCellId, *CurrBlockPos.ToString());
			}
			else
			{
				BlockerGridCellIds.AddUnique(GridCellId);
			}

			// Update Horizontal Length
			{
				float CurColLength = HorizontalLength + GridCellLength;
				if (CurColLength > BlockSize.Y)
				{
					HorizontalLength += HorizonRemainder;
				}
				else
				{
					HorizontalLength = CurColLength;
				}
			}
		}

		// Update Vertical Length & Init Horizontal Length
		{
			float CurRowLength = VerticalLength + GridCellLength;
			if (CurRowLength > BlockSize.X)
			{
				VerticalLength += VerticalRemainder;
			}
			else
			{
				VerticalLength = CurRowLength;
			}

			HorizontalLength = 0;
		}
	}

	return BlockerGridCellIds;
}

TArray<int32> ABattleGridActor::FindPathOnBattleGridAStar(const int32 StartCellId, const int32 GoalCellId, const TSet<int32>& MovableSet, bool& PathExists)
{
	TArray<int32> PathCellIds;



	return PathCellIds;
}

int32 ABattleGridActor::GetGridSlotIdByWorldLocation(const FVector& WorldLocation) const
{
	int32 SlotCoreId = GetGridCellIdByWorldLocation(WorldLocation) - GridCellSize.Y - 1;
	return SlotCoreId;
}

// 월드 로케이션에 해당하는 그리드 셀 인덱스 반환
int32 ABattleGridActor::GetGridCellIdByWorldLocation(const FVector& WorldLocation) const
{
	// Find LeftTop GridCell World 2D(X,Y) Location
	FVector gridWorldLoc = GetActorLocation();
	FVector2D gridActorWorldLoc2D = FVector2D(gridWorldLoc.X, gridWorldLoc.Y);
	FVector2D gridLeftTopWorldLoc2D = FVector2D(gridActorWorldLoc2D.X + BattleGridHalfSize.X, gridActorWorldLoc2D.Y - BattleGridHalfSize.Y);

	// GridCell 의 WorldLocation 을 GridActor의 LeftTop 을 Base 하는 Local 2D 좌표로 변형
	FVector2D gridCellWorldLoc2D = FVector2D(WorldLocation.X, WorldLocation.Y);
	FVector2D gridLeftTopBasedLoc2D = gridCellWorldLoc2D - gridLeftTopWorldLoc2D;
	FVector2D gridCellLoc2D = FVector2D(gridLeftTopBasedLoc2D.X * -1.0, gridLeftTopBasedLoc2D.Y);	// X 축 진행 방향을 위에서 아래로 변경

	if (gridCellLoc2D.X < 0.0 ||							// Top border
		gridCellLoc2D.X > BattleGridHalfSize.X * 2.0 ||		// Bottom Border
		gridCellLoc2D.Y < 0.0 ||							// Left Border
		gridCellLoc2D.Y > BattleGridHalfSize.Y * 2.0)		// Right Border
	{
		//TRACE(Warning, "Invalid World Location : %s | Grid Loc : %s | LeftTop : %s", *gridCellWorldLoc2D.ToString(), *gridCellLoc2D.ToString(), *gridLeftTopWorldLoc2D.ToString());
		return -1;
	}

	int32 Row = FMath::TruncToInt(gridCellLoc2D.X / GridCellLength);
	int32 Column = FMath::TruncToInt(gridCellLoc2D.Y / GridCellLength);
	int32 retGridCellId = Row * GridCellSize.Y + Column;

	return retGridCellId;
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
	FVector ConerWorldLocation = FVector(-CornerLoc.X, CornerLoc.Y, 0.0) + GetActorLocation();

	return ConerWorldLocation;
}

// 해당 인덱스의 월드 로케이션 반환
FVector ABattleGridActor::GetGridCellWorldLocation(const int32 GridCellId) const
{
	FVector2D GridLoc2D = GetGrid2DLocByCellId(GridCellId);

	//FIntPoint GridCoord = GetGridCoordinateByCellId(GridCellId);

	//float halfGridCellLength = GridCellLength * 0.5f;

	//float locX = GridCoord.X * GridCellLength + halfGridCellLength - BattleGridHalfSize.X;
	//float locY = GridCoord.Y * GridCellLength + halfGridCellLength - BattleGridHalfSize.Y;

	GridLoc2D -= BattleGridHalfSize;

	FVector GridCellLoc(-GridLoc2D.X, GridLoc2D.Y, 0.0);
	FVector GridCellWorldLocation = GridCellLoc + GetActorLocation();

	return GridCellWorldLocation;
}

// BP_Grid::GetCellLocalCoordinates
// 해당 인덱스의 그리드 상의 우하단 진행 방향 2D 좌표 반환
FVector2D ABattleGridActor::GetGrid2DLocByCellId(const int32 GridCellId) const
{
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

	FIntPoint GridCoord = GetGridCoordinateByCellId(GridCellId);

	float halfGridCellLength = GridCellLength * 0.5f;

	float coordinateX = GridCoord.X * GridCellLength + halfGridCellLength;
	float coordinateY = GridCoord.Y * GridCellLength + halfGridCellLength;

	FVector2D GridCellCoordinates(coordinateX, coordinateY);
	
	return GridCellCoordinates;
}

// 그리드 상의 해당 인덱스 행열 반환
FIntPoint ABattleGridActor::GetGridCoordinateByCellId(const int32 GridCellId) const
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
	int32 ArroundGridCellId = -1;

	switch (eDirection)
	{
		case EGridDirection::Left:
		{
			// Current 의 Column 이 0 이라면 Left 없음.
			ArroundGridCellId = ((currGridCellId % GridCellSize.Y) == 0) ? -1 : currGridCellId - 1;
			break;
		}
		case EGridDirection::Top:
		{
			// Current 의 Row 가 0 이라면 (첫번째 행의 max id 보다 작다면) Top 없음.
			ArroundGridCellId = (currGridCellId < GridCellSize.Y) ? -1 : currGridCellId - GridCellSize.Y;
			break;
		}
		case EGridDirection::Right:
		{
			// Current 가 Column 의 마지막이라면 Right 없음.
			ArroundGridCellId = ((currGridCellId % GridCellSize.Y) + 1 == GridCellSize.Y) ? -1 : currGridCellId + 1;
			break;
		}
		case EGridDirection::Bottom:
		{
			// Current 가 Row 의 마지막이라면 (마지막 행의 min id 작지 않다면) Bottom 없음.
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

	/**
	 * SlotCoreGridCellId 는 Slot 의 좌상단이 기준이기에 좌측 과 상단을 넘어간다는 개념 자체가 없음.
	 * 오직 우측 과 하단만 체크하며 이 체크 역시 정상적인 Not Placeable 이기 때문에 디버깅 외엔 별도의 로그 출력 없음.
	 */

	// SlotCoreGridCellId 을 기준으로 하는 Slot 의 우측이 맵의 우측 안에 있는지 체크
	{
		// 이값 보다 크면 맵의 우측 영역 벗어남.
		int32 MaxColumnBySlot = GridCellSize.Y - BattleSlotSquareSize;
		
		if (SlotCoreGridCellId % GridCellSize.Y > MaxColumnBySlot)
		{
			//TRACE(Warning, "Out Of Right Side Of Map Area - Slot Core Grid Cell Id : %d", SlotCoreGridCellId);
			return false;
		}
	}

	// SlotCoreGridCellId 을 기준으로 하는 Slot 의 하단이 맵의 하단 안에 있는지 체크
	{
		// 이값 보다 크면 맵의 하단 영역 벗어남.
		int32 MaxRightBottomGridCellIdBySlot = GridCellTotalSize - (BattleSlotSquareSize - 1) * GridCellSize.Y - 1;
		
		if (SlotCoreGridCellId > MaxRightBottomGridCellIdBySlot)
		{
			//TRACE(Warning, "Out Of Bottom Side Of Map Area - Slot Core Grid Cell Id : %d", SlotCoreGridCellId);
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

