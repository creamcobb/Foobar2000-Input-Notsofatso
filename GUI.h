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
#include <Windows.h>

#define CONFIG_SET_CHANNEL_OPTIONS(SPACE, DIFF, ORG_CHAN, MIX, VOL, PAN, INV) \
{\
	int CHAN;wchar_t str[32];\
	CHAN = ORG_CHAN;CHAN += DIFF;\
	CheckDlgButton(SPACE::hWnd, IDC_MIX_1 + CHAN, MIX != 0);\
	CheckDlgButton(SPACE::hWnd, IDC_INV_1 + CHAN * 2, INV != 0);\
	SendMessage(SPACE::hVol[CHAN], TBM_SETPOS, 1, VOL);\
	swprintf(str, L"%d", VOL);\
	SetWindowText(SPACE::hVolBox[CHAN], str);\
	SendMessage(SPACE::hPan[CHAN], TBM_SETPOS, 1, PAN);\
	swprintf(str, L"%d", PAN);\
	SetWindowText(SPACE::hPanBox[CHAN], str);\
}

#define CONFIG_GET_CHANNEL_OPTIONS(SPACE, DIFF, ORG_CHAN, P) \
{\
	int CHAN;\
	CHAN = ORG_CHAN;CHAN += DIFF;\
	(P)->nMix[ORG_CHAN] = IsDlgButtonChecked(SPACE::hWnd, IDC_MIX_1 + CHAN);\
	(P)->nInv[ORG_CHAN] = IsDlgButtonChecked(SPACE::hWnd, IDC_INV_1 + CHAN * 2);\
	(P)->nVol[ORG_CHAN] = SendMessage(SPACE::hVol[CHAN], TBM_GETPOS, 0, 0);\
	(P)->nPan[ORG_CHAN] = SendMessage(SPACE::hPan[CHAN], TBM_GETPOS, 0, 0);\
}

#define CONFIG_CHANGE_NOTIFY() \
	*MainDlg::lpStateChanged = preferences_state::changed | preferences_state::resettable;\
	(*MainDlg::lpCallback)->on_state_changed();\
	break;	

namespace MainDlg
{
	extern t_uint32 *lpStateChanged;
	extern const preferences_page_callback::ptr *lpCallback;
	extern HWND hWnd, hFrame, hTabCtrl;

	BOOL __stdcall DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

namespace ChannelsDlg
{
	extern HWND hWnd, hMix[6], hVol[6], hInv[6], hPan[6], hVolBox[6], hPanBox[6];

	BOOL __stdcall DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

namespace ConfigDlg
{
	extern HWND hWnd, hSampleRate, hStereo, hMasterVol, hVolumeDisplay, hDefaultSpeed;
	extern HWND hSpeedThrottle, hSpeedDisplay, hDefaultLength, hPreferPAL;
	extern HWND hHighPassBar, hHighPass, hHighPassOn;
	extern HWND hLowPassBar, hLowPass, hLowPassOn;
	extern HWND hPrePassBar, hPrePass, hPrePassOn;

	void SetOptions(const NSF_COMMONOPTIONS com, const NSF_ADVANCEDOPTIONS opt);
	void GetOptions(NSF_COMMONOPTIONS &com, NSF_ADVANCEDOPTIONS &opt);
	BOOL __stdcall DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

namespace ConfigDlg2
{
	extern HWND hWnd, hIgnore4011, hIgnoreBRK, hIgnoreIllegal, hNoWait, hCleanAXY;
	extern HWND hResetDuty, hDMCPopReducer, hForce4017, hN106PopReducer;
	extern HWND hFDSPopReducer;
	extern HWND hInvertCutoff, hInvertCutoffHz, hIgnoreVersion;

	void SetOptions(const NSF_ADVANCEDOPTIONS opt, BYTE ignoreversion);
	void GetOptions(NSF_ADVANCEDOPTIONS &opt, BYTE &ignoreversion);
	BOOL __stdcall DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

namespace FME07Dlg
{
	extern HWND hWnd, hMix[3], hVol[3], hInv[3], hPan[3], hVolBox[3], hPanBox[3];

	BOOL __stdcall DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

namespace MMC5Dlg
{
	extern HWND hWnd, hMix[3], hVol[3], hInv[3], hPan[3], hVolBox[3], hPanBox[3];

	BOOL __stdcall DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

namespace N106Dlg
{
	extern HWND hWnd, hMix[8], hVol[8], hInv[8], hPan[8], hVolBox[8], hPanBox[8];

	BOOL __stdcall DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

namespace VRC6Dlg
{
	extern HWND hWnd, hMix[3], hVol[3], hInv[3], hPan[3], hVolBox[3], hPanBox[3];

	BOOL __stdcall DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

namespace VRC7Dlg
{
	extern HWND hWnd, hMix[6], hVol[6], hInv[6], hPan[6], hVolBox[6], hPanBox[6];

	BOOL __stdcall DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
}