/***********************************************************************************************************************
*                                                                                                                      *
* libscopeprotocols                                                                                                    *
*                                                                                                                      *
* Copyright (c) 2012-2021 Andrew D. Zonenberg and contributors                                                         *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

#include "../scopehal/scopehal.h"
#include "EthernetProtocolDecoder.h"
#include "Ethernet10BaseTDecoder.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Ethernet10BaseTDecoder::Ethernet10BaseTDecoder(const string& color)
	: EthernetProtocolDecoder(color)
{
}

Ethernet10BaseTDecoder::~Ethernet10BaseTDecoder()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Ethernet10BaseTDecoder::GetProtocolName()
{
	return "Ethernet - 10baseT";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual decoder logic

void Ethernet10BaseTDecoder::SetDefaultName()
{
	char hwname[256];
	snprintf(hwname, sizeof(hwname), "10BaseT(%s)", GetInputDisplayName(0).c_str());
	m_hwname = hwname;
	m_displayname = m_hwname;
}

void Ethernet10BaseTDecoder::Refresh()
{
	ClearPackets();

	//Get the input data
	if(!VerifyAllInputsOKAndAnalog())
	{
		SetData(NULL, 0);
		return;
	}

	auto din = GetAnalogInputWaveform(0);
	size_t len = din->m_samples.size();

	//Copy our time scales from the input
	auto cap = new EthernetWaveform;
	cap->m_timescale = din->m_timescale;
	cap->m_startTimestamp = din->m_startTimestamp;
	cap->m_startFemtoseconds = din->m_startFemtoseconds;

	const int64_t ui_width 		= 100000 * 1000;
	const int64_t ui_halfwidth 	= 50000 * 1000;
	const int64_t jitter_tol 	= 10000 * 1000;

	const int64_t eye_start = ui_halfwidth - jitter_tol;
	const int64_t eye_end = ui_halfwidth + jitter_tol;

	size_t i = 0;
	bool done = false;
	while(i < len)
	{
		if(done)
			break;

		//Look for a falling edge with at least -500 mV differential (falling edge of the first preamble bit)
		if(!FindFallingEdge(i, din))
		{
			LogTrace("Capture ended before finding another preamble\n");
			break;
		}
		LogTrace("Start of frame\n");

		uint8_t current_byte = 0;
		int bitcount = 0;

		//Set of recovered bytes and timestamps
		vector<uint8_t> bytes;
		vector<uint64_t> starts;
		vector<uint64_t> ends;

		//Recover the Manchester bitstream
		bool current_state = false;
		int64_t ui_start = din->m_offsets[i] * cap->m_timescale;
		int64_t byte_start = ui_start;
		//LogDebug("[T = %.3f ns] Found initial falling edge\n", ui_start * 1e-6f);
		while(i < len)
		{
			//When we get here, i points to the start of our UI
			//Expect an opposite polarity edge at the center of our bit
			//LogDebug("Looking for %d -> %d edge\n", current_state, !current_state);
			if(!FindEdge(i, din, !current_state))
			{
				LogTrace("Capture ended while looking for middle of this bit\n");
				done = true;
				break;
			}

			//If the edge came too soon or too late, possible sync error - restart from this edge
			//If the delta was more than ten UIs, it's a new frame - end this one
			int64_t edgepos = din->m_offsets[i] * cap->m_timescale;
			int64_t delta = edgepos - ui_start;
			/*LogDebug("[T = %.3f ns] Found edge! edgepos=%d ui_start = %d, Delta = %.3f ns (%.2f UI)\n",
				edgepos * 1e-6f,
				(int)edgepos,
				(int)ui_start,
				delta * 1e-6f,
				delta * 1.0f / ui_width);*/
			if(delta > 10 * ui_width)
			{
				LogTrace("Premature end of frame (middle of a bit)\n");
				i++;
				break;
			}
			if( (delta < eye_start) || (delta > eye_end) )
			{
				LogTrace("Edge was in the wrong place, skipping it and attempting resync\n");
				i++;
				ui_start = din->m_offsets[i] * cap->m_timescale;
				current_state = !current_state;
				continue;
			}
			int64_t i_middle = i;
			int64_t ui_middle = din->m_offsets[i] * cap->m_timescale;

			//Edge is in the right spot! Decode it. Ethernet sends LSB first.
			//Ethernet says rising edge in the middle of the bit = 1
			if(bitcount == 0)
				byte_start = ui_start;
			if(!current_state)
				current_byte = (current_byte >> 1) | 0x80;
			else
				current_byte = (current_byte >> 1);
			bitcount ++;
			if(bitcount == 8)
			{
				//Save this byte
				bytes.push_back(current_byte);
				starts.push_back(byte_start);
				ends.push_back(ui_start + ui_width);

				current_byte = 0;
				bitcount = 0;
			}

			//See if we have an edge at the end of this bit period
			if(!FindEdge(i, din, current_state))
			{
				LogTrace("Capture ended while looking for end of this bit\n");
				done = true;
				break;
			}
			edgepos = din->m_offsets[i] * cap->m_timescale;
			delta = edgepos - ui_middle;

			//If the next edge is more than ten UIs after this one, declare the frame over
			if(delta > 10*ui_width)
			{
				LogTrace("Normal end of frame\n");
				i++;
				break;
			}

			//Next edge is way after the end of this bit.
			//It must be the middle of our next bit, deal with it later
			if(delta > eye_end)
			{
				//LogDebug("Next edge is after our end (must be middle of subsequent bit\n");
				current_state = !current_state;

				//Move back until we're about half a UI after the center edge of this bit
				i = i_middle;
				int64_t target = ui_middle + ui_halfwidth;
				while(i < len)
				{
					int64_t pos = din->m_offsets[i] * cap->m_timescale;
					if(pos >= target)
						break;
					else
						i++;
				}
			}

			//Next edge is at the end of this bit.
			//Move to the position of the edge and look for an opposite-polarity one
			else
			{
				//LogDebug("Edge is at end of this bit\n");
				//i already points to the edge, don't touch it
			}

			//Either way, i now points to the beginning of the next bit's UI
			ui_start = din->m_offsets[i] * cap->m_timescale;
		}

		//Crunch the Manchester-coded data
		BytesToFrames(bytes, starts, ends, cap);
	}

	SetData(cap, 0);
}

bool Ethernet10BaseTDecoder::FindFallingEdge(size_t& i, AnalogWaveform* cap)
{
	size_t j = i;
	size_t len = cap->m_samples.size();
	while(j < len)
	{
		if(cap->m_samples[j] < -1)
		{
			i = j;
			return true;
		}
		j++;
	}

	return false;	//not found
}

bool Ethernet10BaseTDecoder::FindRisingEdge(size_t& i, AnalogWaveform* cap)
{
	size_t j = i;
	size_t len = cap->m_samples.size();
	while(j < len)
	{
		if(cap->m_samples[j] > 1)
		{
			i = j;
			return true;
		}
		j++;
	}

	return false;	//not found
}
