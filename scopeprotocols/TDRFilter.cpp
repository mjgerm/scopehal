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
#include "TDRFilter.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

TDRFilter::TDRFilter(const string& color)
	: Filter(OscilloscopeChannel::CHANNEL_TYPE_ANALOG, color, CAT_ANALYSIS)
{
	//Set up channels
	CreateInput("voltage");

	m_modeName = "Output Format";
	m_parameters[m_modeName] = FilterParameter(FilterParameter::TYPE_ENUM, Unit(Unit::UNIT_COUNTS));
	m_parameters[m_modeName].AddEnumValue("Reflection coefficient", MODE_RHO);
	m_parameters[m_modeName].AddEnumValue("Impedance", MODE_IMPEDANCE);
	m_parameters[m_modeName].SetIntVal(MODE_IMPEDANCE);

	m_portImpedanceName = "Port impedance";
	m_parameters[m_portImpedanceName] = FilterParameter(FilterParameter::TYPE_FLOAT, Unit(Unit::UNIT_OHMS));
	m_parameters[m_portImpedanceName].SetFloatVal(50);

	m_stepStartVoltageName = "Step start";
	m_parameters[m_stepStartVoltageName] = FilterParameter(FilterParameter::TYPE_FLOAT, Unit(Unit::UNIT_VOLTS));
	m_parameters[m_stepStartVoltageName].SetFloatVal(0);

	m_stepEndVoltageName = "Step end";
	m_parameters[m_stepEndVoltageName] = FilterParameter(FilterParameter::TYPE_FLOAT, Unit(Unit::UNIT_VOLTS));
	m_parameters[m_stepEndVoltageName].SetFloatVal(1);

	m_range = 20;
	m_offset = -50;

	m_oldMode = MODE_IMPEDANCE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Factory methods

bool TDRFilter::ValidateChannel(size_t i, StreamDescriptor stream)
{
	if(stream.m_channel == NULL)
		return false;

	if( (i == 0) && (stream.m_channel->GetType() == OscilloscopeChannel::CHANNEL_TYPE_ANALOG) )
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

double TDRFilter::GetVoltageRange()
{
	return m_range;
}

double TDRFilter::GetOffset()
{
	return m_offset;
}

void TDRFilter::SetVoltageRange(double range)
{
	m_range = range;
}

void TDRFilter::SetOffset(double offset)
{
	m_offset = offset;
}

string TDRFilter::GetProtocolName()
{
	return "TDR";
}

bool TDRFilter::IsOverlay()
{
	//we create a new analog channel
	return false;
}

bool TDRFilter::NeedsConfig()
{
	return true;
}

void TDRFilter::SetDefaultName()
{
	char hwname[256];
	if(m_parameters[m_modeName].GetIntVal() == MODE_IMPEDANCE)
		snprintf(hwname, sizeof(hwname), "TDRImpedance(%s)", GetInputDisplayName(0).c_str());
	else
		snprintf(hwname, sizeof(hwname), "TDRReflection(%s)", GetInputDisplayName(0).c_str());

	m_hwname = hwname;
	m_displayname = m_hwname;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual decoder logic

void TDRFilter::Refresh()
{
	//Make sure we've got valid inputs
	if(!VerifyAllInputsOKAndAnalog())
	{
		SetData(NULL, 0);
		return;
	}

	auto din = GetAnalogInputWaveform(0);
	auto len = din->m_samples.size();

	//Extract parameters
	auto mode = static_cast<OutputMode>(m_parameters[m_modeName].GetIntVal());
	auto z0 = m_parameters[m_portImpedanceName].GetFloatVal();
	auto vlo = m_parameters[m_stepStartVoltageName].GetFloatVal();
	auto vhi = m_parameters[m_stepEndVoltageName].GetFloatVal();
	auto pulseAmplitude = (vhi - vlo);

	//Set up units
	if(mode == MODE_IMPEDANCE)
		m_yAxisUnit = Unit(Unit::UNIT_OHMS);
	else
		m_yAxisUnit = Unit(Unit::UNIT_RHO);

	//Reset gain/offset if output mode was changed
	if(mode != m_oldMode)
	{
		if(mode == MODE_IMPEDANCE)
		{
			m_range = 20;
			m_offset = -50;
		}
		else
		{
			m_range = 2;
			m_offset = 0;
		}
		m_oldMode = mode;
	}

	//Set up the output waveform
	auto cap = SetupOutputWaveform(din, 0, 0, 0);

	float pulseScale = 1.0 / pulseAmplitude;
	for(size_t i=0; i<len; i++)
	{
		//Reflection coefficient is trivial
		float rho = (din->m_samples[i] - vhi) * pulseScale;

		//Impedance takes a bit more work to calculate
		if(mode == MODE_IMPEDANCE)
			cap->m_samples[i] = z0 * (1 + rho)/(1 - rho);

		else
			cap->m_samples[i] = rho;
	}
}
