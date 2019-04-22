/* ======== Basic Admin tool ========
* Copyright (C) 2004-2007 Erling K. Sæterdal
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Erling K. Sæterdal ( EKS )
* Credits:
*	Menu code based on code from CSDM ( http://www.tcwonline.org/~dvander/cssdm ) Created by BAILOPAN
*	Adminloading function from Xaphan ( http://www.sourcemod.net/forums/viewtopic.php?p=25807 ) Created by Devicenull
* Helping on misc errors/functions: BAILOPAN,sslice,devicenull,PMOnoTo,cybermind ( most who idle in #sourcemod on GameSurge realy )
* ============================ */

#ifndef _INCLUDE_SAMPLEPLUGIN_H
#define _INCLUDE_SAMPLEPLUGIN_H

#include <ISmmPlugin.h>
#include <sourcehook/sourcehook.h>
#include "hl2sdk/BATInterface.h"
#include <igameevents.h>
#include "ienginesound.h"
#include <iplayerinfo.h>
#include "convar.h"
#include "cvars.h"
#include "const.h"
#include <sh_vector.h>


//SH_DECL_HOOK0_void(ConCommand, Dispatch, SH_NOATTRIB, 0);

class ForgiveTK : 
	public ISmmPlugin, 
	public IMetamodListener, 
	public IConCommandBaseAccessor,
	public AdminInterfaceListner //  public IGameEventListener2
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	void AllPluginsLoaded();
	bool Pause(char *error, size_t maxlen)
	{
		return true;
	}
	bool Unpause(char *error, size_t maxlen)
	{
		return true;
	}
public:
	int GetApiVersion() { return METAMOD_PLAPI_VERSION; }
public:
	const char *GetAuthor()
	{
		return "EKS";
	}
	const char *GetName()
	{
		return "Forgive TK";
	}
	const char *GetDescription()
	{
		return "A basic Forgive TK plugin, forgive either via menu or chat";
	}
	const char *GetURL()
	{
		return "http://www.sourcemm.net/";
	}
	const char *GetLicense()
	{
		return "zlib/libpng";
	}
	const char *GetVersion()
	{
		return FTK_VERSION;
	}
	const char *GetDate()
	{
		return __DATE__;
	}
	const char *GetLogTag()
	{
		return "ForgiveTK";
	}
public:
	//These functions are from IServerPluginCallbacks
	//Note, the parameters might be a little different to match the actual calls!


	//Called on ServerActivate.  Same definition as server plugins
	void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
	//Called when a client uses a command.  Unlike server plugins, it's void.
	// You can still supercede the gamedll through RETURN_META(MRES_SUPERCEDE).
	//Called on a game tick.  Same definition as server plugins
	void GameFrame(bool simulating);
	//Client disconnects - same as server plugins
	void ClientDisconnect(edict_t *pEntity);
	bool ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
	//Client is put in server - same as server plugins
	void ClientPutInServer(edict_t *pEntity, char const *playername);
	//Sets the client index - same as server plugins
	void SetCommandClient(int index);

#if BAT_ORANGEBOX == 1
	void ClientCommand(edict_t *pEntity, const CCommand &args);
	void ClientSay( const CCommand &command ); // This is where we get have the client say hooked.	
#else
	void ClientCommand(edict_t *pEntity);
	void ClientSay();
#endif

	void LevelShutdown();
	virtual bool RegisterConCommandBase(ConCommandBase *pVar);
	
	IVEngineServer *GetEngine() { return m_Engine; }
	IPlayerInfoManager *PlayerInfo() { return m_InfoMngr; }
	IServerPluginHelpers *GetHelpers() {return m_Helpers;}
	IServerGameDLL *GetServerGameDll(){ return m_ServerDll; }

	FTKVars  GetFTKVar() { return g_FTKVars; }
	bool HookConCommands(bool ReleaseHook);
	bool HookCvars();
	bool IsFriendlyFireOn();

	void MessagePlayer(int index, const char *msg, ...);
	void SendCSay(int id,const char *msg, ...);
	void ServerCommand(const char *fmt, ...);
	void LogPrint( const char *msg, ...);
	void RegisterTeamKill(int id);
	void RegisterTeamAttack(int id);
	void ForgivePlayerTK(int id);
	void ForgivePlayerTA(int id);

	bool IsInsideSpawnProtection(int attacker,int victim);
	bool BasicEventChecks(int attacker,int victim); // This funcito just make the code simpler to read. Instead of having a insaly long line inside the event code in both player death and damege we have a shared func for it
			
	int GetMenuMsgId();
	int GetUserTeam(int id);
	int FindPlayer(int UserID);
	bool IsUserConnected(int id);
	bool IsUserAlive(int id);
	bool HasAdminImmunity(int id);

	const char *GetPlayerName(int id);
	void RemoveTeamAttack(int id);
	
	void OnAdminInterfaceUnload();
	void Client_Authorized(int id);
	int str_replace(char *str, const char *from, const char *to, int maxlen); // Function from sslice 

private:
	IGameEventManager2 *m_GameEventManager;	
	IVEngineServer *m_Engine;
	IServerGameDLL *m_ServerDll;
	IServerGameClients *m_ServerClients;
	IPlayerInfoManager *m_InfoMngr;
	IServerPluginHelpers *m_Helpers;
	IEngineSound *m_Sound;
	ICvar *m_ICvar;
	SourceHook::CallClass<IVEngineServer> *m_Engine_CC;
	SourceHook::CVector<int> m_hooks;
	ConCommand *pSayCmd;
	ConCommand *pSayTeamCmd;
	ConVar *pFriendlyFire;

	void LoadPluginSettings(int clientMax);

	void CheckIfPlayerReconnected(int id);
	void SaveReconnectInfo(int id);

	ConCommand *HookConsoleCmd(const char *CmdName);
	ConVar * GetCvarPointer(const char *CvarName);
};
class EventListenPlayerHurt :  public IGameEventListener2
{
	public:
		void FireGameEvent(IGameEvent *event); // Only catches Player_Hurt event. Save the poor cpu the annoying strcmp
};
class EventListenPlayerSpawn :  public IGameEventListener2
{
public:
	void FireGameEvent(IGameEvent *event); // Only catches Player_Hurt event. Save the poor cpu the annoying strcmp
};
class EventListenPlayerDeath :  public IGameEventListener2
{
public:
	void FireGameEvent(IGameEvent *event); // Only catches Player_Hurt event. Save the poor cpu the annoying strcmp
};
extern ForgiveTK g_ForgiveTKCore;
extern FTKVars g_FTKVars;
extern ConstPlayerInfo g_UserInfo[MAXPLAYERS+1];
extern int g_MaxClients;
extern bool g_IsConnected[MAXPLAYERS+1];

PLUGIN_GLOBALVARS();

#if BAT_ORANGEBOX == 0
class CCommand
{
public:
	const char *ArgS()
	{
		return g_ForgiveTKCore.GetEngine()->Cmd_Args();
	}
	int ArgC()
	{
		return g_ForgiveTKCore.GetEngine()->Cmd_Argc();
	}

	const char *Arg(int index)
	{
		return g_ForgiveTKCore.GetEngine()->Cmd_Argv(index);
	}
};
extern ICvar *g_pCVar;
ICvar * GetICVar();
#endif
extern CCommand g_LastCCommand;
#endif //_INCLUDE_SAMPLEPLUGIN_H
