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

#include "scopeprotocols.h"
#include "EmphasisFilter.h"
#include "TappedDelayLineFilter.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

EmphasisFilter::EmphasisFilter(const string& color)
	: Filter(OscilloscopeChannel::CHANNEL_TYPE_ANALOG, color, CAT_ANALYSIS)
	, m_dataRateName("Data Rate")
	, m_emphasisTypeName("Emphasis Type")
	, m_emphasisAmountName("Emphasis Amount")
{
	CreateInput("in");

	m_range = 1;
	m_offset = 0;
	m_min = FLT_MAX;
	m_max = -FLT_MAX;

	m_parameters[m_dataRateName] = FilterParameter(FilterParameter::TYPE_INT, Unit(Unit::UNIT_BITRATE));
	m_parameters[m_dataRateName].SetIntVal(1250e6);

	m_parameters[m_emphasisTypeName] = FilterParameter(FilterParameter::TYPE_ENUM, Unit(Unit::UNIT_COUNTS));
	m_parameters[m_emphasisTypeName].AddEnumValue("De-emphasis", DE_EMPHASIS);
	m_parameters[m_emphasisTypeName].AddEnumValue("Pre-emphasis", PRE_EMPHASIS);
	m_parameters[m_emphasisTypeName].SetIntVal(DE_EMPHASIS);

	m_parameters[m_emphasisAmountName] = FilterParameter(FilterParameter::TYPE_FLOAT, Unit(Unit::UNIT_DB));
	m_parameters[m_emphasisAmountName].SetFloatVal(6);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Factory methods

bool EmphasisFilter::ValidateChannel(size_t i, StreamDescriptor stream)
{
	if(stream.m_channel == NULL)
		return false;

	if( (i == 0) && (stream.m_channel->GetType() == OscilloscopeChannel::CHANNEL_TYPE_ANALOG) )
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

void EmphasisFilter::ClearSweeps()
{
	m_range = 1;
	m_offset = 0;
	m_min = FLT_MAX;
	m_max = -FLT_MAX;
}

void EmphasisFilter::SetDefaultName()
{
	char hwname[256];
	snprintf(hwname, sizeof(hwname), "Emphasis(%s, %s)",
		GetInputDisplayName(0).c_str(),
		m_parameters[m_emphasisAmountName].ToString().c_str());
	m_hwname = hwname;
	m_displayname = m_hwname;
}

string EmphasisFilter::GetProtocolName()
{
	return "Emphasis";
}

bool EmphasisFilter::IsOverlay()
{
	//we create a new analog channel
	return false;
}

bool EmphasisFilter::NeedsConfig()
{
	return true;
}

double EmphasisFilter::GetVoltageRange()
{
	return m_range;
}

double EmphasisFilter::GetOffset()
{
	return m_offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual decoder logic

void EmphasisFilter::Refresh()
{
	if(!VerifyAllInputsOKAndAnalog())
	{
		SetData(NULL, 0);
		return;
	}

	//Get the input data
	auto din = GetAnalogInputWaveform(0);
	size_t len = din->m_samples.size();
	if(len < 8)
	{
		SetData(NULL, 0);
		return;
	}
	m_xAxisUnit = m_inputs[0].m_channel->GetXAxisUnits();
	m_yAxisUnit = m_inputs[0].m_channel->GetYAxisUnits();

	//Set up output
	const int64_t tap_count = 8;
	int64_t tap_delay = round(FS_PER_SECOND / m_parameters[m_dataRateName].GetFloatVal());
	int64_t samples_per_tap = tap_delay / din->m_timescale;
	auto cap = SetupOutputWaveform(din, 0, tap_count * samples_per_tap, 0);

	//Calculate the tap values
	//Reference: "Dealing with De-Emphasis in Jitter Testing", P. Pupalaikis, LeCroy technical brief, 2008
	float db = m_parameters[m_emphasisAmountName].GetFloatVal();
	float emphasisLevel = pow(10, -db/20);
	float coeff = 0.5 * emphasisLevel;
	float c = coeff + 0.5;
	float p = coeff - 0.5;
	float taps[tap_count] = {0};
	taps[0] = c;
	taps[1] = p;

	//If we're doing pre-emphasis rather than de-emphasis, we need to scale everything accordingly.
	auto type = static_cast<EmphasisType>(m_parameters[m_emphasisTypeName].GetIntVal());
	if(type == PRE_EMPHASIS)
	{
		for(int64_t i=0; i<tap_count; i++)
			taps[i] /= emphasisLevel;
	}

	//Run the actual filter
	float vmin;
	float vmax;
	TappedDelayLineFilter::DoFilterKernel(tap_delay, taps, din, cap, vmin, vmax);

	//Calculate bounds
	m_max = max(m_max, vmax);
	m_min = min(m_min, vmin);
	m_range = (m_max - m_min) * 1.05;
	m_offset = -( (m_max - m_min)/2 + m_min );
}
