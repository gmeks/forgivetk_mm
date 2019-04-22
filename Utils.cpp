/* ======== Basic Admin tool ========
* Copyright (C) 2004-2006 Erling K. Sæterdal
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Erling K. Sæterdal ( EKS )
* Credits:
*	Menu code based on code from CSDM ( http://www.tcwonline.org/~dvander/cssdm ) Created by BAILOPAN
*	Helping on misc errors/functions: BAILOPAN,LDuke,sslice,devicenull,PMOnoTo,cybermind ( most who idle in #sourcemod on GameSurge realy )
* ============================ */

#include <stdio.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h> 
#endif

#include "Utils.h"
#include "ForgiveTK.h"
#include "server_class.h"

int g_nHealthOffset;
bool g_UseSignatureScanner;


void Utils::UTIL_PrecacheOffsets()
{
	g_nHealthOffset = UTIL_FindOffset("CBasePlayer", "m_iHealth");	
}

void Utils::UTIL_SetHealth(edict_t *pEntity, int Health)
{
	if (!g_nHealthOffset || !g_UseSignatureScanner) return;

	IServerUnknown *pUnknown = (IServerUnknown *)pEntity->GetUnknown();
	if (!pUnknown)	return;

	CBaseEntity *pCBase = pUnknown->GetBaseEntity();
	*(int *)((char *)pCBase + g_nHealthOffset) = Health;
	//pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}
int Utils::UTIL_GetHealth(edict_t *pEntity)
{
	//if (!g_nHealthOffset || !g_UseSignatureScanner) return -1;

	IServerUnknown *pUnknown = (IServerUnknown *)pEntity->GetUnknown();
	if (!pUnknown) return 0;


	CBaseEntity *pCBase = pUnknown->GetBaseEntity();
	return *(int *)((char *)pCBase + g_nHealthOffset);
}
void Utils::CallBackUseSigScannerCvar()
{
	//g_UseSignatureScanner = g_BATCore.GetBATVar().GetUseSigScanner();
}

int Utils::UTIL_FindOffset(char *ClassName, char *Property)
{	
	ServerClass *sc = UTIL_FindServerClass(ClassName);
	if(!sc)	return 0;

	SendProp *pProp = UTIL_FindSendProp(sc->m_pTable, Property);
	if(!pProp)	return 0;

	int NewOffset = pProp->GetOffset();
	return NewOffset;
}
void Utils::UTIL_DumpOffsets()
{
	FILE *fp = fopen("offsets.txt","w");
	if (!fp) return;

	ServerClass *sc = g_ForgiveTKCore.GetServerGameDll()->GetAllServerClasses();
	while (sc)
	{
		int NumProps = sc->m_pTable->GetNumProps();
		fprintf(fp, "%s\n", sc->m_pTable->GetName());
		for (int i = 0; i < NumProps; i++)
		{
			SendProp *prop = sc->m_pTable->GetProp(i);
			if (prop)
				fprintf(fp, "  %5d %s\n", prop->GetOffset() / 4, prop->GetName());
		}
		sc = sc->m_pNext;
	}
	fclose(fp);
} 


/**
* Searches for a named Server Class.
*
* @param name		Name of the top-level server class.
* @return 		Server class matching the name, or NULL if none found.
*/
ServerClass *Utils::UTIL_FindServerClass(const char *name)
{
	ServerClass *pClass = g_ForgiveTKCore.GetServerGameDll()->GetAllServerClasses();
	while (pClass)
	{
		if (strcmp(pClass->m_pNetworkName, name) == 0)
		{
			return pClass;
		}
		pClass = pClass->m_pNext;
	}
	return NULL;
}
/**
* Recursively looks through a send table for a given named property.
*
* @param pTable	Send table to browse.
* @param name		Property to search for.
* @return 		SendProp pointer on success, NULL on failure.
*/
SendProp *Utils::UTIL_FindSendProp(SendTable *pTable, const char *name)
{
	int count = pTable->GetNumProps();
	SendProp *pProp;
	for (int i=0; i<count; i++)
	{
		pProp = pTable->GetProp(i);
		if (strcmp(pProp->GetName(), name) == 0)
		{
			return pProp;
		}
		if (pProp->GetDataTable())
		{
			if ((pProp=UTIL_FindSendProp(pProp->GetDataTable(), name)) != NULL)
			{
				return pProp;
			}
		}
	}

	return NULL;
}
