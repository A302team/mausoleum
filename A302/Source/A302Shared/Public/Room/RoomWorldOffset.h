#pragma once

#include "CoreMinimal.h"
#include "Misc/Crc.h"

namespace A302RoomWorldOffset
{
	// Keep room offsets in a safer range for local/dedicated tests while preserving room separation.
	inline constexpr int32 DefaultRoomSlotCount = 128;
	inline constexpr double DefaultOffsetStepX = 50000.0;

	inline FString NormalizeRoomCode(const FString& RoomCode)
	{
		return RoomCode.TrimStartAndEnd().ToUpper();
	}

	inline int32 ResolveRoomSlot(const FString& RoomCode, int32 SlotCount = DefaultRoomSlotCount)
	{
		if (RoomCode.IsEmpty() || SlotCount <= 0)
		{
			return 0;
		}

		const FString NormalizedRoomCode = NormalizeRoomCode(RoomCode);
		const uint32 RoomHash = FCrc::StrCrc32(*NormalizedRoomCode);
		return 1 + static_cast<int32>(RoomHash % SlotCount);
	}

	inline FVector ResolveRoomOffset(const FString& RoomCode, double OffsetStepX = DefaultOffsetStepX, int32 SlotCount = DefaultRoomSlotCount)
	{
		const int32 RoomSlot = ResolveRoomSlot(RoomCode, SlotCount);
		if (RoomSlot <= 0 || OffsetStepX <= 0.0)
		{
			return FVector::ZeroVector;
		}

		return FVector(RoomSlot * OffsetStepX, 0.0, 0.0);
	}

	inline FString BuildLevelInstanceNameOverride(const FString& RoomCode)
	{
		FString SafeRoomCode = NormalizeRoomCode(RoomCode);
		for (TCHAR& Ch : SafeRoomCode)
		{
			if (!FChar::IsAlnum(Ch))
			{
				Ch = TEXT('_');
			}
		}

		if (SafeRoomCode.IsEmpty())
		{
			SafeRoomCode = TEXT("DEFAULT");
		}

		return FString::Printf(TEXT("A302_Room_%s"), *SafeRoomCode);
	}
}
