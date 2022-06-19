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
    const CHAR *args[] = {"retval", "lpFileName", "dwDesiredAccess", "dwShareMode",
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
    const CHAR* args[] = { "retval", "lpLibFileName" };
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
