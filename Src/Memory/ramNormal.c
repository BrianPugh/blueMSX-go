/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/ramNormal.c,v $
**
** $Revision: 1.14 $
**
** $Date: 2008-03-30 18:38:42 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#ifdef TARGET_GNW
#include "build/config.h"
#endif

#if !defined(TARGET_GNW) || (defined(TARGET_GNW) &&  defined(ENABLE_EMULATOR_MSX))
#include "ramNormal.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#ifndef TARGET_GNW
#include "DebugDeviceManager.h"
#endif
#include "SaveState.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef TARGET_GNW
#include "gw_malloc.h"
#endif


typedef struct {
    int deviceHandle;
#ifndef TARGET_GNW
    int debugHandle;
#endif
    int slot;
    int sslot;
    int startPage;
    int pages;
    UInt8 *ramData;
} RamNormal;

#ifdef TARGET_GNW
extern UInt8 msxRam_global[0x4000*8];
#endif

static void saveState(void* rmv)
{
    RamNormal *rm = (RamNormal *)rmv;
    SaveState* state = saveStateOpenForWrite("mapperNormalRam");

    saveStateSet(state, "pages", rm->pages);
    saveStateSetBuffer(state, "ramData", rm->ramData, rm->pages * 0x2000);

    saveStateClose(state);
}

static void loadState(void* rmv)
{
    RamNormal *rm = (RamNormal *)rmv;
    SaveState* state = saveStateOpenForRead("mapperNormalRam");
    int i;

    rm->pages = saveStateGet(state, "pages", 0);
    
    saveStateGetBuffer(state, "ramData", rm->ramData, rm->pages * 0x2000);

    saveStateClose(state);

    for (i = 0; i < rm->pages; i++) {
//        slotMapPage(rm->slot, rm->sslot, i + rm->startPage, rm->ramData + 0x2000 * i, 1, 1);
    }
}

static void destroy(void* rmv)
{
    RamNormal *rm = (RamNormal *)rmv;
#ifndef TARGET_GNW
    debugDeviceUnregister(rm->debugHandle);
#endif

    slotUnregister(rm->slot, rm->sslot, 0);
    deviceManagerUnregister(rm->deviceHandle);

#ifndef MSX_NO_MALLOC
    free(rm->ramData);
    free(rm);
#endif
}

#ifndef TARGET_GNW
static void getDebugInfo(RamNormal* rm, DbgDevice* dbgDevice)
{
    dbgDeviceAddMemoryBlock(dbgDevice, langDbgMemRamNormal(), 0, 0, rm->pages * 0x2000, rm->ramData);
}

static int dbgWriteMemory(RamNormal* rm, char* name, void* data, int start, int size)
{
    if (strcmp(name, "Normal") || start + size > rm->pages * 0x2000) {
        return 0;
    }

    memcpy(rm->ramData + start, data, size);

    return 1;
}
#endif

int ramNormalCreate(int size, int slot, int sslot, int startPage, UInt8** ramPtr, UInt32* ramSize) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
#ifndef TARGET_GNW
    DebugCallbacks dbgCallbacks = { getDebugInfo, dbgWriteMemory, NULL, NULL };
#endif
    RamNormal* rm;
    int pages = size / 0x2000;
    int i;

    if (size > 0x10000 || (size & 0x1fff)) {
        return 0;
    }

    // Start page must be zero (only full slot allowed)
    if (startPage + pages > 8) {
        return 0;
    }

#ifndef TARGET_GNW
    rm = malloc(sizeof(RamNormal));
#else
    rm = itc_malloc(sizeof(RamNormal));
#endif

    rm->slot      = slot;
    rm->sslot     = sslot;
    rm->startPage = startPage;
    rm->pages     = pages;

    size = pages * 0x4000;
#ifndef TARGET_GNW
    rm->ramData = malloc(size);
#else
    rm->ramData = msxRam_global;
#endif

    memset(rm->ramData, 0xff, size);

#ifndef TARGET_GNW
    rm->debugHandle = debugDeviceRegister(DBGTYPE_RAM, langDbgDevRam(), &dbgCallbacks, rm);
#endif

    for (i = 0; i < pages; i++) {
        slotMapPage(slot, sslot, i + startPage, rm->ramData + 0x2000 * i, 1, 1);
    }

    rm->deviceHandle = deviceManagerRegister(RAM_NORMAL, &callbacks, rm);
    slotRegister(slot, sslot, startPage, pages, NULL, NULL, NULL, destroy, rm);

    if (ramPtr != NULL) {
        *ramPtr = rm->ramData;
    }

    if (ramSize != NULL) {
        *ramSize = pages * 0x2000;
    }

    return 1;
}
#endif
