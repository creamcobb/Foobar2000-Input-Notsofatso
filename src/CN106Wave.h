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

// I love this chip!  Although it's not as easy as the others to emulate... but it sure sounds nice.
// 

class CN106Wave
{
public:
  // // // // // // // // // // // // // // // 
  // All Channel Stuff
  BYTE nActiveChannels;
  BYTE bAutoIncrement;
  BYTE nCurrentAddress;
  BYTE nRAM[0x100]; // internal memory for registers/wave data
  float fFrequencyLookupTable[8]; // lookup table for frequency conversions


  // // // // // // // // // // // // // // // 
  // Individual channel stuff
  // // // // // // // // // // // // // // // 
  // Wavelength / Frequency
  QUAD  nFreqReg[8];
  float fFreqTimer[8];
  float fFreqCount[8];

  // // // // // // // // // // // // // // // 
  // Wave data length / remaining
  BYTE nWaveSize[8];
  BYTE nWaveRemaining[8];

  // // // // // // // // // // // // // // // 
  // Wave data position
  BYTE nWavePosStart[8];
  BYTE nWavePos[8];
  BYTE nOutput[8];

  // // // // // // // // // // // // // // // 
  // Volume
  BYTE nVolume[8];

  // // // // // // // // // // // // // // // 
  // Pop Reducer
  BYTE nPreVolume[8];
  BYTE nPopCheck[8];

  // // // // // // // // // // // // // // // 
  // Mixing
  short nOutputTable_L[8][0x10][0x10];
  short nOutputTable_R[8][0x10][0x10];
  int nMixL[8];
  int nMixR[8];

  // // // // // // // // // // // // // // // 
  // Inverting
  BYTE bInvert[8];
  BYTE bDoInvert[8];
  UINT nInvertFreqCutoff[8][8];
  BYTE nWaveSizeWritten[8];
  BYTE nInvCheck[8];

  
  // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
  // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
  // Functions
  // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /
  // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // /

  void DoTicks(int ticks, BYTE* mix);
  void Mix_Mono(int& mix, int downsample);
  void Mix_Stereo(int& mixL, int& mixR, int downsample);
};
