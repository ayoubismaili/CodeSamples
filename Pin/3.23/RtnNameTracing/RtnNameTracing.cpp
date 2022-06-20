/*
 * Copyright (C) 2022 Ayoub Ismaili.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include "pin.H"

FILE* traceF;

// This function is called before every CreateFileA routine is executed
// and prints the Arguments if possible

VOID Routine_CreateFileA(ADDRINT ip, ADDRINT sp)
{
    ADDRINT value[8];
    CHAR lpFileName[50];
    const CHAR *args[] = {"retAddress", "lpFileName", "dwDesiredAccess", "dwShareMode",
        "lpSecurityAttributes", "dwCreationDisposition", "dwFlagsAndAttributes", "hTemplateFile"};
    std::string rtnName = RTN_FindNameByAddress(ip);
    //Zero Memory
    memset(value, 0, sizeof(value));
    memset(lpFileName, 0, sizeof(lpFileName));
    //Read value pointed by SP
    PIN_SafeCopy(value, (VOID*)sp, sizeof(value));
    PIN_SafeCopy(lpFileName, (VOID*)value[1], sizeof(lpFileName)-1);
    if (!rtnName.empty())
    {
        fprintf(traceF, "ip=%p, rtn_name=%s, sp=%p\n", ip, rtnName.c_str(), sp);
        for (int i = 0; i < 8; i++)
        {
            if (i == 1) //lpFileName
            {
                fprintf(traceF, "  sp[%d]=%p (%s) = %s\n", i, value[i], args[i], lpFileName);
            }
            else
            {
                fprintf(traceF, "  sp[%d]=%p (%s)\n", i, value[i], args[i]);
            }
        }
    }
}

VOID Routine_LoadLibraryA(ADDRINT ip, ADDRINT sp)
{
    ADDRINT value[2];
    CHAR lpLibFileName[50];
    const CHAR* args[] = { "retAddress", "lpLibFileName" };
    std::string rtnName = RTN_FindNameByAddress(ip);
    //Zero Memory
    memset(value, 0, sizeof(value));
    memset(lpLibFileName, 0, sizeof(lpLibFileName));
    //Read value pointed by SP
    PIN_SafeCopy(value, (VOID*)sp, sizeof(value));
    PIN_SafeCopy(lpLibFileName, (VOID*)value[1], sizeof(lpLibFileName)-1);
    if (!rtnName.empty())
    {
        fprintf(traceF, "ip=%p, rtn_name=%s, sp=%p\n", ip, rtnName.c_str(), sp);
        for (int i = 0; i < 2; i++)
        {
            if (i == 1) //lpLibFileName
            {
                fprintf(traceF, "  sp[%d]=%p (%s) = %s\n", i, value[i], args[i], lpLibFileName);
            }
            else
            {
                fprintf(traceF, "  sp[%d]=%p (%s)\n", i, value[i], args[i]);
            }
        }
    }
}

VOID Routine_lstrlenA(ADDRINT ip, ADDRINT sp)
{
    ADDRINT value[2];
    CHAR lpString[50];
    const CHAR* args[] = { "retAddress", "lpString" };
    std::string rtnName = RTN_FindNameByAddress(ip);
    //Zero Memory
    memset(value, 0, sizeof(value));
    memset(lpString, 0, sizeof(lpString));
    //Read value pointed by SP
    PIN_SafeCopy(value, (VOID*)sp, sizeof(value));
    PIN_SafeCopy(lpString, (VOID*)value[1], sizeof(lpString) - 1);
    if (!rtnName.empty())
    {
        fprintf(traceF, "ip=%p, rtn_name=%s, sp=%p\n", ip, rtnName.c_str(), sp);
        for (int i = 0; i < 2; i++)
        {
            if (i == 1) //lpString
            {
                fprintf(traceF, "  sp[%d]=%p (%s) = %s\n", i, value[i], args[i], lpString);
            }
            else
            {
                fprintf(traceF, "  sp[%d]=%p (%s)\n", i, value[i], args[i]);
            }
        }
    }
}

VOID Routine_RegOpenKeyExA(ADDRINT ip, ADDRINT sp)
{
    ADDRINT value[6];
    CHAR lpSubKey[50];
    const CHAR* args[] = { "retAddress", "hKey", "lpSubKey", "ulOptions", "samDesired", "phkResult"};
    std::string rtnName = RTN_FindNameByAddress(ip);
    //Zero Memory
    memset(value, 0, sizeof(value));
    memset(lpSubKey, 0, sizeof(lpSubKey));
    //Read value pointed by SP
    PIN_SafeCopy(value, (VOID*)sp, sizeof(value));
    PIN_SafeCopy(lpSubKey, (VOID*)value[2], sizeof(lpSubKey) - 1);
    if (!rtnName.empty())
    {
        fprintf(traceF, "ip=%p, rtn_name=%s, sp=%p\n", ip, rtnName.c_str(), sp);
        for (int i = 0; i < 6; i++)
        {
            if (i == 2) //lpSubKey
            {
                fprintf(traceF, "  sp[%d]=%p (%s) = %s\n", i, value[i], args[i], lpSubKey);
            }
            else
            {
                fprintf(traceF, "  sp[%d]=%p (%s)\n", i, value[i], args[i]);
            }
        }
    }
}

VOID Routine_WriteFile(ADDRINT ip, ADDRINT sp)
{
    ADDRINT value[6];
    //CHAR lpSubKey[50];
    const CHAR* args[] = { "retAddress", "hFile", "lpBuffer", "nNumberOfBytesToWrite", "lpNumberOfBytesWritten", "lpOverlapped" };
    std::string rtnName = RTN_FindNameByAddress(ip);
    //Zero Memory
    memset(value, 0, sizeof(value));
    //memset(lpSubKey, 0, sizeof(lpSubKey));
    //Read value pointed by SP
    PIN_SafeCopy(value, (VOID*)sp, sizeof(value));
    //PIN_SafeCopy(lpSubKey, (VOID*)value[2], sizeof(lpSubKey) - 1);
    if (!rtnName.empty())
    {
        fprintf(traceF, "ip=%p, rtn_name=%s, sp=%p\n", ip, rtnName.c_str(), sp);
        for (int i = 0; i < 6; i++)
        {
            fprintf(traceF, "  sp[%d]=%p (%s)\n", i, value[i], args[i]);
        }
    }
}

VOID Routine_Unknown(ADDRINT ip, ADDRINT sp)
{
    ADDRINT value[1];
    //CHAR lpSubKey[50];
    const CHAR* args[] = { "retAddress" };
    std::string rtnName = RTN_FindNameByAddress(ip);
    
    //Zero Memory
    memset(value, 0, sizeof(value));
    //memset(lpSubKey, 0, sizeof(lpSubKey));
    //Read value pointed by SP
    PIN_SafeCopy(value, (VOID*)sp, sizeof(value));

    PIN_LockClient();
    IMG img = IMG_FindByAddress(value[0]);
    std::string imgName;
    size_t exist = -1;
    if (IMG_Valid(img))
    {
        exist = IMG_Name(img).find("C:\\Windows\\SysWOW64");
        imgName = IMG_Name(img);
    }
    PIN_UnlockClient();
    if (exist == 0)
    {
        return;
    }

    //PIN_SafeCopy(lpSubKey, (VOID*)value[2], sizeof(lpSubKey) - 1);
    if (!rtnName.empty())
    {
        fprintf(traceF, "img=%s, ip=%p, rtn_name=%s, sp=%p\n", imgName.c_str(), ip, rtnName.c_str(), sp);
        for (int i = 0; i < 1; i++)
        {
            fprintf(traceF, "  sp[%d]=%p (%s)\n", i, value[i], args[i]);
        }
    }
}

// Pin calls this function every time a new trace is encountered
VOID Routine(RTN rtn, VOID* v)
{
    ADDRINT address;
    std::string name;

    address = RTN_Address(rtn);
    name =  RTN_Name(rtn);
    if (RTN_Valid(rtn))
    {
        // Try to insert a call to Routine_CreateFileA before every call to CreateFileA Routine
        if (name == "CreateFileA")
        {
            RTN_Open(rtn);
            RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)Routine_CreateFileA,
                IARG_ADDRINT, address, IARG_REG_VALUE, REG_STACK_PTR, IARG_END);
            RTN_Close(rtn);
        }
        else if (name == "LoadLibraryA")
        {
            RTN_Open(rtn);
            RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)Routine_LoadLibraryA,
                IARG_ADDRINT, address, IARG_REG_VALUE, REG_STACK_PTR, IARG_END);
            RTN_Close(rtn);
        }
        else if (name == "lstrlenA")
        {
            RTN_Open(rtn);
            RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)Routine_lstrlenA,
                IARG_ADDRINT, address, IARG_REG_VALUE, REG_STACK_PTR, IARG_END);
            RTN_Close(rtn);
        }
        else if (name == "RegOpenKeyExA")
        {
            RTN_Open(rtn);
            RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)Routine_RegOpenKeyExA,
                IARG_ADDRINT, address, IARG_REG_VALUE, REG_STACK_PTR, IARG_END);
            RTN_Close(rtn);
        }
        else if (name == "WriteFile")
        {
            RTN_Open(rtn);
            RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)Routine_WriteFile,
                IARG_ADDRINT, address, IARG_REG_VALUE, REG_STACK_PTR, IARG_END);
            RTN_Close(rtn);
        }
        else
        {
            RTN_Open(rtn);
            RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)Routine_Unknown,
                IARG_ADDRINT, address, IARG_REG_VALUE, REG_STACK_PTR, IARG_END);
            RTN_Close(rtn);
        }
    }
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID* v)
{
    fprintf(traceF, "#eof\n");
    fclose(traceF);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints the Routine Name of the trace if available\n" + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char* argv[])
{
    PIN_InitSymbols();

    traceF = fopen("routine_names.txt", "w");

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    // Register Trace to be called to instrument traces
    RTN_AddInstrumentFunction(Routine, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}
