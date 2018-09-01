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

input_nosofatso::input_nosofatso()
{
	char buffer[100];
	char secondbuffer[20];

	memcpy(&pri_cfg_nsf_common_option, &cfg_nsf_common_option.get_value(), sizeof(NSF_COMMONOPTIONS));
	memcpy(&pri_cfg_nsf_advanced_option, &cfg_nsf_advanced_option.get_value(), sizeof(NSF_ADVANCEDOPTIONS));
	m_buf_size = (576 << pri_cfg_nsf_common_option.nChannels) << 1;
	m_p_array = new byte[200 + m_buf_size];
}

input_nosofatso::~input_nosofatso()
{
	SAFE_DELETE_ARRAY(m_p_array);
}

void input_nosofatso::open(service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort)
{
	if (p_reason == input_open_info_write)
		throw exception_tagging_unsupported();
	m_file = p_filehint;
	nsfCore.Destroy();
	nsfCore.SetMasterVolume(pri_cfg_nsf_common_option.fMasterVolume);
	nsfCore.SetPlaybackSpeed(pri_cfg_nsf_common_option.fSpeedThrottle);
	nsfCore.SetPlaybackOptions(pri_cfg_nsf_common_option.nSampleRate, pri_cfg_nsf_common_option.nChannels);
	for (int i = 0; i < 29; i++)
		nsfCore.SetChannelOptions(i, pri_cfg_nsf_common_option.nMix[i],
			pri_cfg_nsf_common_option.nVol[i], pri_cfg_nsf_common_option.nPan[i], pri_cfg_nsf_common_option.nInv[i]);
	nsfCore.SetAdvancedOptions(&pri_cfg_nsf_advanced_option);
	strcpy(szLoadedPath, p_path);
	input_open_file_helper(m_file, p_path, p_reason, p_abort);
	if (nsfFile.LoadFile(m_file, p_abort, TRUE, pri_cfg_nsf_common_option.bIgnoreNSFVersion) != 0)
		throw exception_io_data();
}

unsigned input_nosofatso::get_subsong_count()
{
	return nsfFile.nTrackCount;
}

t_uint32 input_nosofatso::get_subsong(unsigned p_index)
{
	return p_index;
}

void input_nosofatso::get_info(t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort)
{
	SAFE_META_SET(p_info, "title", nsfFile.szGameTitle);
	SAFE_META_SET(p_info, "artist", nsfFile.szArtist);
	SAFE_META_SET(p_info, "dumper", nsfFile.szRipper);
	SAFE_META_SET(p_info, "copyright", nsfFile.szCopyright);
	if (nsfFile.pTrackTime == NULL)
		p_info.set_length(115);
	else
		p_info.set_length(nsfFile.pTrackTime[p_subsong]);
}

t_filestats input_nosofatso::get_file_stats(abort_callback & p_abort)
{
	return m_file->get_stats(p_abort);
}

void input_nosofatso::decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort)
{
	m_file->reopen(p_abort);
	// Equivalent to seek to zero, except it also works on nonseekable streams
	if (!nsfCore.Initialize())
		throw exception_out_of_resources();
	if (nsfCore.SetPlaybackOptions(pri_cfg_nsf_common_option.nSampleRate, pri_cfg_nsf_common_option.nChannels) == 0)
		throw exception_service_duplicated();
	if (nsfCore.LoadNSF(&nsfFile) == 0)
		throw exception_io_data_truncation();
	if ((nsfFile.nIsPal & 0x03) == 0x01)
		pri_cfg_nsf_common_option.fBasedPlaysPerSec = PAL_NMIRATE;
	else
		pri_cfg_nsf_common_option.fBasedPlaysPerSec = NTSC_NMIRATE;
	if (pri_cfg_nsf_common_option.bUseDefaultSpeed)
		nsfCore.SetPlaybackSpeed(0);
	else
		nsfCore.SetPlaybackSpeed(pri_cfg_nsf_common_option.fSpeedThrottle);
	if (p_subsong >= nsfFile.nTrackCount)
		throw exception_io_seek_out_of_range();
	m_subsong = p_subsong;
	nsfCore.SetTrack(p_subsong);
}

bool input_nosofatso::decode_run(audio_chunk & p_chunk, abort_callback & p_abort)
{
	// 200 bytes padding, just in case of a little overflow???
	if (nsfCore.GetSamples(m_p_array, m_buf_size) <= 0)
		return false;
	p_chunk.set_data_fixedpoint(m_p_array, m_buf_size, pri_cfg_nsf_common_option.nSampleRate, pri_cfg_nsf_common_option.nChannels, 16,
		audio_chunk::g_guess_channel_config(pri_cfg_nsf_common_option.nChannels));
	return true;
}

void input_nosofatso::decode_seek(double p_seconds, abort_callback & p_abort)
{
	nsfCore.SetWrittenTime(p_seconds * 1000, pri_cfg_nsf_common_option.fBasedPlaysPerSec);
}

bool input_nosofatso::decode_can_seek()
{
	return true;
}

bool input_nosofatso::decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta)
{
	p_out.info_set_int("samplerate", pri_cfg_nsf_common_option.nSampleRate);
	p_out.info_set_int("channels", pri_cfg_nsf_common_option.nChannels);
	p_out.info_set_int("bitspersample", 16);
	p_out.info_set("encoding", "lossless");
	p_out.info_set_bitrate((16 * pri_cfg_nsf_common_option.nChannels * pri_cfg_nsf_common_option.nSampleRate + 500 /* rounding for bps to kbps*/) / 1000 /* bps to kbps */);
	return true;
}

bool input_nosofatso::decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta)
{
	return false;
}

void input_nosofatso::decode_on_idle(abort_callback & p_abort)
{
	m_file->on_idle(p_abort);
}

bool input_nosofatso::g_is_our_content_type(const char *)
{
	return false;
}

bool input_nosofatso::g_is_our_path(const char *, const char * p_extension)
{
	if (stricmp_utf8(p_extension, "nsf") == 0)
		return true;
	else if (stricmp_utf8(p_extension, "nsfe") == 0)
		return true;
	else
		return false;
}

static input_factory_t<input_nosofatso> g_input_nosofatso_factory;
DECLARE_COMPONENT_VERSION("Notsofatso", "0.8.6.0",
\
"Program : Nintendo Entertainment System Audio Processing Unit Emulator\n"\
"Platform : Intel 80386\n"\
"Programmer : Slick Productions (Disch), Wilbert Lee\n"\
"\n"\
"Credits and stuff\n"
"\n"\
"Disch, Drag - Coding, design and all that good stuff.\n"\
"Tatsuyuki Satoh, Quietust, Xodnizel - VRC7 Support.\n"\
"Tzar - That owns.\n"\
"Brad Taylor, Blargg, Kent Hansen, Miyama, Goroh - For the great sound docs.\n"\
"\n"\
"Copyright\n"
"\n"\
"This program is free software; you can redistribute it and / or modify "\
"it under the terms of the GNU General Public License as published by "\
"the Free Software Foundation; either version 2 of the License, or "\
"(at your option) any later version.\n"\
"\n"\
"This program is distributed in the hope that it will be useful, "\
"but WITHOUT ANY WARRANTY; without even the implied warranty of "\
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the "\
"GNU General Public License for more details.\n"\
"\n"\
"You should have received a copy of the GNU General Public License "\
"along with this program; if not, write to the Free Software "\
"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111 - 1307  USA\n"\
"\n"\
"Copyright (C) 2004 Disch\n"\
"Copyright (C) 2015 Wilbert Lee\n"\
);
DECLARE_FILE_TYPE("NSF Sound Format", "*.NSF;*.NSFE");
// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_input_notsofatso.dll");