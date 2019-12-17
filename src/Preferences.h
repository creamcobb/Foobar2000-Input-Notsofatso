#include "NSF.h"
#pragma once
// our config class
class NotsofatsoPreferences : public CDialogImpl<NotsofatsoPreferences>, public preferences_page_instance
{
public:
	NotsofatsoPreferences(preferences_page_callback::ptr callback) : m_callback(callback);
	enum { IDD = IDD_MAINCONTROL };
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(SnesApuPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_CLOSE(OnCloseDialog)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnCloseDialog();
	const preferences_page_callback::ptr m_callback;
};