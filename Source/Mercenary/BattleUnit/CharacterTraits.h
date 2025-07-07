
#pragma once

#include "CoreMinimal.h"

#include "CharacterTraits.generated.h"


UENUM(BlueprintType)
enum class ETraitsType : uint8
{
	None,

	// 대립 특성
	Brave,			// 용감한
	Coward,			// 겁쟁이

	Ambitious,		// 야심 있는
	Content,		// 만족하는, 자족하는

	Passionate,		// 열렬한
	Indifferent,	// 무관심한, 냉담한

	Optimistic,		// 낙관적인
	Cynical,		// 냉소적인, 비꼬는

	Diligent,		// 근면한, 부지런한
	Lazy,			// 게으른, 나태한

	Greedy,			// 탐욕스러운
	Frugal,			// 검소한, 절약하는

	Gregarious,		// 사교적인
	Shy,			// 수줍은, 부끄러워하는

	Honest,			// 정직한, 성실한
	Deceitful,		// 속이는, 기만하는

	Humble,			// 겸손한, 겸허한
	Arrogant,		// 거만한, 오만한

	Impetuous,		// 충동적인, 성급한
	Prudent,		// 신중한, 조심성 있는

	Forgiving,		// 용서하는, 관대한
	Vengeful,		// 복수심에 불타는, 앙심을 품은

	Just,			// 공정한, 정의로운	
	Arbitary,		// 제멋대로의, 임의적인

	Sadistic,		// 가학적인, 잔인한
	Merciful,		// 자비로운, 인정 많은







	// 기벽 특성
	Melancholic,	// 우울한, 침울한
	Lunatic,		// 미친, 정신병자

	Drunkard,		// 술고래
	Glutton,		// 대식가
	Inappetetic,	// 식욕이 없는, 식욕부진의
	Rakish,			// 한량
	Profligate,		// 방탕한, 낭비하는
	Reclusive,		// 은둔하는, 세상을 등진

	//
	Count			UMETA(Hidden)
};


// @see https://benui.ca/unreal/iterate-over-enum-tenumrange/
ENUM_RANGE_BY_COUNT(ETraitsType, ETraitsType::Count);
//ENUM_RANGE_BY_FIRST_AND_LAST(ETraitsType, ETraitsType::Gun, ETraitsType::RightFoot);
//ENUM_RANGE_BY_FIRST_AND_LAST(ETraitsType, ETraitsType::Gun, ETraitsType::RightKnee);

template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept
{
	return static_cast<typename std::underlying_type<E>::type>(e);
}

constexpr int32 TrackTypeHmd = to_underlying(ETrackObject::HMD);


USTRUCT(BlueprintType)
struct FCharacterTraits
{
	GENERATED_USTRUCT_BODY()

};
