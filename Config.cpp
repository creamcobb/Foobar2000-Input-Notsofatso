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
#define MAKE_FUNCTION_PARAM(x, i) x.nMix[i], x.nVol[i], x.nPan[i], x.nInv[i]

#define DEFAULT_MIX { 1,1,1,1,1, 1,1,1, 1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1 }
#define DEFAULT_INV { 1,1,0,0,0, 1,0,0, 1,1,0, 1,1,0,0,1,1,0,0, 1,1,0,0,1,1, 0,1,0,0 }
#define DEFAULT_PAN { 0,0,0,0,0, 0,0,0, 0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0 }
#define DEFAULT_VOL { 255,255,255,255,255, 255,255,255, 255,255,255, 255,255,255,255,255,255,255,255, 255,255,255,255,255,255, 255,255,255,255 }

const NSF_COMMONOPTIONS default_nsf_common_cfg =
{ 44100, 2, 115000, NTSC_NMIRATE, 1.0f, TRUE, 100.f, FALSE, FALSE, DEFAULT_MIX, DEFAULT_INV, DEFAULT_VOL, DEFAULT_PAN };
const NSF_ADVANCEDOPTIONS default_nsf_advanced_cfg =
{ 1200,
TRUE, FALSE, TRUE, TRUE,
TRUE, TRUE, TRUE, TRUE,
FALSE, TRUE, TRUE,
TRUE, TRUE, TRUE, 150, 40000, 40 };

const GUID guid_nsf_common_option =
{ 0xa42f2fea, 0x931f, 0x426e,{ 0xa6, 0xce, 0xea, 0x5b, 0x6d, 0xac, 0x21, 0x71 } };
const GUID guid_nsf_advanced_option =
{ 0xd5b5dfc, 0xdc14, 0x422f,{ 0xa3, 0xb5, 0x5e, 0x54, 0x20, 0x86, 0x24, 0x90 } };
const GUID guid_preference_page =
{ 0xd5b5dfc, 0xdc14, 0x422f,{ 0xa3, 0xb5, 0x5e, 0x54, 0x20, 0x86, 0x24, 0x91 } };

cfg_struct_t<NSF_COMMONOPTIONS> cfg_nsf_common_option(guid_nsf_common_option, default_nsf_common_cfg);
cfg_struct_t<NSF_ADVANCEDOPTIONS> cfg_nsf_advanced_option(guid_nsf_advanced_option, default_nsf_advanced_cfg);

nosofatso_preferences::nosofatso_preferences(HWND parent, preferences_page_callback::ptr callback)
	: m_parent(parent), m_callback(callback)
{
	cs.create();
	m_hwnd = create(m_parent);
}

nosofatso_preferences::~nosofatso_preferences()
{
	DestroyWindow(m_hwnd);
	cs.destroy();
}

HWND nosofatso_preferences::get_wnd()
{
	return m_hwnd;
}

HWND nosofatso_preferences::create(HWND p_parent)
{
	m_hwnd = CreateDialogParam(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_MAINCONTROL),
		p_parent, MainDlg::DlgProc, (LPARAM)this);
	m_state = 0;
	return m_hwnd;
}

t_uint32 nosofatso_preferences::get_state()
{
	return m_state;
}

void nosofatso_preferences::apply()
{
	BYTE i;
	NSF_COMMONOPTIONS c;
	cs.enter();
	ConfigDlg::GetOptions(cfg_nsf_common_option.get_value(), cfg_nsf_advanced_option.get_value());
	ConfigDlg2::GetOptions(cfg_nsf_advanced_option.get_value(), cfg_nsf_common_option.get_value().bIgnoreNSFVersion);
	for (i = 0; i < 29; i++)
	{
		if (i < 5)
			CONFIG_GET_CHANNEL_OPTIONS(ChannelsDlg, 0, i, &cfg_nsf_common_option.get_value())
		else if (i < 8)
			CONFIG_GET_CHANNEL_OPTIONS(VRC6Dlg, -5, i, &cfg_nsf_common_option.get_value())
		else if (i < 11)
			CONFIG_GET_CHANNEL_OPTIONS(MMC5Dlg, -8, i, &cfg_nsf_common_option.get_value())
		else if (i < 19)
			CONFIG_GET_CHANNEL_OPTIONS(N106Dlg, -11, i, &cfg_nsf_common_option.get_value())
		else if (i < 25)
			CONFIG_GET_CHANNEL_OPTIONS(VRC7Dlg, -19, i, &cfg_nsf_common_option.get_value())
		else if (i == 28)
			CONFIG_GET_CHANNEL_OPTIONS(ChannelsDlg, -23, i, &cfg_nsf_common_option.get_value())
		else
			CONFIG_GET_CHANNEL_OPTIONS(FME07Dlg, -25, i, &cfg_nsf_common_option.get_value())
	}
	cs.leave();
}

void nosofatso_preferences::reset()
{
	int i, vol, pan, mix, inv;
	cs.enter();
	cfg_nsf_common_option = default_nsf_common_cfg;
	cfg_nsf_advanced_option = default_nsf_advanced_cfg;
	ConfigDlg::SetOptions(cfg_nsf_common_option, cfg_nsf_advanced_option.get_value());
	ConfigDlg2::SetOptions(cfg_nsf_advanced_option.get_value(), cfg_nsf_common_option.get_value().bIgnoreNSFVersion);
	for (i = 0; i < 29; i++)
	{
		pan = cfg_nsf_common_option.get_value().nPan[i];
		mix = cfg_nsf_common_option.get_value().nMix[i];
		vol = cfg_nsf_common_option.get_value().nVol[i];
		inv = cfg_nsf_common_option.get_value().nInv[i];
		if (vol < 0)	vol = 0;
		if (vol > 255)	vol = 255;
		if (pan < -127)	pan = -127;
		if (pan > 127)	pan = 127;
		if (mix != 0)	mix = 1;
		if (inv != 1)	inv = 0;
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
	cs.leave();
}

const char *preferences_page_nosofatso::get_name()
{
	return "Nosofatso";
}

GUID preferences_page_nosofatso::get_guid()
{
	return guid_preference_page;
}

GUID preferences_page_nosofatso::get_parent_guid()
{
	return guid_input;
}

preferences_page_instance::ptr preferences_page_nosofatso::instantiate(HWND parent, preferences_page_callback::ptr callback)
{
	return new service_impl_t<nosofatso_preferences>(parent, callback);
}

static preferences_page_factory_t<preferences_page_nosofatso> g_preferences_factory;