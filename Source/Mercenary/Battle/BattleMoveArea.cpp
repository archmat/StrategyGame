// Fill out your copyright notice in the Description page of Project Settings.


#include "Mercenary/Battle/BattleMoveArea.h"

// Engine
#include "Kismet/GameplayStatics.h"

// Project
//#include "Mercenary/Battle/BattleGridActor.h"

#include "Mercenary/Logging.h"


// Sets default values
ABattleMoveArea::ABattleMoveArea()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ABattleMoveArea::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
//void ABattleMoveArea::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

void ABattleMoveArea::GetGridCornerLocations(const TArray<int32>& GridCornerIds, TArray<FVector>& GridCornerWorldLocations) const
{
	if (!IsValid(BattleGridActor))
	{
		TRACE(Error, "Invalid Battle Grid Actor");
		return;
	}

	if (GridCornerIds.IsEmpty())
	{
		TRACE(Warning, "Border Points is Empty");
		return;
	}

	int32 PointsNum = GridCornerIds.Num();
	GridCornerWorldLocations.Empty(PointsNum);

	for (int i = 0; i < PointsNum; ++i)
	{
		FVector GridCornerLoc = BattleGridActor->GetGridCornerWorldLocation(GridCornerIds[i]);
		GridCornerWorldLocations.Emplace(GridCornerLoc);
	}
}

// BP_MoveArea::Update Border
void ABattleMoveArea::DrawMoveArea(const FVector WorldStartLoc, const float fMovePoints)
{
	// Setup Battle Grid Actor
	if (!IsValid(BattleGridActor))
	{
		AActor* GridActor = UGameplayStatics::GetActorOfClass(GetWorld(), ABattleGridActor::StaticClass());
		BattleGridActor = Cast<ABattleGridActor>(GridActor);

		if (BattleGridActor == nullptr)
		{
			TRACE(Error, "Invalid Battle Grid Actor");
			return;
		}
	}

	FIntPoint GridCellSize = BattleGridActor->GetGridCellSize();
	RowSizeOfGridActor = GridCellSize.X;
	ColumnSizeOfGridActor = GridCellSize.Y;

	// Setup Start Grid Cell Id
	int32 GridCellId = BattleGridActor->GetGridCellIdByWorldLocation(WorldStartLoc);
	if (false == BattleGridActor->IsValidGridCellId(GridCellId))
	{
		TRACE(Error, "Invalid Start Grid Cell Id : %d, %s", GridCellId, *WorldStartLoc.ToString());
		return;
	}

	// Setup Move Info
	StartGridCellId = GridCellId;;
	MoveDistance = FMath::Clamp(fMovePoints, 0.f, fMovePoints);

	/**
	 * 1. 시작 지점서 부터 이동 가능한 모든 Grid Cell 을 찾아 MovableAreaSet 구성.
	 * 2. 구성된 MovableAreaSet 을 순회 하면서 Border 로 판별 되는 Grid Cell 과 해당 방향으로 BorderEdgeInfos 구성.
	 * 3. 구성된 BorderEdgeInfos 을 기반으로 FilterdGridCornerAndDrawBorder 함수를 통해 Border 를 그린다.
	 */
	{
		double StartTime = FPlatformTime::Seconds();

		// Clear Previous Movable Border Line
		ResetPathTracer();

		// Find the Grid Cell that make up the boundary of the movable area
		FindGridCellForMoveArea();

		//
		MakeBorderEdgeInfos();

		// 
		DrawBorderByEdgeInfos();

		double EndTime = FPlatformTime::Seconds();
		double ElapsedTime = EndTime - StartTime;

		TRACE(Log, "Find Move Area Elapsed Time : %.3f seconds", ElapsedTime);
	}
}

// BP_MoveArea::FindBorderPoints
void ABattleMoveArea::FindGridCellForMoveArea()
{
	if (!IsValid(BattleGridActor))
	{
		TRACE(Error, "Invalid Battle Grid Actor");
		return;
	}

	if (false == BattleGridActor->IsValidGridCellId(StartGridCellId))
	{
		TRACE(Error, "Invalid Start Grid Cell Id : %d", StartGridCellId);
		return;
	}

	// 시작	Grid Cell Id 가 배치 가능 여부를 체크하고 MovableAreaSet 을 초기화 한다.
	{
		if (false == BattleGridActor->IsBattleUnitPlaceable(StartGridCellId, BattleSlotSquareSize))
		{
			TRACE(Error, "Non-placable Start Grid Cell Id : %d, Slot Square Size : %d", StartGridCellId, BattleSlotSquareSize);
			return;
		}

		// Clear Previous Movable Area
		MovableGridCellSet.Empty();
		MovableAreaSet.Empty();
		BorderAreaSet.Empty();

		BorderEdgeInfos.Empty();
		DuplicateStartGridCornerIds.Empty();
	}

	// Setup First Searching, Visited Info
	TArray<FMoveInfo> SearchingGirdCellIds;
	TArray<bool> AllVisitedFlags;
	{
		// 전체 Grid Cell 방문여부 를 체크하기 위한 배열 초기화.
		int32 TotalGridCellSize = BattleGridActor->GetGridCellTotalSize();

		AllVisitedFlags.SetNumUninitialized(TotalGridCellSize);
		FMemory::Memzero((uint8*)AllVisitedFlags.GetData(), TotalGridCellSize * sizeof(bool));
	}

	// Set Start Grid Cell 을 이동 영역에 추가. 방문 여부 설정. 탐색 해야할 영역에 추가.
	AddGridCellToMovableAreaBySlot(StartGridCellId);
	AllVisitedFlags[StartGridCellId] = true;
	SearchingGirdCellIds.Emplace(FMoveInfo(StartGridCellId, MoveDistance));


	/**
	 * Cell 단위 이동 이라면 이곳에서 이동 범위의 경계를 알 수 있지만 Slot 단위 에선 알 수 없음.
	 * Cell 의 Slot 이 배치 가능 하면 Slot 을 구성하는 Cell 을 이동 영역으로 추가.
	 * 경계라고 판단되는 Slot 은 경계 영역으로 추가
	 * 모든 이동 영역을 찾으면 경계 영역중에서 경계에 해당하는 Cell 만 필터링 해서 이동 범위 라인 구성.
	 */
	while (SearchingGirdCellIds.Num() > 0)
	{
		int32 CurrGridCellId = SearchingGirdCellIds[0].GridCellId;
		float CurrMovePoint = SearchingGirdCellIds[0].MovePoint;

		// Start 위치에서 부터 확장해가며 탐색해야 하기에 순서 유지 필요!!! RemoveAtSwap 사용 불가!!!
		SearchingGirdCellIds.RemoveAt(0, 1, false);

		for(EGridDirection eDirection : TEnumRange<EGridDirection>())
		{
			int32 ArroundGridCellId = BattleGridActor->GetArroundGridCellId(CurrGridCellId, eDirection);
			
			if (AllVisitedFlags.IsValidIndex(ArroundGridCellId))
			{
				// 방문 여부 체크
				// 이동 영역에 이미 추가된 곳이므로 경계 영역으로 추가할 필요 없음.
				if (AllVisitedFlags[ArroundGridCellId])
				{
					continue;
				}
			}
			else
			{
				// Out of Map Area 체크 - 유효한 Grid Cell Id 가 아니라면 Grid Map 영역 밖에 있는 것.
				AddGridCellToBorderAreaBySlot(CurrGridCellId);
				continue;
			}

			// 배치 가능여부를 통해 이동 가능 한지 체크
			// #todo : 캐릭터 특성에 따라 아군, 적군 통과 가능한 경우 Passable 함수를 통해 체크.
			if (false == BattleGridActor->IsBattleUnitPlaceable(ArroundGridCellId, BattleSlotSquareSize))
			{
				// 배치 불가에 의한 Border Line.
				AddGridCellToBorderAreaBySlot(CurrGridCellId);
				continue;
			}

			// Move Cost 체크
			// #todo : Move Cost 체크를 Slot 단위로 한다면 GetGridSlotMoveCost 사용.
			float ArroundMoveCost = BattleGridActor->GetGridCellMoveCost(ArroundGridCellId);
			//float ArroundMoveCost = BattleGridActor->GetGridSlotMoveCost(ArroundGridCellId, BattleSlotSquareSize);
			if (CurrMovePoint < ArroundMoveCost)
			{
				// Move Cost 에 의한 Border Line.
				AddGridCellToBorderAreaBySlot(CurrGridCellId);
				continue;
			}

			// 이동 영역에 추가
			AddGridCellToMovableAreaBySlot(ArroundGridCellId);
			AllVisitedFlags[ArroundGridCellId] = true;
			SearchingGirdCellIds.Emplace(FMoveInfo(ArroundGridCellId, CurrMovePoint - ArroundMoveCost));
		}
	}
}

/**
 * Border Line 을 그리기 위해 필요한 데이터 구성.
 * 경계 영역 Set 를 순회 하면서 Cell 의 주변 방향 Cell 이 이동 영역 set 에 없다면 BorderEdgeInfos 에 추가
 */
void ABattleMoveArea::MakeBorderEdgeInfos()
{
	BorderEdgeInfos.Empty();

	for (int32 CurrGridCellId : BorderAreaSet)
	{
		for (EGridDirection eDirection : TEnumRange<EGridDirection>())
		{
			int32 ArroundGridCellId = BattleGridActor->GetArroundGridCellId(CurrGridCellId, eDirection);

			// Map End Line
			if (ArroundGridCellId == -1)
			{
				AddBorderEdgeInfo(CurrGridCellId, eDirection);
				continue;
			}

			// Movable Border Line
			if (MovableAreaSet.Find(ArroundGridCellId) == nullptr)
			{
				AddBorderEdgeInfo(CurrGridCellId, eDirection);
				continue;
			}
		}
	}
}

void ABattleMoveArea::DrawBorderByEdgeInfos()
{
	TArray<int32> GridCornerIdsForDraw;
	int32 NextGridCornerId;
	int32 CurrGridCornerId;
	int32 LastDirection;

	int32 CurrentIndex;

	while (BorderEdgeInfos.Num() > 0)
	{
		GridCornerIdsForDraw.Empty();

		NextGridCornerId = BorderEdgeInfos[0].StartGridCornerId;
		LastDirection = -1;

		int32 EdgeInfoCount = BorderEdgeInfos.Num();

		/**
		 * 1. BorderEdgeInfos 순회하면서 CurrGridCornerId 찾기
		 *	1.1 찾으면 중복인지 체크
		 *		1.1.1 중복 체크용 컨테이너에서 해당 중복 제거
		 *		1.1.2 CurrGridCornerId 의 방향에서 우선시 되는 방향 선택 (시계방향 우선)
		 *		1.1.3 다른게 우선이면 BorderEdgeInfos 에서 다른 GridCornerId 찾아서 Current 로 설정.
		 * 
		 *	1.2 CurrGridCornerId 를 GridCornerIdsForDraw 에 추가
		 *	1.3 BorderEdgeInfos 에서 CurrGridCornerId 제거
		 * 
		 * 2. 순회 종료시 GridCornerIdsForDraw 로 Border Line 그리기
		 */

		// 1. Make GridCornerIdsForDraw
		for (int32 i = 0; i < EdgeInfoCount; ++i)
		{
			CurrGridCornerId = NextGridCornerId;

			CurrentIndex = FindBorderEdgeInfo(CurrGridCornerId);
			if (CurrentIndex == INDEX_NONE)
			{
				break;
			}

			// Step 1. 중첩된 EdgeInfo 일때 LastDirection 과 NextDirection 이 같으면 두번째 EdgeInfo 를 Current 로 변경
			int32 DuplicateIndex = DuplicateStartGridCornerIds.Find(CurrGridCornerId);
			if (DuplicateIndex != INDEX_NONE)
			{
				// Step 1.1
				DuplicateStartGridCornerIds.RemoveAtSwap(DuplicateIndex, 1, false);

				// Step 1.2
				const FEdgeInfo& CurrEgetInfo = BorderEdgeInfos[CurrentIndex];
				int32 CurrDirection = CurrEgetInfo.EdgeDirection;
				int32 NextDirection = CurrDirection == 3 ? 0 : CurrDirection + 1;	// ClockWise Left > Top > Right > Bottom > Left

				if (LastDirection == NextDirection)
				{
					CurrentIndex = FindSecondBorderEdgeInfo(CurrGridCornerId);
				}
			}

			// Step 2.
			{
				const FEdgeInfo& CurrEgetInfo = BorderEdgeInfos[CurrentIndex];

				NextGridCornerId = CurrEgetInfo.EndGridCornerId;
				LastDirection = CurrEgetInfo.EdgeDirection;

				GridCornerIdsForDraw.Emplace(CurrGridCornerId);
			}

			// Step 3.
			{
				BorderEdgeInfos.RemoveAtSwap(CurrentIndex, 1, false);
			}
		}

		// 2. Draw the current edge line among the edge lines that make up the move area.
		DrawBorderLine(GridCornerIdsForDraw);
	}
}

/**
 * Slot 단위로 Map 의 영역 안에 있는지 체크 없음.
 * 호출 이전에 Slot 단위로 Map 의 영역 안에 있다는 것이 보장 되어야 함.
 */
void ABattleMoveArea::AddGridCellToMovableAreaBySlot(const int32 TargetGridCellId)
{
	int32 currGridCellId;

	for (int i = 0; i < BattleSlotSquareSize; ++i)
	{
		for (int j = 0; j < BattleSlotSquareSize; ++j)
		{
			currGridCellId = TargetGridCellId + (i * ColumnSizeOfGridActor) + j;
			MovableAreaSet.Emplace(currGridCellId);
		}
	}

	MovableGridCellSet.Emplace(TargetGridCellId);
}

void ABattleMoveArea::AddGridCellToBorderAreaBySlot(const int32 TargetGridCellId)
{
	int32 currGridCellId;

	for (int i = 0; i < BattleSlotSquareSize; ++i)
	{
		for (int j = 0; j < BattleSlotSquareSize; ++j)
		{
			currGridCellId = TargetGridCellId + (i * ColumnSizeOfGridActor) + j;
			BorderAreaSet.Emplace(currGridCellId);
		}
	}
}

/**
 * 이동 가능 영역의 Border Line 을 그리기 위해 Grid Corner Id 를 BorderEdgeInfos 에 추가.
 * CoreGridCellId 를 기준으로 하는 Slot 에서 방향에 해당하는 Grid Corner Id 를 구해서 추가함.
 */
void ABattleMoveArea::AddBorderEdgeInfo(const int32 CoreGridCellId, const EGridDirection eDirection)
{
	if ((eDirection < EGridDirection::Count) == false)
	{
		TRACE(Error, "Invalid Grid Direction : %s", *EnumToFString(eDirection));
		return;
	}

	if (!IsValid(BattleGridActor))
	{
		TRACE(Error, "Invalid Battle Grid Actor");
		return;
	}

	FIntPoint GridCoord = BattleGridActor->GetGridCoordinateByCellId(CoreGridCellId);

	/**
	 * Grid Corner(테두리) 와 GridCellId(테두리 안쪽) 와의 상관 관계
	 * Grid Map 가로측 크기(ColumnSizeOfGridActor)가 3 이면 다음과 같음.
	 * 
	 *							Coord.X
	 * 0 --- 1 --- 2 --- 3
	 * |  0  |  1  |  2  |		0 Row
	 * 4 --- 5 --- 6 --- 7
	 * |  3  |  4  |  5  |		1 Row
	 * 8 --- 9 --- 10 -- 11
	 * |  6  |  7  |  8  |		2 Row
	 * 12 -- 13 -- 14 -- 15
	 * 
	 */
	int32 LeftTopCornerId = CoreGridCellId + GridCoord.X;
	int32 RightTopCornerId = LeftTopCornerId + 1;
	int32 LeftBottomCornerId = RightTopCornerId + ColumnSizeOfGridActor;
	int32 RightBottomCornerId = LeftBottomCornerId + 1;

	int32 StartCornerId = -1;
	int32 EndCornerId = -1;
	int32 EdgeDirection = -1;

	switch (eDirection)
	{
		case EGridDirection::Left:
		{
			StartCornerId = LeftBottomCornerId;
			EndCornerId = LeftTopCornerId;
			EdgeDirection = LEFT_DIRECTION;
			break;
		}
		case EGridDirection::Top:
		{
			StartCornerId = LeftTopCornerId;
			EndCornerId = RightTopCornerId;
			EdgeDirection = TOP_DIRECTION;
			break;
		}
		case EGridDirection::Right:
		{
			StartCornerId = RightTopCornerId;
			EndCornerId = RightBottomCornerId;
			EdgeDirection = RIGHT_DIRECTION;
			break;
		}
		case EGridDirection::Bottom:
		{
			StartCornerId = RightBottomCornerId;
			EndCornerId = LeftBottomCornerId;
			EdgeDirection = BOTTOM_DIRECTION;
			break;
		}
	}

	FEdgeInfo edgeInfo = FEdgeInfo(StartCornerId, EndCornerId, EdgeDirection);

	const int32 FindIndex = FindBorderEdgeInfo(StartCornerId);
	if (FindIndex != INDEX_NONE)
	{
		// FEdgeInfo 를 추가 하기전에 StartGridCornerId 를 사용하는 다른 FEdgeInfo 가 이미 있다면 DuplicateStartGridCornerIds 에 추가
		DuplicateStartGridCornerIds.Emplace(StartCornerId);
	}

	BorderEdgeInfos.Emplace(edgeInfo);
}

const int32 ABattleMoveArea::FindBorderEdgeInfo(const int32 StartGridCornerId) const
{
	int32 FoundIndex = BorderEdgeInfos.IndexOfByPredicate([StartGridCornerId](const FEdgeInfo& EdgeInfo)
	{
		return EdgeInfo.StartGridCornerId == StartGridCornerId;
	});

	return FoundIndex;
}

const int32 ABattleMoveArea::FindSecondBorderEdgeInfo(const int32 StartGridCornerId) const
{
	int32 FindCount = 0;

	const int32 FoundIndex = BorderEdgeInfos.IndexOfByPredicate([StartGridCornerId, &FindCount](const FEdgeInfo& EdgeInfo)
	{
		if (EdgeInfo.StartGridCornerId == StartGridCornerId)
		{
			++FindCount;
			return FindCount == 2; // 두번째 EdgeInfo 를 찾으면 true 반환
		}

		return false; // 첫번째가 아니면 false 반환
	});

	return FoundIndex;
}

