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
#include "ForgiveTKMenu.h"
#include "BATMenu.h"
#include "Utils.h"

extern int g_ForgiveTKMenu;
extern int g_MaxClients;
extern BATMenuMngr g_MenuMngr;
extern Utils *m_Utils;

extern ConstPlayerInfo g_UserInfo[MAXPLAYERS+1];
extern TeamKillOptionsStruct TKSettings;
extern bool g_IsConnected[MAXPLAYERS+1];

bool ForgiveTKMenu::Display(BATMenuBuilder *make, int playerIndex)
{
	make->SetKeys( (1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7)| (1<<8) | (1<<9));

	char Title[48];
	_snprintf(Title,47,"Forgive %s for killing you",g_ForgiveTKCore.GetPlayerName(g_UserInfo[playerIndex].LastTKIndex));
	make->SetTitle(Title);

	make->AddOption("Yes");
	make->AddOption(GetPunishmentText());

	if(g_MenuMngr.GetMenuType() == 2)
		g_ForgiveTKCore.MessagePlayer(0,"Press ESC to either forgive %s or not",g_ForgiveTKCore.GetPlayerName(g_UserInfo[playerIndex].LastTKIndex));
	return true;
}
MenuSelectionType ForgiveTKMenu::MenuChoice(player_t player, int option)
{
	int id = player.index;
	int IndexAttacker = g_UserInfo[id].LastTKIndex;
	if(g_IsConnected[IndexAttacker] == false)
		return MENUSELECTION_NOSOUND;

	//if(!g_ModSettings.DefaultRadioMenuSupport)
	//		g_ForgiveTKCore.MessagePlayer(id,"You need to press ESC to see the menu");

	if(option == 1)	// Forgives the player
	{
		g_ForgiveTKCore.ForgivePlayerTK(id);
		//g_ForgiveTKCore.MessagePlayer(0,"[ForgiveTK] %s choose to forgive %s",g_ForgiveTKCore.GetPlayerName(id),g_ForgiveTKCore.GetPlayerName(g_UserInfo[id].LastTKIndex));
		return MENUSELECTION_GOOD;
	}
	else if(option == 2)
	{
		if(TKSettings.PunishOption == PunishDisabled)
			g_ForgiveTKCore.MessagePlayer(0,"[ForgiveTK] %s choose to NOT forgive %s",g_ForgiveTKCore.GetPlayerName(id),g_ForgiveTKCore.GetPlayerName(g_UserInfo[id].LastTKIndex));
		else
			PunishPlayer(id);
		return MENUSELECTION_GOOD;
	}
	else
		g_MenuMngr.ShowMenu(id,g_ForgiveTKMenu);	
	
	return MENUSELECTION_BAD;
}
void ForgiveTKMenu::PunishPlayer(int id)
{
	int TKIndex = g_UserInfo[id].LastTKIndex;

	if(TKSettings.PunishOption == PunishSlay)
	{
		g_ForgiveTKCore.MessagePlayer(0,"[ForgiveTK] %s choose to not forgive %s and have him slayed",g_ForgiveTKCore.GetPlayerName(id),g_ForgiveTKCore.GetPlayerName(TKIndex));	
		g_ForgiveTKCore.GetHelpers()->ClientCommand(g_UserInfo[TKIndex].PlayerEdict,"kill");		
	}
	else if(TKSettings.PunishOption == PunishSlap)
	{
		int Health = m_Utils->UTIL_GetHealth(g_UserInfo[TKIndex].PlayerEdict) * 0.5;
		m_Utils->UTIL_SetHealth(g_UserInfo[TKIndex].PlayerEdict,Health);
	}
}
const char *ForgiveTKMenu::GetPunishmentText()
{
/*
// 	static char PunishText[48];
// 
// 	if(TKSettings.PunishOption == PunishSlay)
// 		_snprintf(PunishText,47,"No (Slayed)");
// 
// 	else if(TKSettings.PunishOption == PunishSlap)
// 		_snprintf(PunishText,47,"No (Slap away 50%% of HP)");
// 
// 	else
// 		_snprintf(PunishText,47,"No");
// 
// 	return PunishText;
*/
	
	switch(TKSettings.PunishOption)
	{
	case PunishSlay:
		return "No (Slayed)";

	case PunishSlap:
		return "No (Slap away 50%% of HP)";

	default:
		return "No";
	}
}