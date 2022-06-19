/*
 * Copyright (C) 2022 Ayoub Ismaili.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include "pin.H"

FILE* trace;

const char WANTED_STRING[] = "you chose.. poorly";

// This function is called before every instruction is executed
// and prints the IP
VOID printip(ADDRINT eip, ADDRINT* ptr, UINT32 img_id) 
{ 
    UINT8 value[sizeof(WANTED_STRING)];
    IMG img = IMG_FindImgById(img_id);
    memset(value, 0, sizeof(value));
    PIN_SafeCopy(value, ptr, sizeof(value) - 1);
    if (strcmp((char*)value, WANTED_STRING) == 0)
    {
        ADDRINT base = IMG_LowAddress(img);
        ADDRINT rva = eip - base;
        fprintf(trace, "img_id=%d -> img_name=%s -> base=%p -> rva=%p -> ptr=%p -> %s\n", img_id, IMG_Name(img).c_str(), base, rva, ptr, value);
        fflush(trace);
    }
}

// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID* v)
{
    // Insert a call to printip before every instruction, and pass it the IP if the ins is mov from memory to register
    if (INS_Opcode(ins) == XED_ICLASS_MOV && INS_IsMemoryRead(ins) && INS_OperandIsReg(ins, 0) && INS_OperandIsMemory(ins, 1))
    {
        IMG img = IMG_FindByAddress(INS_Address(ins));
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)printip, IARG_INST_PTR, IARG_MEMORYREAD_EA, IARG_UINT32, IMG_Id(img), IARG_END);
    }
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID* v)
{
    fprintf(trace, "#eof\n");
    fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints the IPs of the instruction if the register contains a specific value\n" + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char* argv[])
{
    trace = fopen("find_location.txt", "w");

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}
