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

void CMMC5SquareWave::ClockMajor()		// decay
{
	if (nDecayCount)
		nDecayCount--;
	else
	{
		nDecayCount = nDecayTimer;
		if (nDecayVolume)
			nDecayVolume--;
		else
		{
			if (bDecayLoop)
				nDecayVolume = 0x0F;
		}

		if (bDecayEnable)
			nVolume = nDecayVolume;
	}
}

void CMMC5SquareWave::ClockMinor()		// length
{
	if (bLengthEnabled && nLengthCount)
		nLengthCount--;
}

void CMMC5SquareWave::DoTicks(int ticks, BYTE mix)
{
	register int mn;

	if (nFreqTimer.W < 8) return;

	while (ticks)
	{
		mn = min(nFreqCount, ticks);
		ticks -= mn;

		nFreqCount -= mn;

		if (mix && (nDutyCount < nDutyCycle) && nLengthCount)
		{
			nMixL += nOutputTable_L[nVolume] * mn;
			nMixR += nOutputTable_R[nVolume] * (bDoInvert ? -mn : mn);
		}

		if (!nFreqCount)
		{
			nFreqCount = nFreqTimer.W + 1;
			nDutyCount = (nDutyCount + 1) & 0x0F;
			if (!nDutyCount)
			{
				bDoInvert = bInvert;
				if (nInvertFreqCutoff < nFreqTimer.W)
					bDoInvert = 0;
			}
		}
	}
}

void CMMC5SquareWave::Mix_Mono(int& mix, int downsample)
{
	mix += (nMixL / downsample);
	nMixL = 0;
}

void CMMC5SquareWave::Mix_Stereo(int& mixL, int& mixR, int downsample)
{
	mixL += (nMixL / downsample);
	mixR += (nMixR / downsample);

	nMixL = nMixR = 0;
}

void CMMC5VoiceWave::DoTicks(int ticks)
{
	nMixL += nOutputTable_L[nOutput] * ticks;
	nMixR += nOutputTable_R[nOutput] * (bInvert ? -ticks : ticks);
}

void CMMC5VoiceWave::Mix_Mono(int& mix, int downsample)
{
	mix += (nMixL / downsample);
	nMixL = 0;
}

void CMMC5VoiceWave::Mix_Stereo(int& mixL, int& mixR, int downsample)
{
	mixL += (nMixL / downsample);
	mixR += (nMixR / downsample);

	nMixL = nMixR = 0;
}
