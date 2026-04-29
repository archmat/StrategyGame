// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BattleUnitBase.generated.h"


UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EDamageType : uint32
{
	None = 0 UMETA(Hidden),

	//
	Slash = 1 << 0,
	Pierce = 1 << 1,
	Crush = 1 << 2,

	Physical = Slash | Pierce | Crush,

	//
	Fire = 1 << 3,
	Ice = 1 << 4,
	Lightning = 1 << 5,

	Nature = Fire | Ice | Lightning,

	//
	Magic = 1 << 6,
};
ENUM_CLASS_FLAGS(EDamageType)


USTRUCT(BlueprintType)
struct FUnitAbility
{
	GENERATED_BODY()

#pragma region -- Ability Score (Raw Stats) --
	// 신체적인 힘, 근육량, 물리적 파워. 장비 사용가능한 힘. 이를 넘어서는 힘은 데미지에 보너스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Strength;
	
	// 유연성, 반사 신경, 균형 감각, 손재주. 장비를 얼마나 효율적으로 다루는가의 척도
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Dexterity;
	
	// 체력, 회복력, 질병이나 독에 대한 저항력.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Constitution;
	
	// 논리적 사고, 기억력, 교육 수준, 분석력.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Intelligence;
	
	// 인식, 지각, 견해, 주변 상황을 살피는 감각
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Perception;
	
	// 꺽마
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Will;
#pragma endregion

#pragma region -- Attributes (Derived Stats)--
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MaxHP;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CurrHP;

	// 활력 상태 (100% - 쾌적한 상태, 120% - 버프로 인해 기운이 솓아나는 상태, 50% - 숨이 참, 10% - 탈진)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Vitality;

	// 사기 수치 (0 ~ 100). 사기가 높을수록 전투 능력치에 보너스. 사기가 낮으면 패널티.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Morale;
	
	// 유닛 크기 (ex. 1x1, 2x2, 3x3 GridCell 차지)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 UnitSize;
	
	// 유닛 질량 (Kg 단위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float UnitMass;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MovePoint;
#pragma endregion

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BodyArmor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ShieldArmor;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ShieldDefence;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MeleeDamage;

	// 근접 공격 명중 수치 (ex. 근거리 명중 확률 = 공격자의 MeleeAttack - 방어자의 MeleeDefence)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MeleeAttack;
	
	// 근접 공격 방어 수치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MeleeDefence;

	// 원거리 무기 데미지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MissileDamage;

	// 원거리 공격 명중 확률 (원거리 명중 확률은 Melee 와는 다르게 공격자 요소만 있음. 멀수록 명중 확률 감소)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Accuracy;

	// 유효 사거리. (ex. 사거리 [0%] - Accuracy * 0.5 패널티 - [10%] - Accuracy - [70%] - Accuracy 에 거리만큼 패널티 - [100%] / 명사수 특성은 패널티 없음)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Range;

};


UCLASS()
class MERCENARY_API ABattleUnitBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABattleUnitBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
