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

/*
*  I tried my best to comment everything so that it's understandable.  Be sure
*  to check out the readmes for more information
*
*  -Disch-
*/

/*
*  I made pretty much every file include this header... kinda sloppy.
*  feel free to change it if you don't like the compile time.
*/

#define SAFE_META_SET(i, n, d) if (n != NULL && d != NULL) i.meta_set(n, d)
#define SAFE_NEW(a, t, s, b) a = new t[s]; 
#define SAFE_DELETE(p) if (p != NULL) { delete p; p = NULL; }
#define SAFE_DELETE_ARRAY(p) if (p != NULL) { delete[]p; p = NULL; }

#include <Windows.h>
#include <CommCtrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "resource.h"
#include "CNSFCore.h"
#include "CNSFFile.h"
#include "GUI.h"

extern const NSF_COMMONOPTIONS default_nsf_common_cfg;
extern const NSF_ADVANCEDOPTIONS default_nsf_advanced_cfg;
extern const GUID guid_nsf_common_option;
extern const GUID guid_nsf_advanced_option;
extern const GUID guid_preference_page;
extern cfg_struct_t<NSF_COMMONOPTIONS> cfg_nsf_common_option;
extern cfg_struct_t<NSF_ADVANCEDOPTIONS> cfg_nsf_advanced_option;

class input_nosofatso
{
public:
  input_nosofatso();
  ~input_nosofatso();
  void open(service_ptr_t<file> p_filehint, const char * p_path,
  t_input_open_reason p_reason, abort_callback & p_abort);
  void process_ftm(const TCHAR *p_sz_exist_file, const TCHAR *p_sz_target_file);
  unsigned get_subsong_count();
  t_uint32 get_subsong(unsigned p_index);
  void get_info(t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort);
  t_filestats get_file_stats(abort_callback & p_abort);
  void decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback &p_abort);
  bool decode_run(audio_chunk & p_chunk, abort_callback & p_abort);
  void decode_seek(double p_seconds, abort_callback & p_abort);
  bool decode_can_seek();
  bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta);
  bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta);
  void decode_on_idle(abort_callback & p_abort);
  void retag_set_info(t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort) { retag_commit(p_abort); }
  void retag_commit(abort_callback & p_abort) { throw exception_io_data(); }
  static bool g_is_our_content_type(const char *);
  static bool g_is_our_path(const char *, const char * p_extension);
public:
  int m_subsong;
  byte *m_p_array;
  size_t m_buf_size;
  service_ptr_t<file> m_file;
  pfc::string8 m_tmp_file;
  // The Core!  Does ALL the sound emulation... see it for further info
  CNSFCore nsfCore;
  CNSFFile nsfFile; // Currently loaded file... contains track lengths and other crap
  NSF_COMMONOPTIONS pri_cfg_nsf_common_option;
  NSF_ADVANCEDOPTIONS pri_cfg_nsf_advanced_option;
};

class nosofatso_preferences : public preferences_page_instance
{
public:
  nosofatso_preferences(HWND parent, preferences_page_callback::ptr callback);
  ~nosofatso_preferences();

  enum { IDD = IDD_MAINCONTROL };

  HWND create(HWND p_parent);
  t_uint32 get_state();
  HWND get_wnd();
  void apply();
  void reset();

  t_uint32 m_state;
  const preferences_page_callback::ptr m_callback;
private:
  /* Passive configuration detection */
  HWND m_hwnd, m_parent;
  critical_section cs;
};

class preferences_page_nosofatso : public preferences_page_v3
{
public:
  const char * get_name();
  GUID get_guid();
  GUID get_parent_guid();
  preferences_page_instance::ptr instantiate(HWND parent, preferences_page_callback::ptr callback);
};
