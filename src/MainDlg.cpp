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

namespace MainDlg
{
#define MAX_TAB_PAGE 8
	RECT rcDisplay;
	HWND hChildDlg[MAX_TAB_PAGE], hTabCtrl;
	t_uint32 *lpStateChanged;
	const preferences_page_callback::ptr *lpCallback;
	void Create(HWND hDlg)
	{
		int i;
		TCITEM tie;
		DLGPROC dlg[MAX_TAB_PAGE] = { ConfigDlg::DlgProc, ConfigDlg2::DlgProc, ChannelsDlg::DlgProc, FME07Dlg::DlgProc,
			MMC5Dlg::DlgProc, N106Dlg::DlgProc, VRC6Dlg::DlgProc, VRC7Dlg::DlgProc };
		int res[MAX_TAB_PAGE] = { IDD_CONFIG, IDD_CONFIG_2, IDD_CHANNELS, IDD_FME07, IDD_MMC5, IDD_N106, IDD_VRC6, IDD_VRC7 };
		wchar_t *szName[MAX_TAB_PAGE] = { L"Common", L"Advanced", L"Channels", L"FME07", L"MMC5", L"N106", L"VRC6", L"VRC7" };
		// Add a tab for each of the three child dialog boxes. 
		for (i = 0, tie.mask = TCIF_TEXT; i < MAX_TAB_PAGE; i++)
		{
			tie.pszText = szName[i];
			TabCtrl_InsertItem(hTabCtrl, i, &tie);
			hChildDlg[i] = CreateDialog(core_api::get_my_instance(), MAKEINTRESOURCE(res[i]), hDlg, dlg[i]);
		}
	}

	void Destroy()
	{
		DestroyWindow(ConfigDlg::hWnd);
		DestroyWindow(ConfigDlg2::hWnd);
		DestroyWindow(ChannelsDlg::hWnd);
		DestroyWindow(FME07Dlg::hWnd);
		DestroyWindow(MMC5Dlg::hWnd);
		DestroyWindow(N106Dlg::hWnd);
		DestroyWindow(VRC6Dlg::hWnd);
		DestroyWindow(VRC7Dlg::hWnd);
	}

	void ShowPage(int i)
	{
		// On recupere la taille du Dialog enfant a afficher
		for (int j = 0; j < MAX_TAB_PAGE; j++)
			ShowWindow(hChildDlg[j], SW_HIDE);
		SetWindowPos(hChildDlg[i], NULL, rcDisplay.left, rcDisplay.top,
			rcDisplay.right - rcDisplay.left, rcDisplay.bottom - rcDisplay.top, SWP_NOZORDER | SWP_NOREDRAW);
		ShowWindow(hChildDlg[i], SW_SHOW);
		//UpdateWindow(hChildDlg[i]);
	}

	BOOL __stdcall DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
		{
			int i, vol, pan, mix, inv;

			hTabCtrl = GetDlgItem(hDlg, IDC_TAB);
			lpCallback = (const preferences_page_callback::ptr *)
				&((nosofatso_preferences *)lParam)->m_callback;
			lpStateChanged = (t_uint32 *)
				&((nosofatso_preferences *)lParam)->m_state;
			Create(hDlg);
			GetClientRect(hTabCtrl, &rcDisplay);
			TabCtrl_AdjustRect(hTabCtrl, FALSE, (LPARAM)&rcDisplay);
			MapWindowPoints(hTabCtrl, hDlg, (LPPOINT)&rcDisplay, 2);
			ConfigDlg::SetOptions(cfg_nsf_common_option.get_value(),
				cfg_nsf_advanced_option.get_value());
			ConfigDlg2::SetOptions(cfg_nsf_advanced_option.get_value(),
				cfg_nsf_common_option.get_value().bIgnoreNSFVersion);
			for (i = 0; i < 29; i++)
			{
				pan = cfg_nsf_common_option.get_value().nPan[i];
				mix = cfg_nsf_common_option.get_value().nMix[i];
				vol = cfg_nsf_common_option.get_value().nVol[i];
				inv = cfg_nsf_common_option.get_value().nInv[i];
				/* Check Params*/
				if (vol < 0)	vol = 0;
				if (vol > 255)	vol = 255;
				if (pan < -127)	pan = -127;
				if (pan > 127)	pan = 127;
				if (mix != 0)	mix = 1;
				if (inv != 1)	inv = 0;
				/* Set GUI Config */
				if (i < 5)
					CONFIG_SET_CHANNEL_OPTIONS(ChannelsDlg, 0, i, mix, vol, pan, inv)
				else if (i < 8)
					CONFIG_SET_CHANNEL_OPTIONS(VRC6Dlg, -5, i, mix, vol, pan, inv)
				else if (i < 11)
					CONFIG_SET_CHANNEL_OPTIONS(MMC5Dlg, -8, i, mix, vol, pan, inv)
				else if (i < 19)
					CONFIG_SET_CHANNEL_OPTIONS(N106Dlg, -11, i, mix, vol, pan, inv)
				else if (i < 25)
					CONFIG_SET_CHANNEL_OPTIONS(VRC7Dlg, -19, i, mix, vol, pan, inv)
				else if (i == 28)
					CONFIG_SET_CHANNEL_OPTIONS(ChannelsDlg, -23, i, mix, vol, pan, inv)
				else
					CONFIG_SET_CHANNEL_OPTIONS(FME07Dlg, -25, i, mix, vol, pan, inv)
			}

			ShowPage(0);
			ShowWindow(hDlg, SW_SHOW);
			UpdateWindow(hDlg);
		}
		break;
		case WM_NOTIFY:
		{
			const LPNMHDR lpNmhdr = reinterpret_cast<LPNMHDR>(lParam);
			switch (lpNmhdr->code)
			{
			case TCN_SELCHANGE:
			{
				ShowPage(TabCtrl_GetCurSel(hTabCtrl));
			}
			break;
			}
		}
		break;
		}
		return 0;
	}
}