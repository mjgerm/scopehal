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

#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <cmath>
#endif

#include "../scopehal/scopehal.h"
#include "UpsampleFilter.h"

using namespace std;

float sinc(float x, float width);
float blackman(float x, float width);

float sinc(float x, float width)
{
	float xi = x - width/2;

	if(fabs(xi) < 1e-7)
		return 1.0f;
	else
	{
		float px = M_PI*xi;
		return sin(px) / px;
	}
}

float blackman(float x, float width)
{
	if(x > width)
		return 0;
	return 0.42 - 0.5*cos(2*M_PI * x / width) + 0.08 * cos(4*M_PI*x/width);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

UpsampleFilter::UpsampleFilter(const string& color)
	: Filter(OscilloscopeChannel::CHANNEL_TYPE_ANALOG, color, CAT_MATH)
{
	//Set up channels
	CreateInput("din");

	m_factorname = "Upsample factor";
	m_parameters[m_factorname] = FilterParameter(FilterParameter::TYPE_INT, Unit(Unit::UNIT_SAMPLEDEPTH));
	m_parameters[m_factorname].SetIntVal(10);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Factory methods

bool UpsampleFilter::ValidateChannel(size_t i, StreamDescriptor stream)
{
	if(stream.m_channel == NULL)
		return false;

	if( (i == 0) && (stream.m_channel->GetType() == OscilloscopeChannel::CHANNEL_TYPE_ANALOG) )
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

void UpsampleFilter::SetDefaultName()
{
	char hwname[256];
	snprintf(hwname, sizeof(hwname), "Upsample(%s)", GetInputDisplayName(0).c_str());
	m_hwname = hwname;
	m_displayname = m_hwname;
}

string UpsampleFilter::GetProtocolName()
{
	return "Upsample";
}

bool UpsampleFilter::IsOverlay()
{
	//we create a new analog channel
	return false;
}

bool UpsampleFilter::NeedsConfig()
{
	return true;
}

double UpsampleFilter::GetOffset()
{
	auto chan = m_inputs[0].m_channel;
	if(chan == NULL)
		return 0;
	else
		return chan->GetOffset();
}

double UpsampleFilter::GetVoltageRange()
{
	auto chan = m_inputs[0].m_channel;
	if(chan == NULL)
		return 0;
	else
		return chan->GetVoltageRange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual decoder logic

void UpsampleFilter::Refresh()
{
	//Make sure we've got valid inputs
	if(!VerifyAllInputsOKAndAnalog())
	{
		SetData(NULL, 0);
		return;
	}

	//Get the input data
	auto din = GetAnalogInputWaveform(0);

	//Configuration parameters that eventually have to be user specified
	size_t upsample_factor = m_parameters[m_factorname].GetIntVal();
	size_t window = 5;
	size_t kernel = window*upsample_factor;

	//Create the interpolation filter
	float frac_kernel = kernel * 1.0f / upsample_factor;
	vector<float> filter;
	for(size_t i=0; i<kernel; i++)
	{
		float frac = i*1.0f / upsample_factor;
		filter.push_back(sinc(frac, frac_kernel) * blackman(frac, frac_kernel));
	}

	//Create the output and configure it
	auto cap = new AnalogWaveform;

	//TODO: make this work on not-dense-packed waveforms

	//Fill out the input with samples
	size_t len = din->m_samples.size();
	for(size_t i=0; i < len; i++)
	{
		for(size_t j=0; j<upsample_factor; j++)
		{
			cap->m_offsets.push_back(i * upsample_factor + j);
			cap->m_durations.push_back(1);
			cap->m_samples.push_back(0);
		}
	}

	//Logically, we upsample by inserting zeroes, then convolve with the sinc filter.
	//Optimization: don't actually waste time multiplying by zero
	//TODO: vectorize this instead of multithreading
	size_t imax = len - window;
	#pragma omp parallel for
	for(size_t i=0; i < imax; i++)
	{
		size_t offset = i * upsample_factor;
		for(size_t j=0; j<upsample_factor; j++)
		{
			size_t start = 0;
			size_t sstart = 0;
			if(j > 0)
			{
				sstart = 1;
				start = upsample_factor - j;
			}

			float f = 0;
			for(size_t k = start; k<kernel; k += upsample_factor, sstart ++)
				f += filter[k] * din->m_samples[i + sstart];

			cap->m_samples[offset + j] = f;
		}
	}

	//Copy our time scales from the input, and correct for the upsampling
	cap->m_timescale = din->m_timescale / upsample_factor;
	cap->m_startTimestamp = din->m_startTimestamp;
	cap->m_startFemtoseconds = din->m_startFemtoseconds;

	SetData(cap, 0);
}
