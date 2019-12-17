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

#include "CNSFCore.h"

void CTNDWaves::ClockMajor()		// decay (noise), linear (tri)
{
	// noise's decay
	if (bNoiseDecayReset) {
		bNoiseDecayReset = 0;
		nNoiseDecayCount = nNoiseDecayTimer;
		nNoiseDecayVolume = 0x0F;
		if (bNoiseDecayEnable)
			nNoiseVolume = nNoiseDecayVolume;
	}
	else {
		if (nNoiseDecayCount)
			nNoiseDecayCount--;
		else
		{
			nNoiseDecayCount = nNoiseDecayTimer;
			if (nNoiseDecayVolume)
				nNoiseDecayVolume--;
			else
			{
				if (bNoiseDecayLoop)
					nNoiseDecayVolume = 0x0F;
			}

			if (bNoiseDecayEnable)
				nNoiseVolume = nNoiseDecayVolume;
		}
	}

	// triangle's linear
	if (bTriLinearHalt)
		nTriLinearCount = nTriLinearLoad;
	else if (nTriLinearCount)
		nTriLinearCount--;

	if (!bTriLinearControl)
		bTriLinearHalt = 0;
}

void CTNDWaves::ClockMinor()		// length
{
	if (bNoiseLengthEnabled && nNoiseLengthCount)
		nNoiseLengthCount--;

	if (bTriLengthEnabled && nTriLengthCount)
		nTriLengthCount--;
}

int CTNDWaves::DoTicks(int ticks)		// returns number of burned cycles (burned by DMC's DMA)
{
	register int mn;
	register int out;
	int burnedcycles = 0;

	while (ticks)
	{
		mn = nNoiseFreqCount;
		if (nTriFreqTimer.W > 8)
			mn = min(mn, nTriFreqCount);
		if (bDMCActive)
			mn = min(mn, nDMCFreqCount);
		mn = min(mn, ticks);
		ticks -= mn;

		nNoiseFreqCount -= mn;

		if (nTriFreqTimer.W > 8)
			nTriFreqCount -= mn;

		if (!bTriChannelMix)												out = 0;
		else															out = nTriOutput << 11;

		if (bNoiseRandomOut && nNoiseLengthCount && bNoiseChannelMix)	out |= nNoiseVolume << 7;

		if (bDMCChannelMix)												out |= nDMCOutput;


		nMixL += nOutputTable_L[out] * mn;
		if (bDoInvert & 4)					nMixR -= nOutputTable_R[((bDoInvert ^ 7) << 15) | out] * mn;
		else								nMixR += nOutputTable_R[(bDoInvert << 15) | out] * mn;

		/*	Tri				*/
		if (!nTriFreqCount)
		{
			nTriFreqCount = nTriFreqTimer.W + 1;
			if (nTriLengthCount && nTriLinearCount)
			{
				nTriStep = (nTriStep + 1) & 0x1F;

				if (nTriStep & 0x10)		nTriOutput = nTriStep ^ 0x1F;
				else					nTriOutput = nTriStep;

				if (!nTriStep)
				{
					if (bInvert & 1)
					{
						if (nTriFreqTimer.W <= nInvertFreqCutoff_Tri)	bDoInvert |= 1;
						else											bDoInvert &= 6;
					}
					else	bDoInvert &= 6;
				}
			}
		}

		/*	Noise			*/
		if (!nNoiseFreqCount)
		{
			nNoiseFreqCount = nNoiseFreqTimer;
			nNoiseRandomShift <<= 1;
			bNoiseRandomOut = (((nNoiseRandomShift << bNoiseRandomMode) ^ nNoiseRandomShift) & 0x8000) ? 1 : 0;
			if (bNoiseRandomOut)
				nNoiseRandomShift |= 0x01;
			else
			{
				if (bInvert & 2)
				{
					if (nNoiseFreqTimer <= nInvertFreqCutoff_Noise)	bDoInvert |= 2;
					else											bDoInvert &= 5;
				}
				else	bDoInvert &= 5;
			}
		}


		/*	DMC				*/
		if (bInvert & 4)		bDoInvert |= 4;
		else				bDoInvert &= 3;
		if (bDMCActive)
		{
			nDMCFreqCount -= mn;
			if (nDMCFreqCount > 0)
				continue;

			nDMCFreqCount = nDMCFreqTimer;

			if (bDMCSampleBufferEmpty && nDMCBytesRemaining)
			{
				burnedcycles += 4;		// 4 cycle burn!
				nDMCSampleBuffer = pDMCDMAPtr[nDMCDMABank][nDMCDMAAddr];
				nDMCDMAAddr++;
				if (nDMCDMAAddr & 0x1000)
				{
					nDMCDMAAddr &= 0x0FFF;
					nDMCDMABank = (nDMCDMABank + 1) & 0x07;
				}

				bDMCSampleBufferEmpty = 0;
				nDMCBytesRemaining--;
				if (!nDMCBytesRemaining)
				{
					if (bDMCLoop)
					{
						nDMCDMABank = nDMCDMABank_Load;
						nDMCDMAAddr = nDMCDMAAddr_Load;
						nDMCBytesRemaining = nDMCLength;
					}
					else if (bDMCIRQEnabled)
						bDMCIRQPending = 1;
				}
			}

			if (!nDMCDeltaBit)
			{
				nDMCDeltaBit = 8;
				bDMCDeltaSilent = bDMCSampleBufferEmpty;
				nDMCDelta = nDMCSampleBuffer;
				bDMCSampleBufferEmpty = 1;
			}

			if (nDMCDeltaBit)
			{
				nDMCDeltaBit--;
				if (!bDMCDeltaSilent)
				{
					if (nDMCDelta & 0x01)
					{
						if (nDMCOutput < 0x7E) nDMCOutput += 2;
					}
					else if (nDMCOutput > 1)	nDMCOutput -= 2;
				}
				nDMCDelta >>= 1;
			}

			if (!nDMCBytesRemaining && bDMCSampleBufferEmpty && bDMCDeltaSilent)
				bDMCActive = nDMCDeltaBit = 0;
		}
	}
	return burnedcycles;
}

void CTNDWaves::Mix_Mono(int& mix, int downsample)
{
	mix += (nMixL / downsample);
	nMixL = 0;
}

void CTNDWaves::Mix_Stereo(int& mixL, int& mixR, int downsample)
{
	mixL += (nMixL / downsample);
	mixR += (nMixR / downsample);

	nMixL = nMixR = 0;
}
