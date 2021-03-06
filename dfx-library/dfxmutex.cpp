/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2011  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our mutex shit.
------------------------------------------------------------------------*/

#include "dfxmutex.h"



//------------------------------------------------------------------------
#if _WIN32 && !defined(PLUGIN_SDK_BUILD)

DfxMutex::DfxMutex()
{
	InitializeCriticalSection(&cs);
}

DfxMutex::~DfxMutex()
{
	DeleteCriticalSection(&cs);
}

int DfxMutex::grab()
{
	EnterCriticalSection(&cs);
	return 0;
}

int DfxMutex::try_grab()
{
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0400)
	BOOL success = TryEnterCriticalSection(&cs);
	// map the TryEnterCriticalSection result to the sort of error that every other mutex API uses
	return (success == 0) ? -1 : 0;
#else
	return 0;
#endif
}

int DfxMutex::release()
{
	LeaveCriticalSection(&cs);
	return 0;
}



//------------------------------------------------------------------------
#elif _WIN32 && defined(PLUGIN_SDK_BUILD)

DfxMutex::DfxMutex()
{
}

DfxMutex::~DfxMutex()
{
}

int DfxMutex::grab()
{
	return 0;
}

int DfxMutex::try_grab()
{
	return 0;
}

int DfxMutex::release()
{
	return 0;
}



//------------------------------------------------------------------------
#elif TARGET_OS_MAC && !defined(__MACH__)

#include <MacErrors.h>

DfxMutex::DfxMutex()
{
	if ( MPLibraryIsLoaded() )
		createErr = MPCreateCriticalRegion(&mpcr);
	else
		createErr = kMPInsufficientResourcesErr;
}

DfxMutex::~DfxMutex()
{
	if (createErr == noErr)
		deleteErr = MPDeleteCriticalRegion(mpcr);
}

int DfxMutex::grab()
{
	if (createErr == noErr) 
		return MPEnterCriticalRegion(mpcr, kDurationForever);
	else
		return createErr;
}

int DfxMutex::try_grab()
{
	if (createErr == noErr) 
		return MPEnterCriticalRegion(mpcr, kDurationImmediate);
	else
		return createErr;
}

int DfxMutex::release()
{
	if (createErr == noErr) 
		return MPExitCriticalRegion(mpcr);
	else
		return createErr;
}



//------------------------------------------------------------------------
#else

DfxMutex::DfxMutex()
{
	pthread_mutexattr_t pmutAttr, * pmutAttrPtr = NULL;
	int attrInitErr = pthread_mutexattr_init(&pmutAttr);
	if (attrInitErr == 0)
	{
		int attrSetTypeErr = pthread_mutexattr_settype(&pmutAttr, PTHREAD_MUTEX_RECURSIVE);
		if (attrSetTypeErr == 0)
			pmutAttrPtr = &pmutAttr;
	}
	createErr = pthread_mutex_init(&pmut, pmutAttrPtr);
	if (attrInitErr == 0)
		pthread_mutexattr_destroy(&pmutAttr);
}

DfxMutex::~DfxMutex()
{
	deleteErr = pthread_mutex_destroy(&pmut);
}

int DfxMutex::grab()
{
	return pthread_mutex_lock(&pmut);
}

int DfxMutex::try_grab()
{
	return pthread_mutex_trylock(&pmut);
}

int DfxMutex::release()
{
	return pthread_mutex_unlock(&pmut);
}



#endif
// platforms
