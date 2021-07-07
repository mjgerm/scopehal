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

#ifndef AgilentOscilloscope_h
#define AgilentOscilloscope_h

#include "EdgeTrigger.h"
#include "PulseWidthTrigger.h"
#include "NthEdgeBurstTrigger.h"

class AgilentOscilloscope : public SCPIOscilloscope
{
public:
	AgilentOscilloscope(SCPITransport* transport);
	virtual ~AgilentOscilloscope();

	//not copyable or assignable
	AgilentOscilloscope(const AgilentOscilloscope& rhs) =delete;
	AgilentOscilloscope& operator=(const AgilentOscilloscope& rhs) =delete;

public:

	//Device information
	virtual unsigned int GetInstrumentTypes();

	virtual void FlushConfigCache();

	//Channel configuration
	virtual bool IsChannelEnabled(size_t i);
	virtual void EnableChannel(size_t i);
	virtual void DisableChannel(size_t i);
	virtual OscilloscopeChannel::CouplingType GetChannelCoupling(size_t i);
	virtual void SetChannelCoupling(size_t i, OscilloscopeChannel::CouplingType type);
	virtual std::vector<OscilloscopeChannel::CouplingType> GetAvailableCouplings(size_t i);
	virtual double GetChannelAttenuation(size_t i);
	virtual void SetChannelAttenuation(size_t i, double atten);
	virtual int GetChannelBandwidthLimit(size_t i);
	virtual void SetChannelBandwidthLimit(size_t i, unsigned int limit_mhz);
	virtual double GetChannelVoltageRange(size_t i);
	virtual void SetChannelVoltageRange(size_t i, double range);
	virtual OscilloscopeChannel* GetExternalTrigger();
	virtual double GetChannelOffset(size_t i);
	virtual void SetChannelOffset(size_t i, double offset);

	//Triggering
	virtual Oscilloscope::TriggerMode PollTrigger();
	virtual bool AcquireData();
	virtual void Start();
	virtual void StartSingleTrigger();
	virtual void Stop();
	virtual void ForceTrigger();
	virtual bool IsTriggerArmed();
	virtual void PushTrigger();
	virtual void PullTrigger();
	virtual std::vector<std::string> GetTriggerTypes();

	virtual std::vector<uint64_t> GetSampleRatesNonInterleaved();
	virtual std::vector<uint64_t> GetSampleRatesInterleaved();
	virtual std::set<InterleaveConflict> GetInterleaveConflicts();
	virtual std::vector<uint64_t> GetSampleDepthsNonInterleaved();
	virtual std::vector<uint64_t> GetSampleDepthsInterleaved();
	virtual uint64_t GetSampleRate();
	virtual uint64_t GetSampleDepth();
	virtual void SetSampleDepth(uint64_t depth);
	virtual void SetSampleRate(uint64_t rate);
	virtual void SetTriggerOffset(int64_t offset);
	virtual int64_t GetTriggerOffset();
	virtual bool IsInterleaving();
	virtual bool SetInterleaving(bool combine);

protected:
	OscilloscopeChannel* m_extTrigChannel;

	//Mutexing for thread safety
	std::recursive_mutex m_cacheMutex;

	//hardware analog channel count, independent of LA option etc
	unsigned int m_analogChannelCount;

	enum ProbeType {
		None,
		AutoProbe,
		SmartProbe,
	};

	//config cache
	std::map<size_t, double> m_channelOffsets;
	std::map<size_t, double> m_channelVoltageRanges;
	std::map<size_t, OscilloscopeChannel::CouplingType> m_channelCouplings;
	std::map<size_t, double> m_channelAttenuations;
	std::map<size_t, int> m_channelBandwidthLimits;
	std::map<int, bool> m_channelsEnabled;
	std::map<size_t, ProbeType> m_probeTypes;

	bool m_sampleDepthValid;
	uint64_t m_sampleDepth;
	bool m_sampleRateValid;
	uint64_t m_sampleRate;

	bool m_triggerArmed;
	bool m_triggerOneShot;

	void PullEdgeTrigger();
	void PullNthEdgeBurstTrigger();
	void PullPulseWidthTrigger();

	void GetTriggerSlope(EdgeTrigger* trig, std::string reply);
	void GetTriggerSlope(NthEdgeBurstTrigger* trig, std::string reply);
	Trigger::Condition GetCondition(std::string reply);
	void GetProbeType(size_t i);

	void PushEdgeTrigger(EdgeTrigger* trig);
	void PushNthEdgeBurstTrigger(NthEdgeBurstTrigger* trig);
	void PushPulseWidthTrigger(PulseWidthTrigger* trig);
	void PushCondition(std::string path, Trigger::Condition cond);
	void PushFloat(std::string path, float f);
	void PushSlope(std::string path, EdgeTrigger::EdgeType slope);
	void PushSlope(std::string path, NthEdgeBurstTrigger::EdgeType slope);

private:
	static std::map<uint64_t, uint64_t> m_sampleRateToDuration;

	void SetSampleRateAndDepth(uint64_t rate, uint64_t depth);


public:
	static std::string GetDriverNameInternal();
	OSCILLOSCOPE_INITPROC(AgilentOscilloscope)
};

#endif
