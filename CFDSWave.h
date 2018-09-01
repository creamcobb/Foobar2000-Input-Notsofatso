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

#pragma once
#include <math.h>
//#include <minwindef.h>

class CFDSWave
{
public:
	/*	Envelope Unit	*/
	BYTE		bEnvelopeEnable;
	BYTE		nEnvelopeSpeed;

	/*	Volume Envelope	*/
	BYTE		nVolEnv_Mode;
	BYTE		nVolEnv_Decay;
	BYTE		nVolEnv_Gain;
	int			nVolEnv_Timer;
	int			nVolEnv_Count;
	BYTE		nVolume;
	BYTE		bVolEnv_On;

	/*	Sweep Envenlope	*/
	BYTE		nSweep_Mode;
	BYTE		nSweep_Decay;
	int			nSweep_Timer;
	int			nSweep_Count;
	BYTE		nSweep_Gain;
	BYTE		bSweepEnv_On;

	/*	Effector / LFO / Modulation	Unit	*/
	int			nSweepBias;
	BYTE		bLFO_Enabled;
	TWIN		nLFO_Freq;
	float		fLFO_Timer;
	float		fLFO_Count;
	BYTE		nLFO_Addr;
	BYTE		nLFO_Table[0x40];
	BYTE		bLFO_On;

	/*	Main Output		*/
	BYTE		nMainVolume;
	BYTE		bEnabled;
	TWIN		nFreq;
	float		fFreqCount;
	BYTE		nMainAddr;
	BYTE		nWaveTable[0x40];
	BYTE		bWaveWrite;
	BYTE		bMain_On;

	/*	Output and Downsampling	*/
	short		nOutputTable_L[4][0x21][0x40];
	short		nOutputTable_R[4][0x21][0x40];
	int			nMixL;
	int			nMixR;

	/*	Inversion				*/
	BYTE		bInvert;

	/*	Pop Reducer				*/
	BYTE		bPopReducer;
	BYTE		nPopOutput;
	int			nPopCount;

	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// Functions
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /

	void DoTicks(int ticks, BYTE mix);

	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // 
	// Mixing

	void Mix_Mono(int& mix, int downsample);
	void Mix_Stereo(int& mixL, int& mixR, int downsample);
};