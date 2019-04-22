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

#include <metamod_oslink.h>
#include "ForgiveTK.h"
#include "time.h"
#include "cvars.h"
#include "convar.h"
#include "const.h"
#include <string.h>
#include <bitbuf.h>
#include <ctype.h>
#include "hl2sdk/recipientfilters.h"

extern int g_MaxClients;
extern bool g_IsConnected[MAXPLAYERS+1];
extern ConstPlayerInfo g_UserInfo[MAXPLAYERS+1];
extern ConstReconnectInfo ReconnectInfo[RECONNECT_REMEMEBERCOUNT];
extern TeamKillOptionsStruct TKSettings;
extern AdminInterface *m_AdminManager;

void ForgiveTK::RegisterTeamAttack(int id)
{
	g_UserInfo[id].TACount++;

	if(g_UserInfo[id].TACount >= TKSettings.MaxTA)
	{
		MessagePlayer(id,"[ForgiveTK] You have done %d team attacks, so your team kill has increased by 1",g_UserInfo[id].TACount);
		g_UserInfo[id].TACount = 0;
		RegisterTeamKill(id);
	}
	else
		SendCSay(id,"Team attack Warning %d/%d",g_UserInfo[id].TACount,TKSettings.MaxTA);
}
void ForgiveTK::RemoveTeamAttack(int id)
{	
	if(g_UserInfo[id].TACount == 0)
	{
		if(g_UserInfo[id].TKCount > 0)
            g_UserInfo[id].TKCount--;
	}
	else
		g_UserInfo[id].TACount--;
}
void ForgiveTK::RegisterTeamKill(int id)
{
	g_UserInfo[id].TKCount++;

	if(g_UserInfo[id].TKCount >= TKSettings.MaxTK)
	{
		if(HasAdminImmunity(id))
		{
			MessagePlayer(0,"[ForgiveTK] %s has admin immunity and cannot be kicked, was slayed instead",GetPlayerName(id));
			m_Helpers->ClientCommand(g_UserInfo[id].PlayerEdict,"kill");			
			return;
		}
		int Action = g_FTKVars.GetFTKAction();
		int TKCount = g_UserInfo[id].TKCount;
		
		g_UserInfo[id].TKCount = 0;
		g_UserInfo[id].TACount = 0;
		g_IsConnected[id] = false;

		if(Action == 0)
		{
			MessagePlayer(0,"[ForgiveTK] %s was kicked for having %d team kills",GetPlayerName(id),TKCount);

			ServerCommand("kickid %d To many Team kills %d",g_UserInfo[id].Userid,TKCount);
		}
		else
		{
			MessagePlayer(0,"[ForgiveTK] %s was banned for having %d team kills",GetPlayerName(id),TKCount);

			ServerCommand("banid %d %d",Action,g_UserInfo[id].Userid);
			ServerCommand("kickid %d To many Team kills %d",g_UserInfo[id].Userid,TKCount);
		}
	}
	else
	{
		SendCSay(id,"Team kill Warning %d/%d",g_UserInfo[id].TKCount,TKSettings.MaxTK);
	}
}
void ForgiveTK::SendCSay(int id,const char *msg, ...) 
{
	char vafmt[192];
	va_list ap;
	va_start(ap, msg);
	_vsnprintf(vafmt,191,msg,ap);
	va_end(ap);

	RecipientFilter mrf;

	if(id == 0 )
		mrf.AddAllPlayers(g_MaxClients);
	else
		mrf.AddPlayer(id);


	if(g_ModSettings.ValveCenterSay)
	{
		bf_write *buffer = g_ForgiveTKCore.GetEngine()->UserMessageBegin(static_cast<IRecipientFilter *>(&mrf), g_ModSettings.TextMsg);
		//FIXME
		//		buffer->WriteByte(HUD_PRINTCENTER);
		buffer->WriteString(vafmt);
		m_Engine->MessageEnd();
	}
	else
	{
		bf_write *msg = g_ForgiveTKCore.GetEngine()->UserMessageBegin((RecipientFilter *)&mrf,g_ModSettings.HudMsg);
		//msg->WriteByte(4); //4
		//msg->WriteString(temp);
		msg->WriteByte( 0); //textparms.channel
		msg->WriteFloat( -1 ); // textparms.x ( -1 = center )
		msg->WriteFloat( -.25 ); // textparms.y ( -1 = center )
		msg->WriteByte( 0xFF ); //textparms.r1
		msg->WriteByte( 0x00 ); //textparms.g1
		msg->WriteByte( 0x00 ); //textparms.b1
		msg->WriteByte( 0xFF ); //textparms.a2
		msg->WriteByte( 0xFF ); //textparms.r2
		msg->WriteByte( 0xFF ); //textparms.g2
		msg->WriteByte( 0xFF ); //textparms.b2
		msg->WriteByte( 0xFF ); //textparms.a2
		msg->WriteByte( 0x00); //textparms.effect
		msg->WriteFloat( 0 ); //textparms.fadeinTime
		msg->WriteFloat( 0 ); //textparms.fadeoutTime
		msg->WriteFloat( 10 ); //textparms.holdtime
		msg->WriteFloat( 45 ); //textparms.fxtime
		msg->WriteString( vafmt ); //Message
		m_Engine->MessageEnd();
	}
}
bool ForgiveTK::IsUserAlive(int id)
{
	IPlayerInfo *pPlayerInfo = m_InfoMngr->GetPlayerInfo(g_UserInfo[id].PlayerEdict);
	if(!pPlayerInfo)
		return false;

	bool IsDead = pPlayerInfo->IsDead();

	if(!IsDead)
		return true;
	return false;	
}
bool ForgiveTK::IsFriendlyFireOn()
{
	if(pFriendlyFire->GetInt() > 0)
		return true;
	
	return false;
}
bool ForgiveTK::IsUserConnected(int id)
{
	if(!g_IsConnected[id])		// We save the poor cpu some work and check our internal bool first
		return false;

	edict_t *e = m_Engine->PEntityOfEntIndex(id);
	if(!e || e->IsFree() )
		return false;

	//IPlayerInfo *pInfo = g_BATCore.PlayerInfo()->GetPlayerInfo(e);	
	IPlayerInfo *pInfo = m_InfoMngr->GetPlayerInfo(e);
	if(!pInfo || !pInfo->IsConnected())
		return false;
	return true;
}
bool ForgiveTK::BasicEventChecks(int attacker,int victim)
{	
	//ServerCommand("echo Debug: %s (team %d) attacked: %s (team %d)",GetPlayerName(attacker),GetUserTeam(attacker),GetPlayerName(victim),GetUserTeam(victim));
	if(victim  < 0 || attacker < 0 || victim == attacker || attacker == 0 || attacker > g_MaxClients || victim > g_MaxClients || GetUserTeam(victim) != GetUserTeam(attacker) || IsInsideSpawnProtection(attacker,victim))
		return true;

	return false;
}
int ForgiveTK::GetUserTeam(int id)
{
	edict_t *e = m_Engine->PEntityOfEntIndex(id);
	if(!e || e->IsFree())
		return TEAM_NOT_CONNECTED;

	IPlayerInfo *pInfo = m_InfoMngr->GetPlayerInfo(e);
	if(!pInfo->IsConnected())
		return TEAM_NOT_CONNECTED;

	return pInfo->GetTeamIndex();
}
void ForgiveTK::CheckIfPlayerReconnected(int id)
{
	int RecIndex = -1;
	for(int i=0;i<RECONNECT_REMEMEBERCOUNT;i++) if(ReconnectInfo[i].HasInformation == true)
	{
		if(strcmp(ReconnectInfo[i].Steamid,g_UserInfo[id].Steamid) == 0)
		{
			RecIndex = i;
			ReconnectInfo[i].HasInformation = false;
			break;
		}
	}
	if(RecIndex == -1)
		return;
	
	g_UserInfo[id].TACount = ReconnectInfo[RecIndex].TACount;
	g_UserInfo[id].TKCount = ReconnectInfo[RecIndex].TKCount;
	//MessagePlayer(id,"[ForgiveTK] You have just reconnected, your old TA(%d) & TK(%d) count was restored",g_UserInfo[id].TACount,g_UserInfo[id].TKCount);
}
void ForgiveTK::SaveReconnectInfo(int id)
{
	int LowestTimeIndex = -1;
	float HighestTime = 0;
	for(int i=0;i<RECONNECT_REMEMEBERCOUNT;i++)
	{
		if(!ReconnectInfo[i].HasInformation)
		{
			LowestTimeIndex = i;
			break;
		}
		else if(ReconnectInfo[i].HasInformation && ReconnectInfo[i].DisconnectTime > HighestTime)
		{
			HighestTime = ReconnectInfo[i].DisconnectTime;
			LowestTimeIndex = i;
		}
	}
	ReconnectInfo[LowestTimeIndex].HasInformation = true;
	_snprintf(ReconnectInfo[LowestTimeIndex].Steamid,MAX_NETWORKID_LENGTH,g_UserInfo[id].Steamid);
	ReconnectInfo[LowestTimeIndex].TACount = g_UserInfo[id].TACount;
	ReconnectInfo[LowestTimeIndex].TKCount = g_UserInfo[id].TKCount;
}
void ForgiveTK::ServerCommand(const char *fmt, ...)
{
	char buffer[512];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(buffer, sizeof(buffer)-2, fmt, ap);
	va_end(ap);
	strcat(buffer,"\n");
	m_Engine->ServerCommand(buffer);
}
int ForgiveTK::FindPlayer(int UserID)	// Finds the player index based on userid.
{
	for(int i=1;i<=g_MaxClients;i++)
	{
		if(g_IsConnected[i] == true)
		{
			if(UserID == g_UserInfo[i].Userid )	// Name or steamid matches TargetInfo so we return the index of the player
			{
				return i;
			}
		}
	}
	return -1;
}
void ForgiveTK::ForgivePlayerTK(int id)
{
	int attacker = g_UserInfo[id].LastTKIndex;
	if(attacker == 0 || !IsUserConnected(attacker) || g_UserInfo[attacker].TKCount == 0)
	{
		MessagePlayer(id,"[ForgiveTK] There is no team kill you can forgive");
		return;
	}
	g_UserInfo[attacker].TKCount--;
	g_UserInfo[attacker].LastTKIndex = 0;

	MessagePlayer(attacker,"[ForgiveTK] %s has forgiven you team kill, your team kill count is now: %d/%d",GetPlayerName(id),g_UserInfo[id].LastTKIndex,TKSettings.MaxTK);
	MessagePlayer(0,"[ForgiveTK] %s forgave %s for his team kill",GetPlayerName(id),GetPlayerName(attacker));
}
bool ForgiveTK::IsInsideSpawnProtection(int attacker,int victim)
{
	int SPT = g_FTKVars.GetFTKSpawnProtectionTime();

	if( g_UserInfo[victim].SpawnTime == -1 || SPT == 0 || g_UserInfo[attacker].IsBot  )
		return false;

	int Gametime = g_SMAPI->GetCGlobals()->curtime;
	int SpawnTime = g_UserInfo[victim].SpawnTime + SPT;

	if(SpawnTime >= Gametime)
	{
		MessagePlayer(0,"[ForgiveTK] %s was slayed for attacking %s inside the spawn protection time",GetPlayerName(attacker),GetPlayerName(victim));
#if FTK_DEBUG == 1
		ServerCommand("echo Was %d left of spawn protection time ( SpawnTime %d GameTime %d )",(Gametime - SpawnTime),SpawnTime,Gametime);
#else
		m_Helpers->ClientCommand(g_UserInfo[attacker].PlayerEdict,"kill");
#endif
		return true;
	}
	return false;
}
void ForgiveTK::ForgivePlayerTA(int id)
{
	int attacker = g_UserInfo[id].LastTAIndex;
	if(attacker == 0 || !IsUserConnected(attacker) || g_UserInfo[attacker].TACount == 0 && g_UserInfo[attacker].TKCount > 0)
	{
		MessagePlayer(id,"[ForgiveTK] There is no team attack you can forgive");
		return;
	}
	RemoveTeamAttack(attacker);
	g_UserInfo[attacker].LastTAIndex = 0;
	MessagePlayer(attacker,"[ForgiveTK] %s has forgiven you team attack, your team attack count is now: %d/%d",GetPlayerName(id),g_UserInfo[attacker].LastTAIndex,TKSettings.MaxTA);
		
	MessagePlayer(id,"[ForgiveTK] You forgave %s for his team attack",GetPlayerName(attacker));
}
void ForgiveTK::LogPrint( const char *msg, ...)
{
	char vafmt[192];
	va_list ap;
	va_start(ap, msg);
	int len = _vsnprintf(vafmt, 189, msg, ap);
	va_end(ap);
	len += _snprintf(&(vafmt[len]),191-len," \n");

	m_Engine->LogPrint(vafmt);
}
void ForgiveTK::MessagePlayer(int index, const char *msg, ...)
{
	 RecipientFilter rf;
	 if (index > g_MaxClients || index < 0)
		 return;

	 if (index == 0)
	 {
		 rf.AddAllPlayers(g_MaxClients);
	 } else {
		rf.AddPlayer(index);
	 }
	 rf.MakeReliable();

	 char vafmt[192];
	 va_list ap;
	 va_start(ap, msg);
	 int len = _vsnprintf(vafmt, 189, msg, ap);
	 va_end(ap);
	 len += _snprintf(&(vafmt[len]),191-len," \n");

	 bf_write *buffer = m_Engine->UserMessageBegin(static_cast<IRecipientFilter *>(&rf), g_ModSettings.SayText);
	 buffer->WriteByte(0);
	 buffer->WriteString(vafmt);
	 buffer->WriteByte(1);
	 m_Engine->MessageEnd();
}
const char *ForgiveTK::GetPlayerName(int id)
{
	return m_Engine->GetClientConVarValue(id,"name");
}
ConVar * ForgiveTK::GetCvarPointer(const char *CvarName)
{
	//ConVar *Var;
	ConCommandBase *pCmd = m_ICvar->FindVar(CvarName);
	if (pCmd && strcmpi(pCmd->GetName(),CvarName) == 0)
	{
		return (ConVar *)pCmd;
	}
	return NULL;
}
bool ForgiveTK::HasAdminImmunity(int id)
{
	if(m_AdminManager == NULL || !g_FTKVars.UseAdminInterface())
		return false;

	return m_AdminManager->HasFlag(id,"ForgiveTK","immunity");
}
int ForgiveTK::str_replace(char *str, const char *from, const char *to, int maxlen) // Function from sslice 
{
	char  *pstr   = str;
	int   fromlen = Q_strlen(from);
	int   tolen   = Q_strlen(to);
	int	  RC=0;		// Removed count

	while (*pstr != '\0' && pstr - str < maxlen) {
		if (Q_strncmp(pstr, from, fromlen) != 0) {
			*pstr++;
			continue;
		}
		Q_memmove(pstr + tolen, pstr + fromlen, maxlen - ((pstr + tolen) - str) - 1);
		Q_memcpy(pstr, to, tolen);
		pstr += tolen;
		RC++;
	}
	return RC;
}
