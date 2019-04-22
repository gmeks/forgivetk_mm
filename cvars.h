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

#ifndef _INCLUDE_CVARS_H
#define _INCLUDE_CVARS_H

#include <convar.h>

class FTKVars : public IConCommandBaseAccessor
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase *pVar);
	int GetFTKOption();
	int GetFTKTACount();
	int GetFTKTeamKillCount();
	int GetPunishOption();
	int GetFTKAction();
	int GetFTKSpawnProtectionTime();

	const char *GetTACustomText(); // { return g_VarFTKTeamKillCustom.GetString(); } ;
	const char *GetTKCustomText(); //{ return g_VarFTKTeamAttackCustom.GetString(); };

	bool UseAdminInterface();

	void SetupCallBacks();
	

private:
	static void CvarCallback();
};

extern FTKVars g_FTKVars;

#endif //_INCLUDE_CVARS_H
