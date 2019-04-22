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

#include "ForgiveTK.h"
#include "cvars.h"

extern TeamKillOptionsStruct TKSettings;
extern ForgiveTK g_ForgiveTKCore;

extern char g_CustomTKText[40];
extern char g_CustomTAText[40];
extern bool g_CustomTextTKEnabled;
extern bool g_CustomTextTAEnabled;

FTKVars g_FTKVars;

ConVar g_VarFTKVersion("ftk_version",FTK_VERSION, FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY, "The version of ForgiveTK");
ConVar g_VarFTKOption("ftk_option", "1", 0, "\n0 = System Disabled\n1 = Enable the TK system ( Radio Menu powered)\n2 = Enable the TK system ( Chat powered)\n3 = Only responds to the say /ff to chat, does not monitor TK/TA\n4 = Enable the TK system ( ESC Menu powered)");
ConVar g_VarFTKTKPunish("ftk_punishoption","0",0,"What punishment should be given to players if their not forgiven when they TK ( Only with menu powered system)\n0 = No punishment\n1 = User is slayed when not forgiven\n2 = Takes 50% of users Health");
ConVar g_VarFTKAction("ftk_action","0", 0, "What action is taken when a player goes above his allowed TK count\n0= Player is kicked, and number above this will be the players ban time");
ConVar g_VarFTKSpawnProtection("ftk_spawnprotection","10", 0, "How long a player is protected after spawning, if he is attacked / damaged the offender is instantly slayed");
ConVar g_VarFTKAdminInterFace("ftk_admininterface","1", 0, "Use the AdminInterface to check for admin immunity");

ConVar g_VarFTKTeamAttackCount("ftk_teamattackcount", "10", 0, "How many team attack counts as 1 team kill");
ConVar g_VarFTKTeamKillCount("ftk_teamkillcount", "3", 0, "How many team kills is required before the user is kicked");

ConVar g_VarFTKTeamKillCustom("ftk_teamkillcustom", "", 0, "A custom text to be used in chat text to forgive team kills, only updated on mapchange");
ConVar g_VarFTKTeamAttackCustom("ftk_teamattackcustom", "",0 , "A custom text to be used in chat text to forgive team attack, only updated on mapchange");

bool FTKVars::RegisterConCommandBase(ConCommandBase *pVar)
{
	//this will work on any type of concmd!
	return META_REGCVAR(pVar);
}
void FTKVars::SetupCallBacks() // Yes i know we read all the cvars, when just 1 changed ( Or we can end up reading all 4 chars 4 times. But who cares, this should only happen on a mapchange and im lazy )
{
	g_VarFTKOption.InstallChangeCallback((FnChangeCallback_t) &FTKVars::CvarCallback);
	g_VarFTKTKPunish.InstallChangeCallback((FnChangeCallback_t) &FTKVars::CvarCallback);
	g_VarFTKTeamAttackCount.InstallChangeCallback((FnChangeCallback_t) &FTKVars::CvarCallback);
	g_VarFTKTeamKillCount.InstallChangeCallback((FnChangeCallback_t) &FTKVars::CvarCallback);
}
void FTKVars::CvarCallback()
{
	TKSettings.MaxTA = g_FTKVars.GetFTKTACount();
	TKSettings.MaxTK = g_FTKVars.GetFTKTeamKillCount();
	TKSettings.Option = g_FTKVars.GetFTKOption();
	TKSettings.PunishOption = g_FTKVars.GetPunishOption();

	_snprintf(g_CustomTKText,39,"%s",g_FTKVars.GetTKCustomText());
	_snprintf(g_CustomTAText,39,"%s",g_FTKVars.GetTACustomText());
	if(strlen(g_CustomTAText) > 0)
		g_CustomTextTAEnabled = true;
	else
		g_CustomTextTAEnabled = false;

	if(strlen(g_CustomTKText) > 0)
		g_CustomTextTKEnabled = true;
	else
		g_CustomTextTKEnabled = false;
}

const char *FTKVars::GetTACustomText() { return g_VarFTKTeamKillCustom.GetString(); }
const char *FTKVars::GetTKCustomText() { return g_VarFTKTeamAttackCustom.GetString(); }
int FTKVars::GetFTKSpawnProtectionTime() {	return g_VarFTKSpawnProtection.GetInt(); }
int FTKVars::GetFTKAction() { return g_VarFTKAction.GetInt(); }
int FTKVars::GetFTKOption() { return g_VarFTKOption.GetInt(); }
int FTKVars::GetFTKTACount() { return g_VarFTKTeamAttackCount.GetInt();	}
int FTKVars::GetFTKTeamKillCount() { return g_VarFTKTeamKillCount.GetInt();	}
int FTKVars::GetPunishOption() { return g_VarFTKTKPunish.GetInt(); }
bool FTKVars::UseAdminInterface() { return g_VarFTKAdminInterFace.GetBool(); }
/*
CON_COMMAND(admin_test, "Sample Plugin command")
{
	META_LOG(g_PLAPI, "This sentence is in Spanish when you're not looking.");
}
*/