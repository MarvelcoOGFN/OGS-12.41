#pragma once
#include "framework.h"

namespace PE {
    static void* (*ProcessEventOG)(UObject*, UFunction*, void*);
    void* ProcessEvent(UObject* Obj, UFunction* Function, void* Params)
    {
        if (Function) {

        }

        return ProcessEventOG(Obj, Function, Params);
    }

    void Hook() {
        MH_CreateHook((LPVOID)(ImageBase + 0x02E13BF0), ProcessEvent, (LPVOID*)&ProcessEventOG);

        Log("PE Hooked!");
    }
}