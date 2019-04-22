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
* ============================
	
	Reconnect looses TA TK count
*/
#define DEBUG_REVERTMENU 0
#include <metamod_oslink.h>
#include "ModInfo.h"
#include "hl2sdk/BATInterface.h"
#include "ForgiveTK.h"
#include "ForgiveTKMenu.h"
#include "Utils.h"
#include "cvars.h"
#include "const.h"
#include "BATMenu.h"
#include <time.h>

#include "hl2sdk/recipientfilters.h"
#include <bitbuf.h>

int g_IndexOfLastUserCmd;
int g_MaxClients;
int g_ForgiveTKMenu;

float g_FlTime;
bool g_IsConnected[MAXPLAYERS+1];
bool g_FirstLoad;

ForgiveTK g_ForgiveTKCore;
ConstReconnectInfo ReconnectInfo[RECONNECT_REMEMEBERCOUNT];
ConstPlayerInfo g_UserInfo[MAXPLAYERS+1];
TeamKillOptionsStruct TKSettings;
AdminInterface *m_AdminManager;
Utils *m_Utils;
ICvar *g_pCVar;
BATMenuMngr g_MenuMngr;
CCommand g_LastCCommand;

EventListenPlayerHurt *pPlayerHurt;
EventListenPlayerSpawn *pPlayerSpawn;
EventListenPlayerDeath *pPlayerDeath;

char g_CustomTKText[40];
char g_CustomTAText[40];
bool g_CustomTextTKEnabled = false;
bool g_CustomTextTAEnabled = false;

bool g_EventHookActive=false;


SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
//SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);
SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char*, const char *, char *, int);
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK0_void(ConCommand, Dispatch, SH_NOATTRIB, 0);


PLUGIN_EXPOSE(ForgiveTK, g_ForgiveTKCore);

void ForgiveTK::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	LoadPluginSettings(clientMax);
	RETURN_META(MRES_IGNORED);
}
void ForgiveTK::LevelShutdown( void )
{
	RETURN_META(MRES_IGNORED);
}
void ForgiveTK::ClientDisconnect(edict_t *pEntity)
{
	if(!pEntity)
		return;

	int id = m_Engine->IndexOfEdict(pEntity);
	if(!id)
		return;

	if(!g_IsConnected[id]) 
		return;

	if(!g_UserInfo[id].IsBot)
		SaveReconnectInfo(id);

	g_IsConnected[id] = false;
	RETURN_META(MRES_IGNORED);
}
bool ForgiveTK::ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	
	RETURN_META_VALUE(MRES_IGNORED, true);
}
void ForgiveTK::ClientPutInServer(edict_t *pEntity, char const *playername)
{
	if(!pEntity) return;

	int id = m_Engine->IndexOfEdict(pEntity);
	g_IsConnected[id] = true;
	g_UserInfo[id].PlayerEdict = pEntity;
	g_UserInfo[id].Userid = m_Engine->GetPlayerUserId(pEntity);

	g_UserInfo[id].TACount = 0;
	g_UserInfo[id].TKCount = 0;
	g_UserInfo[id].SpawnTime = -1;		// We do this in case the mod dont fire the player_spawn event. So if this value does not change, we dont check for spawn killing

	const char * SteamID = m_Engine->GetPlayerNetworkIDString(pEntity);
	if(!SteamID || strlen(SteamID) == 0)
	{
		LogPrint("ForgiveTK: ERROR Failed to find a valid steamid for player %d-%s",id,playername);		
		return;
	}

	sprintf(g_UserInfo[id].Steamid,"%s",SteamID);

	if(strcmp(g_UserInfo[id].Steamid,"BOT") == 0 || strcmp(g_UserInfo[id].Steamid,"HLTV") == 0 ) // || strcmp(g_UserInfo[id].steamid,"STEAM_ID_LAN") == 0
		g_UserInfo[id].IsBot = true;
	else 
	{
		g_UserInfo[id].IsBot = false;
		if(strcmp(g_UserInfo[id].Steamid,"STEAM_ID_LAN") != 0)
			CheckIfPlayerReconnected(id);
	}

	RETURN_META(MRES_IGNORED);
}

void ForgiveTK::SetCommandClient(int index)
{
	//META_LOG(g_PLAPI, "SetCommandClient() called: index=%d", index);
	g_IndexOfLastUserCmd = index +1;
	RETURN_META(MRES_IGNORED);
}
void ForgiveTK::GameFrame(bool simulating) // We dont hook GameFrame, we leave it in here incase we ever need some timer system 
{
/*
	float CurTime = g_SMAPI->GetCGlobals()->curtime;
	if(CurTime - g_FlTime >= TASK_CHECKTIME)
	{
		g_FlTime = CurTime;
		
		//ServerCommand("echo GameFrame() Timeleft:%.2f (g_ChangeMapTime: %.0f  g_PublicVoteStatus: %d)",g_TimeLeft,g_ChangeMapTime,g_PublicVoteStatus);
	}
*/
	RETURN_META(MRES_IGNORED);
}
#if BAT_ORANGEBOX == 1
void ForgiveTK::ClientCommand(edict_t *pEntity, const CCommand &args)
{
#else
void ForgiveTK::ClientCommand(edict_t *pEntity)
{
	CCommand args;
#endif
	int id = m_Engine->IndexOfEdict(pEntity);
	g_LastCCommand = args;

	if (strcmp(g_LastCCommand.Arg(0),"menuselect") == 0)
		g_ForgiveTKCore.ServerCommand("menuselect called %d %d",id,g_MenuMngr.HasMenuOpen(id));

	if(g_MenuMngr.HasMenuOpen(id) == false)
		RETURN_META(MRES_IGNORED);
	
	const char *command = g_LastCCommand.Arg(0);

	if (strcmp(command,"menuselect") == 0)
	{
		const char *arg1 = g_LastCCommand.Arg(1);
		//catch menu commands
		if (arg1)
		{
			int arg = atoi(arg1);
			if(arg < 1 || arg > 10) // Make sure makes no invalid selection.
				return;

			g_MenuMngr.MenuChoice(id, arg);
			RETURN_META(MRES_SUPERCEDE);
		}
	}
	RETURN_META(MRES_IGNORED);
}
void EventListenPlayerHurt::FireGameEvent(IGameEvent *event)
{
	if (!event || !event->GetName() || g_FTKVars.GetFTKOption() == OPTION_DISABLED )
		return;

	int victim = g_ForgiveTKCore.FindPlayer(event->GetInt("userid"));
	int attacker = g_ForgiveTKCore.FindPlayer(event->GetInt("attacker"));
	float Gametime = g_SMAPI->GetCGlobals()->curtime;

#if FTK_DEBUG == 1
	g_ForgiveTKCore.ServerCommand("echo %d attacked %d, alive: %d BasicCheck: %d",attacker,victim,g_ForgiveTKCore.IsUserAlive(victim),g_ForgiveTKCore.BasicEventChecks(attacker,victim));
#endif
	if(g_ForgiveTKCore.BasicEventChecks(attacker,victim) || !g_ForgiveTKCore.IsUserAlive(victim) )
		return;

	g_UserInfo[victim].LastTAIndex = attacker;
	g_UserInfo[attacker].LastAttackTime = Gametime;
	g_UserInfo[attacker].LastTAVictim = victim;

#if FTK_DEBUG == 1
	g_ForgiveTKCore.ServerCommand("echo %s has team attacked %s",g_ForgiveTKCore.GetPlayerName(attacker),g_ForgiveTKCore.GetPlayerName(victim));
#endif
	g_ForgiveTKCore.RegisterTeamAttack(attacker);
}
void EventListenPlayerSpawn::FireGameEvent(IGameEvent *event)
{
	if (!event || !event->GetName() || g_FTKVars.GetFTKOption() == OPTION_DISABLED )
		return;

	int id = g_ForgiveTKCore.FindPlayer(event->GetInt("userid"));	
	g_UserInfo[id].SpawnTime = g_SMAPI->GetCGlobals()->curtime;
}
void EventListenPlayerDeath::FireGameEvent(IGameEvent *event)
{
	if (!event || !event->GetName() || g_FTKVars.GetFTKOption() == OPTION_DISABLED )
		return;

	const char *EventName = event->GetName();

	int victim = g_ForgiveTKCore.FindPlayer(event->GetInt("userid"));
	int attacker = g_ForgiveTKCore.FindPlayer(event->GetInt("attacker"));

	if(g_ForgiveTKCore.BasicEventChecks(attacker,victim))
		return;

	float Gametime = g_SMAPI->GetCGlobals()->curtime;

#if FTK_DEBUG == 1
	g_ForgiveTKCore.ServerCommand("echo %s has team killed %s",g_ForgiveTKCore.GetPlayerName(attacker),g_ForgiveTKCore.GetPlayerName(victim));
#endif
	if(Gametime == g_UserInfo[attacker].LastAttackTime && victim == g_UserInfo[attacker].LastTAVictim)
		g_ForgiveTKCore.RemoveTeamAttack(attacker);

	g_UserInfo[victim].LastTKIndex = attacker;
	g_UserInfo[attacker].LastAttackTime = Gametime;
	g_ForgiveTKCore.RegisterTeamKill(attacker);

#if DEBUG_REVERTMENU == 1
	if((TKSettings.Option == OPTION_MENUBASED || TKSettings.Option == OPTION_ESCMENUBASED) && g_IsConnected[attacker] == true)
		g_MenuMngr.ShowMenu(attacker,g_ForgiveTKMenu);
#else
	if((TKSettings.Option == OPTION_MENUBASED || TKSettings.Option == OPTION_ESCMENUBASED) && g_UserInfo[victim].IsBot == false && g_IsConnected[attacker] == true)
		g_MenuMngr.ShowMenu(victim,g_ForgiveTKMenu);
#endif

	else if(g_ForgiveTKCore.IsUserConnected(attacker) == true && g_ForgiveTKCore.IsUserConnected(victim) == true)
	{
		if(g_CustomTextTKEnabled)
			g_ForgiveTKCore.MessagePlayer(victim,"[ForgiveTK] You can forgive %s for killing you by saying 'say forgivetk' or 'say %s'",g_ForgiveTKCore.GetPlayerName(attacker),g_CustomTKText);
		else
			g_ForgiveTKCore.MessagePlayer(victim,"[ForgiveTK] You can forgive %s for killing you by saying 'say forgivetk'",g_ForgiveTKCore.GetPlayerName(attacker));
	}

	if(g_ModSettings.GameMod == MOD_FIREARMS2)
	{
		g_ForgiveTKCore.MessagePlayer(victim,"The \"creators\" of this mods are thiefs, for more information check http://www.firearmsmod.com");
	}
	return;
}
#if BAT_ORANGEBOX == 1
void ForgiveTK::ClientSay(const CCommand &args)
{
#else
void ForgiveTK::ClientSay()
{
	CCommand args;
#endif
	g_LastCCommand = args;
	bool isTeamSay = false;

	char FirstWord[192];
	_snprintf(FirstWord,191,"%s",g_LastCCommand.Arg(1));
	char *pFirstWord = (char *)FirstWord;
	int TextLen = strlen(pFirstWord);

	if(g_ModSettings.GameMod == MOD_INSURGENCY)
	{
		int SayType = atoi(g_LastCCommand.Arg(1));

		if(SayType != 1 && SayType != 2)
			return; // This was some odd say type like squad / server say or whatever we dont care but we dont handle it.

		_snprintf(FirstWord,191,"%s",g_LastCCommand.Arg(3));
		pFirstWord = (char *)FirstWord;
		TextLen = strlen(pFirstWord);
	}

	int id = g_IndexOfLastUserCmd;
	if(stricmp(pFirstWord,"ff") == 0 || stricmp(pFirstWord,"/ff") == 0)
	{
		if(g_ForgiveTKCore.IsFriendlyFireOn())
			g_ForgiveTKCore.MessagePlayer(0,"[ForgiveTK] Friend fire is: ON");
		else
			g_ForgiveTKCore.MessagePlayer(0,"[ForgiveTK] Friend fire is: OFF");
		//g_ForgiveTKCore.ServerCommand("echo firstword: %s  %d",firstword,g_ForgiveTKCore.IsFriendlyFireOn());
	}
	if(TKSettings.Option == OPTION_CHATBASED  || TKSettings.Option == OPTION_MENUBASED )
	{
		if(stricmp(pFirstWord,"/tacount") == 0 || stricmp(pFirstWord,"/tkcount") == 0)
		{
			g_ForgiveTKCore.MessagePlayer(id,"[ForgiveTK] You have %d/%d Team attacks and %d/%d Team kills",g_UserInfo[id].TACount,TKSettings.MaxTA,g_UserInfo[id].TKCount,TKSettings.MaxTK);
		}
		else if(stricmp(pFirstWord,"forgivetk") == 0 || stricmp(pFirstWord,"/forgivetk") == 0 || g_CustomTextTKEnabled && stricmp(pFirstWord,g_CustomTKText) == 0 ) //TKSettings.Option == OPTION_CHATBASED && (
		{
			g_ForgiveTKCore.ForgivePlayerTK(id);
		}
		else if(stricmp(pFirstWord,"forgiveta") == 0 || stricmp(pFirstWord,"/forgiveta") == 0 || g_CustomTextTAEnabled && stricmp(pFirstWord,g_CustomTAText) == 0 )
		{
			g_ForgiveTKCore.ForgivePlayerTA(id);
		}
	}
}

void ForgiveTK::LoadPluginSettings(int clientMax)
{	
	if(g_FirstLoad == false)
	{
		ModInfo *pModInfo = new ModInfo;
		pModInfo->GetModInformation(m_ServerDll,m_Engine);
		delete pModInfo;

		m_Sound->PrecacheSound(SOUND_CHOICEMADE,true);
		m_Sound->PrecacheSound(SOUND_INVALID,true);

		g_FirstLoad = true;
		g_MaxClients = clientMax;

		m_Utils = new Utils;
		m_Utils->UTIL_PrecacheOffsets();
		ForgiveTKMenu *FTKMenu = new ForgiveTKMenu;
		g_ForgiveTKMenu = g_MenuMngr.RegisterMenu(FTKMenu);
		
		g_FTKVars.SetupCallBacks();		
		g_MenuMngr.SetMenuType(1);

		
		if(!g_ModSettings.DefaultRadioMenuSupport && g_FTKVars.GetFTKOption() == OPTION_MENUBASED)
		{
			ServerCommand("ftk_option %d",OPTION_ESCMENUBASED);
			ServerCommand("echo [ForgiveTK] ERROR! Menus are not supported by this mod use ftk_option %d (For ESC based menus) ( Mod Found %s)",OPTION_ESCMENUBASED,g_ModSettings.GameModName);
			g_MenuMngr.SetMenuType(2);
		}
		else
			g_MenuMngr.SetMenuType(1);

		g_MenuMngr.ClearForMapchange();

		if(g_ModSettings.GameMod == MOD_INSURGENCY)
			pSayCmd = HookConsoleCmd("say2");	
		else
			pSayCmd = HookConsoleCmd("say");

		pSayTeamCmd = HookConsoleCmd("say_team");

		pFriendlyFire = GetCvarPointer("mp_friendlyfire");
		
		if(g_ModSettings.GameMod == MOD_INSURGENCY && !pFriendlyFire)
			pFriendlyFire = GetCvarPointer("ins_friendlyfire");

		if(!pFriendlyFire)
			ServerCommand("echo [ForgiveTK] ERROR! Plugin was unable to hook mp_friendlyfire this plugin will crash SOON!");
	}
	for(int i=1;i<=g_MaxClients;i++)
		g_IsConnected[i] = false;
	
	for(int i=0;i<RECONNECT_REMEMEBERCOUNT;i++)
		ReconnectInfo[i].HasInformation = false;
	
	ServerCommand("exec FTKConfig.cfg");

	if(TKSettings.Option == OPTION_ONLYCHATRESPOND && g_EventHookActive == true)
	{
		g_EventHookActive = false;
		ServerCommand("echo [ForgiveTK] Game event hooking removed");
		m_GameEventManager->Reset();
	}
	else if(g_EventHookActive == false && TKSettings.Option != OPTION_ONLYCHATRESPOND)
	{
		pPlayerHurt = new EventListenPlayerHurt;
		pPlayerSpawn = new EventListenPlayerSpawn;
		pPlayerDeath = new EventListenPlayerDeath;
		
		m_GameEventManager->AddListener(pPlayerHurt,"player_hurt",true);
		m_GameEventManager->AddListener(pPlayerDeath,"player_death",true);
		m_GameEventManager->AddListener(pPlayerSpawn,"player_spawn",true);

		ServerCommand("echo [ForgiveTK] Game event hooking active");
		g_EventHookActive  = true;
	}
}
bool ForgiveTK::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_ANY(GetServerFactory, m_ServerDll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetServerFactory, m_ServerClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetServerFactory, m_InfoMngr, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);

	GET_V_IFACE_CURRENT(GetEngineFactory, m_Engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, m_GameEventManager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_CURRENT(GetEngineFactory, m_ICvar, ICvar, VENGINE_CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, m_Helpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE_CURRENT(GetEngineFactory, m_Sound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);

	
	//Init our cvars/concmds
	ConCommandBaseMgr::OneTimeInit(&g_FTKVars);

	//m_hooks.push_back(SH_ADD_HOOK(IServerGameDLL, LevelInit, m_ServerDll, SH_MEMBER(this, &BATCore::LevelInit), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameDLL, ServerActivate, m_ServerDll, SH_MEMBER(this, &ForgiveTK::ServerActivate), true));
	//m_hooks.push_back(SH_ADD_HOOK(IServerGameDLL, GameFrame, m_ServerDll, SH_MEMBER(this, &BATCore::GameFrame), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameDLL, LevelShutdown, m_ServerDll, SH_MEMBER(this, &ForgiveTK::LevelShutdown), false));
	//m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientActive, gameclients, SH_MEMBER(this, &g_BATCore::Hook_ClientActive), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientDisconnect, m_ServerClients, SH_MEMBER(this, &ForgiveTK::ClientDisconnect), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientPutInServer, m_ServerClients, SH_MEMBER(this, &ForgiveTK::ClientPutInServer), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, SetCommandClient, m_ServerClients, SH_MEMBER(this, &ForgiveTK::SetCommandClient), true));
	//m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientSettingsChanged, gameclients, SH_MEMBER(this, &g_BATCore::Hook_ClientSettingsChanged), false));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientConnect, m_ServerClients, SH_MEMBER(this, &ForgiveTK::ClientConnect), false));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientCommand, m_ServerClients, SH_MEMBER(this, &ForgiveTK::ClientCommand), false));

	m_Engine_CC = SH_GET_CALLCLASS(m_Engine); // invoking their hooks (when needed).
	SH_CALL(m_Engine_CC, &IVEngineServer::LogPrint)("[BAT] All hooks started!\n");

	//Init our cvars/concmds
	/* NOTE! g_pCVar must be set to a valid ICvar instance first. */
	g_pCVar = GetICVar();

#if BAT_ORANGEBOX == 1
	ConVar_Register(0,this);
#else
	ConCommandBaseMgr::OneTimeInit(this);
#endif

	if(late)
	{
		LoadPluginSettings(ismm->GetCGlobals()->maxClients);
		LogPrint("[ForgiveTK] Late load is not really supported, plugin will function properly after 1 mapchange");
	}
	return true;
}
bool ForgiveTK::RegisterConCommandBase(ConCommandBase *pVar)
{
	bool test = META_REGCVAR(pVar);
	if(!test)
		return test;
	return test;
}
bool ForgiveTK::Unload(char *error, size_t maxlen)
{
	
	//IT IS CRUCIAL THAT YOU REMOVE CVARS.
	//As of Metamod:Source 1.00-RC2, it will automatically remove them for you.
	//But this is only if you've registered them correctly!

	for (size_t i = 0; i < m_hooks.size(); i++)
	{
		if (m_hooks[i] != 0)
		{
			SH_REMOVE_HOOK_ID(m_hooks[i]);
		}
	}
	m_hooks.clear();

	m_GameEventManager->RemoveListener(pPlayerSpawn);
	m_GameEventManager->RemoveListener(pPlayerHurt);
	m_GameEventManager->RemoveListener(pPlayerDeath);

	if(m_AdminManager) // We remove the admin interface hook
	{
		m_AdminManager->RemoveListner(this);
		m_AdminManager = NULL;
	}
	

	//this, sourcehook does not keep track of.  we must do this.
	SH_RELEASE_CALLCLASS(m_Engine_CC);


	return true;
}
ICvar *GetICVar()
{
#if BAT_ORANGEBOX == 1
	return (ICvar *)((g_SMAPI->GetEngineFactory())(CVAR_INTERFACE_VERSION, NULL));
#else
	return (ICvar *)((g_SMAPI->GetEngineFactory())(VENGINE_CVAR_INTERFACE_VERSION, NULL));
#endif
}

void ForgiveTK::AllPluginsLoaded()
{
	//AdminInterfaceListner *AdminInterfacePointer;

	if(m_AdminManager || !g_FTKVars.UseAdminInterface()) // We dont need to find the AdminInterface again, we allready found it once.
		return;

	//we don't really need this for anything other than interplugin communication
	//and that's not used in this plugin.
	//If we really wanted, we could override the factories so other plugins can request
	// interfaces we make.  In this callback, the plugin could be assured that either
	// the interfaces it requires were either loaded in another plugin or not.
	PluginId id2; 

	void *ptr = g_SMAPI->MetaFactory("AdminInterface", NULL, &id2); 
	
	if (!ptr) 
	{
		ServerCommand("echo [ForgiveTK Error] Did not find AdminInterface, plugin will not check admin rights"); 
	} else if(id2 <= 100) { // This is to prevent a crash bug, If the id2 is to high, then the pointer can be pass the null check, but its still just garbage and crashes
		m_AdminManager = (AdminInterface *)ptr;
		m_AdminManager->AddEventListner(this);
		int InterfaceVersion = m_AdminManager->GetInterfaceVersion();
		
		if(InterfaceVersion == ADMININTERFACE_VERSION)
		{
			const char *AdminTool = m_AdminManager->GetModName();
			if(strcmp("BAT",AdminTool) != 0)
			{
				//m_AdminManager->RegisterFlag("ForgiveTK","immunity","If the player should be immune to kick/slay");
				if(m_AdminManager->RegisterFlag("ForgiveTK","immunity","immunity slay/kick"))
					ServerCommand("echo [ForgiveTK] A custom admin right with the class: \"ForgiveTK\" and the right \"immunity\" has been registered with the admin tool, you need to configure it for immunity to work");
				else
					ServerCommand("echo [ForgiveTK ERROR] There was a error trying to register the new custom access, check the admin tool logs for details");
			}
			ServerCommand("echo [ForgiveTK] Found AdminInterface via %s (Interface version: %d)",AdminTool,InterfaceVersion); 

#if FTK_DEBUG == 1
			ServerCommand("echo [ForgiveTK] Found AdminInterface[%d] at (%p), via %s (Interface version: %d)", id2, ptr,m_AdminManager->GetModName(),InterfaceVersion); 
#endif
		}
		else
		{
			if(InterfaceVersion < ADMININTERFACE_VERSION)
			{
				ServerCommand("echo [ForgiveTK ERROR] Found a version of the AdminInterface LOWER then this plugin expects. AdminInterface disabled");
				ServerCommand("ftk_admininterface 0");
			}
			else
				ServerCommand("echo [ForgiveTK] Found AdminInterface via %s (Interface version: %d), but internal version is %d this could mean crashes",m_AdminManager->GetModName(),InterfaceVersion,ADMININTERFACE_VERSION); 
		}
	}
	else
		ServerCommand("echo [ForgiveTK] Found AdminInterface but the PluginID returned was to high(%d), we will not use the interface",id2); 
}
void ForgiveTK::OnAdminInterfaceUnload() // Called when the admin interface is getting unloaded, this happens if a admin uses meta unload or the server is shutting down.
{
	m_AdminManager = NULL;
	//g_ForgiveTKCore.ServerCommand("echo AdminInterface was unloaded");
}

void ForgiveTK::Client_Authorized(int id) // This is called when a client is fully connected and authed by the admin tool, this normaly happens after the client has gotten a valid steamid & the admin tool has checked it against its internal user lists.
{
#if FTK_DEBUG == 1
	ServerCommand("echo [ForgiveTK Debug]Client_Authorized: %d",id);
#endif
}
ConCommand *ForgiveTK::HookConsoleCmd(const char *CmdName)
{
	ConCommandBase *pCmd = g_pCVar->GetCommands();
	ConCommand* pTemp;

	while (pCmd)
	{
		if (pCmd->IsCommand() && stricmp(pCmd->GetName(),CmdName) == 0) 
		{
			pTemp = (ConCommand *)pCmd;

			m_hooks.push_back(SH_ADD_HOOK(ConCommand, Dispatch, pTemp, SH_MEMBER(this, &ForgiveTK::ClientSay), false));
			return pTemp;
		}

		pCmd = const_cast<ConCommandBase *>(pCmd->GetNext());
	}
	return NULL;
}