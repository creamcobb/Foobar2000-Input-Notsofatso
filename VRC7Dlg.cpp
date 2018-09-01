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

namespace VRC7Dlg
{
	HWND hWnd, hMix[6], hVol[6], hInv[6], hPan[6], hVolBox[6], hPanBox[6];

	BOOL __stdcall	 DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
		{
			hWnd = hDlg;

			for (int i = 0; i < 6; i++)
			{
				// get vars for all the controls
				hMix[i] = GetDlgItem(hDlg, IDC_MIX_1 + i);
				hVol[i] = GetDlgItem(hDlg, IDC_VOL_1 + i);
				hPan[i] = GetDlgItem(hDlg, IDC_PAN_1 + i);
				hInv[i] = GetDlgItem(hDlg, IDC_INV_1 + i * 2);
				hVolBox[i] = GetDlgItem(hDlg, IDC_VOLBOX_1 + i);
				hPanBox[i] = GetDlgItem(hDlg, IDC_PANBOX_1 + i);

				// set the ranges for the scrollbars
				SendMessage(hVol[i], TBM_SETRANGE, 1, MAKELONG(0, 255));
				SendMessage(hPan[i], TBM_SETRANGE, 1, MAKELONG(-127, 127));
			}
		}
		break;
		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam), wmEvent = HIWORD(wParam);
			if (wmEvent == BN_CLICKED)
			{
				if (wmId >= IDC_MIX_1 && wmId <= IDC_MIX_6)
				{
					CONFIG_CHANGE_NOTIFY();
				}
				else
				{
					for (int i = 0; i < 6 * 2; i += 2)
					{
						if (wmId == IDC_INV_1 + i && wmEvent == BN_CLICKED)
						{
							CONFIG_CHANGE_NOTIFY();
						}
					}
				}
			}
			else if (wmEvent == EN_UPDATE)
			{
				if (wmId >= IDC_VOLBOX_1 && wmId <= IDC_VOLBOX_6)
				{
					CONFIG_CHANGE_NOTIFY();
				}
				else if (wmId >= IDC_PANBOX_1 && wmId <= IDC_PANBOX_6)
				{
					CONFIG_CHANGE_NOTIFY();
				}
			}
		}
		break;
		case WM_HSCROLL:
		{
			wchar_t str[4];
			register int i, j;

			for (i = 0; i < 6; i++)
			{
				if (reinterpret_cast<HWND>(lParam) == hVol[i])
				{
					j = SendMessage(hVol[i], TBM_GETPOS, 0, 0);
					swprintf(str, L"%d", j);
					SetWindowText(hVolBox[i], str);
					//nsfCore.SetChannelOptions(i + 19, -1, j, 1000, -1);
					CONFIG_CHANGE_NOTIFY();
				}
				if (reinterpret_cast<HWND>(lParam) == hPan[i])
				{
					j = SendMessage(hPan[i], TBM_GETPOS, 0, 0);
					swprintf(str, L"%d", j);
					SetWindowText(hPanBox[i], str);
					//nsfCore.SetChannelOptions(i + 19, -1, -1, j, -1);
					CONFIG_CHANGE_NOTIFY();
				}
			}
		}
		break;
		default:
			break;
		}
		return 0;
	}
}