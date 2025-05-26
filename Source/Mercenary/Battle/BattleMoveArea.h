// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Mercenary/Battle/BattleGridActor.h"

#include "BattleMoveArea.generated.h"


/**
 * Move Area 는 GridCell 의 Border Line 으로 구성.
 * 해당 Border Line(Edge) 을 구성하기 위해 사용하는 구조체.
 */
USTRUCT(BlueprintType)
struct FEdgeInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 StartGridCornerId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 EndGridCornerId;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EGridDirection EdgeDirection;
};

USTRUCT(BlueprintType)
struct FMoveInfo
{
	GENERATED_BODY()

	int32 GridCellId;
	float MovePoint;
};


UCLASS()
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
	// Find the Grid Cell that make up the boundary of the movable area
	void FindGridCellIdsForMoveArea();

	// Filter the Grid Corner Ids
	void FilterdGridCornerIdsAndDrawBorder();

	// Add the Grid Cells to Movable Area (By Battle Slot)
	void AddGridCellToMovableAreaBySlot(const int32 SrcGridCellId);

	// Add the edges Info of the grid cell for make border points(Grid Corner Ids)
	void AddGridCellEdgeForBorderPoints(const int32 GridCellId, const EGridDirection eDirection);


protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ABattleGridActor> BattleGridActor;

	UPROPERTY(BlueprintReadOnly)
	int32 StartGridCellId;

	UPROPERTY(BlueprintReadOnly)
	float MoveDistance;

	// Move Area 를 구성하는 모든 Border Edge
	UPROPERTY(BlueprintReadOnly)
	TArray<FEdgeInfo> BorderEdgeInfos;

	// 이동 영역의 Grid Cell Ids.
	UPROPERTY(BlueprintReadOnly)
	TSet<int32> MovableAreaSet;

	// 이동 영역의 Border 에 해당하는 Grid Cell Ids.
	UPROPERTY(BlueprintReadOnly)
	TSet<int32> BorderAreaSet;

	// 중첩된 Edge 의 시작 Grid Corner ID
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> DuplicateGridCornerIds;


private:
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
	uint8 BattleSlotSquareSize = 3;

	int32 RowSizeOfGridActor;
	int32 ColumnSizeOfGridActor;

};
