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

class CVRC6SawWave
{
public:

	// // // // // // // // // // // // // // // // // /
	// Frequency Control
	TWIN		nFreqTimer;
	int			nFreqCount;

	// // // // // // // // // // // // // // // // // /
	// Flags
	BYTE		bChannelEnabled;

	// // // // // // // // // // // // // // // // // /
	// Phase Accumulator
	BYTE		nAccumRate;
	BYTE		nAccum;
	BYTE		nAccumStep;

	// // // // // // // // // // // // // // // // // /
	// Output and Downsampling
	short		nOutputTable_L[0x20];
	short		nOutputTable_R[0x20];
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

	void DoTicks(int ticks, BYTE mix);
	void Mix_Mono(int& mix, int downsample);
	void Mix_Stereo(int& mixL, int& mixR, int downsample);
};
