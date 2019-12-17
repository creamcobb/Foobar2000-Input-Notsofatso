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
// These are similar to the native NES square waves, only they lack a sweep unit and aren't
// (as far as I know), interdependant on each other's output.
// 
// The voice is similar to the $4011 register, but without the DMC's sample playback capabilities.
// so it's rather useless.  I haven't been able to find any game to test it with (since I'm not aware of
// any who use it... nor do I see how it could be used in an NSF because of lack of IRQ support).  But it's
// included anyway.  Theoretically it should work... but like I said, can't test it.
class CMMC5SquareWave
{
public:

	// // // // // // // // // // // // // // // // // /
	// Programmable Timer
	TWIN		nFreqTimer;
	int			nFreqCount;

	// // // // // // // // // // // // // // // // // /
	// Length Counter
	BYTE		nLengthCount;
	BYTE		bLengthEnabled;
	BYTE		bChannelEnabled;

	// // // // // // // // // // // // // // // // // /
	// Volume / Decay
	BYTE		nVolume;
	BYTE		nDecayVolume;
	BYTE		bDecayEnable;
	BYTE		bDecayLoop;
	BYTE		nDecayTimer;
	BYTE		nDecayCount;

	// // // // // // // // // // // // // // // // // /
	// Duty Cycle
	BYTE		nDutyCount;
	BYTE		nDutyCycle;

	// // // // // // // // // // // // // // // // // /
	// Output and Downsampling
	BYTE		bChannelMix;
	short		nOutputTable_L[0x10];
	short		nOutputTable_R[0x10];
	int			nMixL;
	int			nMixR;

	// // // // // // // // // // // // // // // // // /
	// Inverting
	BYTE		bDoInvert;
	BYTE		bInvert;
	WORD		nInvertFreqCutoff;

	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// Functions
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /

	void ClockMajor();
	void ClockMinor();
	void DoTicks(int ticks, BYTE mix);
	void Mix_Mono(int& mix, int downsample);
	void Mix_Stereo(int& mixL, int& mixR, int downsample);
};


class CMMC5VoiceWave
{
public:
	// // // // // // // // // // // // // // // // // /
	// Everything
	BYTE		nOutput;
	short		nOutputTable_L[0x80];
	short		nOutputTable_R[0x80];
	int			nMixL;
	int			nMixR;
	BYTE		bInvert;

	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// Functions
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /


	void DoTicks(int ticks);
	void Mix_Mono(int& mix, int downsample);
	void Mix_Stereo(int& mixL, int& mixR, int downsample);
};
