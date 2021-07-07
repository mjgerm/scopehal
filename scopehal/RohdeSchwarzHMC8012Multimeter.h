/***********************************************************************************************************************
*                                                                                                                      *
* libscopehal v0.1                                                                                                     *
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

#ifndef RohdeSchwarzHMC8012Multimeter_h
#define RohdeSchwarzHMC8012Multimeter_h

/**
	@brief A Rohde & Schwarz HMC8012 multimeter
 */
class RohdeSchwarzHMC8012Multimeter
	: public virtual Multimeter
	, public SCPIDevice
{
public:
	RohdeSchwarzHMC8012Multimeter(SCPITransport* transport);
	virtual ~RohdeSchwarzHMC8012Multimeter();

	//Device information
	virtual std::string GetName();
	virtual std::string GetVendor();
	virtual std::string GetSerial();

	virtual unsigned int GetInstrumentTypes();

	virtual unsigned int GetMeasurementTypes();

	//Channel info
	virtual int GetMeterChannelCount();
	virtual std::string GetMeterChannelName(int chan);
	virtual int GetCurrentMeterChannel();
	virtual void SetCurrentMeterChannel(int chan);

	//Meter operating mode
	virtual MeasurementTypes GetMeterMode();
	virtual void SetMeterMode(MeasurementTypes type);

	//Control
	virtual void SetMeterAutoRange(bool enable);
	virtual bool GetMeterAutoRange();
	virtual void StartMeter();
	virtual void StopMeter();

	virtual int GetMeterDigits();

	//Get readings
	virtual double GetMeterValue();

protected:
	MeasurementTypes m_mode;
};

#endif
