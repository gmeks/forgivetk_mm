/* ======== Basic Admin tool ========
* Copyright (C) 2004-2007 Erling K. Sæterdal
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Erling K. Sæterdal ( EKS )
* Credits:
*	Menu code based on code from CSDM ( http://www.tcwonline.org/~dvander/cssdm ) Created by BAILOPAN
*	Adminloading function from Xaphan ( http://www.sourcemod.net/forums/viewtopic.php?p=25807 ) Created by devicenull
* Helping on misc errors/functions: BAILOPAN,sslice,devicenull,PMOnoTo,cybermind ( most who idle in #sourcemod on GameSurge realy )
* ============================ */

#ifndef _INCLUDE_CONST_H
#define _INCLUDE_CONST_H

#include "edict.h"
#include "ModInfo.h"
#include "hl2sdk/strtools.h"

#define FTK_VERSION	"1.2.3"
#define FTK_DEBUG 0
#define BAT_ORANGEBOX 1

#define PluginCore g_ForgiveTKCore
#define PluginSpesficLogPrint LogPrint

#define RECONNECT_REMEMEBERCOUNT 5
#define TEAM_NOT_CONNECTED -1

#define MAXPLAYERS ABSOLUTE_PLAYER_LIMIT +1

// ---------- GameFrame()
#define TASK_CHECKTIME 1.0			// This is how often the plugin checks if any tasks should be executed

// SayMessage id
//#define MESSAGE_CLIENT	3

#define OPTION_DISABLED 0
#define OPTION_MENUBASED 1
#define OPTION_CHATBASED 2
#define OPTION_ONLYCHATRESPOND 3
#define OPTION_ESCMENUBASED 4

#define MENUSYSTEM_NOTWORKING -1

enum TKPunishOption
{
	PunishDisabled=0,
	PunishSlay=1,
	PunishSlap=2,
};
typedef struct 
{
	int MaxTK;
	int MaxTA;
	int Option;
	int PunishOption;
}TeamKillOptionsStruct;
typedef struct 
{
	char Steamid[MAX_NETWORKID_LENGTH+1];
	float DisconnectTime;
	int TKCount;
	int TACount;
	bool HasInformation;
}ConstReconnectInfo;
typedef struct 
{
	char Steamid[MAX_NETWORKID_LENGTH+1];
	int Userid;
	int TKCount;
	int TACount;
	int LastTAVictim;		// Contains the index of your last TA victim
	int LastTKIndex;		// Last person that killed the user
	int LastTAIndex;		// Last person that team attacked the user
	bool IsBot;				// is a hltv or a bot
	float LastAttackTime;	// The Last time the user attacked someone, so we dont get multiple TA/TK in 1 attack
	int SpawnTime;			// The time the player spawned at.
	edict_t *PlayerEdict;
}ConstPlayerInfo;
#endif //_INCLUDE_CONST_H