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

// Because of inter-dependencies between channel output, the Triangle,
// Noise, and DMC are all combined here in this class.

class CTNDWaves
{
public:

	/*	Triangle			*/

	// // // // // // // // // // // // // // // // // /
	// Programmable Timer
	TWIN		nTriFreqTimer;
	int			nTriFreqCount;

	// // // // // // // // // // // // // // // // // /
	// Length Counter
	BYTE		nTriLengthCount;
	BYTE		bTriLengthEnabled;
	BYTE		bTriChannelEnabled;

	// // // // // // // // // // // // // // // // // /
	// Linear Counter
	BYTE		nTriLinearCount;
	BYTE		nTriLinearLoad;
	BYTE		bTriLinearHalt;
	BYTE		bTriLinearControl;

	// // // // // // // // // // // // // // // // // /
	// Tri-Step Generator / Output
	BYTE		nTriStep;
	BYTE		nTriOutput;
	BYTE		bTriChannelMix;


	/*	Noise				*/

	// // // // // // // // // // // // // // // // // /
	// Programmable Timer
	WORD		nNoiseFreqTimer;
	int			nNoiseFreqCount;

	// // // // // // // // // // // // // // // // // /
	// Length Counter
	BYTE		nNoiseLengthCount;
	BYTE		bNoiseLengthEnabled;
	BYTE		bNoiseChannelEnabled;

	// // // // // // // // // // // // // // // // // /
	// Volume / Decay
	BYTE		nNoiseVolume;
	BYTE		nNoiseDecayVolume;
	BYTE		bNoiseDecayEnable;
	BYTE		bNoiseDecayLoop;
	BYTE		nNoiseDecayTimer;
	BYTE		nNoiseDecayCount;
	BYTE		bNoiseDecayReset;

	// // // // // // // // // // // // // // // // // /
	// Random Number Generator
	WORD		nNoiseRandomShift;
	BYTE		bNoiseRandomMode;			// 1 = 32k, 6 = 93-bit
	BYTE		bNoiseRandomOut;
	BYTE		bNoiseChannelMix;


	/*	DMC					*/

	// // // // // // // // // // // // // // // // // /
	// Play Mode
	BYTE		bDMCLoop;
	BYTE		bDMCIRQEnabled;
	BYTE		bDMCIRQPending;

	// // // // // // // // // // // // // // // // // /
	// Address / DMA
	BYTE		nDMCDMABank_Load;
	WORD		nDMCDMAAddr_Load;
	BYTE		nDMCDMABank;
	WORD		nDMCDMAAddr;
	BYTE*		pDMCDMAPtr[8];

	// // // // // // // // // // // // // // // // // 
	// Length / Input
	WORD		nDMCLength;
	WORD		nDMCBytesRemaining;
	BYTE		nDMCDelta;
	BYTE		nDMCDeltaBit;
	BYTE		bDMCDeltaSilent;
	BYTE		nDMCSampleBuffer;
	BYTE		bDMCSampleBufferEmpty;

	// // // // // // // // // // // // // // // // // 
	// Frequency
	WORD		nDMCFreqTimer;
	WORD		nDMCFreqCount;

	// // // // // // // // // // // // // // // // // 
	// Output
	BYTE		bDMCActive;
	BYTE		nDMCOutput;
	BYTE		bDMCChannelMix;

	/*	All's Output table			*/
	short*		nOutputTable_L;
	short*		nOutputTable_R;
	int			nMixL;
	int			nMixR;

	// // // // // // // // // // // // // // // // // /
	// Inverting
	BYTE		bInvert;
	BYTE		bDoInvert;
	WORD		nInvertFreqCutoff_Noise;
	WORD		nInvertFreqCutoff_Tri;
	
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// Functions
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
	// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /

	void ClockMajor();
	void ClockMinor();
	int DoTicks(int ticks);
	void Mix_Mono(int& mix, int downsample);
	void Mix_Stereo(int& mixL, int& mixR, int downsample);
};
