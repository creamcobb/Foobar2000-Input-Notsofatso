#include "Preferences.h"
static const GUID guid_nsf_common_option =
{ 0xa42f2fea, 0x931f, 0x426e,{ 0xa6, 0xce, 0xea, 0x5b, 0x6d, 0xac, 0x21, 0x71 } };
static const GUID guid_nsf_advanced_option =
{ 0xd5b5dfc, 0xdc14, 0x422f,{ 0xa3, 0xb5, 0x5e, 0x54, 0x20, 0x86, 0x24, 0x90 } };
static const GUID guid_preference_page =
{ 0xd5b5dfc, 0xdc14, 0x422f,{ 0xa3, 0xb5, 0x5e, 0x54, 0x20, 0x86, 0x24, 0x91 } };
static const char defaults[29][12] = {
	"1:255:-45:1","1:255:45:1", "1:255:0:0", "1:255:0:0", "1:255:0:0", /* native */
	"1:255:-50:1", "1:255:50:0", "1:255:0:0",  /* VRC6 */
	"1:255:-50:1", "1:255:50:1", "1:255:0:0",  /* MMC5 */
	"1:255:-32:1", "1:255:32:1", "1:255:-32:0", "1:255:32:0", "1:255:-32:1", "1:255:32:1", "1:255:-32:0", "1:255:32:0", /* N106 */
	"1:255:-35:1", "1:255:35:1", "1:255:-30:0", "1:255:30:0", "1:255:-20:1", "1:255:20:1", /* VRC7 */
	"1:255:-40:0", "1:255:0:1", "1:255:40:0",  /* FME-07 */
	"1:255:0:0"    /* FDS */
};
static const BYTE defaultinv[29] = { 1,1,0,0,0, 1,0,0, 1,1,0, 1,1,0,0,1,1,0,0, 1,1,0,0,1,1, 0,1,0, 0 };
static const BYTE defaultmix[29] = { 1,1,1,1,1, 1,1,1, 1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1 };
//static const NSF_COMMONOPTIONS default_nsf_common_cfg =
//{ 44100, 2, 115, NTSC_NMIRATE, 1.0f, TRUE, 100.f, FALSE, FALSE, defaultmix, defaultinv, defaults };
//static const NSF_ADVANCEDOPTIONS default_nsf_advanced_cfg =
//{ 1200, 210,
//TRUE, FALSE, TRUE, TRUE,
//TRUE, TRUE, TRUE, TRUE,
//FALSE, TRUE, TRUE, TRUE,
//TRUE, TRUE, TRUE, 150, 40000, 40 };

RECT rcTabCtrl;
HWND hFrame, hTabCtrl;
cfg_struct_t<NSF_COMMONOPTIONS> cfg_nsf_common_option(guid_nsf_common_option, default_nsf_common_cfg);
cfg_struct_t<NSF_ADVANCEDOPTIONS> cfg_nsf_advanced_option(guid_nsf_advanced_option, default_nsf_advanced_cfg);

void ShowPage(int nSelect)
{
	TCITEM tci;
	tci.mask = TCIF_PARAM;
	TabCtrl_GetItem(hTabCtrl, nSelect, &tci);
	if (hFrame != NULL)
		ShowWindow(hFrame, SW_HIDE);
	hFrame = (HWND)tci.lParam;
	SetWindowPos(hFrame, 0, rcTabCtrl.left, rcTabCtrl.top,
		rcTabCtrl.right - rcTabCtrl.left, rcTabCtrl.bottom - rcTabCtrl.top, SWP_NOZORDER);
	ShowWindow(hFrame, SW_SHOW);
}

NotsofatsoPreferences::NotsofatsoPreferences(preferences_page_callback::ptr callback) : m_callback(callback)
{
}

t_uint32 NotsofatsoPreferences::get_state()
{
	t_uint32 state = preferences_state::resettable;
	state |= preferences_state::changed;
	return state;
}

void NotsofatsoPreferences::apply()
{
	//SaveConfiguration(true);
	char buffer[20], secondbuffer[20];
	int i, vol, pan, mix, inv;

	for (i = 0; i < 29; i++)
	{
		if (i == 28)
			ChannelsDlg::GetOptions(i, mix, vol, pan, inv);
		else if (i < 5)
			ChannelsDlg::GetOptions(i, mix, vol, pan, inv);
		else if (i < 8)
			VRC6Dlg::GetOptions(i, mix, vol, pan, inv);
		else if (i < 11)
			MMC5Dlg::GetOptions(i, mix, vol, pan, inv);
		else if (i < 19)
			N106Dlg::GetOptions(i, mix, vol, pan, inv);
		else if (i < 25)
			VRC7Dlg::GetOptions(i, mix, vol, pan, inv);
		else
			FME07Dlg::GetOptions(i, mix, vol, pan, inv);
		sprintf(buffer, "%d:%d:%d:%d", mix, vol, pan, inv);
		cfg_nsf_common_option.get_value().nInv[i] = inv;
		cfg_nsf_common_option.get_value().nMix[i] = mix;
		strcpy(cfg_nsf_common_option.get_value().Channel[i], buffer);
		nsfCore.SetChannelOptions(i, mix, vol, pan, inv);
	}

	ConfigDlg::GetPlayMode(&cfg_nsf_common_option.get_value().nSampleRate, &cfg_nsf_common_option.get_value().nChannels);
	ConfigDlg::GetDefaultTimes(nDefaultSongLength);
}

void NotsofatsoPreferences::reset()
{
	char buffer[20], secondbuffer[20];
	int i, vol, pan, mix, inv;
	cfg_nsf_common_option = default_nsf_common_cfg;
	cfg_nsf_advanced_option = default_nsf_advanced_cfg;
	for (i = 0; i < 29; i++)
	{
		sprintf(secondbuffer, "Channel %02d", i);
		//AppApi.lpProfile->GetString("NotSo Fatso", secondbuffer, defaults[i], buffer, 100);
		pan = 0;
		mix = 1;
		vol = 255;
		inv = defaultinv[i];
		sscanf(buffer, "%d:%d:%d:%d", &mix, &vol, &pan, &inv);
		if (vol < 0) vol = 0;
		if (vol > 255) vol = 255;
		if (pan < -127) pan = -127;
		if (pan > 127) pan = 127;
		if (mix != 0) mix = 1;
		if (inv != 1) inv = 0;

		// load while open track
		nsfCore.SetChannelOptions(i, mix, vol, pan, inv);

		if (i == 28)
			ChannelsDlg::SetOptions(i, mix, vol, pan, inv);
		else if (i < 5)
			ChannelsDlg::SetOptions(i, mix, vol, pan, inv);
		else if (i < 8)
			VRC6Dlg::SetOptions(i, mix, vol, pan, inv);
		else if (i < 11)
			MMC5Dlg::SetOptions(i, mix, vol, pan, inv);
		else if (i < 19)
			N106Dlg::SetOptions(i, mix, vol, pan, inv);
		else if (i < 25)
			VRC7Dlg::SetOptions(i, mix, vol, pan, inv);
		else
			FME07Dlg::SetOptions(i, mix, vol, pan, inv);
	}
}

BOOL NotsofatsoPreferences::OnInitDialog(CWindow w, LPARAM)
{
	TCITEM tci;
	hFrame = NULL;
	hTabCtrl = GetDlgItem(IDC_TAB);
	tci.mask = TCIF_PARAM | TCIF_TEXT;
	tci.pszText = L"Notsofatso";
	tci.lParam = (LPARAM)
		CreateDialog(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_CONFIG),
			w, ConfigDlg::DlgProc);
	TabCtrl_InsertItem(hTabCtrl, 0, &tci);

	tci.pszText = L"Advanced";
	tci.lParam = (LPARAM)
		CreateDialog(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_CONFIG_2),
			w, ConfigDlg2::DlgProc);
	TabCtrl_InsertItem(hTabCtrl, 1, &tci);

	tci.pszText = L"Channels";
	tci.lParam = (LPARAM)
		CreateDialog(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_CHANNELS),
			w, ChannelsDlg::DlgProc);
	TabCtrl_InsertItem(hTabCtrl, 2, &tci);

	tci.pszText = L"VRC6";
	tci.lParam = (LPARAM)
		CreateDialog(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_VRC6),
			w, VRC6Dlg::DlgProc);
	TabCtrl_InsertItem(hTabCtrl, 3, &tci);

	tci.pszText = L"MMC5";
	tci.lParam = (LPARAM)
		CreateDialog(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_MMC5),
			w, MMC5Dlg::DlgProc);
	TabCtrl_InsertItem(hTabCtrl, 4, &tci);

	tci.pszText = L"N106";
	tci.lParam = (LPARAM)
		CreateDialog(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_N106),
			w, N106Dlg::DlgProc);
	TabCtrl_InsertItem(hTabCtrl, 5, &tci);

	tci.pszText = L"VRC7";
	tci.lParam = (LPARAM)
		CreateDialog(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_VRC7),
			w, VRC7Dlg::DlgProc);
	TabCtrl_InsertItem(hTabCtrl, 6, &tci);

	tci.pszText = L"FME07";
	tci.lParam = (LPARAM)
		CreateDialog(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_FME07),
			w, FME07Dlg::DlgProc);
	TabCtrl_InsertItem(hTabCtrl, 7, &tci);
	LoadConfiguration(true);
	::GetClientRect(hTabCtrl, &rcTabCtrl);
	TabCtrl_AdjustRect(hTabCtrl, FALSE, &rcTabCtrl);
	::MapWindowPoints(hTabCtrl, w, (LPPOINT)&rcTabCtrl, 2);
	ShowPage(0);
	return 0;
}

void NotsofatsoPreferences::OnCloseDialog()
{
	::DestroyWindow(ConfigDlg::hWnd);
	::DestroyWindow(ConfigDlg2::hWnd);
	::DestroyWindow(ChannelsDlg::hWnd);
	::DestroyWindow(VRC6Dlg::hWnd);
	::DestroyWindow(VRC7Dlg::hWnd);
	::DestroyWindow(MMC5Dlg::hWnd);
	::DestroyWindow(FME07Dlg::hWnd);
	::DestroyWindow(N106Dlg::hWnd);
}

class NotsofatsoPreferencesPage : public preferences_page_impl<NotsofatsoPreferences>
{
public:
	const char * get_name() { return "Notsofatso"; }
	GUID get_guid() { return guid_preference_page; }
	GUID get_parent_guid() { return guid_input; }
};

static preferences_page_factory_t<NotsofatsoPreferencesPage> g_preferences_factory;