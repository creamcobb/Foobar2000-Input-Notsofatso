/*
  Notsofatso, a nintendo sound format player derived from in_notsofatso
  Copyright (C) 2004 Disch
  Copyright (C) 2015 Wilbert Lee

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <windows.h>
#include "CNSFCore.h"

//////////////////////////////////////////////////////////////////////////
// Memory reading/writing and other defines

#define Zp(a) pRAM[a] // Reads zero page memory
#define ZpWord(a) (Zp(a) | (Zp((BYTE)(a + 1)) << 8)) // Reads zero page memory in word form

#define Rd(a) ((this->*ReadMemory[((WORD)(a)) >> 12])(a)) // Reads memory
#define RdWord(a) (Rd(a) | (Rd(a + 1) << 8)) // Reads memory in word form

#define Wr(a,v) ((this->*WriteMemory[((WORD)(a)) >> 12])(a,v)) // Writes memory
#define WrZ(a,v) pRAM[a] = v // Writes zero paged memory

#define PUSH(v) pStack[SP--] = v // Pushes a value onto the stack
#define PULL(v) v = pStack[++SP] // Pulls a value from the stack

//////////////////////////////////////////////////////////////////////////
// Addressing Modes

// First set - gets the value that's being addressed
#define Ad_VlIm() val = Rd(PC.W); PC.W++ // Immediate
#define Ad_VlZp() final.W = Rd(PC.W); val = Zp(final.W); PC.W++ // Zero Page
#define Ad_VlZx() front.W = final.W = Rd(PC.W); final.B.l += X;  \
 val = Zp(final.B.l); PC.W++ // Zero Page, X
#define Ad_VlZy() front.W = final.W = Rd(PC.W); final.B.l += Y;  \
 val = Zp(final.B.l); PC.W++ // Zero Page, Y
#define Ad_VlAb() final.W = RdWord(PC.W); val = Rd(final.W); PC.W += 2 // Absolute
#define Ad_VlAx() front.W = final.W = RdWord(PC.W); final.W += X; PC.W += 2; \
 if(front.B.h != final.B.h) nCPUCycle++; val = Rd(final.W) // Absolute, X [uses extra cycle if crossed page]
#define Ad_VlAy() front.W = final.W = RdWord(PC.W); final.W += Y; PC.W += 2; \
 if(front.B.h != final.B.h) nCPUCycle++; val = Rd(final.W) // Absolute, X [uses extra cycle if crossed page]
#define Ad_VlIx() front.W = final.W = Rd(PC.W); final.B.l += X; PC.W++; \
 final.W = ZpWord(final.B.l); val = Rd(final.W) // (Indirect, X)
#define Ad_VlIy() val = Rd(PC.W); front.W = final.W = ZpWord(val); PC.W++;\
 final.W += Y; if(final.B.h != front.B.h) nCPUCycle++;  \
 front.W = val; val = Rd(final.W) // (Indirect), Y [uses extra cycle if crossed page]

// Second set - gets the ADDRESS that the mode is referring to (for operators that write to memory)
// Note that AbsoluteX, AbsoluteY, and IndirectY modes do NOT check for page boundary crossing here
// Since that extra cycle isn't added for operators that write to memory (it only applies to ones that
// Only read from memory.. in which case the 1st set should be used)
#define Ad_AdZp() final.W = Rd(PC.W); PC.W++ // Zero Page
#define Ad_AdZx() final.W = front.W = Rd(PC.W); final.B.l += X; PC.W++ // Zero Page, X
#define Ad_AdZy() final.W = front.W = Rd(PC.W); final.B.l += Y; PC.W++ // Zero Page, Y
#define Ad_AdAb() final.W = RdWord(PC.W); PC.W += 2 // Absolute
#define Ad_AdAx() front.W = final.W = RdWord(PC.W); PC.W += 2;  \
 final.W += X // Absolute, X
#define Ad_AdAy() front.W = final.W = RdWord(PC.W); PC.W += 2;  \
 final.W += Y // Absolute, Y
#define Ad_AdIx() front.W = final.W = Rd(PC.W); PC.W++; final.B.l += X; \
 final.W = ZpWord(final.B.l) // (Indirect, X)
#define Ad_AdIy() front.W = Rd(PC.W); final.W = ZpWord(front.W) + Y;  \
 PC.W++ // (Indirect), Y

// Third set - reads memory, performs the desired operation on the value, then writes back to memory
// Used for operators that directly change memory (ASL, INC, DEC, etc)
#define MRW_Zp(cmd) Ad_AdZp(); val = Zp(final.W); cmd(val); WrZ(final.W,val) // Zero Page
#define MRW_Zx(cmd) Ad_AdZx(); val = Zp(final.W); cmd(val); WrZ(final.W,val) // Zero Page, X
#define MRW_Zy(cmd) Ad_AdZy(); val = Zp(final.W); cmd(val); WrZ(final.W,val) // Zero Page, Y
#define MRW_Ab(cmd) Ad_AdAb(); val = Rd(final.W); cmd(val); Wr(final.W,val) // Absolute
#define MRW_Ax(cmd) Ad_AdAx(); val = Rd(final.W); cmd(val); Wr(final.W,val) // Absolute, X
#define MRW_Ay(cmd) Ad_AdAy(); val = Rd(final.W); cmd(val); Wr(final.W,val) // Absolute, Y
#define MRW_Ix(cmd) Ad_AdIx(); val = Rd(final.W); cmd(val); Wr(final.W,val) // (Indirect, X)
#define MRW_Iy(cmd) Ad_AdIy(); val = Rd(final.W); cmd(val); Wr(final.W,val) // (Indirect), Y

// Relative modes are special in that they're only used by branch commands
// This macro handles the jump, and should only be called if the branch condition was true
// If the branch condition was false, the PC must be incrimented

#define RelJmp(cond) val = Rd(PC.W); PC.W++; final.W = PC.W + (char)(val);  \
  if(cond) { nCPUCycle += ((final.B.h != PC.B.h) ? 2 : 1); PC.W = final.W; }



//////////////////////////////////////////////////////////////////////////
// Status Flags

#define C_FLAG  0x01 // Carry flag
#define Z_FLAG  0x02 // Zero flag
#define I_FLAG  0x04 // Mask interrupt flag
#define D_FLAG  0x08 // Decimal flag (decimal mode is unsupported on NES)
#define B_FLAG  0x10 // Break flag (not really in the status register!  It's value in ST is never used.  When ST is put in memory (by an interrupt or PHP), this flag is set only if BRK was called) ** also when PHP is called due to a bug
#define R_FLAG  0x20 // Reserved flag (not really in the register.  It's value is never used.  Whenever ST is put in memory, this flag is always set)
#define V_FLAG  0x40 // Overflow flag
#define N_FLAG  0x80 // Sign flag


//////////////////////////////////////////////////////////////////////////
// Lookup Tables

static const BYTE CPU_Cycles[0x100] = { // The number of CPU cycles used for each instruction
  7, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  6, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
  2, 6, 0, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
  2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
  2, 5, 0, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
  2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7 };


static const BYTE NZTable[0x100] = { // The status of the NZ flags for the given value
  Z_FLAG, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG,
  N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG,
  N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG,
  N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG,
  N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG,
  N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG,
  N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG,
  N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG, N_FLAG };

// A quick macro for working with the above table
#define UpdateNZ(v) ST = (ST & ~(N_FLAG|Z_FLAG)) | NZTable[v]


//////////////////////////////////////////////////////////////////////////
// Opcodes
//
// These opcodes perform the action with the given value (changing that value
// if necessary).  Registers and flags associated with the operation are
// changed accordingly.  There are a few exceptions which will be noted when they arise


/* ADC
  Adds the value to the accumulator with carry
  Changes: A, NVZC
  - Decimal mode not supported on the NES
  - Due to a bug, NVZ flags are not altered if the Decimal flag is on --(taken out)-- */
#define ADC()  \
 tw.W = A + val + (ST & C_FLAG);  \
 ST = (ST & (I_FLAG|D_FLAG)) | tw.B.h | NZTable[tw.B.l] |  \
  ( (0x80 & ~(A ^ val) & (A ^ tw.B.l)) ? V_FLAG : 0 );  \
 A = tw.B.l

/* AND
Combines the value with the accumulator using a bitwise AND operation
Changes: A, NZ */
#define AND()  \
 A &= val;  \
 UpdateNZ(A)

/* ASL
Left shifts the value 1 bit.  The bit that gets shifted out goes to
the carry flag.
Changes: value, NZC */
#define ASL(value)  \
 tw.W = value << 1;   \
 ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | tw.B.h | NZTable[tw.B.l];  \
 value = tw.B.l

/* BIT
 Compares memory with the accumulator with an AND operation, but changes neither.
 The two high bits of memory get transferred to the status reg
 Z is set if the AND operation yielded zero, otherwise it's cleared
 Changes: NVZ */
#define BIT()  \
 ST = (ST & ~(N_FLAG|V_FLAG|Z_FLAG)) | (val & (N_FLAG|V_FLAG)) |  \
  ((A & val) ? 0 : Z_FLAG)

/* CMP, CPX, CPY
Compares memory with the given register with a subtraction operation.
Flags are set accordingly depending on the result:
Reg < Memory: Z=0, C=0
Reg = Memory: Z=1, C=1
Reg > Memory: Z=0, C=1
N is set according to the result of the subtraction operation
Changes: NZC

NOTE -- CMP, CPX, CPY all share this same routine, so the desired register
(A, X, or Y respectively) must be given when calling this macro... as well
as the memory to compare it with. */
#define CMP(reg)  \
 tw.W = reg - val;   \
 ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | (tw.B.h ? 0 : C_FLAG) |  \
  NZTable[tw.B.l]

/* DEC, DEX, DEY
Decriments a value by one.
Changes: value, NZ */
#define DEC(value)  \
 value--;  \
 UpdateNZ(value)

/* EOR
Combines a value with the accumulator using a bitwise exclusive-OR operation
Changes: A, NZ */
#define EOR()  \
 A ^= val;  \
 UpdateNZ(A)

/* INC, INX, INY
 Incriments a value by one.
 Changes: value, NZ */
#define INC(value)  \
 value++;  \
 UpdateNZ(value)

/* LSR
Shifts value one bit to the right.  Bit that gets shifted out goes to the
Carry flag.
Changes: value, NZC  */
#define LSR(value)  \
 tw.W = value >> 1;   \
 ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[tw.B.l] | \
  (value & 0x01);   \
 value = tw.B.l

/* ORA
 Combines a value with the accumulator using a bitwise inclusive-OR operation
 Changes: A, NZ */
#define ORA()  \
 A |= val;  \
 UpdateNZ(A)

/* ROL
Rotates a value one bit to the left:
C <-  7<-6<-5<-4<-3<-2<-1<-0  <- C
Changes: value, NZC  */
#define ROL(value)  \
 tw.W = (value << 1) | (ST & 0x01);  \
 ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[tw.B.l] | tw.B.h;  \
 value = tw.B.l

/* ROR
Rotates a value one bit to the right:
C ->  7->6->5->4->3->2->1->0  -> C
Changes: value, NZC  */
#define ROR(value)  \
 tw.W = (value >> 1) | (ST << 7);  \
 ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[tw.B.l] | \
  (value & 0x01);   \
 value = tw.B.l

/* SBC
Subtracts a value from the accumulator with borrow (inverted carry)
Changes: A, NVZC
- Decimal mode not supported on the NES
- Due to a bug, NVZ flags are not altered if the Decimal flag is on --(taken out)-- */
#define SBC()  \
 tw.W = A - val - ((ST & C_FLAG) ? 0 : 1);  \
 ST = (ST & (I_FLAG|D_FLAG)) | (tw.B.h ? 0 : C_FLAG) | NZTable[tw.B.l] | \
 (((A ^ val) & (A ^ tw.B.l) & 0x80) ? V_FLAG : 0);  \
 A = tw.B.l

//////////////////////////////////////////////////////////////////////////
// Undocumented Opcodes
//
// These opcodes are not included in the official specifications.  However,
// some of the unused opcode values perform operations which have since been
// documented.


/* ASO
 Left shifts a value, then ORs the result with the accumulator
 Changes: value, A, NZC  */
#define ASO(value)  \
 tw.W = value << 1;   \
 A |= tw.B.l;  \
 ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[A] | tw.B.h;  \
 value = tw.B.l

/* RLA
Roll memory left 1 bit, then AND the result with the accumulator
Changes: value, A, NZC  */
#define RLA(value)  \
 tw.W = (value << 1) | (ST & 0x01);  \
 A &= tw.B.l;  \
 ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[A] | tw.B.h;  \
 value = tw.B.l

/* LSE
Right shifts a value one bit, then EORs the result with the accumulator
Changes: value, A, NZC  */
#define LSE(value)  \
 tw.W = value >> 1;   \
 A ^= tw.B.l;  \
 ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[A] | (value & 0x01); \
 value = tw.B.l

/* RRA
Roll memory right one bit, then ADC the result
Changes: value, A, NVZC */
#define RRA(value)  \
 tw.W = (value >> 1) | (ST << 7);  \
 ST = (ST & ~C_FLAG) | (value & 0x01);  \
 value = tw.B.l;  \
 ADC()

/* AXS
ANDs the contents of the X and A registers and stores the result
int memory.
Changes: value  [DOES NOT CHANGE X, A, or any flags]  */
#define AXS(value)  \
 value = A & X

/* DCM
 Decriments a value and compares it with the A register.
 Changes: value, NZC  */
#define DCM(value)  \
 value--;  \
 CMP(A)

/* INS
Incriments a value then SBCs it
Changes: value, A, NVZC */
#define INS(value)  \
 value++;  \
 SBC()

/* AXA */
#define AXA(value)  \
 value = A & X & (Rd(PC.W - 1) + 1)


//////////////////////////////////////////////////////////////////////////
//
// The 6502 emulation function!
//
//

TWIN front;
TWIN final;
BYTE val;
BYTE op;

UINT CNSFCore::Emulate6502(UINT runto)
{
  /////////////////////////////////////////
  // If the CPU is jammed... don't bother
  if (bCPUJammed == 1)
    return 0;

  register TWIN tw; // Used in calculations
  register TWIN PC;
  register BYTE SP = regSP;
  register BYTE ST = regP;
  register BYTE A = regA;
  register BYTE X = regX;
  register BYTE Y = regY;
  TWIN front;
  TWIN final;
  PC.W = regPC;

  UINT ret = nCPUCycle;

  ////////////////////
  // Start the loop

  while (nCPUCycle < runto)
  {
    op = Rd(PC.W);
    PC.W++;

    nCPUCycle += CPU_Cycles[op];

    switch (op)
    {
      //////////////////////////////////////////////////////////////////////////
      // Documented Opcodes first

      //////////////////////////////////////////////////////////////////////////
      // Flag setting/clearing
    case 0x18: ST &= ~C_FLAG; continue; /* CLC */
    case 0x38: ST |= C_FLAG; continue; /* SEC */
    case 0x58: ST &= ~I_FLAG; continue; /* CLI */
    case 0x78: ST |= I_FLAG; continue; /* SEI */
    case 0xB8: ST &= ~V_FLAG; continue; /* CLV */
    case 0xD8: ST &= ~D_FLAG; continue; /* CLD */
    case 0xF8: ST |= D_FLAG; continue; /* SED */
    }

    switch (op)
    {
      //////////////////////////////////////////////////////////////////////////
      // Branch commands
    case 0x10: RelJmp(!(ST & N_FLAG)); continue; /* BPL */
    case 0x30: RelJmp((ST & N_FLAG)); continue; /* BMI */
    case 0x50: RelJmp(!(ST & V_FLAG)); continue; /* BVC */
    case 0x70: RelJmp((ST & V_FLAG)); continue; /* BVS */
    case 0x90: RelJmp(!(ST & C_FLAG)); continue; /* BCC */
    case 0xB0: RelJmp((ST & C_FLAG)); continue; /* BCS */
    case 0xD0: RelJmp(!(ST & Z_FLAG)); continue; /* BNE */
    case 0xF0: RelJmp((ST & Z_FLAG)); continue; /* BEQ */
    }

    switch (op)
    {
      //////////////////////////////////////////////////////////////////////////
      // Direct stack alteration commands (push/pull commands)
    case 0x08: PUSH(ST | R_FLAG | B_FLAG);  continue; /* PHP */
    case 0x28: PULL(ST);  continue; /* PLP */
    case 0x48: PUSH(A);  continue; /* PHA */
    case 0x68: PULL(A); UpdateNZ(A);  continue; /* PLA */
    }

    switch (op)
    {
      //////////////////////////////////////////////////////////////////////////
      // Register Transfers
    case 0x8A: A = X; UpdateNZ(A);  continue; /* TXA */
    case 0x98: A = Y; UpdateNZ(A);  continue; /* TYA */
    case 0x9A: SP = X;  continue; /* TXS */
    case 0xA8: Y = A; UpdateNZ(A);  continue; /* TAY */
    case 0xAA: X = A; UpdateNZ(A);  continue; /* TAX */
    case 0xBA: X = SP; UpdateNZ(X);  continue; /* TSX */
    }
    //////////////////////////////////////////////////////////////////////////
    // Other commands
    switch (op)
    {
      /* ADC */
    case 0x61: Ad_VlIx(); ADC(); continue;
    case 0x65: Ad_VlZp(); ADC(); continue;
    case 0x69: Ad_VlIm(); ADC(); continue;
    case 0x6D: Ad_VlAb(); ADC(); continue;
    case 0x71: Ad_VlIy(); ADC(); continue;
    case 0x75: Ad_VlZx(); ADC(); continue;
    case 0x79: Ad_VlAy(); ADC(); continue;
    case 0x7D: Ad_VlAx(); ADC(); continue;
    }

    switch (op)
    {
      /* AND */
    case 0x21: Ad_VlIx(); AND(); continue;
    case 0x25: Ad_VlZp(); AND(); continue;
    case 0x29: Ad_VlIm(); AND(); continue;
    case 0x2D: Ad_VlAb(); AND(); continue;
    case 0x31: Ad_VlIy(); AND(); continue;
    case 0x35: Ad_VlZx(); AND(); continue;
    case 0x39: Ad_VlAy(); AND(); continue;
    case 0x3D: Ad_VlAx(); AND(); continue;
    }

    switch (op)
    {
      /* ASL */
    case 0x0A: ASL(A);  continue;
    case 0x06: MRW_Zp(ASL); continue;
    case 0x0E: MRW_Ab(ASL); continue;
    case 0x16: MRW_Zx(ASL); continue;
    case 0x1E: MRW_Ax(ASL); continue;
    }

    switch (op)
    {
      /* BIT */
    case 0x24: Ad_VlZp(); BIT(); continue;
    case 0x2C: Ad_VlAb(); BIT(); continue;
    }

    switch (op)
    {
      /* BRK */
    case 0x00:
      if (bIgnoreBRK)
        continue;
      PC.W++; // BRK has a padding byte
      PUSH(PC.B.h); // Push high byte of the return address
      PUSH(PC.B.l); // Push low byte of return address
      PUSH(ST | R_FLAG | B_FLAG); // Push processor status with R|B flags
      ST |= I_FLAG; // Mask interrupts
      PC.W = RdWord(0xFFFE); // Read the IRQ vector and jump to it

      // Extra check to make sure we didn't hit an infinite BRK loop
      if (!Rd(PC.W)) // Next command will be BRK
      {
        bCPUJammed = 1; // The CPU will endlessly loop... just just jam it to ease processing power
        goto jammed;
      }
      continue;
    }

    switch (op)
    {
      /* CMP */
    case 0xC1: Ad_VlIx(); CMP(A); continue;
    case 0xC5: Ad_VlZp(); CMP(A); continue;
    case 0xC9: Ad_VlIm(); CMP(A); continue;
    case 0xCD: Ad_VlAb(); CMP(A); continue;
    case 0xD1: Ad_VlIy(); CMP(A); continue;
    case 0xD5: Ad_VlZx(); CMP(A); continue;
    case 0xD9: Ad_VlAy(); CMP(A); continue;
    case 0xDD: Ad_VlAx(); CMP(A); continue;
    }

    switch (op)
    {
      /* CPX */
    case 0xE0: Ad_VlIm(); CMP(X); continue;
    case 0xE4: Ad_VlZp(); CMP(X); continue;
    case 0xEC: Ad_VlAb(); CMP(X); continue;
    }

    switch (op)
    {
      /* CPY */
    case 0xC0: Ad_VlIm(); CMP(Y); continue;
    case 0xC4: Ad_VlZp(); CMP(Y); continue;
    case 0xCC: Ad_VlAb(); CMP(Y); continue;
    }

    switch (op)
    {
      /* DEC */
    case 0xCA: DEC(X);  continue;  /* DEX */
    case 0x88: DEC(Y);  continue;  /* DEY */
    case 0xC6: MRW_Zp(DEC); continue;
    case 0xCE: MRW_Ab(DEC); continue;
    case 0xD6: MRW_Zx(DEC); continue;
    case 0xDE: MRW_Ax(DEC); continue;
    }

    switch (op)
    {
      /* EOR */
    case 0x41: Ad_VlIx(); EOR(); continue;
    case 0x45: Ad_VlZp(); EOR(); continue;
    case 0x49: Ad_VlIm(); EOR(); continue;
    case 0x4D: Ad_VlAb(); EOR(); continue;
    case 0x51: Ad_VlIy(); EOR(); continue;
    case 0x55: Ad_VlZx(); EOR(); continue;
    case 0x59: Ad_VlAy(); EOR(); continue;
    case 0x5D: Ad_VlAx(); EOR(); continue;
    }

    switch (op)
    {
      /* INC */
    case 0xE8: INC(X);  continue;  /* INX */
    case 0xC8: INC(Y);  continue;  /* INY */
    case 0xE6: MRW_Zp(INC); continue;
    case 0xEE: MRW_Ab(INC); continue;
    case 0xF6: MRW_Zx(INC); continue;
    case 0xFE: MRW_Ax(INC); continue;
    }

    switch (op)
    {
      /* JMP */
    case 0x4C: final.W = RdWord(PC.W);  PC.W = final.W; val = 0; continue;  /* Absolute JMP */
    case 0x6C: front.W = final.W = RdWord(PC.W);
      PC.B.l = Rd(final.W); final.B.l++;
      PC.B.h = Rd(final.W); final.W = PC.W;
      continue;  /* Indirect JMP -- must take caution:
      Indirection at 01FF will read from 01FF and 0100 (not 0200) */
      /* JSR */
    case 0x20:
      val = 0;
      final.W = RdWord(PC.W);
      PC.W++; //JSR only incriments the return address by one.  It's incrimented again upon RTS
      PUSH(PC.B.h); //push high byte of return address
      PUSH(PC.B.l); //push low byte of return address
      PC.W = final.W;
      continue;
    }

    switch (op)
    {
      /* LDA */
    case 0xA1: Ad_VlIx(); A = val; UpdateNZ(A); continue;
    case 0xA5: Ad_VlZp(); A = val; UpdateNZ(A); continue;
    case 0xA9: Ad_VlIm(); A = val; UpdateNZ(A); continue;
    case 0xAD: Ad_VlAb(); A = val; UpdateNZ(A); continue;
    case 0xB1: Ad_VlIy(); A = val; UpdateNZ(A); continue;
    case 0xB5: Ad_VlZx(); A = val; UpdateNZ(A); continue;
    case 0xB9: Ad_VlAy(); A = val; UpdateNZ(A); continue;
    case 0xBD: Ad_VlAx(); A = val; UpdateNZ(A); continue;
    }

    switch (op)
    {
      /* LDX */
    case 0xA2: Ad_VlIm(); X = val; UpdateNZ(X); continue;
    case 0xA6: Ad_VlZp(); X = val; UpdateNZ(X); continue;
    case 0xAE: Ad_VlAb(); X = val; UpdateNZ(X); continue;
    case 0xB6: Ad_VlZy(); X = val; UpdateNZ(X); continue;
    case 0xBE: Ad_VlAy(); X = val; UpdateNZ(X); continue;
    }

    switch (op)
    {
      /* LDY */
    case 0xA0: Ad_VlIm(); Y = val; UpdateNZ(Y); continue;
    case 0xA4: Ad_VlZp(); Y = val; UpdateNZ(Y); continue;
    case 0xAC: Ad_VlAb(); Y = val; UpdateNZ(Y); continue;
    case 0xB4: Ad_VlZx(); Y = val; UpdateNZ(Y); continue;
    case 0xBC: Ad_VlAx(); Y = val; UpdateNZ(Y); continue;
    }

    switch (op)
    {
      /* LSR */
    case 0x4A: LSR(A);  continue;
    case 0x46: MRW_Zp(LSR); continue;
    case 0x4E: MRW_Ab(LSR); continue;
    case 0x56: MRW_Zx(LSR); continue;
    case 0x5E: MRW_Ax(LSR); continue;
    }

    switch (op)
    {
      /* NOP */
    case 0xEA:

      /* --- Undocumented ---
      These opcodes perform the same action as NOP */
    case 0x1A: case 0x3A: case 0x5A:
    case 0x7A: case 0xDA: case 0xFA: continue;
    }

    switch (op)
    {
      /* ORA */
    case 0x01: Ad_VlIx(); ORA(); continue;
    case 0x05: Ad_VlZp(); ORA(); continue;
    case 0x09: Ad_VlIm(); ORA(); continue;
    case 0x0D: Ad_VlAb(); ORA(); continue;
    case 0x11: Ad_VlIy(); ORA(); continue;
    case 0x15: Ad_VlZx(); ORA(); continue;
    case 0x19: Ad_VlAy(); ORA(); continue;
    case 0x1D: Ad_VlAx(); ORA(); continue;
    }

    switch (op)
    {
      /* ROL */
    case 0x2A: ROL(A);  continue;
    case 0x26: MRW_Zp(ROL); continue;
    case 0x2E: MRW_Ab(ROL); continue;
    case 0x36: MRW_Zx(ROL); continue;
    case 0x3E: MRW_Ax(ROL); continue;
    }

    switch (op)
    {
      /* ROR */
    case 0x6A: ROR(A);  continue;
    case 0x66: MRW_Zp(ROR); continue;
    case 0x6E: MRW_Ab(ROR); continue;
    case 0x76: MRW_Zx(ROR); continue;
    case 0x7E: MRW_Ax(ROR); continue;
    }

    switch (op)
    {
      /* RTI */
    case 0x40:
      PULL(ST); // Pull processor status
      PULL(PC.B.l); // Pull low byte of return address
      PULL(PC.B.h); // Pull high byte of return address
      continue;
    }

    switch (op)
    {
      /* RTS */
    case 0x60:
      PULL(PC.B.l);
      PULL(PC.B.h);
      PC.W++; // The return address is one less of what it needs
      continue;
    }

    switch (op)
    {
      /* SBC */
    case 0xE1: Ad_VlIx(); SBC(); continue;
    case 0xE5: Ad_VlZp(); SBC(); continue;
    case 0xEB: /* -- Undocumented --  EB performs the same operation as SBC immediate */
    case 0xE9: Ad_VlIm(); SBC(); continue;
    case 0xED: Ad_VlAb(); SBC(); continue;
    case 0xF1: Ad_VlIy(); SBC(); continue;
    case 0xF5: Ad_VlZx(); SBC(); continue;
    case 0xF9: Ad_VlAy(); SBC(); continue;
    case 0xFD: Ad_VlAx(); SBC(); continue;
    }
    switch (op)
    {
      /* STA */
    case 0x81: Ad_AdIx(); val = A; Wr(final.W, A); continue;
    case 0x85: Ad_AdZp(); val = A; WrZ(final.W, A); continue;
    case 0x8D: Ad_AdAb(); val = A; Wr(final.W, A); continue;
    case 0x91: Ad_AdIy(); val = A; Wr(final.W, A); continue;
    case 0x95: Ad_AdZx(); val = A; WrZ(final.W, A); continue;
    case 0x99: Ad_AdAy(); val = A; Wr(final.W, A); continue;
    case 0x9D: Ad_AdAx(); val = A; Wr(final.W, A); continue;
    }
    switch (op)
    {
      /* STX */
    case 0x86: Ad_AdZp(); val = X; WrZ(final.W, X); continue;
    case 0x8E: Ad_AdAb(); val = X; Wr(final.W, X); continue;
    case 0x96: Ad_AdZy(); val = X; WrZ(final.W, X); continue;
    }
    switch (op)
    {
      /* STY */
    case 0x84: Ad_AdZp(); val = Y; WrZ(final.W, Y); continue;
    case 0x8C: Ad_AdAb(); val = Y; Wr(final.W, Y); continue;
    case 0x94: Ad_AdZx(); val = Y; WrZ(final.W, Y); continue;
    }
    //////////////////////////////////////////////////////////////////////////
    // Undocumented Opcodes
    switch (op)
    {
      /* HLT / JAM
      Jams up CPU operation  */
    case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
    case 0x62: case 0x72: case 0x92: case 0xB2: case 0xD2: case 0xF2:
      if (PC.W == 0x5004) bCPUJammed = 2; //it's not -really- jammed... only the NSF code has ended
      else
      {
        if (bIgnoreIllegalOps) continue;
        bCPUJammed = 1;
      }
      goto jammed;

    }
    if (bIgnoreIllegalOps) continue;
    switch (op)
    {
      /* ASO */
    case 0x03: MRW_Ix(ASO); continue;
    case 0x07: MRW_Zp(ASO); continue;
    case 0x0F: MRW_Ab(ASO); continue;
    case 0x13: MRW_Iy(ASO); continue;
    case 0x17: MRW_Zx(ASO); continue;
    case 0x1B: MRW_Ay(ASO); continue;
    case 0x1F: MRW_Ax(ASO); continue;
    }
    switch (op)
    {
      /* RLA */
    case 0x23: MRW_Ix(RLA); continue;
    case 0x27: MRW_Zp(RLA); continue;
    case 0x2F: MRW_Ab(RLA); continue;
    case 0x33: MRW_Iy(RLA); continue;
    case 0x37: MRW_Zx(RLA); continue;
    case 0x3B: MRW_Ay(RLA); continue;
    case 0x3F: MRW_Ax(RLA); continue;
    }
    switch (op)
    {
      /* LSE */
    case 0x43: MRW_Ix(LSE); continue;
    case 0x47: MRW_Zp(LSE); continue;
    case 0x4F: MRW_Ab(LSE); continue;
    case 0x53: MRW_Iy(LSE); continue;
    case 0x57: MRW_Zx(LSE); continue;
    case 0x5B: MRW_Ay(LSE); continue;
    case 0x5F: MRW_Ax(LSE); continue;
    }
    switch (op)
    {
      /* RRA */
    case 0x63: MRW_Ix(RRA); continue;
    case 0x67: MRW_Zp(RRA); continue;
    case 0x6F: MRW_Ab(RRA); continue;
    case 0x73: MRW_Iy(RRA); continue;
    case 0x77: MRW_Zx(RRA); continue;
    case 0x7B: MRW_Ay(RRA); continue;
    case 0x7F: MRW_Ax(RRA); continue;
    }
    switch (op)
    {
      /* AXS */
    case 0x83: MRW_Ix(AXS); continue;
    case 0x87: MRW_Zp(AXS); continue;
    case 0x8F: MRW_Ab(AXS); continue;
    case 0x97: MRW_Zy(AXS); continue;
    }
    switch (op)
    {
      /* LAX */
    case 0xA3: Ad_VlIx(); X = A = val; UpdateNZ(A); continue;
    case 0xA7: Ad_VlZp(); X = A = val; UpdateNZ(A); continue;
    case 0xAF: Ad_VlAb(); X = A = val; UpdateNZ(A); continue;
    case 0xB3: Ad_VlIy(); X = A = val; UpdateNZ(A); continue;
    case 0xB7: Ad_VlZy(); X = A = val; UpdateNZ(A); continue;
    case 0xBF: Ad_VlAy(); X = A = val; UpdateNZ(A); continue;
    }
    switch (op)
    {
      /* DCM */
    case 0xC3: MRW_Ix(DCM); continue;
    case 0xC7: MRW_Zp(DCM); continue;
    case 0xCF: MRW_Ab(DCM); continue;
    case 0xD3: MRW_Iy(DCM); continue;
    case 0xD7: MRW_Zx(DCM); continue;
    case 0xDB: MRW_Ay(DCM); continue;
    case 0xDF: MRW_Ax(DCM); continue;
    }
    switch (op)
    {
      /* INS */
    case 0xE3: MRW_Ix(INS); continue;
    case 0xE7: MRW_Zp(INS); continue;
    case 0xEF: MRW_Ab(INS); continue;
    case 0xF3: MRW_Iy(INS); continue;
    case 0xF7: MRW_Zx(INS); continue;
    case 0xFB: MRW_Ay(INS); continue;
    case 0xFF: MRW_Ax(INS); continue;
    }
    switch (op)
    {
      /* ALR
      AND Accumulator with memory and LSR the result */
    case 0x4B: Ad_VlIm(); A &= val; LSR(A); continue;

      /* ARR
      ANDs memory with the Accumulator and RORs the result */
    case 0x6B: Ad_VlIm(); A &= val; ROR(A); continue;

      /* XAA
      Transfers X -> A, then ANDs A with memory */
    case 0x8B: Ad_VlIm(); A = X & val; UpdateNZ(A); continue;

      /* OAL
      OR the Accumulator with #EE, AND Accumulator with Memory, Transfer A -> X */
    case 0xAB: Ad_VlIm(); X = (A &= (val | 0xEE));
      UpdateNZ(A); continue;

      /* SAX
      ANDs A and X registers (does not change A), subtracts memory from result (CMP style, not SBC style)
      result is stored in X  */
    case 0xCB:
      Ad_VlIm(); tw.W = (X & A) - val; X = tw.B.l;
      ST = (ST & ~(N_FLAG | Z_FLAG | C_FLAG)) | NZTable[X] | (tw.B.h ? C_FLAG : 0); continue;
    }
    switch (op)
    {
      /* SKB
      Skip Byte... or DOP - Double No-Op
      These bytes do nothing, but take a parameter (which can be ignored) */
    case 0x04: case 0x14: case 0x34: case 0x44: case 0x54: case 0x64:
    case 0x80: case 0x82: case 0x89: case 0xC2: case 0xD4: case 0xE2: case 0xF4:

      PC.W++; //skip unused byte
      continue;
    }
    switch (op)
    {
      /* SKW
      Swip Word... or TOP - Tripple No-Op
      These bytes are the same as SKB, only they take a 2 byte parameter.
      This can be ignored in some cases, but the read needs to be performed in a some cases
      Because an extra clock cycle may be used in the process */
    case 0x0C: // Absolute address... no need for operator

      PC.W += 2; continue;
    case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: // Absolute X address... may cross page, have to perform the read

      Ad_VlAx(); continue;
    }

    switch (op)
    {
      /* TAS */
    case 0x9B:

      Ad_AdAy();
      SP = A & X & (Rd(PC.W - 1) + 1);
      Wr(final.W, SP);
      continue;

      /* SAY */
    case 0x9C:

      Ad_AdAx();
      Y &= (Rd(PC.W - 1) + 1);
      Wr(final.W, Y);
      continue;

      /* XAS */
    case 0x9E:

      Ad_AdAy();
      X &= (Rd(PC.W - 1) + 1);
      Wr(final.W, X);
      continue;

      /* AXA */
    case 0x93: MRW_Iy(AXA); continue;
    case 0x9F: MRW_Ay(AXA); continue;

    }

    switch (op)
    {
      /* ANC */
    case 0x0B: case 0x2B:

      Ad_VlIm();
      A &= val;
      ST = (ST & ~(N_FLAG | Z_FLAG | C_FLAG)) | NZTable[A] | ((A & 0x80) ? C_FLAG : 0);
      continue;

      /* LAS */
    case 0xBB:

      Ad_VlAy();
      X = A = (SP &= val);
      UpdateNZ(A);
      continue;
    }
  }

jammed:
  regPC = PC.W;
  regA = A;
  regX = X;
  regY = Y;
  regSP = SP;
  regP = ST;

  return (nCPUCycle - ret);
}
