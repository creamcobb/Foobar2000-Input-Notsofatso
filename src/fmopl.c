/*
  Taken from FCE Ultra - NES/Famicom Emulator

  Copyright notice for this file:
  Copyright (C) 1999 Tatsuyuki Satoh
  Copyright (C) 2001 Ben Parnell
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

/*
  This file has been heavily modified from the original(mostly unused
  code was removed).  If you want to use it for anything other than
  VRC7 sound emulation, you should get the original from the AdPlug
  source distribution or the MAME(version 0.37b16) source distribution
  (but be careful about the different licenses).
  - Xodnizel
*/

/*
  I added a few modifications to YM3812UpdateOne and OPL_CALC_CH
  to get channel volume/pan control.  I also added stuff to YM3812UpdateOne
  to get Right channel inversion and frequency cutoff working.

  Other than that, things are pretty much the way Xodnizel left them.

  All my changes are commented, so search the file for "Disch" to see alterations.
  - Disch
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "fmopl.h"

// This is a constant used when determining if it's safe for the channel to be inverted without
// producing a popping noise (try to flip it when output is as close to 0 as possible).  Since
// it's possible that ouput will never be exactly zero... anything below this value (and above
// it's negative value) will be treated as safe to invert)
//   -Disch
#ifdef __cplusplus
extern "C" {
#endif

#define INVERT_SAFE_MIN  10


// These warnings pop up everywhere in this file, so rather than go through the file and typecast
// The 60000 areas that need it, I just shut the warning off.  It's re-instated at the end of the file
//   -Disch
#pragma warning( disable : 4244 )


#ifndef PI
#define PI 3.14159265358979323846
#endif

/* -------------------- Preliminary define section --------------------- */
/* Attack/Decay rate time rate */
#define OPL_ARRATE   141280  /* RATE 4 =  2826.24ms @ 3.6MHz */
#define OPL_DRRATE  1956000  /* RATE 4 = 39280.64ms @ 3.6MHz */

#define DELTAT_MIXING_LEVEL (1) /* DELTA-T ADPCM MIXING LEVEL */

#define FREQ_BITS 24  /* Frequency turn */

/* Counter bits = 20 , octerve 7 */
#define FREQ_RATE  (1<<(FREQ_BITS-20))
#define TL_BITS  (FREQ_BITS+2)

/* Final output shift , limit minimum and maximum */
#define OPL_OUTSB  (TL_BITS+3-16)  /* OPL output final shift 16bit */
#define OPL_MAXOUT (0x7fff<<OPL_OUTSB<<3)
#define OPL_MINOUT (-0x8000<<OPL_OUTSB<<3)

/* -------------------- Quality selection --------------------- */

/* Sinwave entries */
/* Used static memory = SIN_ENT * 4 (byte) */
#define SIN_ENT 2048

/* Output level entries (envelope,sinwave) */
/* Envelope counter lower bits */
#define ENV_BITS 16
/* Envelope output entries */
#define EG_ENT  4096
/* Used dynamic memory = EG_ENT*4*4(byte)or EG_ENT*6*4(byte) */
/* Used static  memory = EG_ENT*4 (byte) */

#define EG_OFF  ((2*EG_ENT)<<ENV_BITS)  /* OFF */
#define EG_DED  EG_OFF
#define EG_DST  (EG_ENT<<ENV_BITS)  /* DECAY  START */
#define EG_AED  EG_DST
#define EG_AST  0  /* ATTACK START */

#define EG_STEP (96.0/EG_ENT) /* OPL is 0.1875 dB step */

/* LFO table entries */
#define VIB_ENT 512
#define VIB_SHIFT (32-9)
#define AMS_ENT 512
#define AMS_SHIFT (32-9)

#define VIB_RATE 256

/* -------------------- Local defines , macros --------------------- */

/* Register number to channel number , slot offset */
#define SLOT1 0
#define SLOT2 1

/* Envelope phase */
#define ENV_MOD_RR  0x00
#define ENV_MOD_DR  0x01
#define ENV_MOD_AR  0x02

/* -------------------- Tables --------------------- */
static const int slot_array[32] =
{
  0, 2, 4, 1, 3, 5, -1, -1,
  6, 8, 10, 7, 9, 11, -1, -1,
  12, 14, 16, 13, 15, 17, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1
};

/* Key scale level */
/* Table is 3dB/OCT , DV converts this in TL step at 6dB/OCT */

#define DV (EG_STEP/2)
  static const FM_UINT32 KSL_TABLE[8 * 16] =
  {
    /* OCT 0 */
    0.000 / DV, 0.000 / DV, 0.000 / DV, 0.000 / DV,
    0.000 / DV, 0.000 / DV, 0.000 / DV, 0.000 / DV,
    0.000 / DV, 0.000 / DV, 0.000 / DV, 0.000 / DV,
    0.000 / DV, 0.000 / DV, 0.000 / DV, 0.000 / DV,
    /* OCT 1 */
    0.000 / DV, 0.000 / DV, 0.000 / DV, 0.000 / DV,
    0.000 / DV, 0.000 / DV, 0.000 / DV, 0.000 / DV,
    0.000 / DV, 0.750 / DV, 1.125 / DV, 1.500 / DV,
    1.875 / DV, 2.250 / DV, 2.625 / DV, 3.000 / DV,
    /* OCT 2 */
    0.000 / DV, 0.000 / DV, 0.000 / DV, 0.000 / DV,
    0.000 / DV, 1.125 / DV, 1.875 / DV, 2.625 / DV,
    3.000 / DV, 3.750 / DV, 4.125 / DV, 4.500 / DV,
    4.875 / DV, 5.250 / DV, 5.625 / DV, 6.000 / DV,
    /* OCT 3 */
    0.000 / DV, 0.000 / DV, 0.000 / DV, 1.875 / DV,
    3.000 / DV, 4.125 / DV, 4.875 / DV, 5.625 / DV,
    6.000 / DV, 6.750 / DV, 7.125 / DV, 7.500 / DV,
    7.875 / DV, 8.250 / DV, 8.625 / DV, 9.000 / DV,
    /* OCT 4 */
    0.000 / DV, 0.000 / DV, 3.000 / DV, 4.875 / DV,
    6.000 / DV, 7.125 / DV, 7.875 / DV, 8.625 / DV,
    9.000 / DV, 9.750 / DV, 10.125 / DV, 10.500 / DV,
    10.875 / DV, 11.250 / DV, 11.625 / DV, 12.000 / DV,
    /* OCT 5 */
    0.000 / DV, 3.000 / DV, 6.000 / DV, 7.875 / DV,
    9.000 / DV, 10.125 / DV, 10.875 / DV, 11.625 / DV,
    12.000 / DV, 12.750 / DV, 13.125 / DV, 13.500 / DV,
    13.875 / DV, 14.250 / DV, 14.625 / DV, 15.000 / DV,
    /* OCT 6 */
    0.000 / DV, 6.000 / DV, 9.000 / DV, 10.875 / DV,
    12.000 / DV, 13.125 / DV, 13.875 / DV, 14.625 / DV,
    15.000 / DV, 15.750 / DV, 16.125 / DV, 16.500 / DV,
    16.875 / DV, 17.250 / DV, 17.625 / DV, 18.000 / DV,
    /* OCT 7 */
    0.000 / DV, 9.000 / DV, 12.000 / DV, 13.875 / DV,
    15.000 / DV, 16.125 / DV, 16.875 / DV, 17.625 / DV,
    18.000 / DV, 18.750 / DV, 19.125 / DV, 19.500 / DV,
    19.875 / DV, 20.250 / DV, 20.625 / DV, 21.000 / DV
  };
#undef DV

/* Sustain lebel table (3db per step) */
/* 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)*/
#define SC(db) (db*((3/EG_STEP)*(1<<ENV_BITS)))+EG_DST
  static const FM_INT32 SL_TABLE[16] = {
    SC(0), SC(1), SC(2), SC(3), SC(4), SC(5), SC(6), SC(7),
    SC(8), SC(9), SC(10), SC(11), SC(12), SC(13), SC(14), SC(31)
  };
#undef SC

#define TL_MAX (EG_ENT*2) /* limit(tl + ksr + envelope) + sinwave */
  /* TotalLevel : 48 24 12  6  3 1.5 0.75 (dB) */
  /* TL_TABLE[ 0  to TL_MAX  ] : plus  section */
  /* TL_TABLE[ TL_MAX to TL_MAX+TL_MAX-1 ] : minus section */
  static FM_INT32 *TL_TABLE;

  /* Pointers to TL_TABLE with sinwave output offset */
  static FM_INT32 **SIN_TABLE;

  /* LFO table */
  static FM_INT32 *AMS_TABLE;
  static FM_INT32 *VIB_TABLE;

  /* Envelope output curve table */
  /* Attack + decay + OFF */
  static FM_INT32 ENV_CURVE[2 * EG_ENT + 1];

/* Multiple table */
#define ML 2
  static const FM_UINT32 MUL_TABLE[16] = {
    /* 1/2, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 */
    0.50*ML, 1.00*ML, 2.00*ML, 3.00*ML, 4.00*ML, 5.00*ML, 6.00*ML, 7.00*ML,
    8.00*ML, 9.00*ML, 10.00*ML, 10.00*ML, 12.00*ML, 12.00*ML, 15.00*ML, 15.00*ML
  };
#undef ML

  /* Dummy attack / decay rate ( when rate == 0 ) */
  static FM_INT32 RATE_0[16] =
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  /* -------------------- Static state --------------------- */

  /* Lock level of common table */
  static int num_lock = 0;

  /* Work table */
  static void *cur_chip = NULL;  /* Current chip point */

  /* Currenct chip state */
  /* Static OPLSAMPLE  *bufL,*bufR; */
  static OPL_CH *S_CH;
  static OPL_CH *E_CH;
  OPL_SLOT *SLOT7_1, *SLOT7_2, *SLOT8_1, *SLOT8_2;

  static FM_INT32 outd[2];
  static FM_INT32 ams;
  static FM_INT32 vib;
  FM_INT32 *ams_table;
  FM_INT32 *vib_table;
  static FM_INT32 amsIncr;
  static FM_INT32 vibIncr;
  static FM_INT32 feedback2;  /* Connect for SLOT 2 */

  /* --------------------- Subroutines  --------------------- */
  __inline int Limit(int val, int max, int min) {
    if (val > max)
      val = max;
    else if (val < min)
      val = min;

    return val;
  }

  /* ----- Key on  ----- */
  __inline void OPL_KEYON(OPL_SLOT *SLOT)
  {
    /* Sin wave restart */
    SLOT->Cnt = 0;
    /* Set attack */
    SLOT->evm = ENV_MOD_AR;
    SLOT->evs = SLOT->evsa;
    SLOT->evc = EG_AST;
    SLOT->eve = EG_AED;
  }

  /* ----- Key off ----- */
  __inline void OPL_KEYOFF(OPL_SLOT *SLOT)
  {
    if (SLOT->evm > ENV_MOD_RR)
    {
      /* Set envelope counter from envleope output */
      SLOT->evm = ENV_MOD_RR;
      if (!(SLOT->evc&EG_DST))
        // SLOT->evc = (ENV_CURVE[SLOT->evc>>ENV_BITS]<<ENV_BITS) + EG_DST;
        SLOT->evc = EG_DST;
      SLOT->eve = EG_DED;
      SLOT->evs = SLOT->evsr;
    }
  }

  /* ---------- Calcrate Envelope Generator & Phase Generator ---------- */
  /* Return : envelope output */
  __inline FM_UINT32 OPL_CALC_SLOT(OPL_SLOT *SLOT)
  {
    /* Calcrate envelope generator */
    if ((SLOT->evc += SLOT->evs) >= SLOT->eve)
    {
      switch (SLOT->evm) {
      case ENV_MOD_AR: /* ATTACK -> DECAY1 */
        /* Next DR */
        SLOT->evm = ENV_MOD_DR;
        SLOT->evc = EG_DST;
        SLOT->eve = SLOT->SL;
        SLOT->evs = SLOT->evsd;
        break;
      case ENV_MOD_DR: /* DECAY -> SL or RR */
        SLOT->evc = SLOT->SL;
        SLOT->eve = EG_DED;
        if (SLOT->eg_typ)
        {
          SLOT->evs = 0;
        }
        else
        {
          SLOT->evm = ENV_MOD_RR;
          SLOT->evs = SLOT->evsr;
        }
        break;
      case ENV_MOD_RR: /* RR -> OFF */
        SLOT->evc = EG_OFF;
        SLOT->eve = EG_OFF + 1;
        SLOT->evs = 0;
        break;
      }
    }
    /* Calcrate envelope */
    return SLOT->TLL + ENV_CURVE[SLOT->evc >> ENV_BITS] + (SLOT->ams ? ams : 0);
  }

  /* Set algorythm connection */
  static void set_algorythm(OPL_CH *CH)
  {
    FM_INT32 *carrier = &outd[0];
    CH->connect1 = CH->CON ? carrier : &feedback2;
    CH->connect2 = carrier;
  }

  /* ---------- Frequency Counter For Operater Update ---------- */
  __inline void CALC_FCSLOT(OPL_CH *CH, OPL_SLOT *SLOT)
  {
    int ksr;

    /* Frequency step counter */
    SLOT->Incr = CH->fc * SLOT->mul;
    ksr = CH->kcode >> SLOT->KSR;

    if (SLOT->ksr != ksr)
    {
      SLOT->ksr = ksr;
      /* Attack , decay rate recalcration */
      SLOT->evsa = SLOT->AR[ksr];
      SLOT->evsd = SLOT->DR[ksr];
      SLOT->evsr = SLOT->RR[ksr];
    }
    SLOT->TLL = SLOT->TL + (CH->ksl_base >> SLOT->ksl);
  }

  /* Set multi,am,vib,EG-TYP,KSR,mul */
  __inline void set_mul(FM_OPL *OPL, int slot, int v)
  {
    OPL_CH  *CH = &OPL->P_CH[slot / 2];
    OPL_SLOT *SLOT = &CH->SLOT[slot & 1];

    SLOT->mul = MUL_TABLE[v & 0x0f];
    SLOT->KSR = (v & 0x10) ? 0 : 2;
    SLOT->eg_typ = (v & 0x20) >> 5;
    SLOT->vib = (v & 0x40);
    SLOT->ams = (v & 0x80);
    CALC_FCSLOT(CH, SLOT);
  }

  /* Set ksl & tl */
  __inline void set_ksl_tl(FM_OPL *OPL, int slot, int v)
  {
    OPL_CH  *CH = &OPL->P_CH[slot / 2];
    OPL_SLOT *SLOT = &CH->SLOT[slot & 1];
    int ksl = v >> 6; /* 0 / 1.5 / 3 / 6 db/OCT */

    SLOT->ksl = ksl ? ksl : 31;
    //  SLOT->ksl = ksl ? 3-ksl : 31;
    SLOT->TL = (v & 0x3f)*(0.75 / EG_STEP); /* 0.75db step */

    if (!(OPL->mode & 0x80))
    {  /* not CSM latch total level */
      SLOT->TLL = SLOT->TL + (CH->ksl_base >> SLOT->ksl);
    }
  }

  /* Set attack rate & decay rate */
  __inline void set_ar_dr(FM_OPL *OPL, int slot, int v)
  {
    OPL_CH  *CH = &OPL->P_CH[slot / 2];
    OPL_SLOT *SLOT = &CH->SLOT[slot & 1];
    int ar = v >> 4;
    int dr = v & 0x0f;

    SLOT->AR = ar ? &OPL->AR_TABLE[ar << 2] : RATE_0;
    SLOT->evsa = SLOT->AR[SLOT->ksr];
    if (SLOT->evm == ENV_MOD_AR) SLOT->evs = SLOT->evsa;

    SLOT->DR = dr ? &OPL->DR_TABLE[dr << 2] : RATE_0;
    SLOT->evsd = SLOT->DR[SLOT->ksr];
    if (SLOT->evm == ENV_MOD_DR) SLOT->evs = SLOT->evsd;
  }

  /* Set sustain level & release rate */
  __inline void set_sl_rr(FM_OPL *OPL, int slot, int v)
  {
    OPL_CH  *CH = &OPL->P_CH[slot / 2];
    OPL_SLOT *SLOT = &CH->SLOT[slot & 1];
    int sl = v >> 4;
    int rr = v & 0x0f;

    SLOT->SL = SL_TABLE[sl];
    if (SLOT->evm == ENV_MOD_DR) SLOT->eve = SLOT->SL;
    SLOT->RR = &OPL->DR_TABLE[rr << 2];
    SLOT->evsr = SLOT->RR[SLOT->ksr];
    if (SLOT->evm == ENV_MOD_RR) SLOT->evs = SLOT->evsr;
  }

  /* Operator output calcrator */
#define OP_OUT(slot,env,con)  slot->wavetable[((slot->Cnt+con)/(0x1000000/SIN_ENT))&(SIN_ENT-1)][env]
  /* ---------- Calcrate One Of Channel ---------- */

  // This function -was- a void and output the generated sample to outd[0] directly.  Since that didn't
  // Suit my needs for adding pan control, I instead had it return the generated sample in the form of
  // An INT32
  //   -Disch

  __inline FM_INT32 OPL_CALC_CH(OPL_CH *CH)
  {
    FM_UINT32 env_out;
    OPL_SLOT *SLOT;

    feedback2 = 0;
    /* SLOT 1 */
    SLOT = &CH->SLOT[SLOT1];
    env_out = OPL_CALC_SLOT(SLOT);
    if (env_out < EG_ENT - 1)
    {
      /* PG */
      if (SLOT->vib) SLOT->Cnt += (SLOT->Incr*vib / VIB_RATE);
      else  SLOT->Cnt += SLOT->Incr;
      /* connectoion */
      if (CH->FB)
      {
        int feedback1 = (CH->op1_out[0] + CH->op1_out[1]) >> CH->FB;
        CH->op1_out[1] = CH->op1_out[0];
        *CH->connect1 += CH->op1_out[0] = OP_OUT(SLOT, env_out, feedback1);
      }
      else
      {
        *CH->connect1 += OP_OUT(SLOT, env_out, 0);
      }
    }
    else
    {
      CH->op1_out[1] = CH->op1_out[0];
      CH->op1_out[0] = 0;
    }
    /* SLOT 2 */
    SLOT = &CH->SLOT[SLOT2];
    env_out = OPL_CALC_SLOT(SLOT);
    if (env_out < EG_ENT - 1)
    {
      /* PG */
      if (SLOT->vib) SLOT->Cnt += (SLOT->Incr*vib / VIB_RATE);
      else  SLOT->Cnt += SLOT->Incr;
      /* Connectoion */
      return OP_OUT(SLOT, env_out, feedback2);
    }
    return 0;
  }

  /* ----------- initialize time tabls ----------- */
  static void init_timetables(FM_OPL *OPL, int ARRATE, int DRRATE)
  {
    int i;
    double rate;

    /* Make attack rate & decay rate tables */
    for (i = 0; i < 4; i++) OPL->AR_TABLE[i] = OPL->DR_TABLE[i] = 0;
    for (i = 4; i <= 60; i++) {
      rate = OPL->freqbase;  /* Frequency rate */
      if (i < 60) rate *= 1.0 + (i & 3)*0.25;  /* b0-1 : x1 , x1.25 , x1.5 , x1.75 */
      rate *= 1 << ((i >> 2) - 1);  /* b2-5 : shift bit */
      rate *= (double)(EG_ENT << ENV_BITS);
      OPL->AR_TABLE[i] = rate / ARRATE;
      OPL->DR_TABLE[i] = rate / DRRATE;
    }
    for (i = 60; i < 76; i++)
    {
      OPL->AR_TABLE[i] = EG_AED - 1;
      OPL->DR_TABLE[i] = OPL->DR_TABLE[60];
    }
  }

  /* ---------- Generic Table Initialize ---------- */
  static int OPLOpenTable(void)
  {
    int s, t;
    double rate;
    int i, j;
    double pom;

    /* Allocate dynamic tables */
    if ((TL_TABLE = (FM_INT32*)malloc(TL_MAX * 2 * sizeof(FM_INT32))) == NULL)
      return 0;
    if ((SIN_TABLE = (FM_INT32**)malloc(SIN_ENT * 4 * sizeof(FM_INT32 *))) == NULL)
    {
      free(TL_TABLE);
      return 0;
    }
    if ((AMS_TABLE = (FM_INT32*)malloc(AMS_ENT * 2 * sizeof(FM_INT32))) == NULL)
    {
      free(TL_TABLE);
      free(SIN_TABLE);
      return 0;
    }
    if ((VIB_TABLE = (FM_INT32*)malloc(VIB_ENT * 2 * sizeof(FM_INT32))) == NULL)
    {
      free(TL_TABLE);
      free(SIN_TABLE);
      free(AMS_TABLE);
      return 0;
    }
    /* Make total level table */
    for (t = 0; t < EG_ENT - 1; t++) {
      rate = ((1 << TL_BITS) - 1) / pow(10, EG_STEP*t / 20);  /* dB -> voltage */
      TL_TABLE[t] = (int)rate;
      TL_TABLE[TL_MAX + t] = -TL_TABLE[t];

    }
    /* Fill volume off area */
    for (t = EG_ENT - 1; t < TL_MAX; t++) {
      TL_TABLE[t] = TL_TABLE[TL_MAX + t] = 0;
    }

    /* Make sinwave table (total level offet) */
    /* Degree 0 = degree 180  = off */
    SIN_TABLE[0] = SIN_TABLE[SIN_ENT / 2] = &TL_TABLE[EG_ENT - 1];
    for (s = 1; s <= SIN_ENT / 4; s++) {
      pom = sin(2 * PI*s / SIN_ENT); /* Sin */
      pom = 20 * log10(1 / pom);   /* decibel */
      j = pom / EG_STEP;  /* TL_TABLE steps */

      /* Degree 0  -  90  , degree 180 -  90 : plus section */
      SIN_TABLE[s] = SIN_TABLE[SIN_ENT / 2 - s] = &TL_TABLE[j];
      /* Degree 180 - 270  , degree 360 - 270 : minus section */
      SIN_TABLE[SIN_ENT / 2 + s] = SIN_TABLE[SIN_ENT - s] = &TL_TABLE[TL_MAX + j];

    }
    for (s = 0; s < SIN_ENT; s++)
    {
      SIN_TABLE[SIN_ENT * 1 + s] = s < (SIN_ENT / 2) ? SIN_TABLE[s] : &TL_TABLE[EG_ENT];
      SIN_TABLE[SIN_ENT * 2 + s] = SIN_TABLE[s % (SIN_ENT / 2)];
      SIN_TABLE[SIN_ENT * 3 + s] = (s / (SIN_ENT / 4)) & 1 ? &TL_TABLE[EG_ENT] : SIN_TABLE[SIN_ENT * 2 + s];
    }

    /* Envelope counter -> envelope output table */
    for (i = 0; i < EG_ENT; i++)
    {
      /* ATTACK curve */
      pom = pow(((double)(EG_ENT - 1 - i) / EG_ENT), 8) * EG_ENT;
      /* if( pom >= EG_ENT ) pom = EG_ENT-1; */
      ENV_CURVE[i] = (int)pom;
      /* DECAY ,RELEASE curve */
      ENV_CURVE[(EG_DST >> ENV_BITS) + i] = i;
    }
    /* off */
    ENV_CURVE[EG_OFF >> ENV_BITS] = EG_ENT - 1;
    /* Make LFO ams table */
    for (i = 0; i < AMS_ENT; i++)
    {
      pom = (1.0 + sin(2 * PI*i / AMS_ENT)) / 2; /* Sin */
      AMS_TABLE[i] = (1.0 / EG_STEP)*pom; /* 1dB */
      AMS_TABLE[AMS_ENT + i] = (4.8 / EG_STEP)*pom; /* 4.8dB */
    }
    /* Make LFO vibrate table */
    for (i = 0; i < VIB_ENT; i++)
    {
      /* 100cent = 1seminote = 6% ?? */
      pom = (double)VIB_RATE*0.06*sin(2 * PI*i / VIB_ENT); /* +-100sect step */
      VIB_TABLE[i] = VIB_RATE + (pom*0.07); /* +- 7cent */
      VIB_TABLE[VIB_ENT + i] = VIB_RATE + (pom*0.14); /* +-14cent */
    }
    return 1;
  }


  static void OPLCloseTable(void)
  {
    free(TL_TABLE);
    free(SIN_TABLE);
    free(AMS_TABLE);
    free(VIB_TABLE);
  }

  /* CSM Key Controll */
  __inline void CSMKeyControll(OPL_CH *CH)
  {
    OPL_SLOT *slot1 = &CH->SLOT[SLOT1];
    OPL_SLOT *slot2 = &CH->SLOT[SLOT2];
    /* All key off */
    OPL_KEYOFF(slot1);
    OPL_KEYOFF(slot2);
    /* Total level latch */
    slot1->TLL = slot1->TL + (CH->ksl_base >> slot1->ksl);
    slot1->TLL = slot1->TL + (CH->ksl_base >> slot1->ksl);
    /* Key on */
    CH->op1_out[0] = CH->op1_out[1] = 0;
    OPL_KEYON(slot1);
    OPL_KEYON(slot2);
  }

  /* ---------- OPL initialize ---------- */
  static void OPL_initalize(FM_OPL *OPL)
  {
    int fn;

    /* Frequency base */
    OPL->freqbase = (OPL->rate) ? ((double)OPL->clock / OPL->rate) / 72 : 0;
    /* Make time tables */
    init_timetables(OPL, OPL_ARRATE, OPL_DRRATE);
    /* Make fnumber -> increment counter table */
    for (fn = 0; fn < 1024; fn++)
    {
      OPL->FN_TABLE[fn] = OPL->freqbase * fn * FREQ_RATE * (1 << 7) / 2;
    }
    /* LFO freq.table */
    OPL->amsIncr = OPL->rate ? (double)AMS_ENT*(1 << AMS_SHIFT) / OPL->rate * 3.7 * ((double)OPL->clock / 3600000) : 0;
    OPL->vibIncr = OPL->rate ? (double)VIB_ENT*(1 << VIB_SHIFT) / OPL->rate * 6.4 * ((double)OPL->clock / 3600000) : 0;
  }

  /* ---------- Write a OPL registers ---------- */
  static void OPLWriteReg(FM_OPL *OPL, int r, int v)
  {
    OPL_CH *CH;
    int slot;
    int block_fnum;

    switch (r & 0xe0)
    {
    case 0x00: /* 00-1f:controll */
      switch (r & 0x1f)
      {
      case 0x01:
        /* Wave selector enable */
        if (OPL->type&OPL_TYPE_WAVESEL)
        {
          OPL->wavesel = v & 0x20;
          if (!OPL->wavesel)
          {
            /* Preset compatible mode */
            int c;
            for (c = 0; c < OPL->max_ch; c++)
            {
              OPL->P_CH[c].SLOT[SLOT1].wavetable = &SIN_TABLE[0];
              OPL->P_CH[c].SLOT[SLOT2].wavetable = &SIN_TABLE[0];
            }
          }
        }
        return;
      }
      break;
    case 0x20:  /* Am,vib,ksr,eg type,mul */
      slot = slot_array[r & 0x1f];
      if (slot == -1) return;
      set_mul(OPL, slot, v);
      return;
    case 0x40:
      slot = slot_array[r & 0x1f];
      if (slot == -1) return;
      set_ksl_tl(OPL, slot, v);
      return;
    case 0x60:
      slot = slot_array[r & 0x1f];
      if (slot == -1) return;
      set_ar_dr(OPL, slot, v);
      return;
    case 0x80:
      slot = slot_array[r & 0x1f];
      if (slot == -1) return;
      set_sl_rr(OPL, slot, v);
      return;
    case 0xa0:
      switch (r)
      {
      case 0xbd:
        /* Amsep,vibdep,r,bd,sd,tom,tc,hh */
      {
        OPL->ams_table = &AMS_TABLE[v & 0x80 ? AMS_ENT : 0];
        OPL->vib_table = &VIB_TABLE[v & 0x40 ? VIB_ENT : 0];
      }
      return;
      }
      /* Keyon,block,fnum */
      if ((r & 0x0f) > 8) return;
      CH = &OPL->P_CH[r & 0x0f];
      if (!(r & 0x10))
      {  /* A0-a8 */
        block_fnum = (CH->block_fnum & 0x1f00) | v;
      }
      else
      {  /* b0-b8 */
        int keyon = (v >> 5) & 1;
        block_fnum = ((v & 0x1f) << 8) | (CH->block_fnum & 0xff);
        if (CH->keyon != keyon)
        {
          if ((CH->keyon = keyon))
          {
            CH->op1_out[0] = CH->op1_out[1] = 0;
            OPL_KEYON(&CH->SLOT[SLOT1]);
            OPL_KEYON(&CH->SLOT[SLOT2]);
          }
          else
          {
            OPL_KEYOFF(&CH->SLOT[SLOT1]);
            OPL_KEYOFF(&CH->SLOT[SLOT2]);
          }
        }
      }
      /* update */
      if (CH->block_fnum != block_fnum)
      {
        int blockRv = 7 - (block_fnum >> 10);
        int fnum = block_fnum & 0x3ff;
        CH->block_fnum = block_fnum;

        CH->ksl_base = KSL_TABLE[block_fnum >> 6];
        CH->fc = OPL->FN_TABLE[fnum] >> blockRv;
        CH->kcode = CH->block_fnum >> 9;
        if ((OPL->mode & 0x40) && CH->block_fnum & 0x100) CH->kcode |= 1;
        CALC_FCSLOT(CH, &CH->SLOT[SLOT1]);
        CALC_FCSLOT(CH, &CH->SLOT[SLOT2]);
      }
      return;
    case 0xc0:
      /* FB,C */
      if ((r & 0x0f) > 8) return;
      CH = &OPL->P_CH[r & 0x0f];
      {
        int feedback = (v >> 1) & 7;
        CH->FB = feedback ? (8 + 1) - feedback : 0;
        CH->CON = v & 1;
        set_algorythm(CH);
      }
      return;
    case 0xe0: /* wave type */
      slot = slot_array[r & 0x1f];
      if (slot == -1) return;
      CH = &OPL->P_CH[slot / 2];
      if (OPL->wavesel)
      {
        CH->SLOT[slot & 1].wavetable = &SIN_TABLE[(v & 0x03)*SIN_ENT];
      }
      return;
    }
  }

  /* lock/unlock for common table */
  static int OPL_LockTable(void)
  {
    num_lock++;
    if (num_lock > 1) return 0;
    /* First time */
    cur_chip = NULL;
    /* Allocate total level table (128kb space) */
    if (!OPLOpenTable())
    {
      num_lock--;
      return -1;
    }
    return 0;
  }

  static void OPL_UnLockTable(void)
  {
    if (num_lock) num_lock--;
    if (num_lock) return;
    /* last time */
    cur_chip = NULL;
    OPLCloseTable();
  }

  /*******************************************************************************/
  /*  YM3812 local section */
  /*******************************************************************************/

  // A few changes made to this function, including extra arguments.
  // Mix = pointer to a buffer of 6 BYTEs, which specify whether or not to mix the
  // Coresponding channel (zero = don't mix (channel disabled), non-zero = mix)
  // Stereo = boolean value specifying whether or not to generate samples in stereo mode
  //   -Disch

  /* ---------- Update One Of Chip ----------- */
  void YM3812UpdateOne(FM_OPL *OPL, FM_UINT8 *buffer, int size, FM_UINT8* mix, FM_UINT8 stereo)
  {
    int i, j;
    int temp;
    short *buf = (short*)buffer;
    FM_UINT32 amsCnt = OPL->amsCnt;
    FM_UINT32 vibCnt = OPL->vibCnt;
    OPL_CH *CH, *R_CH;

    if ((void *)OPL != cur_chip) {
      cur_chip = (void *)OPL;
      /* channel pointers */
      S_CH = OPL->P_CH;
      E_CH = &S_CH[6];
      /* LFO state */
      amsIncr = OPL->amsIncr;
      vibIncr = OPL->vibIncr;
      ams_table = OPL->ams_table;
      vib_table = OPL->vib_table;
    }
    R_CH = E_CH;
    i = 0;
    while (i < size)
    {
      /*   channel A  channel B  channel C  */
      /* LFO */
      ams = ams_table[(amsCnt += amsIncr) >> AMS_SHIFT];
      vib = vib_table[(vibCnt += vibIncr) >> VIB_SHIFT];
      outd[0] = outd[1] = 0;
      /* FM part */
      j = 0;
      for (CH = S_CH; CH < R_CH; CH++, j++)
      {
        // Changed this loop around a bit... made it skip a channel if it's not to be mixed
        // And adjusted the output to reflect amplitude adjustment.
        // -Disch
        if (!mix[j])
        {
          OPL->bDoInvert[j] = OPL->bInvert[j];
          continue;
        }
        temp = OPL_CALC_CH(CH);

        outd[0] += (int)(temp * OPL->fLeftMultiplier[j]);
        if (stereo)
        {
          if (temp < INVERT_SAFE_MIN && temp > -INVERT_SAFE_MIN)
            OPL->bDoInvert[j] = OPL->bInvert[j] && (OPL->nFreqReg[j] >= OPL->nInvertFreqCutoff);

          if (OPL->bDoInvert[j])  outd[1] -= (int)(temp * OPL->fRightMultiplier[j]);
          else  outd[1] += (int)(temp * OPL->fRightMultiplier[j]);
        }
      }
      /* Limit check */
      // data = Limit( outd[0] , OPL_MAXOUT, OPL_MINOUT );
      /* Store to sound buffer */
  {

    // Made minor changes to this section.
    // -Disch
    int d = outd[0] >> OPL_OUTSB;

    d += *buf;
    if (d > 32767) d = 32767;
    if (d < -32768) d = -32768;
    *buf = d;
    i += 2;
    buf++;

    if (stereo)
    {
      d = outd[1] >> OPL_OUTSB;

      d += *buf;
      if (d > 32767) d = 32767;
      if (d < -32768) d = -32768;
      *buf = d;
      i += 2;
      buf++;
    }

  }
    }

    OPL->amsCnt = amsCnt;
    OPL->vibCnt = vibCnt;
  }

  /* ---------- Reset One Of Chip ---------- */
  void OPLResetChip(FM_OPL *OPL)
  {
    int c, s;
    int i;

    /* Reset chip */
    OPL->mode = 0;  /* Normal mode */

    /* Reset with register write */
    OPLWriteReg(OPL, 0x01, 0); /* Wabesel disable */
    for (i = 0xff; i >= 0x20; i--) OPLWriteReg(OPL, i, 0);
    /* reset OPerator paramater */
    for (c = 0; c < OPL->max_ch; c++)
    {
      OPL_CH *CH = &OPL->P_CH[c];
      /* OPL->P_CH[c].PAN = OPN_CENTER; */
      for (s = 0; s < 2; s++)
      {
        /* wave table */
        CH->SLOT[s].wavetable = &SIN_TABLE[0];
        /* CH->SLOT[s].evm = ENV_MOD_RR; */
        CH->SLOT[s].evc = EG_OFF;
        CH->SLOT[s].eve = EG_OFF + 1;
        CH->SLOT[s].evs = 0;
      }
    }
  }

  /* ----------  Create One Of Vietual YM3812 ---------- */
  /* 'rate'  is sampling rate and 'bufsiz' is the size of the */
  FM_OPL *OPLCreate(int type, int clock, int rate)
  {
    char *ptr;
    FM_OPL *OPL;
    int state_size;
    int max_ch = 9; /* normaly 9 channels */

    if (OPL_LockTable() == -1) return NULL;
    /* Allocate OPL state space */
    state_size = sizeof(FM_OPL);
    state_size += sizeof(OPL_CH)*max_ch;

    /* Allocate memory block */
    ptr = (char*)malloc(state_size);
    if (ptr == NULL) return NULL;
    /* Clear */
    memset(ptr, 0, state_size);
    OPL = (FM_OPL *)ptr; ptr += sizeof(FM_OPL);
    OPL->P_CH = (OPL_CH *)ptr; ptr += sizeof(OPL_CH)*max_ch;

    /* Set channel state pointer */
    OPL->type = type;
    OPL->clock = clock;
    OPL->rate = rate;
    OPL->max_ch = max_ch;
    /* Init grobal tables */
    OPL_initalize(OPL);
    /* Reset chip */
    OPLResetChip(OPL);

    return OPL;
  }

  /* ----------  Destroy One Of Vietual YM3812 ---------- */
  void OPLDestroy(FM_OPL *OPL)
  {
    OPL_UnLockTable();
    free(OPL);
  }

  /* ---------- YM3812 I/O Interface ---------- */
  void OPLWrite(FM_OPL *OPL, FM_UINT8 a, FM_UINT8 v)
  {
    OPLWriteReg(OPL, a, v);
  }


#pragma warning( default : 4244 )
#ifdef __cplusplus
}
#endif
