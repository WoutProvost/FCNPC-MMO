#if defined _FCNPC_MMO_included
	#endinput
#endif
#define _FCNPC_MMO_included

#include <a_samp>
#include <FCNPC>
#include <FCNPC-AI>
#include <util>

// ========================================FAKE NATIVES========================================
/*
// NPCs
native MMO_ToggleHUDElements(npcid, MMO_E_HUD:elements);
native MMO_E_HUD:MMO_GetHUDElements(npcid);
native MMO_SetName(npcid, const name[]);
native MMO_GetName(npcid, name[], len);
native MMO_SetMana(npcid, Float:mana, Float:max = -1.0);
native Float:MMO_GetMana(npcid, &Float:max = 100.0);
native MMO_SetDisplayRange(npcid, Float:range, playerid = -1);
native Float:MMO_GetDisplayRange(npcid, playerid);
*/

// ========================================CALLBACKS========================================
// None

// ========================================CONSTANTS========================================
// General
#define FCNPC_MMO_VERSION					"1.0.0"

// NPCs
#if !defined MMO_MAX_NPC_NAME
	#define MMO_MAX_NPC_NAME				50
#endif
enum MMO_E_TEXT_DRAW {
	Text:MMO_TEXT_DRAW_NPC_NAME,
	Text:MMO_TEXT_DRAW_HEALTH_BORDER,
	Text:MMO_TEXT_DRAW_HEALTH_BACKGROUND,
	Text:MMO_TEXT_DRAW_HEALTH_FOREGROUND,
	Text:MMO_TEXT_DRAW_HEALTH_PERCENTAGE,
	Text:MMO_TEXT_DRAW_HEALTH_VALUE,
	Text:MMO_TEXT_DRAW_MANA_BORDER,
	Text:MMO_TEXT_DRAW_MANA_BACKGROUND,
	Text:MMO_TEXT_DRAW_MANA_FOREGROUND,
	Text:MMO_TEXT_DRAW_MANA_PERCENTAGE,
	Text:MMO_TEXT_DRAW_MANA_VALUE,
	Text:MMO_TEXT_DRAW_SPELL_BORDER,
	Text:MMO_TEXT_DRAW_SPELL_BACKGROUND,
	Text:MMO_TEXT_DRAW_SPELL_FOREGROUND,
	Text:MMO_TEXT_DRAW_SPELL_NAME,
	Text:MMO_TEXT_DRAW_SPELL_TIME
}
enum MMO_E_HUD (<<=1) {
	MMO_HUD_ALL = -1,
	MMO_HUD_NONE = 0,
	MMO_HUD_NPC_NAME = 1,
	MMO_HUD_HEALTH_BAR,
	MMO_HUD_HEALTH_PERCENTAGE,
	MMO_HUD_HEALTH_VALUE,
	MMO_HUD_MANA_BAR,
	MMO_HUD_MANA_PERCENTAGE,
	MMO_HUD_MANA_VALUE,
	MMO_HUD_SPELL_BAR,
	MMO_HUD_SPELL_NAME,
	MMO_HUD_SPELL_TIME
}

// Debug
#define FAI_DEBUG_DISPLAY_RANGE				FAI_E_DEBUG_RANGE

// ========================================VARIABLES========================================
// Players
static MMO_ClosestNPC[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};

// NPCs
enum MMO_E_NPC {
	// Can't be set by the user
	Text:MMO_NPC_TEXT_DRAWS[MMO_E_TEXT_DRAW], // Use the Text: tag to be able to loop over the enum
	MMO_NPC_DELAY_HEALTH_DISPLAY,
	bool:MMO_NPC_IS_DEAD_BEFORE_RESPAWN,
	// Can be set by the user
	MMO_E_HUD:MMO_NPC_HUD,
	MMO_NPC_NAME[MMO_MAX_NPC_NAME + 1],
	Float:MMO_NPC_MANA, // Can be changed automatically by the script
	Float:MMO_NPC_MAX_MANA,
	Float:MMO_NPC_DISPLAY_RANGE[MAX_PLAYERS]
}
static MMO_NPCs[MAX_PLAYERS][MMO_E_NPC];

// Debug
static FAI_E_DEBUG_RANGE:MMO_DebugRanges[MAX_PLAYERS] = {FAI_DEBUG_RANGE_NONE, ...};
static MMO_DebugRangesPlayerID[MAX_PLAYERS] = INVALID_PLAYER_ID;

// ========================================INTERNAL FUNCTIONS========================================
// General
static MMO_ScriptInit() {
	// Players
	for(new playerid = 0; playerid < MAX_PLAYERS; playerid++) {
		MMO_ClosestNPC[playerid] = INVALID_PLAYER_ID;
	}

	// NPCs
	for(new npcid = 0; npcid < MAX_PLAYERS; npcid++) {
		MMO_ResetNPC(npcid);
	}
}

static MMO_ScriptExit() {
	// Players
	for(new playerid = 0; playerid < MAX_PLAYERS; playerid++) {
		MMO_ClosestNPC[playerid] = INVALID_PLAYER_ID;
	}

	// NPCs
	for(new npcid = 0; npcid < MAX_PLAYERS; npcid++) {
		// In case the user doesn't destroy the NPCs on script exit
		for(new textdraw = 0; MMO_E_TEXT_DRAW:textdraw < MMO_E_TEXT_DRAW; textdraw++) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw] = Text:INVALID_TEXT_DRAW;
			}
		}

		if(MMO_NPCs[npcid][MMO_NPC_DELAY_HEALTH_DISPLAY] != INVALID_TIMER_ID) {
			KillTimer(MMO_NPCs[npcid][MMO_NPC_DELAY_HEALTH_DISPLAY]);
			MMO_NPCs[npcid][MMO_NPC_DELAY_HEALTH_DISPLAY] = INVALID_TIMER_ID;
		}
	}
}

// NPCs
static MMO_ResetNPC(npcid) {
	// Don't use FAI_IsValidNPC
	for(new textdraw = 0; MMO_E_TEXT_DRAW:textdraw < MMO_E_TEXT_DRAW; textdraw++) {
		MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw] = Text:INVALID_TEXT_DRAW;
	}
	MMO_NPCs[npcid][MMO_NPC_DELAY_HEALTH_DISPLAY] = INVALID_TIMER_ID;
	MMO_NPCs[npcid][MMO_NPC_IS_DEAD_BEFORE_RESPAWN] = false;
	MMO_NPCs[npcid][MMO_NPC_HUD] = MMO_HUD_NONE;
	strclr(MMO_NPCs[npcid][MMO_NPC_NAME]);
	MMO_NPCs[npcid][MMO_NPC_MANA] = 100.0;
	MMO_NPCs[npcid][MMO_NPC_MAX_MANA] = 100.0;
	for(new playerid = 0; playerid < MAX_PLAYERS; playerid++) {
		MMO_NPCs[npcid][MMO_NPC_DISPLAY_RANGE][playerid] = 50.0;
	}

	// Debug
	MMO_DebugRanges[npcid] = FAI_DEBUG_RANGE_NONE;
	MMO_DebugRangesPlayerID[npcid] = INVALID_PLAYER_ID;
}

static MMO_UpdateNPCNameDisplay(npcid) {
	// Update the name of the NPC
	if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME] != Text:INVALID_TEXT_DRAW) {
		new name[MMO_MAX_NPC_NAME + 1];
		if(!strempty(MMO_NPCs[npcid][MMO_NPC_NAME])) {
			strcpy(name, MMO_NPCs[npcid][MMO_NPC_NAME], MMO_MAX_NPC_NAME + 1);
		} else {
			strcpy(name, "_", MMO_MAX_NPC_NAME + 1);
		}
		TextDrawSetString(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME], name);
	}
}

static MMO_UpdateNPCHealthDisplay(npcid) {
	if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND] != Text:INVALID_TEXT_DRAW || MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE] != Text:INVALID_TEXT_DRAW || MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE] != Text:INVALID_TEXT_DRAW) {
		new Float:maxHealth;
		new Float:health = FCNPC_GetHealth(npcid, maxHealth);

		// Update the health of the NPC
		if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND] != Text:INVALID_TEXT_DRAW) {
			TextDrawTextSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND], (470.0 - 167.3) * health / maxHealth + 167.3, 0.0);
		}

		// Update the health percentage of the NPC
		if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE] != Text:INVALID_TEXT_DRAW) {
			// Ceil so 0.3% still displays as 1% instead of 0%
			new outputPercentage[4 + 1];
			format(outputPercentage, sizeof(outputPercentage), "%d%%", floatround(health / maxHealth * 100, floatround_ceil));
			TextDrawSetString(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE], outputPercentage);
		}

		// Update the health value of the NPC
		if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE] != Text:INVALID_TEXT_DRAW) {
			// Ceil so 0.3 still displays as 1 instead of 0
			new healthInteger = floatround(health, floatround_ceil);
			new outputValue[14 + 1];
			if(float(healthInteger) / 1000 / 1000 / 1000 >= 1) {
				format(outputValue, sizeof(outputValue), "%.2fB", float(healthInteger) / 1000 / 1000 / 1000);
			} else if(float(healthInteger) / 1000 / 1000 >= 1) {
				format(outputValue, sizeof(outputValue), "%.2fM", float(healthInteger) / 1000 / 1000);
			} else if(float(healthInteger) / 1000 >= 1) {
				format(outputValue, sizeof(outputValue), "%.2fK", float(healthInteger) / 1000);
			} else {
				format(outputValue, sizeof(outputValue), "%d", healthInteger);
			}
			TextDrawSetString(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE], outputValue);
		}
	}

	// Reshow the textdraw to make the changes (size) visible
	if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND] != Text:INVALID_TEXT_DRAW) {
		MMO_ShowTextDraw(MMO_TEXT_DRAW_HEALTH_FOREGROUND);
	}
}

static MMO_UpdateNPCManaDisplay(npcid) {
	if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND] != Text:INVALID_TEXT_DRAW || MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE] != Text:INVALID_TEXT_DRAW || MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE] != Text:INVALID_TEXT_DRAW) {
		// Update the mana of the NPC
		if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND] != Text:INVALID_TEXT_DRAW) {
			TextDrawTextSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND], (470.0 - 167.3) * MMO_NPCs[npcid][MMO_NPC_MANA] / MMO_NPCs[npcid][MMO_NPC_MAX_MANA] + 167.3, 0.0);
		}

		// Update the mana percentage of the NPC
		if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE] != Text:INVALID_TEXT_DRAW) {
			// Ceil so 0.3% still displays as 1% instead of 0%
			new outputPercentage[4 + 1];
			format(outputPercentage, sizeof(outputPercentage), "%d%%", floatround(MMO_NPCs[npcid][MMO_NPC_MANA] / MMO_NPCs[npcid][MMO_NPC_MAX_MANA] * 100, floatround_ceil));
			TextDrawSetString(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE], outputPercentage);
		}

		// Update the mana value of the NPC
		if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE] != Text:INVALID_TEXT_DRAW) {
			// Ceil so 0.3 still displays as 1 instead of 0
			new manaInteger = floatround(MMO_NPCs[npcid][MMO_NPC_MANA], floatround_ceil);
			new outputValue[14 + 1];
			if(float(manaInteger) / 1000 / 1000 / 1000 >= 1) {
				format(outputValue, sizeof(outputValue), "%.2fB", float(manaInteger) / 1000 / 1000 / 1000);
			} else if(float(manaInteger) / 1000 / 1000 >= 1) {
				format(outputValue, sizeof(outputValue), "%.2fM", float(manaInteger) / 1000 / 1000);
			} else if(float(manaInteger) / 1000 >= 1) {
				format(outputValue, sizeof(outputValue), "%.2fK", float(manaInteger) / 1000);
			} else {
				format(outputValue, sizeof(outputValue), "%d", manaInteger);
			}
			TextDrawSetString(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE], outputValue);
		}
	}

	// Reshow the textdraw to make the changes (size) visible
	if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND] != Text:INVALID_TEXT_DRAW) {
		MMO_ShowTextDraw(MMO_TEXT_DRAW_MANA_FOREGROUND);
	}
}

static bool:MMO_IsPlayerInDisplayRange(playerid, npcid) {
	new Float:x, Float:y, Float:z;
	if(!IsPlayerNPC(playerid)) {
		GetPlayerPos(playerid, x, y, z);
	} else {
		FCNPC_GetPosition(playerid, x, y, z);
	}
	new Float:npcRange = GetPlayerDistanceFromPoint(npcid, x, y, z);
	if(MMO_NPCs[npcid][MMO_NPC_DISPLAY_RANGE][playerid] < 0.0 || npcRange <= MMO_NPCs[npcid][MMO_NPC_DISPLAY_RANGE][playerid]) {
		return true;
	}
	return false;
}

static MMO_ClosestNPCInDisplayRange(playerid) {
	new closestNPC = INVALID_PLAYER_ID;
	new Float:closestNPCRange = 0.0;
	for(new npcid = 0; npcid < MAX_PLAYERS; npcid++) {
		// Don't check for death. Otherwise the health textdraw will not be updated when the NPC dies
		if(FAI_IsValidNPCForPlayer(npcid, playerid)) {
			new Float:x, Float:y, Float:z;
			FCNPC_GetPosition(npcid, x, y, z);
			new Float:playerRange = GetPlayerDistanceFromPoint(playerid, x, y, z);
			if(MMO_IsPlayerInDisplayRange(playerid, npcid) && (closestNPC == INVALID_PLAYER_ID || playerRange < closestNPCRange)) {
				closestNPCRange = playerRange;
				closestNPC = npcid;
			}
		}
	}
	return closestNPC;
}

static MMO_ShowTextDraw(MMO_E_TEXT_DRAW:textdraw) {
	for(new playerid = 0, highestPlayerid = GetPlayerPoolSize(); playerid <= highestPlayerid; playerid++) {
		MMO_ShowTextDrawForPlayer(playerid, textdraw);
	}
}

static MMO_ShowAllTextDrawsForPlayer(playerid) {
	if(FAI_IsValidNPC(MMO_ClosestNPC[playerid])) {
		for(new textdraw = 0; MMO_E_TEXT_DRAW:textdraw < MMO_E_TEXT_DRAW; textdraw++) {
			if(MMO_NPCs[MMO_ClosestNPC[playerid]][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw] != Text:INVALID_TEXT_DRAW) {
				MMO_ShowTextDrawForPlayer(playerid, MMO_E_TEXT_DRAW:textdraw);
			}
		}
	}
}

static MMO_ShowTextDrawForPlayer(playerid, MMO_E_TEXT_DRAW:textdraw) {
	// We don't need to display things to an NPC
	if(IsPlayerConnected(playerid) && !IsPlayerNPC(playerid)) {
		// Only show the textdraw for a player of the closest NPC with a valid display range
		if(FAI_IsValidNPC(MMO_ClosestNPC[playerid])) {
			TextDrawShowForPlayer(playerid, MMO_NPCs[MMO_ClosestNPC[playerid]][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw]);
		}
	}
}

static MMO_HideAllTextDrawsForPlayer(playerid) {
	if(FAI_IsValidNPC(MMO_ClosestNPC[playerid])) {
		for(new textdraw = 0; MMO_E_TEXT_DRAW:textdraw < MMO_E_TEXT_DRAW; textdraw++) {
			if(MMO_NPCs[MMO_ClosestNPC[playerid]][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw] != Text:INVALID_TEXT_DRAW) {
				TextDrawHideForPlayer(playerid, MMO_NPCs[MMO_ClosestNPC[playerid]][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw]);
			}
		}

		// Players
		MMO_ClosestNPC[playerid] = INVALID_PLAYER_ID;
	}
}

forward MMO_DelayNPCHealthDisplay(npcid);
public MMO_DelayNPCHealthDisplay(npcid) {
	MMO_UpdateNPCHealthDisplay(npcid);
	KillTimer(MMO_NPCs[npcid][MMO_NPC_DELAY_HEALTH_DISPLAY]);
	MMO_NPCs[npcid][MMO_NPC_DELAY_HEALTH_DISPLAY] = INVALID_TIMER_ID;
}

// Spells
static MMO_UpdateNPCSpellDisplay(npcid) {
	// TODO
	#pragma unused npcid
}

// Debug
static MMO_ToggleDebugRangeObjects(npcid) {
	new Float:iScale = 1.0, Float:radius = 50.0; // Radius of modelid is 50.0
	if(MMO_DebugRanges[npcid] & FAI_DEBUG_DISPLAY_RANGE && MMO_NPCs[npcid][MMO_NPC_DISPLAY_RANGE][MMO_DebugRangesPlayerID[npcid]] > 0.0) { // Blue
		new Float:scale = iScale / radius * MMO_NPCs[npcid][MMO_NPC_DISPLAY_RANGE][MMO_DebugRangesPlayerID[npcid]];
		SetPlayerAttachedObject(npcid, 4, 18843, 1, .fRotY = 90.0, .fScaleX = scale, .fScaleY = scale, .fScaleZ = scale, .materialcolor1 = 0xFF0000FF);
	} else {
		RemovePlayerAttachedObject(npcid, 4);
	}
}

// ========================================HOOKED CALLBACKS========================================
#if defined FILTERSCRIPT
	public OnFilterScriptInit()
	{
		MMO_ScriptInit();

		new ret = 1;
		#if defined MMO_OnFilterScriptInit
			ret = MMO_OnFilterScriptInit();
		#endif
		return ret;
	}
	#if defined _ALS_OnFilterScriptInit
		#undef OnFilterScriptInit
	#else
		#define _ALS_OnFilterScriptInit
	#endif
	#define OnFilterScriptInit MMO_OnFilterScriptInit
	#if defined MMO_OnFilterScriptInit
		forward MMO_OnFilterScriptInit();
	#endif

	public OnFilterScriptExit()
	{
		new ret = 1;
		#if defined MMO_OnFilterScriptExit
			ret = MMO_OnFilterScriptExit();
		#endif

		MMO_ScriptExit(); // Call after
		return ret;
	}
	#if defined _ALS_OnFilterScriptExit
		#undef OnFilterScriptExit
	#else
		#define _ALS_OnFilterScriptExit
	#endif
	#define OnFilterScriptExit MMO_OnFilterScriptExit
	#if defined MMO_OnFilterScriptExit
		forward MMO_OnFilterScriptExit();
	#endif
#else
	public OnGameModeInit()
	{
		MMO_ScriptInit();

		new ret = 1;
		#if defined MMO_OnGameModeInit
			ret = MMO_OnGameModeInit();
		#endif
		return ret;
	}
	#if defined _ALS_OnGameModeInit
		#undef OnGameModeInit
	#else
		#define _ALS_OnGameModeInit
	#endif
	#define OnGameModeInit MMO_OnGameModeInit
	#if defined MMO_OnGameModeInit
		forward MMO_OnGameModeInit();
	#endif

	public OnGameModeExit()
	{
		new ret = 1;
		#if defined MMO_OnGameModeExit
			ret = MMO_OnGameModeExit();
		#endif

		MMO_ScriptExit(); // Call after
		return ret;
	}
	#if defined _ALS_OnGameModeExit
		#undef OnGameModeExit
	#else
		#define _ALS_OnGameModeExit
	#endif
	#define OnGameModeExit MMO_OnGameModeExit
	#if defined MMO_OnGameModeExit
		forward MMO_OnGameModeExit();
	#endif
#endif

public OnPlayerConnect(playerid) {
	// Players
	MMO_ClosestNPC[playerid] = INVALID_PLAYER_ID;

	new ret = 1;
	#if defined MMO_OnPlayerConnect
		ret = MMO_OnPlayerConnect(playerid);
	#endif
	return ret;
}
#if defined _ALS_OnPlayerConnect
	#undef OnPlayerConnect
#else
	#define _ALS_OnPlayerConnect
#endif
#define OnPlayerConnect MMO_OnPlayerConnect
#if defined MMO_OnPlayerConnect
	forward MMO_OnPlayerConnect(playerid);
#endif

public OnPlayerDisconnect(playerid, reason) {
	// Players
	MMO_ClosestNPC[playerid] = INVALID_PLAYER_ID;

	new ret = 1;
	#if defined MMO_OnPlayerDisconnect
		ret = MMO_OnPlayerDisconnect(playerid, reason);
	#endif
	return ret;
}
#if defined _ALS_OnPlayerDisconnect
	#undef OnPlayerDisconnect
#else
	#define _ALS_OnPlayerDisconnect
#endif
#define OnPlayerDisconnect MMO_OnPlayerDisconnect
#if defined MMO_OnPlayerDisconnect
	forward MMO_OnPlayerDisconnect(playerid, reason);
#endif

public OnPlayerRequestClass(playerid, classid)
{
	// NPCs
	// ForceClassSelection doesn't call a state change and thus PLAYER_STATE_WASTED isn't set and thus OnPlayerStateChange or OnPlayerDeath aren't called and thus we need to handle the textdraw hiding here
	MMO_HideAllTextDrawsForPlayer(playerid);

	new ret = 1;
	#if defined MMO_OnPlayerRequestClass
		ret = MMO_OnPlayerRequestClass(playerid, classid);
	#endif
	return ret;
}
#if defined _ALS_OnPlayerRequestClass
	#undef OnPlayerRequestClass
#else
	#define _ALS_OnPlayerRequestClass
#endif
#define OnPlayerRequestClass MMO_OnPlayerRequestClass
#if defined MMO_OnPlayerRequestClass
	forward MMO_OnPlayerRequestClass(playerid, classid);
#endif

public OnPlayerDeath(playerid, killerid, reason)
{
	// NPCs
	MMO_HideAllTextDrawsForPlayer(playerid);

	new ret = 1;
	#if defined MMO_OnPlayerDeath
		ret = MMO_OnPlayerDeath(playerid, killerid, reason);
	#endif
	return ret;
}
#if defined _ALS_OnPlayerDeath
	#undef OnPlayerDeath
#else
	#define _ALS_OnPlayerDeath
#endif
#define OnPlayerDeath MMO_OnPlayerDeath
#if defined MMO_OnPlayerDeath
	forward MMO_OnPlayerDeath(playerid, killerid, reason);
#endif

public OnPlayerUpdate(playerid)
{
	// Players // NPCs
	new closestNPC = MMO_ClosestNPCInDisplayRange(playerid);
	if(MMO_ClosestNPC[playerid] != closestNPC) {
		// Only show textdraws from closest NPC and hide textdraws from any other NPCs
		MMO_HideAllTextDrawsForPlayer(playerid);
		MMO_ClosestNPC[playerid] = closestNPC;
		MMO_ShowAllTextDrawsForPlayer(playerid);
	}

	new ret = 1;
	#if defined MMO_OnPlayerUpdate
		ret = MMO_OnPlayerUpdate(playerid);
	#endif
	return ret;
}
#if defined _ALS_OnPlayerUpdate
	#undef OnPlayerUpdate
#else
	#define _ALS_OnPlayerUpdate
#endif
#define OnPlayerUpdate MMO_OnPlayerUpdate
#if defined MMO_OnPlayerUpdate
	forward MMO_OnPlayerUpdate(playerid);
#endif

// This will both handle NPCs being destroyed programmatically and NPCs being disconnected (kicked/banned)
public FCNPC_OnDestroy(npcid)
{
	// NPCs
	if(FAI_IsValidNPC(npcid)) {
		for(new textdraw = 0; MMO_E_TEXT_DRAW:textdraw < MMO_E_TEXT_DRAW; textdraw++) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw] = Text:INVALID_TEXT_DRAW;
			}
		}
		if(MMO_NPCs[npcid][MMO_NPC_DELAY_HEALTH_DISPLAY] != INVALID_TIMER_ID) {
			KillTimer(MMO_NPCs[npcid][MMO_NPC_DELAY_HEALTH_DISPLAY]);
			MMO_NPCs[npcid][MMO_NPC_DELAY_HEALTH_DISPLAY] = INVALID_TIMER_ID;
		}
		MMO_ResetNPC(npcid);
	}

	new ret = 1;
	#if defined MMO_FCNPC_OnDestroy
		ret = MMO_FCNPC_OnDestroy(npcid);
	#endif
	return ret;
}
#if defined _ALS_FCNPC_OnDestroy
	#undef FCNPC_OnDestroy
#else
	#define _ALS_FCNPC_OnDestroy
#endif
#define FCNPC_OnDestroy MMO_FCNPC_OnDestroy
#if defined MMO_FCNPC_OnDestroy
	forward MMO_FCNPC_OnDestroy(npcid);
#endif

public FCNPC_OnRespawn(npcid)
{
	// NPCs
	if(FAI_IsValidNPC(npcid)) {
		MMO_UpdateNPCHealthDisplay(npcid);
		if(MMO_NPCs[npcid][MMO_NPC_IS_DEAD_BEFORE_RESPAWN]) {
			MMO_NPCs[npcid][MMO_NPC_IS_DEAD_BEFORE_RESPAWN] = false;
			MMO_NPCs[npcid][MMO_NPC_MANA] = MMO_NPCs[npcid][MMO_NPC_MAX_MANA];
			MMO_UpdateNPCManaDisplay(npcid);
		}

		// Debug
		MMO_ToggleDebugRangeObjects(npcid);
	}

	new ret = 1;
	#if defined MMO_FCNPC_OnRespawn
		ret = MMO_FCNPC_OnRespawn(npcid);
	#endif
	return ret;
}
#if defined _ALS_FCNPC_OnRespawn
	#undef FCNPC_OnRespawn
#else
	#define _ALS_FCNPC_OnRespawn
#endif
#define FCNPC_OnRespawn MMO_FCNPC_OnRespawn
#if defined MMO_FCNPC_OnRespawn
	forward MMO_FCNPC_OnRespawn(npcid);
#endif

public FCNPC_OnDeath(npcid, killerid, reason)
{
	// NPCs
	if(FAI_IsValidNPC(npcid)) {
		MMO_UpdateNPCHealthDisplay(npcid);
		MMO_NPCs[npcid][MMO_NPC_IS_DEAD_BEFORE_RESPAWN] = true;
		MMO_NPCs[npcid][MMO_NPC_MANA] = 0.0;
		MMO_UpdateNPCManaDisplay(npcid);
	}

	new ret = 1;
	#if defined MMO_FCNPC_OnDeath
		ret = MMO_FCNPC_OnDeath(npcid, killerid, reason);
	#endif
	return ret;
}
#if defined _ALS_FCNPC_OnDeath
	#undef FCNPC_OnDeath
#else
	#define _ALS_FCNPC_OnDeath
#endif
#define FCNPC_OnDeath MMO_FCNPC_OnDeath
#if defined MMO_FCNPC_OnDeath
	forward MMO_FCNPC_OnDeath(npcid, killerid, reason);
#endif

public FCNPC_OnTakeDamage(npcid, issuerid, Float:amount, weaponid, bodypart)
{
	new ret = 1;
	#if defined MMO_FCNPC_OnTakeDamage
		// The user can override the decision for the damage to be afflicted to the NPC
		ret = MMO_FCNPC_OnTakeDamage(npcid, issuerid, amount, weaponid, bodypart);
	#endif

	if(ret == 1) { // Call after
		// NPCs
		if(FAI_IsValidNPC(npcid)) {
			// Use a 0ms timer to allow the rest of the hooking chain to complete first, so the health value is first updated by the FAI hook and only then displayed by the MMO hook
			MMO_NPCs[npcid][MMO_NPC_DELAY_HEALTH_DISPLAY] = SetTimerEx("MMO_DelayNPCHealthDisplay", 0, false, "i", npcid);
		}
	}
	return ret;
}
#if defined _ALS_FCNPC_OnTakeDamage
	#undef FCNPC_OnTakeDamage
#else
	#define _ALS_FCNPC_OnTakeDamage
#endif
#define FCNPC_OnTakeDamage MMO_FCNPC_OnTakeDamage
#if defined MMO_FCNPC_OnTakeDamage
	forward MMO_FCNPC_OnTakeDamage(npcid, issuerid, Float:amount, weaponid, bodypart);
#endif

// ========================================HOOKED FUNCTIONS========================================
// NPCs
// Using function hook instead of callback hook, so that this script only handles NPCs created in this FS/GM
// The create callback gets called before the FAI function hook is finished, so a function hook is needed again
stock MMO_FCNPC_Create(const name[]) {
	new npcid = FCNPC_Create(name);
	if(npcid != INVALID_PLAYER_ID) {
		// In case something went wrong (gmx, ...)
		for(new textdraw = 0; MMO_E_TEXT_DRAW:textdraw < MMO_E_TEXT_DRAW; textdraw++) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_E_TEXT_DRAW:textdraw] = Text:INVALID_TEXT_DRAW;
			}
		}

		// Default to player name
		strcpy(MMO_NPCs[npcid][MMO_NPC_NAME], name, MMO_MAX_NPC_NAME + 1);
	}
	return npcid;
}
#if defined _ALS_FCNPC_Create
	#undef FCNPC_Create
#else
	#define _ALS_FCNPC_Create
#endif
#define FCNPC_Create MMO_FCNPC_Create

stock MMO_FCNPC_SetHealth(npcid, Float:health, Float:max = -1.0) {
	// Update the health first, then update the display
	new ret = FCNPC_SetHealth(npcid, health, max);
	if(FAI_IsValidNPC(npcid)) {
		MMO_UpdateNPCHealthDisplay(npcid);
	}
	return ret;
}
#if defined _ALS_FCNPC_SetHealth
	#undef FCNPC_SetHealth
#else
	#define _ALS_FCNPC_SetHealth
#endif
#define FCNPC_SetHealth MMO_FCNPC_SetHealth

// Debug
stock MMO_FAI_ToggleRangeDebugging(npcid, playerid, FAI_E_DEBUG_RANGE:ranges) {
	new ret = FAI_ToggleRangeDebugging(npcid, playerid, ranges);
	if(FAI_IsValidNPC(npcid) && playerid >= 0 && playerid < MAX_PLAYERS) {
		MMO_DebugRanges[npcid] = ranges;
		MMO_DebugRangesPlayerID[npcid] = playerid;
		MMO_ToggleDebugRangeObjects(npcid);
	}
	return ret;
}
#if defined _ALS_FAI_ToggleRangeDebugging
	#undef FAI_ToggleRangeDebugging
#else
	#define _ALS_FAI_ToggleRangeDebugging
#endif
#define FAI_ToggleRangeDebugging MMO_FAI_ToggleRangeDebugging

stock MMO_FAI_ToggleAngleDebugging(npcid, playerid, FAI_E_DEBUG_ANGLE:angles) {
	if(FAI_IsValidNPC(npcid) && playerid >= 0 && playerid < MAX_PLAYERS) {
		// Not enough attached object slots to show both (sound) ranges and angles
		MMO_DebugRanges[npcid] = FAI_DEBUG_RANGE_NONE;
		MMO_ToggleDebugRangeObjects(npcid);
	}

	return FAI_ToggleAngleDebugging(npcid, playerid, angles);
}
#if defined _ALS_FAI_ToggleAngleDebugging
	#undef FAI_ToggleAngleDebugging
#else
	#define _ALS_FAI_ToggleAngleDebugging
#endif
#define FAI_ToggleAngleDebugging MMO_FAI_ToggleAngleDebugging

stock MMO_FAI_ToggleSndRngDebugging(npcid, playerid, FAI_E_DEBUG_SOUND_RANGE:ranges) {
	if(FAI_IsValidNPC(npcid) && playerid >= 0 && playerid < MAX_PLAYERS) {
		// Not enough attached object slots to show both (sound) ranges and angles
		MMO_DebugRanges[npcid] = FAI_DEBUG_RANGE_NONE;
		MMO_ToggleDebugRangeObjects(npcid);
	}

	return FAI_ToggleSoundRangeDebugging(npcid, playerid, ranges);
}
#if defined _ALS_FAI_ToggleSndRngDebugging
	#undef FAI_ToggleSoundRangeDebugging
#else
	#define _ALS_FAI_ToggleSndRngDebugging
#endif
#define FAI_ToggleSoundRangeDebugging MMO_FAI_ToggleSndRngDebugging

// ========================================FUNCTIONS========================================
// NPCs
stock MMO_ToggleHUDElements(npcid, MMO_E_HUD:elements) {
	if(FAI_IsValidNPC(npcid)) {
		// SP proportions: new Float:border = 2.0, Float:vborder = 2.2, Float:barvdiff = 10.95;
		new Float:border = 1.0, Float:vborder = border;
		new Float:width = 300.0, Float:topLeftX = 320.0 - width/2 - border, Float:topLeftY = 405.0, Float:namevdiff = 14.0, Float:barvdiff = 10.0;

		MMO_NPCs[npcid][MMO_NPC_HUD] = elements;

		// If the mana bar and its components are not used, position the spell bar just below the health bar instead of leaving a gap where the mana bar is normally positioned
		if(!(MMO_NPCs[npcid][MMO_NPC_HUD] & (MMO_HUD_MANA_BAR | MMO_HUD_MANA_PERCENTAGE | MMO_HUD_MANA_VALUE))) {
			barvdiff /= 2.0;
		}

		if(MMO_NPCs[npcid][MMO_NPC_HUD] & MMO_HUD_NPC_NAME) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME] = TextDrawCreate(topLeftX, topLeftY, "_");
				TextDrawFont(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME], 0);
				TextDrawAlignment(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME], 1);
				TextDrawSetProportional(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME], 1);
				TextDrawColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME], 0xffffffff);
				TextDrawSetShadow(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME], 0);
				TextDrawSetOutline(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME], 1);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME], 0.3, 1.0);
				MMO_UpdateNPCNameDisplay(npcid);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_NPC_NAME);
			}
		} else {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_NPC_NAME] = Text:INVALID_TEXT_DRAW;
			}
		}
		if(MMO_NPCs[npcid][MMO_NPC_HUD] & MMO_HUD_HEALTH_BAR) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BORDER] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BORDER] = TextDrawCreate(topLeftX, topLeftY + namevdiff, "_");
				TextDrawUseBox(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BORDER], 1);
				TextDrawBoxColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BORDER], 0x000000ff);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BORDER], 0.0, 0.7); // SP proportions: 0.0, 0.6
				TextDrawTextSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BORDER], topLeftX + width + border*2, 0.0);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_HEALTH_BORDER);
			}
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BACKGROUND] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BACKGROUND] = TextDrawCreate(topLeftX + border, topLeftY + namevdiff + vborder, "_");
				TextDrawUseBox(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BACKGROUND], 1);
				TextDrawBoxColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BACKGROUND], 0x5a0c0eff);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BACKGROUND], 0.0, 0.5); // SP proportions: 0.0, 0.15
				TextDrawTextSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BACKGROUND], topLeftX + width + border, 0.0);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_HEALTH_BACKGROUND);
			}
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND] = TextDrawCreate(topLeftX + border, topLeftY + namevdiff + vborder, "_");
				TextDrawUseBox(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND], 1);
				TextDrawBoxColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND], 0xb4191dff);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND], 0.0, 0.5); // SP proportions: 0.0, 0.15
				TextDrawTextSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND], topLeftX + width + border, 0.0);
				MMO_UpdateNPCHealthDisplay(npcid); // Due to the nature of resizing the textdraw, it also shows the textdraw
			}
		} else {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BORDER] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BORDER]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BORDER] = Text:INVALID_TEXT_DRAW;
			}
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BACKGROUND] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BACKGROUND]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_BACKGROUND] = Text:INVALID_TEXT_DRAW;
			}
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_FOREGROUND] = Text:INVALID_TEXT_DRAW;
			}
		}
		if(MMO_NPCs[npcid][MMO_NPC_HUD] & MMO_HUD_HEALTH_PERCENTAGE) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE] = TextDrawCreate(topLeftX + border, topLeftY + namevdiff, "_");
				TextDrawFont(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE], 1);
				TextDrawAlignment(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE], 1);
				TextDrawSetProportional(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE], 1);
				TextDrawColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE], 0xffffffff);
				TextDrawSetShadow(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE], 0);
				TextDrawSetOutline(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE], 1);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE], 0.2, 0.7);
				MMO_UpdateNPCHealthDisplay(npcid);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_HEALTH_PERCENTAGE);
			}
		} else {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_PERCENTAGE] = Text:INVALID_TEXT_DRAW;
			}
		}
		if(MMO_NPCs[npcid][MMO_NPC_HUD] & MMO_HUD_HEALTH_VALUE) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE] = TextDrawCreate(topLeftX + width + border, topLeftY + namevdiff, "_");
				TextDrawFont(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE], 1);
				TextDrawAlignment(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE], 3);
				TextDrawSetProportional(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE], 1);
				TextDrawColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE], 0xffffffff);
				TextDrawSetShadow(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE], 0);
				TextDrawSetOutline(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE], 1);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE], 0.2, 0.7);
				MMO_UpdateNPCHealthDisplay(npcid);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_HEALTH_VALUE);
			}
		} else {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_HEALTH_VALUE] = Text:INVALID_TEXT_DRAW;
			}
		}
		if(MMO_NPCs[npcid][MMO_NPC_HUD] & MMO_HUD_MANA_BAR) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BORDER] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BORDER] = TextDrawCreate(topLeftX, topLeftY + namevdiff + barvdiff, "_");
				TextDrawUseBox(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BORDER], 1);
				TextDrawBoxColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BORDER], 0x000000ff);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BORDER], 0.0, 0.7); // SP proportions: 0.0, 0.6
				TextDrawTextSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BORDER], topLeftX + width + border*2, 0.0);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_MANA_BORDER);
			}
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BACKGROUND] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BACKGROUND] = TextDrawCreate(topLeftX + border, topLeftY + namevdiff + barvdiff + vborder, "_");
				TextDrawUseBox(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BACKGROUND], 1);
				TextDrawBoxColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BACKGROUND], 0x0e0c5aff);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BACKGROUND], 0.0, 0.5); // SP proportions: 0.0, 0.15
				TextDrawTextSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BACKGROUND], topLeftX + width + border, 0.0);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_MANA_BACKGROUND);
			}
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND] = TextDrawCreate(topLeftX + border, topLeftY + namevdiff + barvdiff + vborder, "_");
				TextDrawUseBox(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND], 1);
				TextDrawBoxColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND], 0x1d19b4ff);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND], 0.0, 0.5); // SP proportions: 0.0, 0.15
				TextDrawTextSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND], topLeftX + width + border, 0.0);
				MMO_UpdateNPCManaDisplay(npcid); // Due to the nature of resizing the textdraw, it also shows the textdraw
			}
		} else {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BORDER] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BORDER]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BORDER] = Text:INVALID_TEXT_DRAW;
			}
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BACKGROUND] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BACKGROUND]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_BACKGROUND] = Text:INVALID_TEXT_DRAW;
			}
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_FOREGROUND] = Text:INVALID_TEXT_DRAW;
			}
		}
		if(MMO_NPCs[npcid][MMO_NPC_HUD] & MMO_HUD_MANA_PERCENTAGE) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE] = TextDrawCreate(topLeftX + border, topLeftY + namevdiff + barvdiff, "_");
				TextDrawFont(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE], 1);
				TextDrawAlignment(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE], 1);
				TextDrawSetProportional(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE], 1);
				TextDrawColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE], 0xffffffff);
				TextDrawSetShadow(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE], 0);
				TextDrawSetOutline(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE], 1);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE], 0.2, 0.7);
				MMO_UpdateNPCManaDisplay(npcid);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_MANA_PERCENTAGE);
			}
		} else {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_PERCENTAGE] = Text:INVALID_TEXT_DRAW;
			}
		}
		if(MMO_NPCs[npcid][MMO_NPC_HUD] & MMO_HUD_MANA_VALUE) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE] = TextDrawCreate(topLeftX + width + border, topLeftY + namevdiff + barvdiff, "_");
				TextDrawFont(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE], 1);
				TextDrawAlignment(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE], 3);
				TextDrawSetProportional(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE], 1);
				TextDrawColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE], 0xffffffff);
				TextDrawSetShadow(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE], 0);
				TextDrawSetOutline(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE], 1);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE], 0.2, 0.7);
				MMO_UpdateNPCManaDisplay(npcid);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_MANA_VALUE);
			}
		} else {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_MANA_VALUE] = Text:INVALID_TEXT_DRAW;
			}
		}
		if(MMO_NPCs[npcid][MMO_NPC_HUD] & MMO_HUD_SPELL_BAR) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BORDER] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BORDER] = TextDrawCreate(topLeftX, topLeftY + namevdiff + barvdiff*2, "_");
				TextDrawUseBox(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BORDER], 1);
				TextDrawBoxColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BORDER], 0x000000ff);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BORDER], 0.0, 0.7); // SP proportions: 0.0, 0.6
				TextDrawTextSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BORDER], topLeftX + width + border*2, 0.0);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_SPELL_BORDER);
			}
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BACKGROUND] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BACKGROUND] = TextDrawCreate(topLeftX + border, topLeftY + namevdiff + barvdiff*2 + vborder, "_");
				TextDrawUseBox(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BACKGROUND], 1);
				TextDrawBoxColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BACKGROUND], 0x645005ff);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BACKGROUND], 0.0, 0.5); // SP proportions: 0.0, 0.15
				TextDrawTextSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BACKGROUND], topLeftX + width + border, 0.0);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_SPELL_BACKGROUND);
			}
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_FOREGROUND] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_FOREGROUND] = TextDrawCreate(topLeftX + border, topLeftY + namevdiff + barvdiff*2 + vborder, "_");
				TextDrawUseBox(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_FOREGROUND], 1);
				TextDrawBoxColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_FOREGROUND], 0xb4820aff);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_FOREGROUND], 0.0, 0.5); // SP proportions: 0.0, 0.15
				TextDrawTextSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_FOREGROUND], topLeftX + width + border, 0.0);
				MMO_UpdateNPCSpellDisplay(npcid); // Due to the nature of resizing the textdraw, it also shows the textdraw
			}
		} else {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BORDER] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BORDER]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BORDER] = Text:INVALID_TEXT_DRAW;
			}
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BACKGROUND] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BACKGROUND]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_BACKGROUND] = Text:INVALID_TEXT_DRAW;
			}
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_FOREGROUND] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_FOREGROUND]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_FOREGROUND] = Text:INVALID_TEXT_DRAW;
			}
		}
		if(MMO_NPCs[npcid][MMO_NPC_HUD] & MMO_HUD_SPELL_NAME) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_NAME] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_NAME] = TextDrawCreate(topLeftX + border, topLeftY + namevdiff + barvdiff*2, "_");
				TextDrawFont(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_NAME], 1);
				TextDrawAlignment(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_NAME], 1);
				TextDrawSetProportional(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_NAME], 1);
				TextDrawColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_NAME], 0xffffffff);
				TextDrawSetShadow(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_NAME], 0);
				TextDrawSetOutline(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_NAME], 1);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_NAME], 0.2, 0.7);
				MMO_UpdateNPCSpellDisplay(npcid);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_SPELL_NAME);
			}
		} else {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_NAME] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_NAME]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_NAME] = Text:INVALID_TEXT_DRAW;
			}
		}
		if(MMO_NPCs[npcid][MMO_NPC_HUD] & MMO_HUD_SPELL_TIME) {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_TIME] == Text:INVALID_TEXT_DRAW) {
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_TIME] = TextDrawCreate(topLeftX + width + border, topLeftY + namevdiff + barvdiff*2, "_");
				TextDrawFont(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_TIME], 1);
				TextDrawAlignment(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_TIME], 3);
				TextDrawSetProportional(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_TIME], 1);
				TextDrawColor(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_TIME], 0xffffffff);
				TextDrawSetShadow(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_TIME], 0);
				TextDrawSetOutline(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_TIME], 1);
				TextDrawLetterSize(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_TIME], 0.2, 0.7);
				MMO_UpdateNPCSpellDisplay(npcid);
				MMO_ShowTextDraw(MMO_TEXT_DRAW_SPELL_TIME);
			}
		} else {
			if(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_TIME] != Text:INVALID_TEXT_DRAW) {
				TextDrawDestroy(MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_TIME]);
				MMO_NPCs[npcid][MMO_NPC_TEXT_DRAWS][MMO_TEXT_DRAW_SPELL_TIME] = Text:INVALID_TEXT_DRAW;
			}
		}
		return 1;
	}
	return 0;
}

stock MMO_E_HUD:MMO_GetHUDElements(npcid) {
	if(FAI_IsValidNPC(npcid)) {
		return MMO_NPCs[npcid][MMO_NPC_HUD];
	}
	return MMO_HUD_NONE;
}

stock MMO_SetName(npcid, const name[]) {
	if(FAI_IsValidNPC(npcid)) {
		strcpy(MMO_NPCs[npcid][MMO_NPC_NAME], name, MMO_MAX_NPC_NAME + 1);
		MMO_UpdateNPCNameDisplay(npcid);
		return 1;
	}
	return 0;
}

stock MMO_GetName(npcid, name[], len) {
	if(FAI_IsValidNPC(npcid)) {
		strcpy(name, MMO_NPCs[npcid][MMO_NPC_NAME], len);
		return strlen(name);
	}
	return 0;
}

stock MMO_SetMana(npcid, Float:mana, Float:max = -1.0) {
	if(FAI_IsValidNPC(npcid)) {
		if(mana < 0.0) {
			mana = 0.0;
		}

		// Keep previous max mana when using a negative value
		if(max < 0.0) {
			max = MMO_NPCs[npcid][MMO_NPC_MAX_MANA];
		}
		
		if(mana > max) {
			max = mana;
		}
		MMO_NPCs[npcid][MMO_NPC_MANA] = mana;
		MMO_NPCs[npcid][MMO_NPC_MAX_MANA] = max;
		MMO_UpdateNPCManaDisplay(npcid);
		return 1;
	}
	return 0;
}

stock Float:MMO_GetMana(npcid, &Float:max = 100.0) {
	if(FAI_IsValidNPC(npcid)) {
		max = MMO_NPCs[npcid][MMO_NPC_MAX_MANA];
		return MMO_NPCs[npcid][MMO_NPC_MANA];
	}
	return 0.0;
}

stock MMO_SetDisplayRange(npcid, Float:range, playerid = -1) {
	if(FAI_IsValidNPC(npcid) && playerid >= -1 && playerid < MAX_PLAYERS) {
		if(playerid != -1) {
			MMO_NPCs[npcid][MMO_NPC_DISPLAY_RANGE][playerid] = range; // Can be negative
			MMO_ShowAllTextDrawsForPlayer(playerid);
		} else {
			for(new playerid1 = 0; playerid1 < MAX_PLAYERS; playerid1++) {
				MMO_NPCs[npcid][MMO_NPC_DISPLAY_RANGE][playerid1] = range; // Can be negative
				MMO_ShowAllTextDrawsForPlayer(playerid1);
			}
		}

		// Debug
		MMO_ToggleDebugRangeObjects(npcid);
		return 1;
	}
	return 0;
}

stock Float:MMO_GetDisplayRange(npcid, playerid) {
	if(FAI_IsValidNPC(npcid) && playerid >= 0 && playerid < MAX_PLAYERS) {
		return MMO_NPCs[npcid][MMO_NPC_DISPLAY_RANGE][playerid];
	}
	return 0.0;
}