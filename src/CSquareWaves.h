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

class CSquareWaves
{
public:

	// // // // // // // // // // // // // // // // // /
	// Programmable Timer
	TWIN		nFreqTimer[2];
	int			nFreqCount[2];

	// // // // // // // // // // // // // // // // // /
	// Length Counter
	BYTE		nLengthCount[2];
	BYTE		bLengthEnabled[2];
	BYTE		bChannelEnabled[2];

	// // // // // // // // // // // // // // // // // /
	// Volume / Decay
	BYTE		nVolume[2];
	BYTE		nDecayVolume[2];
	BYTE		bDecayEnable[2];
	BYTE		bDecayLoop[2];
	BYTE		nDecayTimer[2];
	BYTE		nDecayCount[2];
	BYTE		bDecayReset[2];

	// // // // // // // // // // // // // // // // // /
	// Sweep Unit
	BYTE		bSweepEnable[2];
	BYTE		bSweepMode[2];
	BYTE		bSweepForceSilence[2];
	BYTE		nSweepTimer[2];
	BYTE		nSweepCount[2];
	BYTE		nSweepShift[2];
	BYTE		bSweepReset[2];

	// // // // // // // // // // // // // // // // // /
	// Duty Cycle
	BYTE		nDutyCount[2];
	BYTE		nDutyCycle[2];

	// // // // // // // // // // // // // // // // // /
	// Output and Downsampling
	BYTE		bChannelMix[2];
	short		nOutputTable_L[0x100];
	short		nOutputTable_R[3][0x100];
	int			nMixL;
	int			nMixR;

	// // // // // // // // // // // // // // // // // /
	// Inverting
	BYTE		bInvert;
	BYTE		bDoInvert;
	WORD		nInvertFreqCutoff;

	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// Functions
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /

	void ClockMajor();
	void CheckSweepForcedSilence(register int i);
	void ClockMinor();
	void DoTicks(int ticks);
	void Mix_Mono(int& mix, int downsample);
	void Mix_Stereo(int& mixL, int& mixR, int downsample);
};
