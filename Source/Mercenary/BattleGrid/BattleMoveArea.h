// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Mercenary/BattleGrid/BattleGridActor.h"

#include "BattleMoveArea.generated.h"


/**
 * Move Area 는 GridCell 의 Border Line 으로 구성.
 * 해당 Border Line(Edge) 을 구성하기 위해 사용하는 구조체.
 */
USTRUCT(BlueprintType)
struct FEdgeInfo
{
	GENERATED_BODY()

	FEdgeInfo()
	{
		StartGridCornerId = -1;
		EndGridCornerId = -1;
		EdgeDirection = -1;
	}

	FEdgeInfo(const int32 startCornerId, const int32 endCornerId, const int32 dirType)
		: StartGridCornerId(startCornerId)
		, EndGridCornerId(endCornerId)
		, EdgeDirection(dirType)
	{
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 StartGridCornerId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 EndGridCornerId;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 EdgeDirection;
};


USTRUCT(BlueprintType)
struct FMoveInfo
{
	GENERATED_BODY()

	int32 GridCellId;
	float MovePoint;
};


UCLASS(BlueprintType, Blueprintable)
class MERCENARY_API ABattleMoveArea : public AActor
{
	GENERATED_BODY()
	

public:	
	// Sets default values for this actor's properties
	ABattleMoveArea();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Battle MoveArea")
	void SetBattleSlotSquareSize(const uint8 SquareSize) { BattleSlotSquareSize = SquareSize; }

	UFUNCTION(BlueprintCallable, Category = "Battle MoveArea")
	void GetGridCornerLocations(const TArray<int32>& GridCornerIds, TArray<FVector>& GridCornerWorldLocations) const;

	// Draw the Border Line of the movable area
	// todo : maybe need to color the Border Line.
	UFUNCTION(BlueprintCallable, Category = "Battle MoveArea")
	void DrawMoveArea(const FVector WorldStartLoc, const float fMovePoints);


protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Battle MoveArea")
	void DrawBorderLine(const TArray<int32>& GridCorderIds);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle MoveArea")
	void ResetPathTracer();


private:
	// Find the Grid Cell that make up the boundary of the movable area.
	void FindGridCellForMoveArea();

	// Configure the Edge Info for drawing the Border.
	void MakeBorderEdgeInfos();

	// Drawing a Border Line based on Edge info
	void DrawBorderByEdgeInfos();

	// Add the Grid Cells to Movable Area (By Battle Slot)
	void AddGridCellToMovableAreaBySlot(const int32 TargetGridCellId);

	// Add the Grid Cell to Border Area (By Battle Slot)
	void AddGridCellToBorderAreaBySlot(const int32 TargetGridCellId);

	// Add the edges Info of the grid cell for make border points(Grid Corner Ids)
	void AddBorderEdgeInfo(const int32 CoreGridCellId, const EGridDirection eDirection);

	const int32 FindBorderEdgeInfo(const int32 StartGridCornerId) const;

	const int32 FindSecondBorderEdgeInfo(const int32 StartGridCornerId) const;


protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ABattleGridActor> BattleGridActor;

	UPROPERTY(BlueprintReadOnly)
	int32 StartGridCellId;

	UPROPERTY(BlueprintReadOnly)
	float MoveDistance;

	// 이동 가능한 Grid Cell Ids (Per Cell). BattleSlot 의 Core Id 가 될 수 있는 Set
	UPROPERTY(BlueprintReadOnly)
	TSet<int32> MovableGridCellSet;

	// 이동 가능 영역 (Per Slot)
	UPROPERTY(BlueprintReadOnly)
	TSet<int32> MovableAreaSet;

	// 이동 가능 영역중에서 Border 에 해당하는 영역
	UPROPERTY(BlueprintReadOnly)
	TSet<int32> BorderAreaSet;

	// Move Area 를 구성하는 모든 Border Edge. For Draw Border Line
	UPROPERTY(BlueprintReadOnly)
	TArray<FEdgeInfo> BorderEdgeInfos;

	// 중첩된 Edge 의 시작 Grid Corner ID
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> DuplicateStartGridCornerIds;

	/**
	 * 기본적으로 정방향.
	 * BattleUnit 의 크기에 기반한 GridCell 를 차지.
	 * 인간형은 2x2, 3x3 (탱커) GridCell 를 차지.
	 * 
	 * 1x1 (1.25m 이하), 2x2 (1.25m ~ 1.75m], 3x3(1.75m ~ 2.25m], 4x4 (2.25m ~ 2.75m], 5x5 (2.75m ~ 3.25m],
	 * 6x6 (3.25m ~ 3.75m], 7x7 (3.75m ~ 4.25m], 8x8 (4.25m ~ 4.75m], 9x9 (4.75m ~ 5.25m], 10x10 (5.25m ~ 5.75m]
	 * 
	 * 코끼리 몸길이가 5.4 ~ 7.5m
	 * 북극곰 수컷 몸길이 2.4 ~ 3m, 암컷 1.8 ~ 2.4m
	 * 불곰 1.4 ~ 2.8m
	 * 말 길이 보통 2.4m
	 */
	UPROPERTY(BlueprintReadOnly)
	uint8 BattleSlotSquareSize = 3;

	int32 RowSizeOfGridActor;
	int32 ColumnSizeOfGridActor;
};

