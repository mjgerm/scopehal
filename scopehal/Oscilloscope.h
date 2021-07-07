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

/**
	@file
	@author Andrew D. Zonenberg
	@brief Declaration of Oscilloscope
 */

#ifndef Oscilloscope_h
#define Oscilloscope_h

class Instrument;

#include "SCPITransport.h"

/**
	@brief Generic representation of an oscilloscope, logic analyzer, or spectrum analyzer.

	An Oscilloscope contains triggering logic and one or more OscilloscopeChannel objects.
 */
class Oscilloscope : public virtual Instrument
{
public:
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Construction / destruction

	Oscilloscope();
	virtual ~Oscilloscope();

	/**
		@brief Returns the instrument's identification string.

		This function MUST NOT CACHE the return value and is safe to use as a barrier synchronization to ensure that
		the instrument has received and processed all previous commands.
	 */
	virtual std::string IDPing() =0;

	/**
		@brief Instruments are allowed to cache configuration settings to reduce round trip queries to the device.

		In order to see updates made by the user at the front panel, the cache must be flushed.

		Cache flushing is recommended to be manually triggered during interactive operation if there is no way to
		push updates from the scope to the driver.

		In scripted/ATE environments where nobody should be touching the instrument, flushing is typically not needed.

		The default implementation of this function does nothing since the base class provides no caching.
		If a derived class caches configuration, it should override this function to clear any cached data.
	 */
	virtual void FlushConfigCache();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Channel information

	/**
		@brief Gets the number of channels this instrument has.

		Only hardware acquisition channels (analog or digital) are included, not math/memory.

		Note that since external trigger inputs have some of the same settings as acquisition channels, they are
		included in the channel count. Call OscilloscopeChannel::GetType() on a given channel to see what kind
		of channel it is.
	 */
	size_t GetChannelCount();

	/**
		@brief Gets a channel by index

		@param i Zero-based index of channel
	 */
	OscilloscopeChannel* GetChannel(size_t i);

	/**
		@brief Gets a channel given the display name
	 */
	OscilloscopeChannel* GetChannelByDisplayName(const std::string& name);

	/**
		@brief Gets a channel given the hardware name
	 */
	OscilloscopeChannel* GetChannelByHwName(const std::string& name);

	/**
		@brief Checks if a channel is enabled in hardware.
	 */
	virtual bool IsChannelEnabled(size_t i) =0;

	/**
		@brief Turn a channel on, given the index

		@param i Zero-based index of channel
	 */
	virtual void EnableChannel(size_t i) =0;

	/**
		@brief Determines if a channel can be enabled.

		@return False if the channel cannot currently be used
				(due to interleave conflicts or other hardware limitations).

				True if the channel is available or is already enabled.

		@param i Zero-based index of channel

		The default implementation always returns true.
	 */
	virtual bool CanEnableChannel(size_t i);

	/**
		@brief Turn a channel off, given the index.

		This function may optionally configure channel interleaving, if supported in hardware.

		@param i Zero-based index of channel
	 */
	virtual void DisableChannel(size_t i) =0;

	/**
		@brief Gets the coupling used for an input channel

		@param i Zero-based index of channel
	 */
	virtual OscilloscopeChannel::CouplingType GetChannelCoupling(size_t i) =0;

	/**
		@brief Sets the coupling used for an input channel

		@param i Zero-based index of channel
	 */
	virtual void SetChannelCoupling(size_t i, OscilloscopeChannel::CouplingType type) =0;

	/**
		@brief Gets the set of legal coupling values for an input channel

		@param i	Zero-based index of channel
	 */
	virtual std::vector<OscilloscopeChannel::CouplingType> GetAvailableCouplings(size_t i) =0;

	/**
		@brief Gets the display name for a channel. This is an arbitrary user-selected string.

		Some instruments allow displaying channel names in the GUI or on probes. If this is supported, the
		driver should override this function.

		The default function simply stores the display name in the Oscilloscope object, and is appropriate for
		instruments which have no concept of a nickname or display name.

		@param i Zero-based index of channel
	 */
	virtual std::string GetChannelDisplayName(size_t i);

	/**
		@brief Sets the display name for a channel. This is an arbitrary user-selected string.

		Some instruments allow displaying channel names in the GUI or on probes. If this is supported, the
		driver should override this function.

		The default function simply stores the display name in the Oscilloscope object, and is appropriate for
		instruments which have no concept of a nickname or display name.

		@param i	Zero-based index of channel
		@param name	Name of the channel
	 */
	virtual void SetChannelDisplayName(size_t i, std::string name);

	/**
		@brief Gets the probe attenuation for an input channel.

		Note that this function returns attenuation, not gain.
		For example, a 10x probe would return 10 and not 0.1.

		@param i Zero-based index of channel
	 */
	virtual double GetChannelAttenuation(size_t i) =0;

	/**
		@brief Sets the probe attenuation used for an input channel.

		@param i		Zero-based index of channel
		@param atten	Attenuation factor
	 */
	virtual void SetChannelAttenuation(size_t i, double atten) =0;

	/**
		@brief Gets the set of available bandwidth limiters for an input channel.

		@param i Zero-based index of channel
	 */
	virtual std::vector<unsigned int> GetChannelBandwidthLimiters(size_t i);

	/**
		@brief Gets the bandwidth limit for an input channel.

		@param i Zero-based index of channel

		@return Bandwidth limit, in MHz. Zero means "no bandwidth limit".
	 */
	virtual int GetChannelBandwidthLimit(size_t i) =0;

	/**
		@brief Sets the bandwidth limit for an input channel.

		@param i			Zero-based index of channel
		@param limit_mhz	Bandwidth limit, in MHz. Zero means "no bandwidth limit".
	 */
	virtual void SetChannelBandwidthLimit(size_t i, unsigned int limit_mhz) =0;

	/**
		@brief Returns the external trigger input channel, if we have one.

		Note that some very high end oscilloscopes have multiple external trigger inputs.
		We do not currently support this.
	 */
	virtual OscilloscopeChannel* GetExternalTrigger() =0;

	/**
		@brief Gets the range of the current channel configuration.

		The range is the distance, in volts, between the most negative/smallest and most positive/largest
		voltage which the ADC can represent using the current vertical gain configuration. This can be calculated
		as the number of vertical divisions times the number of volts per division.

		The range does not depend on the offset.

		@param i			Zero-based index of channel
	 */
	virtual double GetChannelVoltageRange(size_t i) =0;

	/**
		@brief Sets the range of the current channel configuration.

		The range is the distance, in volts, between the most negative/smallest and most positive/largest
		voltage which the ADC can represent using the current vertical gain configuration. This can be calculated
		as the number of vertical divisions times the number of volts per division.

		The range does not depend on the offset.

		@param i			Zero-based index of channel
		@param range		Voltage range
	 */
	virtual void SetChannelVoltageRange(size_t i, double range) =0;

	/**
		@brief Determines if a channel has a probe connected which supports the "auto zero" feature.

		This is typically true for power rail and differential probes and false for most others.

		@param i			Zero-based index of channel
	 */
	virtual bool CanAutoZero(size_t i);

	/**
		@brief Performs an "auto zero" cycle on the attached active probe, if supported by the hardware

		@param i			Zero-based index of channel
	 */
	virtual void AutoZero(size_t i);

	/**
		@brief Returns the name of the probe connected to the scope, if possible.

		If a passive or no probe is connected, or the instrument driver does not support probe identification,
		an empty string may be returned.

		@param i			Zero-based index of channel
	 */
	virtual std::string GetProbeName(size_t i);

	/**
		@brief Checks if a channel has an input multiplexer

		@param i			Zero-based index of channel
	 */
	virtual bool HasInputMux(size_t i);

	/**
		@brief Gets the setting for a channel's input mux (if it has one)

		@param i			Zero-based index of channel
	 */
	virtual size_t GetInputMuxSetting(size_t i);

	/**
		@brief Gets names for the input mux ports of a channel

		@param i			Zero-based index of channel
	 */
	virtual std::vector<std::string> GetInputMuxNames(size_t i);

	/**
		@brief Sets the input mux for a channel

		@param i			Zero-based index of channel
		@param select		Selector for the mux
	 */
	virtual void SetInputMux(size_t i, size_t select);

	/**
		@brief Gets the offset, in volts, for a given channel

		@param i			Zero-based index of channel
	 */
	virtual double GetChannelOffset(size_t i) =0;

	/**
		@brief Sets the offset for a given channel

		@param i			Zero-based index of channel
		@param offset		Offset, in volts
	 */
	virtual void SetChannelOffset(size_t i, double offset) =0;

	/**
		@brief Checks if a channel is capable of hardware polarity inversion

		@param i			Zero-based index of channel
	 */
	virtual bool CanInvert(size_t i);

	/**
		@brief Enables hardware polarity inversion for a channel, if supported

		@param i			Zero-based index of channel
		@param invert		True to invert, false for normal operation
	 */
	virtual void Invert(size_t i, bool invert);

	/**
		@brief Checks if hardware polarity inversion is enabled for a channel

		@param i			Zero-based index of channel
	 */
	virtual bool IsInverted(size_t i);

	//Triggering
	enum TriggerMode
	{
		///Active, waiting for a trigger event
		TRIGGER_MODE_RUN,

		///Triggered once, but not recently
		TRIGGER_MODE_STOP,

		///Just got triggered, data is ready to read
		TRIGGER_MODE_TRIGGERED,

		///WAIT - not yet fully armed
		TRIGGER_MODE_WAIT,

		///Auto trigger - waiting for auto-trigger
		TRIGGER_MODE_AUTO,

		///Placeholder
		TRIGGER_MODE_COUNT
	};

	/**
		@brief Checks the curent trigger status.
	 */
	virtual Oscilloscope::TriggerMode PollTrigger() =0;

	/**
		@brief Checks if the trigger is armed, without altering internal state or checking caches.

		The default implementation of this function simply calls PollTrigger(). This function should be overridden by
		the driver class if PollTrigger() changes any internal driver state or updates caches.
	 */
	virtual bool PeekTriggerArmed();

	/**
		@brief Block until a trigger happens or a timeout elapses.

		Note that this function has no provision to dispatch any UI events etc.
		It's intended as a convenience helper for non-interactive ATE applications only.

		@param timeout	Timeout value, in milliseconds

		@return True if triggered, false if timeout
	 */
	bool WaitForTrigger(int timeout);

	/**
		@brief Sets a new trigger on the instrument and pushes changes.

		Calling SetTrigger() with the currently selected trigger is legal and is equivalent to calling PushTrigger().

		Ownership of the trigger object is transferred to the Oscilloscope.
	 */
	void SetTrigger(Trigger* trigger)
	{
		Trigger* old_trig = m_trigger;

		//Set the new trigger and sync to hardware
		m_trigger = trigger;
		PushTrigger();

		//Delete old trigger *after* pushing the new one.
		//This prevents possible race conditions where we disable the current trigger channel before the new
		//trigger is set.
		if(old_trig != trigger)
			delete old_trig;
	}

	/**
		@brief Pushes changes made to m_trigger to the instrument
	 */
	virtual void PushTrigger() =0;

	/**
		@brief Gets the current trigger.

		Ownership of the trigger object is retained by the Oscilloscope. This pointer may be invalidated by any future
		call to GetTrigger() or PullTrigger().
	 */
	Trigger* GetTrigger(bool sync = false)
	{
		if(sync || (m_trigger == NULL) )
			PullTrigger();
		return m_trigger;
	}

	/**
		@brief Gets a list of triggers this instrument supports
	 */
	virtual std::vector<std::string> GetTriggerTypes();

	/**
		@brief Updates m_trigger with any changes made from the instrument side
	 */
	virtual void PullTrigger() =0;

	/**
		@brief Reads a waveform into the queue of pending waveforms
	 */
	virtual bool AcquireData() =0;

	/**
		@brief Starts the instrument in continuous trigger mode.

		Most drivers will implement this as repeated calls to the "single trigger" function to avoid race conditions
		when the instrument triggers halfway through downloading captured waveforms.
	 */
	virtual void Start() =0;

	/**
		@brief Arms the trigger for a single acquistion.
	 */
	virtual void StartSingleTrigger() =0;

	/**
		@brief Stops triggering
	 */
	virtual void Stop() =0;

	/**
		@brief Forces a single acquisition as soon as possible.

		Note that PollTrigger() may not return 'triggered' immediately, due to command processing latency.
	 */
	virtual void ForceTrigger() =0;

	/**
		@brief Checks if the trigger is currently armed
	 */
	virtual bool IsTriggerArmed() =0;

	/**
		@brief Enables the trigger output, configuring a shared auxiliary port for this purpose if needed

		The default implementation does nothing, and is intended for instruments where the trigger output is always
		enabled.
	 */
	virtual void EnableTriggerOutput();

	/**
		@brief Gets the connection string for our transport
	 */
	virtual std::string GetTransportConnectionString() =0;

	/**
		@brief Gets the name of our transport
	 */
	virtual std::string GetTransportName() =0;

public:
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Memory depth / sample rate control.

	/**
		@brief Get the legal sampling rates (in Hz) for this scope in all-channels mode
	 */
	virtual std::vector<uint64_t> GetSampleRatesNonInterleaved() =0;

	/**
		@brief Get the legal sampling rates (in Hz) for this scope in combined-channels mode
	 */
	virtual std::vector<uint64_t> GetSampleRatesInterleaved() =0;

	/**
		@brief Gets the current sampling rate (in Hz) of this scope
	 */
	virtual uint64_t GetSampleRate() =0;

	/**
		@brief Checks if the scope is currently combining channels
	 */
	virtual bool IsInterleaving() =0;

	/**
		@brief Configures the scope to combine channels.

		This function may fail to enable channel combining if conflicts are present, check the return value!

		@return True if channel combining is enabled, false if not
	 */
	virtual bool SetInterleaving(bool combine) =0;

	/**
		@brief Returns true if we have no interleave conflicts, false if we have conflicts
	 */
	virtual bool CanInterleave();

	/**
		@brief Get the set of conflicting channels.

		If any pair of channels in this list is enabled, channel interleaving is not possible.
	 */
	typedef std::pair<OscilloscopeChannel*, OscilloscopeChannel* > InterleaveConflict;
	virtual std::set< InterleaveConflict > GetInterleaveConflicts() =0;

	/**
		@brief Get the legal memory depths for this scope in all-channels mode
	 */
	virtual std::vector<uint64_t> GetSampleDepthsNonInterleaved() =0;

	/**
		@brief Get the legal memory depths for this scope in combined-channels mode
	 */
	virtual std::vector<uint64_t> GetSampleDepthsInterleaved() =0;

	/**
		@brief Gets the current sample depth of this scope
	 */
	virtual uint64_t GetSampleDepth() =0;

	/**
		@brief Sets the sample depth of the scope
	 */
	virtual void SetSampleDepth(uint64_t depth) =0;

	/**
		@brief Sets the sample rate of the scope, in Hz
	 */
	virtual void SetSampleRate(uint64_t rate) =0;

	enum SamplingMode
	{
		REAL_TIME,
		EQUIVALENT_TIME
	};

	/**
		@brief Returns true if the requested sampling mode is available with the current instrument configuration.

		The default implementation returns true for real-time only.
	 */
	virtual bool IsSamplingModeAvailable(SamplingMode mode);

	/**
		@brief Gets the current sampling mode of the instrument

		The default implementation returns "real time"
	 */
	virtual SamplingMode GetSamplingMode();

	/**
		@brief Sets the current sampling mode of the instrument

		The default implementation is a no-op.
	 */
	virtual void SetSamplingMode(SamplingMode mode);

	/**
		@brief Configures the instrument's clock source

		@param external		True to use external reference
							False to use internal clock

		The default implementation prints an "unsupported operation" warning, and is suitable for lower-end
		instruments that do not support external clock inputs.
	 */
	virtual void SetUseExternalRefclk(bool external);

	/**
		@brief Sets the trigger offset

		@param offset		Femtoseconds from the start of the capture to the trigger point.
							Positive values mean the trigger is within the waveform.
							Negative values mean there is a delay from the trigger point to the start of the waveform.
	 */
	virtual void SetTriggerOffset(int64_t offset)	=0;

	/**
		@brief Gets the trigger offset
	 */
	virtual int64_t GetTriggerOffset() =0;

	/**
		@brief Sets the deskew setting for a channel

		@param channel		The channel to deskew
		@param skew			Skew value, in femtoseconds.
							Negative values move the channel earlier relative to the zero point.
							Positive values move the channel later.

		The default implementation does nothing, and is suitable for lower-end instruments that do not support deskew.
	 */
	virtual void SetDeskewForChannel(size_t channel, int64_t skew);

	/**
		@brief Gets the deskew setting for a channel

		The default implementation returns zero, and is suitable for lower-end instruments that do not support deskew.
	 */
	virtual int64_t GetDeskewForChannel(size_t channel);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Sequenced triggering

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ADC bit depth configuration

	typedef std::vector<OscilloscopeChannel*> AnalogBank;

	/**
		@brief Gets the analog banks for this instrument.

		A bank is a set of one or more channels all sharing a common ADC configuration.
	 */
	virtual std::vector<AnalogBank> GetAnalogBanks();

	/**
		@brief Gets the bank containing a given channel.
	 */
	virtual AnalogBank GetAnalogBank(size_t channel);

	/**
		@brief Returns true if the ADC is configurable, false if it can only run in one mode.
	 */
	virtual bool IsADCModeConfigurable();

	/**
		@brief Gets the names of the ADC modes for the bank a given channel is located in.

		ADC mode names are usually descriptive, like "12 bit, 640 Msps max" or "8 bit, 1 Gsps max"; but some instruments
		may use more generic

		@param channel	Index of the channel to query modes for
	 */
	virtual std::vector<std::string> GetADCModeNames(size_t channel);

	/**
		@brief Gets the ADC mode for a channel
	 */
	virtual size_t GetADCMode(size_t channel);

	/**
		@brief Sets the ADC mode for a channel
	 */
	virtual void SetADCMode(size_t channel, size_t mode);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Logic analyzer configuration

	typedef std::vector<OscilloscopeChannel*> DigitalBank;

	/**
		@brief Gets the digital channel banks for this instrument.

		A bank is a set of one or more channels all sharing a common threshold and hysteresis setting.
	 */
	virtual std::vector<DigitalBank> GetDigitalBanks();

	/**
		@brief Gets the bank containing a given channel.
	 */
	virtual DigitalBank GetDigitalBank(size_t channel);

	/**
		@brief Checks if digital input hysteresis is configurable or fixed.

		@return true if configurable, false if fixed
	 */
	virtual bool IsDigitalHysteresisConfigurable();

	/**
		@brief Checks if digital input threshold is configurable or fixed.

		@return true if configurable, false if fixed
	 */
	virtual bool IsDigitalThresholdConfigurable();

	/**
		@brief Gets the hysteresis for a digital input
	 */
	virtual float GetDigitalHysteresis(size_t channel);

	/**
		@brief Gets the threshold for a digital input
	 */
	virtual float GetDigitalThreshold(size_t channel);

	/**
		@brief Sets the hysteresis for a digital input
	 */
	virtual void SetDigitalHysteresis(size_t channel, float level);

	/**
		@brief Gets the threshold for a digital input
	 */
	virtual void SetDigitalThreshold(size_t channel, float level);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Frequency domain channel configuration

	/**
		@brief Sets the span for frequency-domain channels

		@param span		Span, in Hz
	 */
	virtual void SetSpan(int64_t span);

	/**
		@brief Gets the span for frequency-domain channels
	 */
	virtual int64_t GetSpan();

	/**
		@brief Sets the center frequency for frequency-domain channels

		@param channel	Channel number
		@param freq		Center frequency, in Hz
	 */
	virtual void SetCenterFrequency(size_t channel, int64_t freq);

	/**
		@brief Gets the center frequency for a frequency-domain channel

		@param channel	Channel number
	 */
	virtual int64_t GetCenterFrequency(size_t channel);

	/**
		@brief Gets the resolution bandwidth for frequency-domain channels
	 */
	virtual void SetResolutionBandwidth(int64_t rbw);

	/**
		@brief Gets the resolution bandwidth for frequency-domain channels
	 */
	virtual int64_t GetResolutionBandwidth();

	/**
		@brief Returns true if the instrument has at least one frequency-domain channel
	 */
	virtual bool HasFrequencyControls();

	//TODO: window controls

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration storage

	/**
		@brief Serializes this oscilloscope's configuration to a YAML string.

		@return YAML block with this oscilloscope's configuration
	 */
	virtual std::string SerializeConfiguration(IDTable& table);

	/**
		@brief Load instrument and channel configuration from a save file
	 */
	virtual void LoadConfiguration(const YAML::Node& node, IDTable& idmap);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Sample format conversion
protected:
	void Convert8BitSamples(
		int64_t* offs, int64_t* durs, float* pout, int8_t* pin, float gain, float offset, size_t count, int64_t ibase);
	void Convert8BitSamplesGeneric(
		int64_t* offs, int64_t* durs, float* pout, int8_t* pin, float gain, float offset, size_t count, int64_t ibase);
	void Convert8BitSamplesAVX2(
		int64_t* offs, int64_t* durs, float* pout, int8_t* pin, float gain, float offset, size_t count, int64_t ibase);

	void Convert16BitSamples(
		int64_t* offs, int64_t* durs, float* pout, int16_t* pin, float gain, float offset, size_t count, int64_t ibase);
	void Convert16BitSamplesGeneric(
		int64_t* offs, int64_t* durs, float* pout, int16_t* pin, float gain, float offset, size_t count, int64_t ibase);
	void Convert16BitSamplesAVX2(
		int64_t* offs, int64_t* durs, float* pout, int16_t* pin, float gain, float offset, size_t count, int64_t ibase);
	void Convert16BitSamplesFMA(
		int64_t* offs, int64_t* durs, float* pout, int16_t* pin, float gain, float offset, size_t count, int64_t ibase);

public:
	bool HasPendingWaveforms();
	void ClearPendingWaveforms();
	size_t GetPendingWaveformCount();
	virtual bool PopPendingWaveform();

protected:
	typedef std::map<OscilloscopeChannel*, WaveformBase*> SequenceSet;
	std::list<SequenceSet> m_pendingWaveforms;
	std::mutex m_pendingWaveformsMutex;
	std::recursive_mutex m_mutex;

protected:

	///The channels
	std::vector<OscilloscopeChannel*> m_channels;

	///Display names for channels
	std::map<OscilloscopeChannel*, std::string> m_channelDisplayNames;

	//The trigger
	Trigger* m_trigger;

public:
	typedef Oscilloscope* (*CreateProcType)(SCPITransport*);
	static void DoAddDriverClass(std::string name, CreateProcType proc);

	static void EnumDrivers(std::vector<std::string>& names);
	static Oscilloscope* CreateOscilloscope(std::string driver, SCPITransport* transport);

	virtual std::string GetDriverName() =0;
	//static std::string GetDriverNameInternal();

protected:
	//Class enumeration
	typedef std::map< std::string, CreateProcType > CreateMapType;
	static CreateMapType m_createprocs;
};

#define STRINGIFY(T) #T

/*
	For some reason, certain classes don't like being created with OSCILLOSCOPE_INITPROC.
	sizeof(T) has inconsistent values between .h and .cpp and we get memory bounds errors writing off the end of a
	too-small memory block in the constructor.

	If this happens use OSCILLOSCOPE_INITPROC_H in the header and OSCILLOSCOPE_INITPROC_CPP in the source.

	The cause of this is currently unknown but this workaround shows no errors in valgrind and doesn't crash sooo...
*/
#define OSCILLOSCOPE_INITPROC_H(T) \
	static Oscilloscope* CreateInstance(SCPITransport* transport); \
	virtual std::string GetDriverName() \
	{ return GetDriverNameInternal(); }

#define OSCILLOSCOPE_INITPROC_CPP(T) \
	Oscilloscope* T::CreateInstance(SCPITransport* transport) \
	{ return new T(transport); }

#define OSCILLOSCOPE_INITPROC(T) \
	static Oscilloscope* CreateInstance(SCPITransport* transport) \
	{	return new T(transport); } \
	virtual std::string GetDriverName() \
	{ return GetDriverNameInternal(); }

#define AddDriverClass(T) Oscilloscope::DoAddDriverClass(T::GetDriverNameInternal(), T::CreateInstance)

#endif
