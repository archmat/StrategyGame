// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BattleUnitBase.generated.h"


/**
 * # 데미지 자체의 속성.
 * 
 * 
 * # 일반 무기는 Physical 데미지 타입, 직접적인 물리 데미지를 주는 형태. 
 *	마법도 물체를 구현하여 공격할 경우 마법적인 물리 데미지.
 * ex)
 *  1. 강철 검 : Slash(Physical)
 *  2. 불화살(비마법) : Pierce(Physical) + Fire(Nature) 
 *  3. 불화살(마법) : Arcane & Pierce(Physical) + Arcane & Fire(Nature) 
 *  4. 불붙이기 : Fire(Nature)
 *  5. 발화 마법 : Arcane & Fire(Nature)
 *		- 발화 마법은 구현만 있기에 형태가 없음.
 * 
 * 
 * # 데미지는 주 데미지(구현과 형태), 추가 데미지로 구성.
 *  ex1) 불화살(비마법) : Pierce(Physical) + Fire(Nature) 
 *		Pierce(Physical) + Fire(Nature) 
 *		주 데미지 : 형태	   자연적인 불 추가 데미지
 *  ex2) 불화살(마법) : 비전마법으로 구현한 불화살 형태
 *		Arcane & Pierce(Physical) + Arcane & Fire(Nature)
 *		마법으로 형태구현				마법에 의한 불 추가데미지
 * 
 * 
 * # 데미지는 주 데미지, 추가 데미지 각각 계산
 *	데미지 타입에 물리적인 형태가 있다면 모두 방어도에 의한 데미지 감소가 먼저 적용 (방어구가 직접 피격당할때, 보호막은 방어구 에 해당 안됨)
 *	저항은 해당 데미지 타입에 해당하는 모든 저항의 합연산으로 계산. 
 * 
 * ex1) 발화 마법 : Arcane & Fire -> Arcane 저항 10%, Fire 저항 20% 
 *		100 * (0.1 + 0.2) = 30 으로 100 -> 70 데미지.
 * 
 * ex2) 불화살(마법) : Arcane & Pierce(Physical) + Arcane & Fire(Nature) -> Arcane 저항 10%, Pierce 저항 50%, Fire 저항 20%
 *		방어도 100 으로인한 데미지 감소 30%
 * 
 *		주 데미지 Arcane & Pierce(Physical)
 *			100 * (0.3) = 30 으로 100 -> 70 데미지. : 방어구로 인한 감소
 *			70 * (0.1 + 0.5) = 42 으로 70 -> 28 데미지 : 저항으로 인한 감소
 * 
 *		추가 데미지 Arcane & Fire(Nature) : 별도의 10 추가데미지
 *			10 * (0.1 + 0.2) = 3 으로 10 -> 7 데미지 : 저항으로 인한 감소
 * 
 *		
 * 
 *		형태 저항으로 70 * (0.3 + 0.1) = 28 로 인해 70 -> 42 데미지.
 * 
 * 면역이 아닌 이상 합연산된 저항은 최대치 95% 제한. 취약은 제한 없음.
 * 
 * 바위처럼 이미 형태가 있는 물체를 마법으로 띄어서 던지는 경우는 구현부와 형태가 완전히 별도인 상태로 마법 저항이 큰 의미가 없음.
 * ex) 바위 던지기 마법 일때 던지는 행위 자체(마법)를 캔슬 시키지 않는다면 이미 날라가는 바위는 관성의 영향으로 Physical 데미지 취급이라 마법 저항 적용 안됨.
 *		바위 자체를 마법으로 구현하는 경우는 Arcane + Crush 형태로 구현부와 형태가 모두 마법이기에 마법 저항 적용됨.
 *  
 */
UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EDamageType : uint8
{
	None = 0 UMETA(Hidden),

	// 물리 데미지
	Slash = 1 << 0,		// 베기, 절단, 찢는 형태의 데미지
	Pierce = 1 << 1,	// 찌르기, 관통 형태의 데미지
	Crush = 1 << 2,		// 타격, 충격 형태의 데미지

	Physical = Slash | Pierce | Crush,

	// 자연 데미지
	Fire		= 1 << 3,
	Ice			= 1 << 4,
	Lighting	= 1 << 5,

	Nature		= Fire | Ice | Lighting,

	//// 마법, 초자연적인 데미지
	//Arcane	= 1 << 6,	// 지적인 탐구와 공식에 기반한 주변의 마나를 조작하는 힘
	//Divine	= 1 << 7,	// Holy, Light - 신성계열 권능, 은총
	//Abyssal = 1 << 8,	// Void, Dark - 심연계열 권능, 은총

	//Magic	= Divine | Abyssal | Arcane,
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
