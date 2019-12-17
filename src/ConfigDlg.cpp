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

#include "NSF.h"

namespace ConfigDlg
{
	HWND hWnd, hSampleRate, hStereo, hMasterVol, hVolumeDisplay, hDefaultSpeed;
	HWND hSpeedThrottle, hSpeedDisplay, hDefaultLength, hPreferPAL;
	HWND hHighPassBar, hHighPass, hHighPassOn;
	HWND hLowPassBar, hLowPass, hLowPassOn;
	HWND hPrePassBar, hPrePass, hPrePassOn;

	void SetOptions(const NSF_COMMONOPTIONS com, const NSF_ADVANCEDOPTIONS opt)
	{
		int temp;
		wchar_t str[32];

		swprintf(str, L"%d", com.nSampleRate);
		SetWindowText(hSampleRate, str);

		CheckDlgButton(hWnd, IDC_STEREO, com.nChannels == 2);
		temp = (int)(com.fMasterVolume * 1000);
		SendMessage(hMasterVol, TBM_SETPOS, 1, temp);
		swprintf(str, L"%d.%03d", temp / 1000, temp % 1000);
		SetWindowText(hVolumeDisplay, str);

		CheckDlgButton(hWnd, IDC_DEFAULTSPEED, com.bUseDefaultSpeed != 0);
		SendMessage(hSpeedThrottle, TBM_SETPOS, 1, static_cast<int>(com.fSpeedThrottle * 10));

		temp = (int)(com.fSpeedThrottle * 10);
		swprintf(str, L"%d.%d", temp / 10, temp % 10);
		SetWindowText(hSpeedDisplay, str);

		swprintf(str, L"%d:%02d.%03d", (com.nDefaultSongLength / 60000),
			(com.nDefaultSongLength / 1000) % 60, (com.nDefaultSongLength % 1000));
		SetWindowText(hDefaultLength, str);

		CheckDlgButton(hWnd, IDC_PREFERPAL, opt.bPALPreference != 0);

		swprintf(str, L"%d", opt.nHighPassBase);
		SetWindowText(hHighPass, str);
		SendMessage(hHighPassBar, TBM_SETPOS, 1, opt.nHighPassBase / 10);
		CheckDlgButton(hWnd, IDC_HIGHPASSON, opt.bHighPassEnabled != 0);

		swprintf(str, L"%d", opt.nLowPassBase);
		SetWindowText(hLowPass, str);
		SendMessage(hLowPassBar, TBM_SETPOS, 1, 600 - (opt.nLowPassBase / 100));
		CheckDlgButton(hWnd, IDC_LOWPASSON, opt.bLowPassEnabled != 0);

		swprintf(str, L"%d", opt.nPrePassBase);
		SetWindowText(hPrePass, str);
		SendMessage(hPrePassBar, TBM_SETPOS, 1, opt.nPrePassBase);
		CheckDlgButton(hWnd, IDC_PREPASSON, opt.bPrePassEnabled != 0);
	}

	void GetOptions(NSF_COMMONOPTIONS &com, NSF_ADVANCEDOPTIONS &opt)
	{
		int temp;
		wchar_t str[32];
		UINT mn, sec, ms;
		mn = sec = ms = 0;
		GetWindowText(hDefaultLength, str, 32);
		swscanf(str, L"%u:%u.%u", &mn, &sec, &ms);
		com.nDefaultSongLength = ms + (sec * 1000) + (mn * 60000);
		GetWindowText(hSampleRate, str, 32);
		swscanf(str, L"%d", &com.nSampleRate);
		temp = SendMessage(hMasterVol, TBM_GETPOS, 0, 0);
		com.fMasterVolume = temp / 1000.f;
		temp = SendMessage(hSpeedThrottle, TBM_GETPOS, 0, 0);
		com.fSpeedThrottle = temp / 10.f;
		if (com.nSampleRate < 8000)	com.nSampleRate = 8000;
		if (com.nSampleRate > 96000)	com.nSampleRate = 96000;
		com.nChannels = IsDlgButtonChecked(hWnd, IDC_STEREO) ? 2 : 1;
		opt.bPALPreference = (BYTE)IsDlgButtonChecked(hWnd, IDC_PREFERPAL);
		opt.bPrePassEnabled = (BYTE)IsDlgButtonChecked(hWnd, IDC_PREPASSON);
		opt.bLowPassEnabled = (BYTE)IsDlgButtonChecked(hWnd, IDC_LOWPASSON);
		opt.bHighPassEnabled = (BYTE)IsDlgButtonChecked(hWnd, IDC_HIGHPASSON);
		com.bUseDefaultSpeed = IsDlgButtonChecked(hWnd, IDC_DEFAULTSPEED);
		GetWindowText(hHighPass, str, 32);
		swscanf(str, L"%u", &opt.nHighPassBase);
		GetWindowText(hLowPass, str, 32);
		swscanf(str, L"%u", &opt.nLowPassBase);
		GetWindowText(hPrePass, str, 32);
		swscanf(str, L"%u", &opt.nPrePassBase);
		if (opt.nHighPassBase < 50)			opt.nHighPassBase = 50;
		if (opt.nHighPassBase > 5000)		opt.nHighPassBase = 5000;
		if (opt.nLowPassBase < 8000)		opt.nLowPassBase = 8000;
		if (opt.nLowPassBase > 60000)		opt.nLowPassBase = 60000;
		if (opt.nPrePassBase < 0)			opt.nPrePassBase = 0;
		if (opt.nPrePassBase > 100)			opt.nPrePassBase = 100;
		/*opt.nHighPassBase *= 10;
		opt.nLowPassBase *= 100;
		opt.nLowPassBase += 600;*/
	}

	BOOL __stdcall	 DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
		{
			hWnd = hDlg;
			hSampleRate = GetDlgItem(hDlg, IDC_SAMPLERATE);
			hStereo = GetDlgItem(hDlg, IDC_STEREO);
			hMasterVol = GetDlgItem(hDlg, IDC_MASTERVOL);
			hVolumeDisplay = GetDlgItem(hDlg, IDC_VOLUMEDISPLAY);
			hDefaultSpeed = GetDlgItem(hDlg, IDC_DEFAULTSPEED);
			hSpeedThrottle = GetDlgItem(hDlg, IDC_SPEEDTHROTTLE);
			hSpeedDisplay = GetDlgItem(hDlg, IDC_SPEEDDISPLAY);
			hDefaultLength = GetDlgItem(hDlg, IDC_DEFAULTLENGTH);
			hPreferPAL = GetDlgItem(hDlg, IDC_PREFERPAL);

			hHighPass = GetDlgItem(hDlg, IDC_HIGHPASS);
			hHighPassBar = GetDlgItem(hDlg, IDC_HIGHPASSBAR);
			hHighPassOn = GetDlgItem(hDlg, IDC_HIGHPASSON);
			hLowPass = GetDlgItem(hDlg, IDC_LOWPASS);
			hLowPassBar = GetDlgItem(hDlg, IDC_LOWPASSBAR);
			hLowPassOn = GetDlgItem(hDlg, IDC_LOWPASSON);
			hPrePass = GetDlgItem(hDlg, IDC_PREPASS);
			hPrePassBar = GetDlgItem(hDlg, IDC_PREPASSBAR);
			hPrePassOn = GetDlgItem(hDlg, IDC_PREPASSON);

			SendMessage(hSampleRate, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"11025"));
			SendMessage(hSampleRate, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"22050"));
			SendMessage(hSampleRate, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"32000"));
			SendMessage(hSampleRate, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"44100"));
			SendMessage(hSampleRate, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"48000"));
			SendMessage(hSampleRate, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"64000"));
			SendMessage(hSampleRate, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"72000"));
			SendMessage(hSampleRate, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"88200"));
			SendMessage(hSampleRate, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"96000"));

			SendMessage(hMasterVol, TBM_SETRANGE, 1, MAKELONG(0, 2000));
			SendMessage(hSpeedThrottle, TBM_SETRANGE, 1, MAKELONG(100, 6000));
			SendMessage(hHighPassBar, TBM_SETRANGE, 1, MAKELONG(5, 500));
			SendMessage(hLowPassBar, TBM_SETRANGE, 1, MAKELONG(0, 520));
			SendMessage(hPrePassBar, TBM_SETRANGE, 1, MAKELONG(0, 100));
		}
		break;
		case WM_COMMAND:
		{
			wchar_t str[32];
			int wmId = LOWORD(wParam), wmEvent = HIWORD(wParam), temp;
			if (wmEvent == BN_CLICKED)
			{
				switch (wmId)
				{
				case IDC_STEREO:
				case IDC_PREFERPAL:
				case IDC_PREPASSON:
				case IDC_LOWPASSON:
				case IDC_HIGHPASSON:
				case IDC_DEFAULTSPEED:
					CONFIG_CHANGE_NOTIFY();
				default:
					break;
				}
			}
			else if (wmEvent == EN_UPDATE)
			{
				switch (wmId)
				{
					/*temp = SendMessage(hHighPassBar, TBM_GETPOS, 0, 0) * 10;
					swprintf(str, L"%d", temp);
					SetWindowText(hHighPass, str);
					temp = (600 - SendMessage(hLowPassBar, TBM_GETPOS, 0, 0)) * 100;
					swprintf(str, L"%d", temp);
					SetWindowText(hLowPass, str);
					temp = SendMessage(hPrePassBar, TBM_GETPOS, 0, 0);
					wsprintf(str, L"%d", temp);
					SetWindowText(hPrePass, str);*/
				case IDC_PREPASS:
				case IDC_LOWPASS:
				case IDC_HIGHPASS:
				case IDC_DEFAULTLENGTH:
					CONFIG_CHANGE_NOTIFY();
				default:
					break;
				}
			}
			else if (wmEvent == CBN_EDITCHANGE)
			{
				CONFIG_CHANGE_NOTIFY();
			}
		}
		break;
		case WM_HSCROLL:
		{
			int temp;
			wchar_t str[32];
			if (((HWND)lParam) == hMasterVol)
			{
				temp = SendMessage(hMasterVol, TBM_GETPOS, 0, 0);
				swprintf(str, L"%d.%03d", temp / 1000, temp % 1000);
				SetWindowText(hVolumeDisplay, str);
				CONFIG_CHANGE_NOTIFY();
			}
			else if (((HWND)lParam) == hSpeedThrottle)
			{
				temp = SendMessage(hSpeedThrottle, TBM_GETPOS, 0, 0);
				swprintf(str, L"%d.%d", temp / 10, temp % 10);
				SetWindowText(hSpeedDisplay, str);
				CONFIG_CHANGE_NOTIFY();
			}
			else if (((HWND)lParam) == hHighPassBar)
			{
				temp = SendMessage(hHighPassBar, TBM_GETPOS, 0, 0) * 10;
				swprintf(str, L"%d", temp);
				SetWindowText(hHighPass, str);
				CONFIG_CHANGE_NOTIFY();
			}
			else if (((HWND)lParam) == hLowPassBar)
			{
				temp = (600 - SendMessage(hLowPassBar, TBM_GETPOS, 0, 0)) * 100;
				swprintf(str, L"%d", temp);
				SetWindowText(hLowPass, str);
				CONFIG_CHANGE_NOTIFY();
			}
			else if (((HWND)lParam) == hPrePassBar)
			{
				temp = SendMessage(hPrePassBar, TBM_GETPOS, 0, 0);
				wsprintf(str, L"%d", temp);
				SetWindowText(hPrePass, str);
				CONFIG_CHANGE_NOTIFY();
			}
		}
		break;
		default:
			break;
		}
		return 0;
	}
}

namespace ConfigDlg2
{

	HWND hWnd, hIgnore4011, hIgnoreBRK, hIgnoreIllegal, hNoWait, hCleanAXY;
	HWND hResetDuty, hDMCPopReducer, hForce4017, hN106PopReducer;
	HWND hFDSPopReducer;
	HWND hInvertCutoff, hInvertCutoffHz, hIgnoreVersion;

	void SetOptions(const NSF_ADVANCEDOPTIONS opt, BYTE ignoreversion)
	{
		wchar_t str[32];

		CheckDlgButton(hWnd, IDC_IGNORE4011, opt.bIgnore4011Writes != 0);
		CheckDlgButton(hWnd, IDC_IGNOREBRK, opt.bIgnoreBRK != 0);
		CheckDlgButton(hWnd, IDC_IGNOREILLEGAL, opt.bIgnoreIllegalOps != 0);
		CheckDlgButton(hWnd, IDC_NOWAIT, opt.bNoWaitForReturn != 0);
		CheckDlgButton(hWnd, IDC_CLEANAXY, opt.bCleanAXY != 0);
		CheckDlgButton(hWnd, IDC_RESETDUTY, opt.bResetDuty != 0);
		CheckDlgButton(hWnd, IDC_IGNOREVERSION, ignoreversion != 0);

		CheckDlgButton(hWnd, IDC_INVERTCUTOFF, opt.nInvertCutoffHz > 0);
		swprintf(str, L"%d", ((opt.nInvertCutoffHz >= 0) ? opt.nInvertCutoffHz : -opt.nInvertCutoffHz));
		SetWindowText(hInvertCutoffHz, str);

		CheckDlgButton(hWnd, IDC_DMCPOPREDUCER, opt.bDMCPopReducer != 0);
		SendMessage(hForce4017, CB_SETCURSEL, opt.nForce4017Write, 0);
		// CheckDlgButton(hWnd, IDC_FORCE4017, opt.nForce4017Write);
		CheckDlgButton(hWnd, IDC_N106POPREDUCER, opt.bN106PopReducer != 0);
		CheckDlgButton(hWnd, IDC_FDSPOPREDUCER, opt.bFDSPopReducer != 0);
	}

	void GetOptions(NSF_ADVANCEDOPTIONS &opt, BYTE &ignoreversion)
	{
		wchar_t str[32];

		opt.bIgnore4011Writes = (BYTE)IsDlgButtonChecked(hWnd, IDC_IGNORE4011);
		opt.bIgnoreBRK = (BYTE)IsDlgButtonChecked(hWnd, IDC_IGNOREBRK);
		opt.bIgnoreIllegalOps = (BYTE)IsDlgButtonChecked(hWnd, IDC_IGNOREILLEGAL);
		opt.bNoWaitForReturn = (BYTE)IsDlgButtonChecked(hWnd, IDC_NOWAIT);
		opt.bCleanAXY = (BYTE)IsDlgButtonChecked(hWnd, IDC_CLEANAXY);
		opt.bResetDuty = (BYTE)IsDlgButtonChecked(hWnd, IDC_RESETDUTY);

		opt.bDMCPopReducer = (BYTE)IsDlgButtonChecked(hWnd, IDC_DMCPOPREDUCER);
		opt.nForce4017Write = (BYTE)SendMessage(hForce4017, CB_GETCURSEL, 0, 0);
		opt.bN106PopReducer = (BYTE)IsDlgButtonChecked(hWnd, IDC_N106POPREDUCER);
		opt.bFDSPopReducer = (BYTE)IsDlgButtonChecked(hWnd, IDC_FDSPOPREDUCER);

		opt.nInvertCutoffHz = 0;
		GetWindowText(hInvertCutoffHz, str, 32);
		swscanf(str, L"%u", &opt.nInvertCutoffHz);
		if (!IsDlgButtonChecked(hWnd, IDC_INVERTCUTOFF))
			opt.nInvertCutoffHz = -opt.nInvertCutoffHz;
		ignoreversion = IsDlgButtonChecked(hWnd, IDC_IGNOREVERSION) != 0;
	}

	BOOL __stdcall	DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
		{
			hWnd = hDlg;
			hIgnore4011 = GetDlgItem(hDlg, IDC_IGNORE4011);
			hIgnoreBRK = GetDlgItem(hDlg, IDC_IGNOREBRK);
			hIgnoreIllegal = GetDlgItem(hDlg, IDC_IGNOREILLEGAL);
			hNoWait = GetDlgItem(hDlg, IDC_NOWAIT);
			hCleanAXY = GetDlgItem(hDlg, IDC_CLEANAXY);
			hResetDuty = GetDlgItem(hDlg, IDC_RESETDUTY);

			hDMCPopReducer = GetDlgItem(hDlg, IDC_DMCPOPREDUCER);
			hForce4017 = GetDlgItem(hDlg, IDC_FORCE4017);
			hN106PopReducer = GetDlgItem(hDlg, IDC_N106POPREDUCER);
			hInvertCutoff = GetDlgItem(hDlg, IDC_INVERTCUTOFF);
			hInvertCutoffHz = GetDlgItem(hDlg, IDC_INVERTCUTOFFHZ);
			hFDSPopReducer = GetDlgItem(hDlg, IDC_FDSPOPREDUCER);

			hIgnoreVersion = GetDlgItem(hDlg, IDC_IGNOREVERSION);

			SendMessage(hForce4017, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"None"));
			SendMessage(hForce4017, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Force $00"));
			SendMessage(hForce4017, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Force $80"));
		}
		break;
		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam), wmEvent = HIWORD(wParam);
			if (wmEvent == BN_CLICKED)
			{
				switch (wmId)
				{
				case IDC_NOWAIT:
				case IDC_CLEANAXY:
				case IDC_RESETDUTY:
				case IDC_IGNOREBRK:
				case IDC_IGNORE4011:
				case IDC_IGNOREILLEGAL:
				case IDC_FDSPOPREDUCER:
				case IDC_DMCPOPREDUCER:
				case IDC_N106POPREDUCER:
				case IDC_INVERTCUTOFF:
				case IDC_INVERTCUTOFFHZ:
				case IDC_IGNOREVERSION:
					CONFIG_CHANGE_NOTIFY();
				}
			}
			else if (wmEvent == EN_UPDATE && wmId == IDC_INVERTCUTOFFHZ)
			{
				CONFIG_CHANGE_NOTIFY();
			}
			else if (wmEvent == CBN_SELCHANGE && wmId == IDC_FORCE4017)
			{
				CONFIG_CHANGE_NOTIFY();
			}
		}
		break;
		default:
			break;
		}
		return 0;
	}
}