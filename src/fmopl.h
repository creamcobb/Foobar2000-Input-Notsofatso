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
  to get right channel inversion and frequency cutoff working.

  Other than that, things are pretty much the way Xodnizel left them.

  All my changes are commented, so search the file for "Disch" to see alterations.
   - Disch
*/

/* Section added to get this linkable from a C++ environment  -Disch  */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __FMOPL_H_
#define __FMOPL_H_

/* --- system optimize --- */
/* select bit size of output : 8 or 16 */
#define OPL_OUTPUT_BIT 16

/* compiler dependence */
// #ifndef OSD_CPU_H
// #define OSD_CPU_H

typedef unsigned char FM_UINT8; /* unsigned  8bit */
typedef unsigned short FM_UINT16; /* unsigned 16bit */
typedef unsigned long FM_UINT32; /* unsigned 32bit */
typedef signed char FM_INT8; /* signed  8bit  */
typedef signed short FM_INT16; /* signed 16bit  */
typedef signed long FM_INT32; /* signed 32bit  */
// #endif
// I keep having problems with these defs, so I added FM in front of all of them.

#if (OPL_OUTPUT_BIT==16)
typedef FM_INT16 OPLSAMPLE;
#endif
#if (OPL_OUTPUT_BIT==8)
typedef unsigned char OPLSAMPLE;
#endif


/* !!!!! here is private section , do not access there member direct !!!!! */

#define OPL_TYPE_WAVESEL  0x01  /* waveform select  */

/* Saving is necessary for member of the 'R' mark for suspend/resume */
/* ---------- OPL one of slot  ---------- */
typedef struct fm_opl_slot {
  FM_INT32 TL; /* total level   :TL << 8   */
  FM_INT32 TLL; /* adjusted now TL  */
  FM_UINT8 KSR; /* key scale rate  :(shift down bit)  */
  FM_INT32 *AR; /* attack rate   :&AR_TABLE[AR<<2]  */
  FM_INT32 *DR; /* decay rate  :&DR_TALBE[DR<<2]  */
  FM_INT32 SL; /* sustin level  :SL_TALBE[SL]  */
  FM_INT32 *RR; /* release rate  :&DR_TABLE[RR<<2]  */
  FM_UINT8 ksl; /* keyscale level  :(shift down bits) */
  FM_UINT8 ksr; /* key scale rate  :kcode>>KSR  */
  FM_UINT32 mul; /* multiple  :ML_TABLE[ML]  */
  FM_UINT32 Cnt; /* frequency count :  */
  FM_UINT32 Incr; /* frequency step  :  */
  /* envelope generator state */
  FM_UINT8 eg_typ; /* envelope type flag   */
  FM_UINT8 evm; /* envelope phase  */
  FM_INT32 evc; /* envelope counter  */
  FM_INT32 eve; /* envelope counter end point */
  FM_INT32 evs; /* envelope counter step  */
  FM_INT32 evsa; /* envelope step for AR :AR[ksr]  */
  FM_INT32 evsd; /* envelope step for DR :DR[ksr]  */
  FM_INT32 evsr; /* envelope step for RR :RR[ksr]  */
  /* LFO */
  FM_UINT8 ams; /* ams flag  */
  FM_UINT8 vib; /* vibrate flag   */
  /* wave selector */
  FM_INT32 **wavetable;
}OPL_SLOT;

/* ---------- OPL one of channel  ---------- */
typedef struct fm_opl_channel {
  OPL_SLOT SLOT[2];
  FM_UINT8 CON; /* connection type  */
  FM_UINT8 FB; /* feed back  :(shift down bit)  */
  FM_INT32 *connect1; /* slot1 output pointer  */
  FM_INT32 *connect2; /* slot2 output pointer  */
  FM_INT32 op1_out[2]; /* slot1 output for selfeedback */
  /* phase generator state */
  FM_UINT32 block_fnum; /* block+fnum  :  */
  FM_UINT8 kcode; /* key code  : KeyScaleCode   */
  FM_UINT32 fc; /* Freq. Increment base  */
  FM_UINT32 ksl_base; /* KeyScaleLevel Base step  */
  FM_UINT8 keyon; /* key on/off flag  */
} OPL_CH;

/* OPL state */
typedef struct fm_opl_f {
  FM_UINT8 type; /* chip type  */
  int clock; /* master clock  (Hz)  */
  int rate; /* sampling rate (Hz)  */
  double freqbase; /* frequency base  */
  double TimerBase; /* Timer base time (==sampling time) */
  FM_UINT8 address; /* address register   */
  FM_UINT32 mode; /* Reg.08 : CSM , notesel,etc.  */

  /* FM channel slots */
  OPL_CH *P_CH; /* pointer of CH  */
  int max_ch; /* maximum channel  */

  /* time tables */
  FM_INT32 AR_TABLE[75]; /* atttack rate tables */
  FM_INT32 DR_TABLE[75]; /* decay rate tables  */
  FM_UINT32 FN_TABLE[1024]; /* fnumber -> increment counter */
  /* LFO */
  FM_INT32 *ams_table;
  FM_INT32 *vib_table;
  FM_INT32 amsCnt;
  FM_INT32 amsIncr;
  FM_INT32 vibCnt;
  FM_INT32 vibIncr;
  /* wave selector enable flag */
  FM_UINT8 wavesel;


  /*
  *  Values added to track left/right channel amplitude control (for volume/pan control)
  *  -Disch
  */
  /* Channel Volume/Panning control */
  float fLeftMultiplier[6];
  float fRightMultiplier[6];

  /*
  *  More stuf added by me to get inversion and frequency cutoff
  *  -Disch
  */
  FM_UINT16 nFreqReg[6];
  FM_UINT8 bInvert[6];
  FM_UINT8 bDoInvert[6];
  FM_UINT16 nInvertFreqCutoff;

} FM_OPL;

/* ---------- Generic interface section ---------- */
#define OPL_TYPE_YM3812 (OPL_TYPE_WAVESEL)

FM_OPL *OPLCreate(int type, int clock, int rate);
void OPLDestroy(FM_OPL *OPL);

void OPLResetChip(FM_OPL *OPL);
void OPLWrite(FM_OPL *OPL,FM_UINT8 a,FM_UINT8 v);

/* YM3626/YM3812 local section */  // 'mix' values (for channel disabling) and stereo boolean value added -Disch
void YM3812UpdateOne(FM_OPL *OPL, FM_UINT8 *buffer, int size, FM_UINT8* mix,FM_UINT8 stereo);

#endif


/* Section added to get this linkable from a C++ environment  -Disch  */
#ifdef __cplusplus
}
#endif
