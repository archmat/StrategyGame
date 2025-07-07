// Fill out your copyright notice in the Description page of Project Settings.


#include "Mercenary/BattleGrid/BattleGridBlockActor.h"

// Engine
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"

// Project
#include "Mercenary/Logging.h"


// Sets default values
ABattleGridBlockActor::ABattleGridBlockActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	// Create Default SubObjects
	{
		if (RootComponent == nullptr)
			RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

		DebugBillboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard For Debug"));
		DebugBillboard->SetupAttachment(RootComponent);

		DebugMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh For Debug"));
		DebugMesh->SetupAttachment(RootComponent);

		CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
		CollisionBox->SetupAttachment(RootComponent);

		VisibleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
		VisibleMesh->SetupAttachment(RootComponent);
	}
}

// Called when the game starts or when spawned
void ABattleGridBlockActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
//void ABattleGridBlockActor::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//}

