﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Logging/StructuredLog.h"


/*
* #ksh : NetMode Log Macro
* [see] https://unrealcommunity.wiki/log-macro-with-net-mode-and-color-q3vj8r7c
*/
#define NETMODE_WORLD (((GetWorld() == nullptr)) ? TEXT("") \
	: (GEngine->GetNetMode(GetWorld()) == NM_Client) ? TEXT("[Client] ") \
	: (GEngine->GetNetMode(GetWorld()) == NM_ListenServer) ? TEXT("[ListenServer] ") \
	: (GEngine->GetNetMode(GetWorld()) == NM_DedicatedServer) ? TEXT("[DedicatedServer] ") \
	: TEXT("[Standalone] "))


#if _MSC_VER
#define FUNC_NAME    TEXT(__FUNCTION__)
#else // FIXME - GCC?
#define FUNC_NAME    TEXT(__func__)
#endif


/*
* Screen Log For Debugging
*/

#if NO_LOGGING

#define TRACE(Verbose, Format, ...)
#define TRACE_TOAST(Verbose, Format, ...)
#define TRACE_FMT(Verbose, Format, ...)

#define TRACE_STATIC(Verbose, Format, ...)

#define ScrLog(key, timeToDisplay, Format, ...)
#define ScrColorLog(key, timeToDisplay, Color, Format, ...)

#else

/*
namespace OutputDeviceColor
{
	inline const TCHAR* const COLOR_BLACK = TEXT("0000");
	inline const TCHAR* const COLOR_DARK_RED = TEXT("1000");
	inline const TCHAR* const COLOR_DARK_GREEN = TEXT("0100");
	inline const TCHAR* const COLOR_DARK_BLUE = TEXT("0010");
	inline const TCHAR* const COLOR_DARK_YELLOW = TEXT("1100");
	inline const TCHAR* const COLOR_DARK_CYAN = TEXT("0110");
	inline const TCHAR* const COLOR_DARK_PURPLE = TEXT("1010");
	inline const TCHAR* const COLOR_DARK_WHITE = TEXT("1110");
	inline const TCHAR* const COLOR_GRAY = COLOR_DARK_WHITE;
	inline const TCHAR* const COLOR_RED = TEXT("1001");
	inline const TCHAR* const COLOR_GREEN = TEXT("0101");
	inline const TCHAR* const COLOR_BLUE = TEXT("0011");
	inline const TCHAR* const COLOR_YELLOW = TEXT("1101");
	inline const TCHAR* const COLOR_CYAN = TEXT("0111");
	inline const TCHAR* const COLOR_PURPLE = TEXT("1011");
	inline const TCHAR* const COLOR_WHITE = TEXT("1111");
	inline const TCHAR* const COLOR_NONE = TEXT("");
}
/**/

#define TRACE(Verbose, Format, ...) \
{ \
    SET_WARN_COLOR(COLOR_CYAN);\
    const FString traceLogMsg = FString::Printf(TEXT(Format), ##__VA_ARGS__); \
    UE_LOG(LogTemp, Verbose, TEXT("[%s] %s() - %s"), *GetNameSafe(this), FUNC_NAME, *traceLogMsg );\
    CLEAR_WARN_COLOR();\
}

//#define TRACE_TOAST(Verbose, Format, ...) \
// { \
//	TRACE(Verbose, Format, ##__VA_ARGS__); \
//	const FString toastMsg = FString::Printf(TEXT(Format), ##__VA_ARGS__); \
//	ToastPopupInstance::ShowToastPopupType(toastMsg, ECmesEqualVerbosityType::Verbose); \
// }

#define TRACE_FMT(Verbose, Format, ...) \
{ \
    SET_WARN_COLOR(COLOR_CYAN);\
	UE_LOGFMT(LogTemp, Verbose, "[{SafeName}] {FuncName}() - " Format, *GetNameSafe(this), FUNC_NAME, ##__VA_ARGS__); \
    CLEAR_WARN_COLOR();\
}

#define TRACE_STATIC(Verbose, Format, ...) \
{ \
    SET_WARN_COLOR(COLOR_CYAN);\
    const FString traceLogMsg = FString::Printf(TEXT(Format), ##__VA_ARGS__); \
    UE_LOG(LogTemp, Verbose, TEXT("%s() - %s"), FUNC_NAME, *traceLogMsg );\
    CLEAR_WARN_COLOR();\
}

#define TRACE_STATIC_FMT(Verbose, Format, ...) \
{ \
    SET_WARN_COLOR(COLOR_CYAN);\
    UE_LOGFMT(LogTemp, Verbose, "{FuncName}() - " Format, FUNC_NAME, ##__VA_ARGS__); \
    CLEAR_WARN_COLOR();\
}

#define ScrLog(key, timeToDisplay, Format, ...) \
{\
	if (GEngine) \
    { \
        const FString scrLogMsg = FString::Printf(TEXT(Format), ##__VA_ARGS__); \
		GEngine->AddOnScreenDebugMessage(key, timeToDisplay, FColor::White, *scrLogMsg); \
	} \
}

#define ScrColorLog(key, timeToDisplay, Color, Format, ...) \
{\
	if (GEngine) \
    { \
        const FString scrLogMsg = FString::Printf(TEXT(Format), ##__VA_ARGS__); \
		GEngine->AddOnScreenDebugMessage(key, timeToDisplay, Color, *scrLogMsg); \
	} \
}

#endif  // #if NO_LOGGING


#define DEFAULT_SCR_LOG_TIME	10.f


template<typename EnumType>
static FString EnumToFString(EnumType eType)
{
	return StaticEnum<EnumType>()->GetValueAsString(eType);
}

