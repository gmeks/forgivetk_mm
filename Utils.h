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

#ifndef _INCLUDE_UTILS_H
#define _INCLUDE_UTILS_H

//#include <convar.h>
#include <ISmmPlugin.h>
#include <sourcehook/sourcehook.h>
#include "server_class.h"

class Utils
{
public:
	void UTIL_PrecacheOffsets();

	void UTIL_SetHealth(edict_t *pEntity, int hp);
	int UTIL_GetHealth(edict_t *pEntity);

	static void CallBackUseSigScannerCvar();

private:
	int UTIL_FindOffset(char *ClassName, char *Property);
	SendProp *UTIL_FindSendProp(SendTable *pTable, const char *name);
	ServerClass *UTIL_FindServerClass(const char *name);

	void UTIL_DumpOffsets();

};

struct signature_t
{
	/**
	* @brief Some voodoo.
	*/
	void* allocBase;
	/**
	* @brief Some voodoo.
	*/
	void* memInBase;
	/**
	* @brief Size in memory.
	*/
	size_t memSize;
	/**
	* @brief Memory offset.
	*/
	const void *offset;
	/**
	* @brief Signature.
	*/
	const char* sig;
	/**
	* @brief Length of signature.
	*/
	size_t siglen;
};

/**
* @brief Finds data and functions in memory and binaries.
*
* This is mostly by BAILOPAN and is from his CS:S DM plugin. I (dackz)
* have made some changes to it. All functions are static now and
* I've added ResolveSymbol() (which you shouldn't use).
*/
class CFuncFinder
{
public:
	/**
	* @brief Finds data or a function in memory.
	* @param memInBase Some voodoo.
	* @param pattern Signature to search for. 0x2a is a wildcard.
	* @param siglen Length of signature in bytes.
	*/
	static void* ResolveSig(void* memInBase, const char* pattern, size_t siglen);
	/**
	* @brief Finds a symbol in an ELF binary (Linux only).
	* @param path Path to binary.
	* @param name Symbol name.
	*/
	static void* ResolveSymbol(const char* path, const char* name);
private:
	CFuncFinder();
	~CFuncFinder();
	static bool ResolveAddress(signature_t* sigmem);
};


#endif //_INCLUDE_CVARS_H
