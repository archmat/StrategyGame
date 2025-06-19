// Fill out your copyright notice in the Description page of Project Settings.


#include "Mercenary/Battle/BattleGridActor.h"

// Engine
#include "Components/BillboardComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

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

TArray<int32> ABattleGridActor::FindPathOnBattleGridAStar(const int32 StartCellId, const int32 GoalCellId, const int32 BattleSlotSize, const TSet<int32>& MovableCellSet, bool& PathExists)
{
	TArray<int32> PathCellIds;
	PathExists = false;

	// Find Path 할 필요가 있는지 사전 체크
	{
		// 시작과 끝이 같다면 Path 없음
		if (StartCellId == GoalCellId)
		{
			return PathCellIds;
		}

		// 시작과 끝의 인덱스 둘다 유효하지 않다면 Path 없음
		if (GridCells.IsValidIndex(StartCellId) && GridCells.IsValidIndex(GoalCellId) == false)
		{
			return PathCellIds;
		}

		// Goal 이 이동가능한 Cell Set 에 없다면 Path 없음
		if (MovableCellSet.Find(GoalCellId) == nullptr)
		{
			return PathCellIds;
		}

		// MovableCellSet 에 들어가려면 배치가 가능해야 하기에 이 비교는 의미 없을듯...
		//// 시작과 끝이 배치 가능하지 않다면 Path 없음
		//if (IsBattleUnitPlaceable(StartCellId, BattleSlotSize) && IsBattleUnitPlaceable(GoalCellId, BattleSlotSize) == false)
		//{
		//	return PathCellIds;
		//}
	}

	// Find Path 시작
	// #ksh : 캐릭터의 동선 이기때문에 Battle Slot 의 CoreId 로 계산을 해도 Line 을 그릴땐 BattleSlot 의 가운데 사용해야 함!!!
	FVector2D GoalLocation = GetGridBasedLocation2D(GoalCellId);

	TArray<int32> CellsForVisit;
	TArray<int32> Visited;
	TArray<int32> CameFromCell;
	TArray<float> NeighborDistances;

	// CellsForVist 과 NeighborDistances 는 Pair
	CellsForVisit.Emplace(StartCellId);
	NeighborDistances.Emplace(0.0f);
	
	// Visited 와 CameFromCell 은 Pair
	Visited.Emplace(StartCellId);
	CameFromCell.Emplace(StartCellId);

	int32 IndexOfMinValue = 0;
	int32 CurrentCellId;

	while (CellsForVisit.Num() > 0)
	{
		FMath::Min(NeighborDistances, &IndexOfMinValue);

		CurrentCellId = CellsForVisit[IndexOfMinValue];

		CellsForVisit.RemoveAt(IndexOfMinValue, 1, false);
		NeighborDistances.RemoveAt(IndexOfMinValue, 1, false);

		// 이웃하고 있는 Cell 순회
		for (EGridDirection eDirection : TEnumRange<EGridDirection>())
		{
			int32 ArroundGridCellId = GetArroundGridCellId(CurrentCellId, eDirection);

			if (ArroundGridCellId < 0)
				continue;

			if (MovableCellSet.Find(ArroundGridCellId) == nullptr)
				continue;

			if (Visited.Contains(ArroundGridCellId))
				continue;

			FVector2D ArroundLocation = GetGridBasedLocation2D(ArroundGridCellId);
			float DistanceToGoal = FVector2D::Distance(ArroundLocation, GoalLocation);

			CellsForVisit.Emplace(ArroundGridCellId);
			NeighborDistances.Emplace(DistanceToGoal);

			Visited.Emplace(ArroundGridCellId);
			CameFromCell.Emplace(CurrentCellId);

			if (ArroundGridCellId == GoalCellId)
			{
				int32 PastCellId = GoalCellId;

				TArray<int32> PathToGoal;
				PathToGoal.Emplace(PastCellId);
				//TRACE(Log, "PathToGoal Start : %d, Goal : %d", StartCellId, GoalCellId);
				//TRACE(Log, "PathToGoal Add : %d", PastCellId);

				for(const int32& VisitEntry : Visited)
				{
					int32 PastIndex = Visited.Find(PastCellId);
					int32 PreviousCellId = CameFromCell[PastIndex];

					if (StartCellId == PreviousCellId)
					{
						PathToGoal.Emplace(StartCellId);
						Algo::Reverse(PathToGoal);

						// Remove Path Ladder Effect
						{
							bool wasAddToRemoveArray = false;
							TArray<int32> CellIdsToRemove;
							FVector LastDirection = FVector::ZeroVector;

							int32 ToRemoveCount = PathToGoal.Num() - 2;

							for (int32 i = 0; i < ToRemoveCount; ++i)
							{
								if (wasAddToRemoveArray)
								{
									wasAddToRemoveArray = false;
									continue;
								}

								int32 CurrPathCellId = PathToGoal[i];
								int32 NextPathCellId1 = PathToGoal[i + 1];
								int32 NextPathCellId2 = PathToGoal[i + 2];

								FVector CurrPos = GetGridCellWorldLocation(CurrPathCellId);
								FVector NextPos1 = GetGridCellWorldLocation(NextPathCellId1);
								FVector NextPos2 = GetGridCellWorldLocation(NextPathCellId2);

								FVector CurrDir1 = CurrPos - NextPos1;
								FVector CurrDir2 = NextPos2 - NextPos1;

								CurrDir1.Normalize();
								CurrDir2.Normalize();

								//double dotValue = FGenericPlatformMath::Abs(FVector::DotProduct(CurrDir1, CurrDir2));
								double dotValue = abs(FVector::DotProduct(CurrDir1, CurrDir2));

								if (dotValue < 0.01)
								{
									// 서로 직각인 경우. Stair-stepping(Ladder) 이므로 Cell Id 를 제거할 대상에 추가
									FVector unionDir = CurrDir1 + CurrDir2;
									unionDir.Normalize();

									FVector CornerCheckPos = NextPos1 + (unionDir * GridCellLength);
									int32 CornerCheckCellId = GetGridCellIdByWorldLocation(CornerCheckPos);

									//if (IsBattleUnitPlaceable(CornerCheckCellId, BattleSlotSize))
									if(GridCells[CornerCheckCellId].IsPlaceable())
									{
										CellIdsToRemove.Emplace(NextPathCellId1);
										wasAddToRemoveArray = true;

										if (LastDirection.Equals(unionDir, 0.01))
										{
											CellIdsToRemove.Emplace(CurrPathCellId);
										}
										else
										{
											LastDirection = unionDir;
										}
									}
									else
									{
										// 블럭 오브젝트를 끼고서는 대각선 이동없음
										if (DebugGrid)
										{
											float ExtentSize = GridCellLength * 0.5f;
											UKismetSystemLibrary::DrawDebugBox(GetWorld(), CornerCheckPos, FVector(ExtentSize, ExtentSize, 300.0f), FLinearColor::Red, FRotator::ZeroRotator, 2.f);
										}
									}
								}
								else
								{
									// 직각이 아닌 경우. 방향이 같다면 직선이기에 궂이 필요없는 중간에 있는 Cell Id 이므로 제거
									if (LastDirection.Equals(CurrDir2, 0.01))
									{
										CellIdsToRemove.Emplace(CurrPathCellId);
									}
									else
									{
										LastDirection = CurrDir2;
									}
								}
							}

							// Remove Cell Id Remove cell IDs that cause stair-stepping(Ladder)
							for (const int32& RemoveValue : CellIdsToRemove)
							{
								PathToGoal.RemoveSingle(RemoveValue);
							}
						}

						// Return Find Path Array
						PathExists = true;
						return PathToGoal;
					}
					else
					{
						PastCellId = PreviousCellId;
						PathToGoal.Emplace(PastCellId);
						//TRACE(Log, "PathToGoal Add : %d", PastCellId);
					}
				}

				// Not Find Valid Path
				return PathCellIds;
			}
		}
	}

	// Not Find Path
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
	FVector2D GridBasedLoc2D = GetGridBasedLocation2D(GridCellId);
	GridBasedLoc2D -= BattleGridHalfSize;

	FVector GridCellLoc(-GridBasedLoc2D.X, GridBasedLoc2D.Y, 0.0);
	FVector GridCellWorldLocation = GridCellLoc + GetActorLocation();

	return GridCellWorldLocation;
}

FVector ABattleGridActor::GetGridSlotWorldLocation(const int32 GridCellId, const int32 BattleSlotSize)
{
	FVector SlotCoreWorldLocation = GetGridCellWorldLocation(GridCellId);

	if (BattleSlotSize < 1)
		return SlotCoreWorldLocation;

	double SlotScale = BattleSlotSize - 1;

	FVector SlotWorldLocation = SlotCoreWorldLocation + FVector(-GridCellLength * 0.5, GridCellLength * 0.5, 0.0) * SlotScale;

	return SlotWorldLocation;
}

// 해당 인덱스의 그리드 상의 우하단 진행 방향 2D 좌표 반환
FVector2D ABattleGridActor::GetGridBasedLocation2D(const int32 GridCellId) const
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
	 * GridBased 에선 월드 좌표로 직접 변환은 X 축 뒤집어야 해서 비추.
	 */
	FIntPoint GridRowCol = GetGridRowColumn(GridCellId);

	float halfGridCellLength = GridCellLength * 0.5f;

	float coordinateX = GridRowCol.X * GridCellLength + halfGridCellLength;
	float coordinateY = GridRowCol.Y * GridCellLength + halfGridCellLength;

	FVector2D GridCellCoordinates(coordinateX, coordinateY);
	
	return GridCellCoordinates;
}

FIntPoint ABattleGridActor::GetGridRowColumn(const int32 GridCellId) const
{
	int32 ColumnSize = GridCellSize.Y;
	
	// zero-based Row, Column
	int32 Row = GridCellId / ColumnSize;
	int32 Column = GridCellId % ColumnSize;

	FIntPoint gridRowColumn(Row, Column);

	return gridRowColumn;
}

int32 ABattleGridActor::GetArroundGridCellId(const int32 currGridCellId, const EGridDirection eDirection) const
{
	/**
	 *	GridCellSize.X -> Portrait (Rows)
	 *	GridCellSize.Y -> Landscape (Columns)
	 */
	if (!GridCells.IsValidIndex(currGridCellId))
	{
		TRACE(Warning, "Invalid Grid Cell Id : %d", currGridCellId);
		return -1;
	}

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

