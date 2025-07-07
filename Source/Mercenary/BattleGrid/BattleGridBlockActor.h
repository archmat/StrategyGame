// Fill out your copyright notice in the Description page of Project Settings.

/**
 * Battle 씬에 배치될 배경 오브젝트용 액터
 * FGridCell 의 Obstacle 여부에 반영되는 모든 배경 오브젝트들은 이 액터를 통해 레벨에 배치 되어야 함.
 */
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleGridBlockActor.generated.h"

class UBoxComponent;


UCLASS()
class MERCENARY_API ABattleGridBlockActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABattleGridBlockActor();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	//// Called every frame
	//virtual void Tick(float DeltaTime) override;


protected:
	// For Debug
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UBillboardComponent> DebugBillboard;

	// For Debug
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> DebugMesh;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> VisibleMesh;
};
