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

namespace N106Dlg
{
  HWND hWnd, hMix[8], hVol[8], hInv[8], hPan[8], hVolBox[8], hPanBox[8];

  BOOL __stdcall  DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
      for (int i = 0; i < 8; i++)
      {
        hWnd = hDlg;
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
      break;
    }
    case WM_COMMAND:
    {
      int wmId = LOWORD(wParam), wmEvent = HIWORD(wParam);
      if (wmId >= IDC_MIX_1 && wmId <= IDC_MIX_8 &&wmEvent == BN_CLICKED)
      {
        CONFIG_CHANGE_NOTIFY();
      }
      else if (wmId >= IDC_VOLBOX_1 && wmId <= IDC_VOLBOX_8 && wmEvent == EN_UPDATE)
      {
        CONFIG_CHANGE_NOTIFY();
      }
      else if (wmId >= IDC_PANBOX_1 && wmId <= IDC_PANBOX_8 && wmEvent == EN_UPDATE)
      {
        CONFIG_CHANGE_NOTIFY();
      }
      else
      {
        for (int i = 0; i < 8 * 2; i += 2)
        {
          if (wmId == IDC_INV_1 + i && wmEvent == BN_CLICKED)
          {
            CONFIG_CHANGE_NOTIFY();
          }
        }
      }
    }
    break;
    case WM_HSCROLL:
    {
      wchar_t str[4];
      register int i, j;

      for (i = 0; i < 8; i++)
      {
        if (reinterpret_cast<HWND>(lParam) == hVol[i])
        {
          j = SendMessage(hVol[i], TBM_GETPOS, 0, 0);
          swprintf(str, L"%d", j);
          SetWindowText(hVolBox[i], str);
          CONFIG_CHANGE_NOTIFY();
        }
        if (reinterpret_cast<HWND>(lParam) == hPan[i])
        {
          j = SendMessage(hPan[i], TBM_GETPOS, 0, 0);
          swprintf(str, L"%d", j);
          SetWindowText(hPanBox[i], str);
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
