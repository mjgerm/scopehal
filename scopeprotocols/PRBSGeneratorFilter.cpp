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
#include "PRBSGeneratorFilter.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

PRBSGeneratorFilter::PRBSGeneratorFilter(const string& color)
	: Filter(OscilloscopeChannel::CHANNEL_TYPE_DIGITAL, color, CAT_GENERATION)
	, m_baudname("Data Rate")
	, m_polyname("Polynomial")
	, m_depthname("Depth")
{
	//Set up streams
	ClearStreams();
	AddStream("Data");
	AddStream("Clock");

	m_parameters[m_baudname] = FilterParameter(FilterParameter::TYPE_INT, Unit(Unit::UNIT_BITRATE));
	m_parameters[m_baudname].SetIntVal(103125L * 100L * 1000L);

	m_parameters[m_polyname] = FilterParameter(FilterParameter::TYPE_ENUM, Unit(Unit::UNIT_COUNTS));
	m_parameters[m_polyname].AddEnumValue("PRBS-7", POLY_PRBS7);
	m_parameters[m_polyname].AddEnumValue("PRBS-15", POLY_PRBS15);
	m_parameters[m_polyname].AddEnumValue("PRBS-23", POLY_PRBS23);
	m_parameters[m_polyname].AddEnumValue("PRBS-31", POLY_PRBS31);
	m_parameters[m_polyname].SetIntVal(POLY_PRBS7);

	m_parameters[m_depthname] = FilterParameter(FilterParameter::TYPE_INT, Unit(Unit::UNIT_SAMPLEDEPTH));
	m_parameters[m_depthname].SetIntVal(100 * 1000);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Factory methods

bool PRBSGeneratorFilter::ValidateChannel(size_t /*i*/, StreamDescriptor /*stream*/)
{
	//no inputs
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string PRBSGeneratorFilter::GetProtocolName()
{
	return "PRBS";
}

void PRBSGeneratorFilter::SetDefaultName()
{
	Unit rate(Unit::UNIT_BITRATE);

	string prefix = "";
	switch(m_parameters[m_polyname].GetIntVal())
	{
		case POLY_PRBS7:
			prefix = "PRBS7";
			break;

		case POLY_PRBS15:
			prefix = "PRBS15";
			break;

		case POLY_PRBS23:
			prefix = "PRBS23";
			break;

		case POLY_PRBS31:
		default:
			prefix = "PRBS31";
			break;
	}

	m_hwname = prefix + "(" + rate.PrettyPrint(m_parameters[m_baudname].GetIntVal()).c_str() + ")";
	m_displayname = m_hwname;
}

bool PRBSGeneratorFilter::NeedsConfig()
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual decoder logic

void PRBSGeneratorFilter::Refresh()
{
	size_t depth = m_parameters[m_depthname].GetIntVal();
	int64_t baudrate = m_parameters[m_baudname].GetIntVal();
	int poly = m_parameters[m_polyname].GetIntVal();
	size_t samplePeriod = FS_PER_SECOND / baudrate;

	double t = GetTime();
	int64_t fs = (t - floor(t)) * FS_PER_SECOND;

	//Create the two output waveforms
	DigitalWaveform* dat = dynamic_cast<DigitalWaveform*>(GetData(0));
	if(!dat)
	{
		dat = new DigitalWaveform;
		SetData(dat, 0);
	}
	dat->m_timescale = samplePeriod;
	dat->m_triggerPhase = 0;
	dat->m_startTimestamp = floor(t);
	dat->m_startFemtoseconds = fs;
	dat->m_densePacked = true;
	dat->Resize(depth);

	DigitalWaveform* clk = dynamic_cast<DigitalWaveform*>(GetData(1));
	if(!clk)
	{
		clk = new DigitalWaveform;
		SetData(clk, 1);
	}
	clk->m_timescale = samplePeriod;
	clk->m_triggerPhase = samplePeriod / 2;
	clk->m_startTimestamp = floor(t);
	clk->m_startFemtoseconds = fs;
	clk->m_densePacked = true;
	clk->Resize(depth);

	bool lastclk = false;
	uint32_t prbs = rand();
	for(size_t i=0; i<depth; i++)
	{
		//Fill clock
		clk->m_offsets[i] = i;
		clk->m_durations[i] = 1;
		clk->m_samples[i] = lastclk;
		lastclk = !lastclk;

		//Generate data
		bool value = false;
		uint32_t next;
		switch(poly)
		{
			case POLY_PRBS7:
				next = ( (prbs >> 7) ^ (prbs >> 6) ) & 1;
				break;

			case POLY_PRBS15:
				next = ( (prbs >> 15) ^ (prbs >> 14) ) & 1;
				break;

			case POLY_PRBS23:
				next = ( (prbs >> 23) ^ (prbs >> 18) ) & 1;
				break;

			case POLY_PRBS31:
			default:
				next = ( (prbs >> 31) ^ (prbs >> 28) ) & 1;
				break;
		}
		prbs = (prbs << 1) | next;
		value = (bool)next;

		//Fill data
		dat->m_offsets[i] = i;
		dat->m_durations[i] = 1;
		dat->m_samples[i] = value;
	}
}
