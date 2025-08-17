#pragma once
#include <xkeycheck.h>

namespace Globals {
	bool bIsProdServer = false;

	bool bCreativeEnabled = false;
	bool bSTWEnabled = false;
	bool bEventEnabled = false;

	bool bBossesEnabled = true;
	bool bBotsEnabled = true;

	bool bUseLegacyAI_MANG = true; // Keep this on true bro, I am NOT doing this!

	bool LateGame = false;
	bool Automatics = true;
	bool BattleLab = false;
	bool Blitz = false;
	bool StormKing = false;
	bool Arsenal = false;
	bool TeamRumble = false;
	bool SolidGold = false;
	bool UnVaulted = false;
	bool Siphon = false;
	bool Arena = false;

	int MaxBotsToSpawn = 100;
	int MinPlayersForEarlyStart = 100;
}
