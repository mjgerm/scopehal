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
#include "SubtractFilter.h"
#include <immintrin.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

SubtractFilter::SubtractFilter(const string& color)
	: Filter(OscilloscopeChannel::CHANNEL_TYPE_ANALOG, color, CAT_MATH)
{
	//Set up channels
	CreateInput("IN+");
	CreateInput("IN-");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Factory methods

bool SubtractFilter::ValidateChannel(size_t i, StreamDescriptor stream)
{
	if(stream.m_channel == NULL)
		return false;

	if( (i < 2) && (stream.m_channel->GetType() == OscilloscopeChannel::CHANNEL_TYPE_ANALOG) )
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

void SubtractFilter::SetDefaultName()
{
	char hwname[256];
	snprintf(hwname, sizeof(hwname), "%s - %s",
		GetInputDisplayName(0).c_str(),
		GetInputDisplayName(1).c_str());
	m_hwname = hwname;
	m_displayname = m_hwname;
}

string SubtractFilter::GetProtocolName()
{
	return "Subtract";
}

bool SubtractFilter::IsOverlay()
{
	//we create a new analog channel
	return false;
}

bool SubtractFilter::NeedsConfig()
{
	//we have more than one input
	return true;
}

double SubtractFilter::GetOffset()
{
	double v1 = m_inputs[0].m_channel->GetVoltageRange();
	double v2 = m_inputs[1].m_channel->GetVoltageRange();
	double o1 = m_inputs[0].m_channel->GetOffset();
	double o2 = m_inputs[1].m_channel->GetOffset();

	double vmax_p = v1/2 - o1;
	double vmin_p = -v1/2 - o1;

	//double vmax_n = v2/2 - o2;
	double vmin_n = -v2/2 - o2;

	//Possible output range
	double vout_max = vmax_p - vmin_n;
	double vout_min = vmin_p - vmax_p;

	return -(vout_min + ((vout_max - vout_min) / 2));
}

double SubtractFilter::GetVoltageRange()
{
	double v1 = m_inputs[0].m_channel->GetVoltageRange();
	double v2 = m_inputs[1].m_channel->GetVoltageRange();
	double o1 = m_inputs[0].m_channel->GetOffset();
	double o2 = m_inputs[1].m_channel->GetOffset();

	double vmax_p = v1/2 - o1;
	double vmin_p = -v1/2 - o1;

	//double vmax_n = v2/2 - o2;
	double vmin_n = -v2/2 - o2;

	//Possible output range
	double vout_max = vmax_p - vmin_n;
	double vout_min = vmin_p - vmax_p;

	return (vout_max - vout_min);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual decoder logic

void SubtractFilter::Refresh()
{
	//Make sure we've got valid inputs
	if(!VerifyAllInputsOKAndAnalog())
	{
		SetData(NULL, 0);
		return;
	}

	auto din_p = GetAnalogInputWaveform(0);
	auto din_n = GetAnalogInputWaveform(1);

	//Set up units and complain if they're inconsistent
	m_xAxisUnit = m_inputs[0].m_channel->GetXAxisUnits();
	m_yAxisUnit = m_inputs[0].m_channel->GetYAxisUnits();
	if( (m_xAxisUnit != m_inputs[1].m_channel->GetXAxisUnits()) ||
		(m_yAxisUnit != m_inputs[1].m_channel->GetYAxisUnits()) )
	{
		SetData(NULL, 0);
		return;
	}

	//We need meaningful data
	size_t len = min(din_p->m_samples.size(), din_n->m_samples.size());

	//Create the output and copy timestamps
	auto cap = SetupOutputWaveform(din_p, 0, 0, 0);
	float* out = (float*)&cap->m_samples[0];
	float* a = (float*)&din_p->m_samples[0];
	float* b = (float*)&din_n->m_samples[0];

	//Do the actual subtraction
	if(g_hasAvx2)
		InnerLoopAVX2(out, a, b, len);
	else
		InnerLoop(out, a, b, len);
}

//We probably still have SSE2 or similar if no AVX, so give alignment hints for compiler auto-vectorization
void SubtractFilter::InnerLoop(float* out, float* a, float* b, size_t len)
{
	out = (float*)__builtin_assume_aligned(out, 64);
	a = (float*)__builtin_assume_aligned(a, 64);
	b = (float*)__builtin_assume_aligned(b, 64);

	for(size_t i=0; i<len; i++)
		out[i] 		= a[i] - b[i];
}

__attribute__((target("avx2")))
void SubtractFilter::InnerLoopAVX2(float* out, float* a, float* b, size_t len)
{
	size_t end = len - (len % 8);

	//AVX2
	for(size_t i=0; i<end; i+=8)
	{
		__m256 pa = _mm256_load_ps(a + i);
		__m256 pb = _mm256_load_ps(b + i);
		__m256 o = _mm256_sub_ps(pa, pb);
		_mm256_store_ps(out+i, o);
	}

	//Get any extras
	for(size_t i=end; i<len; i++)
		out[i] 		= a[i] - b[i];
}
