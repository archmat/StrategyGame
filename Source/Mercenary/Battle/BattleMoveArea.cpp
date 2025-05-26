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

	//
	ResetPathTracer();

	//
	FindGridCellIdsForMoveArea();

	// 
	FilterdGridCornerIdsAndDrawBorder();
}

// BP_MoveArea::FindBorderPoints
void ABattleMoveArea::FindGridCellIdsForMoveArea()
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

	//
	{
		if (false == BattleGridActor->IsBattleUnitPlaceable(StartGridCellId, BattleSlotSquareSize))
		{
			TRACE(Error, "Non-placable Start Grid Cell Id : %d, Slot Square Size : %d", StartGridCellId, BattleSlotSquareSize);
			return;
		}

		BorderAreaSet.Empty();

		MovableAreaSet.Empty();
		AddGridCellToMovableAreaBySlot(StartGridCellId);
	}

	// Setup For Find Move Area
	BorderEdgeInfos.Empty();
	DuplicateGridCornerIds.Empty();

	// Setup First Searching, Visited Info
	TArray<FMoveInfo> SearchingGirdCellIds;
	TArray<bool> AllVisitedFlags;
	{
		// Start Grid Cell 을 첫번째 탐색해야할 Grid Cell 로 설정.
		SearchingGirdCellIds.Emplace(FMoveInfo(StartGridCellId, MoveDistance));
	
		// 전체 Grid Cell 방문여부를 새로 초기화 하고 첫번째 방문한 Grid Cell 의 방문 여부 설정.
		int32 TotalGridCellSize = BattleGridActor->GetGridCellTotalSize();

		AllVisitedFlags.SetNumUninitialized(TotalGridCellSize);
		FMemory::Memzero((uint8*)AllVisitedFlags.GetData(), TotalGridCellSize * sizeof(bool));

		AllVisitedFlags[StartGridCellId] = true;
	}

	/**
	 * Cell 단위 이동 이라면 이곳에서 이동 범위의 라인을 알 수 있지만 Slot 단위 에선 알 수 없음.
	 *
	 * 그래서 해당 Cell 의 Slot 이 배치 가능 하면 Slot 을 구성하는 Cell 을 이동 영역으로 추가.
	 * 모든 이동 영역을 찾으면 Border 에 해당하는 Cell 만 필터링 해서 이동 범위 라인 구성.
	 */
	// Find Move Area
	while (SearchingGirdCellIds.Num() > 0)
	{
		int32 CurrGridCellId = SearchingGirdCellIds[0].GridCellId;
		float CurrMovePoint = SearchingGirdCellIds[0].MovePoint;

		for(EGridDirection eDirection : TEnumRange<EGridDirection>())
		{
			int32 ArroundGridCellId = BattleGridActor->GetArroundGridCellId(CurrGridCellId, eDirection);

			if (false == AllVisitedFlags.IsValidIndex(ArroundGridCellId))
			{
				/**
				 * Out of Map Area 에 의한 Border Line. 갈수 없는 Grid Cell 이기에 이동 영역 에 추가 없음.
				 */
				BorderAreaSet.Emplace(CurrGridCellId);

				continue;
			}

			// 이미 방문한 Grid Cell 이라면 Skip
			if (AllVisitedFlags[ArroundGridCellId])
			{
				continue;
			}

			// Move Cost 체크
			if (CurrMovePoint < DEFAULT_GRID_CELL_MOVE_COST)
			{
				/**
				 * Move Cost 에 의한 Border Line.
				 */
				//AddGridCellToMovableAreaBySlot(CurrGridCellId);

				continue;
			}

			// 배치 가능여부를 통해 이동 가능 한지 체크
			// #todo : 캐릭터 특성에 따라 아군, 적군 통과 가능한 경우 Passable 함수를 통해 체크.
			if (false == BattleGridActor->IsBattleUnitPlaceable(ArroundGridCellId, BattleSlotSquareSize))
			{
				/**
				 * 배치 불가에 의한 Border Line.
				 */
			}

			// 이동 영역에 추가
			AddGridCellToMovableAreaBySlot(ArroundGridCellId);

			AllVisitedFlags[ArroundGridCellId] = true;

			float ArroundMoveCost = BattleGridActor->GetGridCellMoveCost(ArroundGridCellId);
			SearchingGirdCellIds.Emplace(FMoveInfo(ArroundGridCellId, CurrMovePoint - ArroundMoveCost));
		}

		SearchingGirdCellIds.RemoveAtSwap(0);
	}
}

// BP_MoveArea::Filtered Points and Draw Border
void ABattleMoveArea::FilterdGridCornerIdsAndDrawBorder()
{
	/**
	 * 이동 가능한 모든 Grid Cell 을 찾습니다.
	 * 1. StartGridCellId 를 기준으로 이동 가능한 Grid Cell 을 찾기.
	 * 2. 더이상 이동할수 없는 곳은 BattleSlot 의 Grid Cell 을 Border 를 구성하기 위해 따로 리스트에 추가.
	 * 3. Border 일 수 있는 Grid Cell 이 모두 추가 되었다면 해당 리스트에서 Border 인 Grid Cell 만 필터링.
	 */



	/**
	 * 점을 순차적으로 배치하려면 필터링해야 합니다.
	 * 테두리에는 고립된 영역, 즉 주 경계 내부의 "구멍"이 포함될 수 있습니다,
	 * 이러한 연결되어 있지 않는 각각의 "구멍"에 대해 고유한 획을 만들어야 하며 이는 추가적인 PathTracer 를 사용합니다.
	 */

	 //TArray<int32> GridCornerIds;

	 //// while (edge infos)
	 //{
	 //	int32 LastDirection = -1;

	 //	int32 NextGridCornerId;

	 //	int32 LastGridCornerId;

	 //	int32 CurrStartGridCornerId = NextGridCornerId;
	 //}


	 //// Draw the current edge line among the edge lines that make up the move area.
	 //DrawBorderLine(GridCornerIds);
}

/**
 * 이 함수는 SrcGridCellId 가 Slot 단위로 Map 의 영역 안에 있는지를 체크하지 않음.
 * 그러므로 호출 하기 전에 Slot 단위로 Map 의 영역 안에 있다는 것이 보장 되어야 함.
 */
void ABattleMoveArea::AddGridCellToMovableAreaBySlot(const int32 SrcGridCellId)
{
	int32 currGridCellId;

	for (int i = 0; i < BattleSlotSquareSize; ++i)
	{
		for (int j = 0; j < BattleSlotSquareSize; ++j)
		{
			currGridCellId = SrcGridCellId + (i * ColumnSizeOfGridActor) + j;

			MovableAreaSet.Emplace(currGridCellId);
		}
	}
}

/**
 * Border Points 를 구성 하기 위해 테두리로 판별된 Grid Cell 의 방향을 추가 합니다.
 */
// BP_MoveArea::AddCellEdgeToDrawBorder
void ABattleMoveArea::AddGridCellEdgeForBorderPoints(const int32 GridCellId, const EGridDirection eDirection)
{

}

