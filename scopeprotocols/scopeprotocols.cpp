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

/**
	@file
	@author Andrew D. Zonenberg
	@brief Scope protocol initialization
 */

#include "scopeprotocols.h"

/**
	@brief Static initialization for protocol list
 */
void ScopeProtocolStaticInit()
{
	AddDecoderClass(ACCoupleFilter);
	AddDecoderClass(AutocorrelationFilter);
	AddDecoderClass(ADL5205Decoder);
	AddDecoderClass(BaseMeasurement);
	AddDecoderClass(CANDecoder);
	AddDecoderClass(ChannelEmulationFilter);
	AddDecoderClass(ClockRecoveryFilter);
	AddDecoderClass(CTLEFilter);
	AddDecoderClass(CurrentShuntFilter);
	AddDecoderClass(DCDMeasurement);
	AddDecoderClass(DCOffsetFilter);
	AddDecoderClass(DDJMeasurement);
	AddDecoderClass(DDR1Decoder);
	AddDecoderClass(DDR3Decoder);
	AddDecoderClass(DeEmbedFilter);
	AddDecoderClass(DeskewFilter);
	AddDecoderClass(DigitalToPAM4Filter);
	AddDecoderClass(DigitalToNRZFilter);
	AddDecoderClass(DownconvertFilter);
	AddDecoderClass(DownsampleFilter);
	AddDecoderClass(DPhyDataDecoder);
	AddDecoderClass(DPhyHSClockRecoveryFilter);
	AddDecoderClass(DPhySymbolDecoder);
	AddDecoderClass(DramClockFilter);
	AddDecoderClass(DramRefreshActivateMeasurement);
	AddDecoderClass(DramRowColumnLatencyMeasurement);
	AddDecoderClass(DSIFrameDecoder);
	AddDecoderClass(DSIPacketDecoder);
	AddDecoderClass(DutyCycleMeasurement);
	AddDecoderClass(DVIDecoder);
	AddDecoderClass(EmphasisFilter);
	AddDecoderClass(EmphasisRemovalFilter);
	AddDecoderClass(ESPIDecoder);
	AddDecoderClass(Ethernet10BaseTDecoder);
	AddDecoderClass(Ethernet100BaseTDecoder);
	AddDecoderClass(Ethernet1000BaseXDecoder);
	AddDecoderClass(Ethernet10GBaseRDecoder);
	AddDecoderClass(Ethernet64b66bDecoder);
	AddDecoderClass(EthernetGMIIDecoder);
	AddDecoderClass(EthernetRGMIIDecoder);
	AddDecoderClass(EthernetAutonegotiationDecoder);
	AddDecoderClass(EyeBitRateMeasurement);
	AddDecoderClass(EyePattern);
	AddDecoderClass(EyeHeightMeasurement);
	AddDecoderClass(EyeJitterMeasurement);
	AddDecoderClass(EyePeriodMeasurement);
	AddDecoderClass(EyeWidthMeasurement);
	AddDecoderClass(FallMeasurement);
	AddDecoderClass(FFTFilter);
	AddDecoderClass(FIRFilter);
	AddDecoderClass(FrequencyMeasurement);
	AddDecoderClass(HistogramFilter);
	AddDecoderClass(HorizontalBathtub);
	AddDecoderClass(I2CDecoder);
	AddDecoderClass(I2CEepromDecoder);
	AddDecoderClass(I2SDecoder);
	AddDecoderClass(IBM8b10bDecoder);
	AddDecoderClass(IPv4Decoder);
	AddDecoderClass(ISIMeasurement);
	AddDecoderClass(JitterFilter);
	AddDecoderClass(JitterSpectrumFilter);
	AddDecoderClass(JtagDecoder);
	AddDecoderClass(MagnitudeFilter);
	AddDecoderClass(MDIODecoder);
	AddDecoderClass(MilStd1553Decoder);
	AddDecoderClass(MovingAverageFilter);
	AddDecoderClass(MultiplyFilter);
	AddDecoderClass(NoiseFilter);
	//AddDecoderClass(OFDMDemodulator);
	AddDecoderClass(OneWireDecoder);
	AddDecoderClass(OvershootMeasurement);
	AddDecoderClass(ParallelBus);
	AddDecoderClass(PCIeDataLinkDecoder);
	AddDecoderClass(PCIeGen2LogicalDecoder);
	AddDecoderClass(PCIeTransportDecoder);
	AddDecoderClass(PeakHoldFilter);
	AddDecoderClass(PeriodMeasurement);
	AddDecoderClass(PhaseMeasurement);
	AddDecoderClass(PkPkMeasurement);
	AddDecoderClass(PRBSGeneratorFilter);
	AddDecoderClass(RjBUjFilter);
	AddDecoderClass(QSPIDecoder);
	AddDecoderClass(QuadratureDecoder);
	AddDecoderClass(RiseMeasurement);
	AddDecoderClass(SDCmdDecoder);
	AddDecoderClass(SDDataDecoder);
	AddDecoderClass(SpectrogramFilter);
	AddDecoderClass(SPIDecoder);
	AddDecoderClass(SPIFlashDecoder);
	AddDecoderClass(StepGeneratorFilter);
	AddDecoderClass(SubtractFilter);
	AddDecoderClass(SWDDecoder);
	AddDecoderClass(SWDMemAPDecoder);
	AddDecoderClass(TachometerFilter);
	AddDecoderClass(TappedDelayLineFilter);
	AddDecoderClass(TDRFilter);
	//AddDecoderClass(TDRStepDeEmbedFilter);
	AddDecoderClass(ThresholdFilter);
	AddDecoderClass(TIEMeasurement);
	AddDecoderClass(TMDSDecoder);
	AddDecoderClass(ToneGeneratorFilter);
	AddDecoderClass(TopMeasurement);
	AddDecoderClass(UARTDecoder);
	AddDecoderClass(UartClockRecoveryFilter);
	AddDecoderClass(UndershootMeasurement);
	AddDecoderClass(UpsampleFilter);
	AddDecoderClass(USB2ActivityDecoder);
	AddDecoderClass(USB2PacketDecoder);
	AddDecoderClass(USB2PCSDecoder);
	AddDecoderClass(USB2PMADecoder);
	AddDecoderClass(VerticalBathtub);
	AddDecoderClass(Waterfall);
	AddDecoderClass(WindowedAutocorrelationFilter);

	AddStatisticClass(AverageStatistic);
	AddStatisticClass(MaximumStatistic);
	AddStatisticClass(MinimumStatistic);
}
