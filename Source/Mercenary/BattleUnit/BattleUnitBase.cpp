﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "Mercenary/BattleUnit/BattleUnitBase.h"


// Sets default values
ABattleUnitBase::ABattleUnitBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABattleUnitBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABattleUnitBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABattleUnitBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

