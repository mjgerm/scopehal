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
#include "USB2PacketDecoder.h"
#include "USB2PCSDecoder.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

USB2PacketDecoder::USB2PacketDecoder(const string& color)
	: PacketDecoder(OscilloscopeChannel::CHANNEL_TYPE_COMPLEX, color, CAT_SERIAL)
{
	//Set up channels
	CreateInput("PCS");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Factory methods

bool USB2PacketDecoder::ValidateChannel(size_t i, StreamDescriptor stream)
{
	if(stream.m_channel == NULL)
		return false;

	if( (i == 0) && (dynamic_cast<USB2PCSDecoder*>(stream.m_channel) != NULL) )
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

void USB2PacketDecoder::SetDefaultName()
{
	char hwname[256];
	snprintf(hwname, sizeof(hwname), "USB2Packet(%s)", GetInputDisplayName(0).c_str());
	m_hwname = hwname;
	m_displayname = m_hwname;
}

string USB2PacketDecoder::GetProtocolName()
{
	return "USB 1.x/2.0 Packet";
}

bool USB2PacketDecoder::IsOverlay()
{
	return true;
}

bool USB2PacketDecoder::NeedsConfig()
{
	return true;
}

double USB2PacketDecoder::GetVoltageRange()
{
	return 1;
}

bool USB2PacketDecoder::GetShowDataColumn()
{
	return false;
}

vector<string> USB2PacketDecoder::GetHeaders()
{
	vector<string> ret;
	ret.push_back("Type");
	ret.push_back("Device");
	ret.push_back("Endpoint");
	ret.push_back("Length");
	ret.push_back("Details");
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual decoder logic

void USB2PacketDecoder::Refresh()
{
	//Make sure we've got valid inputs
	if(!VerifyAllInputsOK())
	{
		SetData(NULL, 0);
		return;
	}

	//Get the input data
	auto din = dynamic_cast<USB2PCSWaveform*>(GetInputWaveform(0));
	size_t len = din->m_samples.size();

	//Make the capture and copy our time scales from the input
	auto cap = new USB2PacketWaveform;
	cap->m_timescale = din->m_timescale;
	cap->m_startTimestamp = din->m_startTimestamp;
	cap->m_startFemtoseconds = din->m_startFemtoseconds;

	enum
	{
		STATE_IDLE,
		STATE_PID,
		STATE_END,
		STATE_TOKEN_0,
		STATE_TOKEN_1,
		STATE_SOF_0,
		STATE_SOF_1,
		STATE_DATA
	} state = STATE_IDLE;

	//Decode stuff
	uint8_t last = 0;
	uint64_t last_offset;
	uint8_t crc5_in[2] = {0};
	uint8_t packet_crc5;
	vector<uint8_t> packet_data;
	for(size_t i=0; i<len; i++)
	{
		auto& sin = din->m_samples[i];
		int64_t halfdur = din->m_durations[i]/2;

		switch(state)
		{
			case STATE_IDLE:

				//Expect SYNC
				switch(sin.m_type)
				{
					//Start a new packet if we see a SYNC
					case USB2PCSSymbol::TYPE_SYNC:
						state = STATE_PID;
						break;

					//Anything else is an error
					default:
						cap->m_offsets.push_back(din->m_offsets[i]);
						cap->m_durations.push_back(din->m_durations[i]);
						cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_ERROR, 0));
						break;
				}

				break;

			//Started a new packet, expect PID
			case STATE_PID:

				//Should be data
				if(sin.m_type != USB2PCSSymbol::TYPE_DATA)
				{
					cap->m_offsets.push_back(din->m_offsets[i]);
					cap->m_durations.push_back(din->m_durations[i]);
					cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_ERROR, 0));

					state = STATE_IDLE;
					continue;
				}

				//If the low bits don't match the complement of the high bits, we have a bad PID
				if( (sin.m_data >> 4) != (0xf & ~sin.m_data) )
				{
					cap->m_offsets.push_back(din->m_offsets[i]);
					cap->m_durations.push_back(din->m_durations[i]);
					cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_ERROR, 0));

					state = STATE_IDLE;
					continue;
				}

				//All good, add the PID
				cap->m_offsets.push_back(din->m_offsets[i]);
				cap->m_durations.push_back(din->m_durations[i]);
				cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_PID, sin.m_data));

				//Look at the PID and decide what to expect next
				switch(sin.m_data & 0xf)
				{
					case USB2PacketSymbol::PID_ACK:
					case USB2PacketSymbol::PID_STALL:
					case USB2PacketSymbol::PID_NAK:
					case USB2PacketSymbol::PID_NYET:
						state = STATE_END;
						break;

					//TODO: handle low bandwidth PRE stuff
					//for now assume USB 2.0 ERR
					case USB2PacketSymbol::PID_PRE_ERR:
						state = STATE_END;
						break;

					case USB2PacketSymbol::PID_IN:
					case USB2PacketSymbol::PID_OUT:
					case USB2PacketSymbol::PID_SETUP:
					case USB2PacketSymbol::PID_PING:
					case USB2PacketSymbol::PID_SPLIT:
						state = STATE_TOKEN_0;
						break;

					case USB2PacketSymbol::PID_SOF:
						state = STATE_SOF_0;
						break;

					case USB2PacketSymbol::PID_DATA0:
					case USB2PacketSymbol::PID_DATA1:
					case USB2PacketSymbol::PID_DATA2:
					case USB2PacketSymbol::PID_MDATA:
						state = STATE_DATA;
						packet_data.clear();
						break;
				}

				break;

			//Done, expect EOP
			case STATE_END:
				if(sin.m_type != USB2PCSSymbol::TYPE_EOP)
				{
					cap->m_offsets.push_back(din->m_offsets[i]);
					cap->m_durations.push_back(din->m_durations[i]);
					cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_ERROR, 0));
				}
				break;

			//Tokens cross byte boundaries YAY!
			case STATE_TOKEN_0:

				//Pull out the 7-bit address
				cap->m_offsets.push_back(din->m_offsets[i]);
				cap->m_durations.push_back(din->m_durations[i]);
				cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_ADDR, sin.m_data & 0x7f));

				crc5_in[0] = sin.m_data;
				last = sin.m_data;

				state = STATE_TOKEN_1;
				break;

			case STATE_TOKEN_1:

				cap->m_offsets.push_back(din->m_offsets[i]);
				cap->m_durations.push_back(halfdur);
				cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_ENDP,
					( last >> 7) | ( (sin.m_data & 0x7) << 1 )));

				//Verify the CRC
				crc5_in[1] = sin.m_data;
				packet_crc5 = (sin.m_data >> 3);
				cap->m_offsets.push_back(din->m_offsets[i] + halfdur);
				cap->m_durations.push_back(halfdur);
				if(VerifyCRC5(crc5_in))
					cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_CRC5_GOOD, packet_crc5));
				else
					cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_CRC5_BAD, packet_crc5));

				state = STATE_END;
				break;

			case STATE_SOF_0:

				last = sin.m_data;
				last_offset = din->m_offsets[i];
				crc5_in[0] = sin.m_data;

				state = STATE_SOF_1;
				break;

			case STATE_SOF_1:

				//Frame number is the entire previous symbol, plus the low 3 bits of this one
				cap->m_offsets.push_back(last_offset);
				cap->m_durations.push_back(din->m_offsets[i] - last_offset + halfdur);
				cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_NFRAME,
						(sin.m_data & 0x7 ) << 8 | last));

				//CRC
				crc5_in[1] = sin.m_data;
				packet_crc5 = (sin.m_data >> 3);
				cap->m_offsets.push_back(din->m_offsets[i] + halfdur);
				cap->m_durations.push_back(halfdur);
				if(VerifyCRC5(crc5_in))
					cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_CRC5_GOOD, packet_crc5));
				else
					cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_CRC5_BAD, packet_crc5));

				state = STATE_END;
				break;

			case STATE_DATA:

				//Assume data bytes are data (but they might be CRC, can't tell yet)
				if(sin.m_type == USB2PCSSymbol::TYPE_DATA)
				{
					cap->m_offsets.push_back(din->m_offsets[i]);
					cap->m_durations.push_back(din->m_durations[i]);
					cap->m_samples.push_back(USB2PacketSymbol(USB2PacketSymbol::TYPE_DATA, sin.m_data));

					packet_data.push_back(sin.m_data);
				}

				//Last two bytes were actually the CRC!
				//Merge them into the first one and delete the second
				else if(sin.m_type == USB2PCSSymbol::TYPE_EOP)
				{
					size_t firstoff = cap->m_samples.size() - 2;
					size_t secondoff = cap->m_samples.size() - 1;

					//Extract the CRC value and remove it from the packet body
					uint16_t crc16 = (cap->m_samples[firstoff].m_data << 8) | cap->m_samples[secondoff].m_data;
					packet_data.pop_back();
					packet_data.pop_back();

					//Verify the CRC
					uint16_t crc16_calculated = CalculateCRC16(packet_data);
					cap->m_durations[firstoff] += cap->m_durations[secondoff];
					if(crc16 == crc16_calculated)
						cap->m_samples[firstoff] = USB2PacketSymbol(USB2PacketSymbol::TYPE_CRC16_GOOD, crc16);
					else
						cap->m_samples[firstoff] = USB2PacketSymbol(USB2PacketSymbol::TYPE_CRC16_BAD, crc16);

					cap->m_offsets.resize(secondoff);
					cap->m_durations.resize(secondoff);
					cap->m_samples.resize(secondoff);
				}

				break;
		}

		//EOP always returns us to idle state
		if(sin.m_type == USB2PCSSymbol::TYPE_EOP)
			state = STATE_IDLE;
	}

	//Done
	SetData(cap, 0);

	//Decode packets in the capture
	FindPackets(cap);
}

/**
	@brief Calculates the USB CRC16
 */
uint16_t USB2PacketDecoder::CalculateCRC16(const std::vector<uint8_t>& data)
{
	uint16_t poly = 0xa001;
	uint16_t crc = 0xffff;

	for(size_t n=0; n<data.size(); n++)
	{
		uint8_t d = data[n];

		for(int i=0; i<8; i++)
		{
			bool b = ( crc ^ (d >> i) ) & 1;
			crc >>= 1;
			if(b)
				crc ^= poly;
		}
	}

	return ~( (crc << 8) | ( (crc >> 8) & 0xff) );
}

/**
	@brief Table based CRC5 implementation

	Ref: "A Fast Compact CRC5 Checker For Microcontrollers", Michael Joost
 */
bool USB2PacketDecoder::VerifyCRC5(uint8_t* data)
{
	const uint8_t table4[16] =
	{
		0x00, 0x0e, 0x1c, 0x12, 0x11, 0x1f, 0x0d, 0x03,
		0x0b, 0x05, 0x17, 0x19, 0x1a, 0x14, 0x06, 0x08
	};

	const uint8_t table0[16] =
	{
		0x00, 0x16, 0x05, 0x13, 0x0a, 0x1c, 0x0f, 0x19,
		0x14, 0x02, 0x11, 0x07, 0x1e, 0x08, 0x1b, 0x0d
	};

	uint8_t crc = 0x1f;
	for(int i=0; i<2; i++)
	{
		uint8_t b = data[i] ^ crc;
		crc = table4[b & 0xf] ^ table0[(b >> 4) & 0xf];
	}

	return (crc == 6);
}

void USB2PacketDecoder::FindPackets(USB2PacketWaveform* cap)
{
	ClearPackets();

	//Stop when we have no chance of fitting a full packet
	if(cap->m_samples.size() < 2)
		return;

	for(size_t i=0; i<cap->m_samples.size() - 2;)
	{
		//Every packet should start with a PID. Discard unknown garbage.
		size_t istart = i;
		auto& psample = cap->m_samples[i];
		if(psample.m_type != USB2PacketSymbol::TYPE_PID)
		{
			i++;
			continue;
		}
		uint8_t pid = psample.m_data & 0xf;
		i++;

		//See what the PID is
		switch(pid)
		{
			case USB2PacketSymbol::PID_SOF:
				DecodeSof(cap, istart, i);
				break;

			case USB2PacketSymbol::PID_SETUP:
				DecodeSetup(cap, istart, i);
				break;

			case USB2PacketSymbol::PID_IN:
			case USB2PacketSymbol::PID_OUT:
				DecodeData(cap, istart, i);
				break;

			default:
				LogDebug("Unexpected PID %x\n", pid);
		}
	}
}

void USB2PacketDecoder::DecodeSof(USB2PacketWaveform* cap, size_t istart, size_t& i)
{
	//A SOF should contain a TYPE_NFRAME and a TYPE_CRC5
	//Bail out if we only have part of the packet
	if(i+1 >= cap->m_samples.size())
	{
		LogDebug("Truncated SOF\n");
		return;
	}

	//TODO: better display for invalid/malformed packets
	auto snframe = cap->m_samples[i++];
	size_t icrc = i++;
	auto scrc = cap->m_samples[icrc];
	if(snframe.m_type != USB2PacketSymbol::TYPE_NFRAME)
		return;
	if(scrc.m_type != USB2PacketSymbol::TYPE_CRC5_GOOD)
		return;

	//Make the packet
	Packet* pack = new Packet;
	pack->m_offset = cap->m_offsets[istart] * cap->m_timescale;
	pack->m_headers["Type"] = "SOF";
	char tmp[128];
	snprintf(tmp, sizeof(tmp), "Sequence = %u", snframe.m_data);
	pack->m_headers["Details"] = tmp;
	pack->m_len = ((cap->m_offsets[icrc] + cap->m_durations[icrc]) * cap->m_timescale) - pack->m_offset;
	m_packets.push_back(pack);

	pack->m_headers["Device"] = "--";
	pack->m_headers["Endpoint"] = "--";
	pack->m_headers["Length"] = "2";
}

void USB2PacketDecoder::DecodeSetup(USB2PacketWaveform* cap, size_t istart, size_t& i)
{
	//A SETUP packet should contain ADDR, ENDP, CRC5
	//Bail out if we only have part of the packet.
	if(i+2 >= cap->m_samples.size())
	{
		LogDebug("Truncated SETUP\n");
		return;
	}
	auto saddr = cap->m_samples[i++];
	auto sendp = cap->m_samples[i++];
	auto scrc = cap->m_samples[i++];

	//TODO: better display for invalid/malformed packets
	if(saddr.m_type != USB2PacketSymbol::TYPE_ADDR)
	{
		LogError("not TYPE_ADDR\n");
		return;
	}
	if(sendp.m_type != USB2PacketSymbol::TYPE_ENDP)
	{
		LogError("not TYPE_ENDP\n");
		return;
	}
	if(scrc.m_type != USB2PacketSymbol::TYPE_CRC5_GOOD)
		return;

	//Expect a DATA0 packet next
	//Should be PID, 8 bytes, CRC16.
	//Bail out if we only have part of the packet.
	if(i+9 >= cap->m_samples.size())
	{
		LogDebug("Truncated data\n");
		return;
	}
	auto sdatpid = cap->m_samples[i++];
	if(sdatpid.m_type != USB2PacketSymbol::TYPE_PID)
	{
		LogError("Not PID\n");
		return;
	}
	if( (sdatpid.m_data & 0xf) != USB2PacketSymbol::PID_DATA0)
	{
		LogError("not DATA0\n");
		return;
	}
	uint16_t data[8] = {0};
	for(int j=0; j<8; j++)
	{
		auto sdat = cap->m_samples[i++];
		if(sdat.m_type != USB2PacketSymbol::TYPE_DATA)
		{
			LogError("not data\n");
			return;
		}
		data[j] = sdat.m_data;
	}
	size_t idcrc = i++;
	auto sdcrc = cap->m_samples[idcrc];
	if(sdcrc.m_type != USB2PacketSymbol::TYPE_CRC16_GOOD)
		return;

	//Expect ACK/NAK
	string ack = "";
	if(i >= cap->m_samples.size())
	{
		LogDebug("Truncated ACK\n");
		return;
	}
	auto sack = cap->m_samples[i++];
	if(sack.m_type == USB2PacketSymbol::TYPE_PID)
	{
		if( (sack.m_data & 0xf) == USB2PacketSymbol::PID_ACK)
			ack = "ACK";
		else if( (sack.m_data & 0xf) == USB2PacketSymbol::PID_NAK)
			ack = "NAK";
		else
			ack = "Unknown end PID";
	}

	//Make the packet
	Packet* pack = new Packet;
	pack->m_offset = cap->m_offsets[istart] * cap->m_timescale;
	pack->m_headers["Type"] = "SETUP";
	pack->m_displayBackgroundColor = m_backgroundColors[PROTO_COLOR_CONTROL];
	char tmp[256];
	snprintf(tmp, sizeof(tmp), "%d", saddr.m_data);
	pack->m_headers["Device"] = tmp;
	snprintf(tmp, sizeof(tmp), "%d", sendp.m_data);
	pack->m_headers["Endpoint"] = tmp;
	pack->m_headers["Length"] = "8";	//constant

	//Decode setup details
	uint8_t bmRequestType = data[0];
	uint8_t bRequest = data[1];
	uint16_t wValue = (data[3] << 8) | data[2];
	uint16_t wIndex = (data[5] << 8) | data[4];
	uint16_t wLength = (data[7] << 8) | data[6];
	bool out = bmRequestType >> 7;
	uint8_t type = (bmRequestType  >> 5) & 3;
	uint8_t dest = bmRequestType & 0x1f;
	string stype;
	switch(type)
	{
		case 0:
			stype = "Standard";
			break;
		case 1:
			stype = "Class";
			break;
		case 2:
			stype = "Vendor";
			break;
		case 3:
		default:
			stype = "Reserved";
			break;
	}
	string sdest;
	switch(dest)
	{
		case 0:
			sdest = "device";
			break;
		case 1:
			sdest = "interface";
			break;
		case 2:
			sdest = "endpoint";
			break;
		case 3:
		default:
			sdest = "reserved";
			break;
	}
	snprintf(
		tmp,
		sizeof(tmp),
		"%s %s req to %s bRequest=%x wValue=%x wIndex=%x wLength=%u %s",
		out ? "Host:" : "Dev:",
		stype.c_str(),
		sdest.c_str(),
		bRequest,
		wValue,
		wIndex,
		wLength,
		ack.c_str());
	pack->m_headers["Details"] = tmp;

	//Done
	pack->m_len = ((cap->m_offsets[idcrc] + cap->m_durations[idcrc]) * cap->m_timescale) - pack->m_offset;
	m_packets.push_back(pack);
}

void USB2PacketDecoder::DecodeData(USB2PacketWaveform* cap, size_t istart, size_t& i)
{
	//The IN/OUT packet should contain ADDR, ENDP, CRC5
	//Bail out if we only have part of the packet.
	if(i+2 >= cap->m_samples.size())
		return;
	size_t iaddr = i++;
	auto saddr = cap->m_samples[iaddr];
	size_t iendp = i++;
	auto sendp = cap->m_samples[iendp];
	size_t icrc = i++;
	auto scrc = cap->m_samples[icrc];

	//TODO: better display for invalid/malformed packets
	if(saddr.m_type != USB2PacketSymbol::TYPE_ADDR)
	{
		LogError("not TYPE_ADDR\n");
		return;
	}
	if(sendp.m_type != USB2PacketSymbol::TYPE_ENDP)
	{
		LogError("not TYPE_ENDP\n");
		return;
	}
	if(scrc.m_type != USB2PacketSymbol::TYPE_CRC5_GOOD)
	{
		LogDebug("bad CRC\n");
		return;
	}

	//Expect minimum DATA, 0 or more data bytes, ACK
	if(i >= cap->m_samples.size())
	{
		LogDebug("Truncated DATA\n");
		return;
	}

	char tmp[256];

	//Look for the DATA packet after the IN/OUT
	auto sdatpid = cap->m_samples[i];
	if(sdatpid.m_type != USB2PacketSymbol::TYPE_PID)
	{
		LogError("Not PID\n");
		return;
	}
	//We can get a SOF thrown in anywhere, handle that first
	if( (sdatpid.m_data & 0xf) == USB2PacketSymbol::PID_SOF)
	{
		LogDebug("Random SOF in data stream (i=%zu)\n", i);
		DecodeSof(cap, i, i);
		sdatpid = cap->m_samples[i];
	}
	else if( (sdatpid.m_data & 0xf) == USB2PacketSymbol::PID_NAK)
	{
		i++;

		//Add a line for the aborted transaction
		Packet* pack = new Packet;
		pack->m_offset = cap->m_offsets[istart] * cap->m_timescale;
		if( (cap->m_samples[istart].m_data & 0xf) == USB2PacketSymbol::PID_IN)
		{
			pack->m_headers["Type"] = "IN";
			pack->m_displayBackgroundColor = m_backgroundColors[PROTO_COLOR_DATA_READ];
		}
		else
		{
			pack->m_headers["Type"] = "OUT";
			pack->m_displayBackgroundColor = m_backgroundColors[PROTO_COLOR_DATA_WRITE];
		}
		snprintf(tmp, sizeof(tmp), "%d", saddr.m_data);
		pack->m_headers["Device"] = tmp;
		snprintf(tmp, sizeof(tmp), "%d", sendp.m_data);
		pack->m_headers["Endpoint"] = tmp;
		pack->m_headers["Details"] = "NAK";
		m_packets.push_back(pack);

		pack->m_len = ((cap->m_offsets[i] + cap->m_durations[i]) * cap->m_timescale) - pack->m_offset;

		return;
	}
	else	//normal data
		i++;
	if( ( (sdatpid.m_data & 0xf) != USB2PacketSymbol::PID_DATA0) &&
		( (sdatpid.m_data & 0xf) != USB2PacketSymbol::PID_DATA1) )
	{
		LogError("Not data PID (%x, i=%zu)\n", sdatpid.m_data, i);

		//DEBUG
		Packet* pack = new Packet;
		pack->m_offset = cap->m_offsets[istart] * cap->m_timescale;
		pack->m_headers["Details"] = "ERROR";
		pack->m_displayBackgroundColor = m_backgroundColors[PROTO_COLOR_ERROR];
		m_packets.push_back(pack);
		return;
	}

	//Create the new packet
	Packet* pack = new Packet;
	pack->m_offset = cap->m_offsets[istart] * cap->m_timescale;
	if( (cap->m_samples[istart].m_data & 0xf) == USB2PacketSymbol::PID_IN)
	{
		pack->m_headers["Type"] = "IN";
		pack->m_displayBackgroundColor = m_backgroundColors[PROTO_COLOR_DATA_READ];
	}
	else
	{
		pack->m_headers["Type"] = "OUT";
		pack->m_displayBackgroundColor = m_backgroundColors[PROTO_COLOR_DATA_WRITE];
	}
	snprintf(tmp, sizeof(tmp), "%d", saddr.m_data);
	pack->m_headers["Device"] = tmp;
	snprintf(tmp, sizeof(tmp), "%d", sendp.m_data);
	pack->m_headers["Endpoint"] = tmp;

	//Read the data
	while(i < cap->m_samples.size())
	{
		auto s = cap->m_samples[i];

		//Keep adding data
		if(s.m_type == USB2PacketSymbol::TYPE_DATA)
			pack->m_data.push_back(s.m_data);

		//Next should be a CRC16
		else if(s.m_type == USB2PacketSymbol::TYPE_CRC16_GOOD)
		{
			i++;
			break;
		}
		else if(s.m_type == USB2PacketSymbol::TYPE_CRC16_BAD)
		{
			i++;
			pack->m_displayBackgroundColor = m_backgroundColors[PROTO_COLOR_ERROR];
			break;
		}

		i++;
	}

	//Expect ACK/NAK
	if(i >= cap->m_samples.size())
	{
		LogDebug("Truncated ACK\n");
		return;
	}
	string ack = "";
	auto sack = cap->m_samples[i];
	if(sack.m_type == USB2PacketSymbol::TYPE_PID)
	{
		if( (sack.m_data & 0xf) == USB2PacketSymbol::PID_ACK)
			ack = "";
		else if( (sack.m_data & 0xf) == USB2PacketSymbol::PID_NAK)
			ack = "NAK";
		else
			ack = "Unknown end PID";
	}

	//TODO: handle errors better
	else
	{
		LogDebug("DecodeData got type %x instead of ACK/NAK\n", sack.m_type);
		ack = "Not a PID";
	}

	pack->m_len = ((cap->m_offsets[i] + cap->m_durations[i]) * cap->m_timescale) - pack->m_offset;
	i++;

	//Format the data
	string details = "";
	for(auto b : pack->m_data)
	{
		snprintf(tmp, sizeof(tmp), "%02x ", b);
		details += tmp;
	}
	details += ack;
	pack->m_headers["Details"] = details;

	snprintf(tmp, sizeof(tmp), "%zu", pack->m_data.size());
	pack->m_headers["Length"] = tmp;

	m_packets.push_back(pack);
}

Gdk::Color USB2PacketDecoder::GetColor(int i)
{
	auto data = dynamic_cast<USB2PacketWaveform*>(GetData(0));
	if(data == NULL)
		return m_standardColors[COLOR_ERROR];
	if(i >= (int)data->m_samples.size())
		return m_standardColors[COLOR_ERROR];

	auto sample = data->m_samples[i];
	switch(sample.m_type)
	{
		case USB2PacketSymbol::TYPE_PID:
			if( (sample.m_data == USB2PacketSymbol::PID_RESERVED) ||
				(sample.m_data == USB2PacketSymbol::PID_STALL) )
				return m_standardColors[COLOR_ERROR];
			else
				return m_standardColors[COLOR_PREAMBLE];

		case USB2PacketSymbol::TYPE_ADDR:
			return m_standardColors[COLOR_ADDRESS];

		case USB2PacketSymbol::TYPE_ENDP:
			return m_standardColors[COLOR_ADDRESS];

		case USB2PacketSymbol::TYPE_NFRAME:
			return m_standardColors[COLOR_DATA];

		case USB2PacketSymbol::TYPE_CRC5_GOOD:
		case USB2PacketSymbol::TYPE_CRC16_GOOD:
			return m_standardColors[COLOR_CHECKSUM_OK];

		case USB2PacketSymbol::TYPE_CRC5_BAD:
		case USB2PacketSymbol::TYPE_CRC16_BAD:
			return m_standardColors[COLOR_CHECKSUM_BAD];

		case USB2PacketSymbol::TYPE_DATA:
			return m_standardColors[COLOR_DATA];

		//invalid state, should never happen
		case USB2PacketSymbol::TYPE_ERROR:
		default:
			return m_standardColors[COLOR_ERROR];
	}
}

string USB2PacketDecoder::GetText(int i)
{
	auto data = dynamic_cast<USB2PacketWaveform*>(GetData(0));
	if(data == NULL)
		return "";
	if(i >= (int)data->m_samples.size())
		return "";

	char tmp[32];

	auto sample = data->m_samples[i];
	switch(sample.m_type)
	{
		case USB2PacketSymbol::TYPE_PID:
		{
			switch(sample.m_data & 0x0f)
			{
				case USB2PacketSymbol::PID_RESERVED:
					return "RESERVED";
				case USB2PacketSymbol::PID_OUT:
					return "OUT";
				case USB2PacketSymbol::PID_ACK:
					return "ACK";
				case USB2PacketSymbol::PID_DATA0:
					return "DATA0";
				case USB2PacketSymbol::PID_PING:
					return "PING";
				case USB2PacketSymbol::PID_SOF:
					return "SOF";
				case USB2PacketSymbol::PID_NYET:
					return "NYET";
				case USB2PacketSymbol::PID_DATA2:
					return "DATA2";
				case USB2PacketSymbol::PID_SPLIT:
					return "SPLIT";
				case USB2PacketSymbol::PID_IN:
					return "IN";
				case USB2PacketSymbol::PID_NAK:
					return "NAK";
				case USB2PacketSymbol::PID_DATA1:
					return "DATA1";
				case USB2PacketSymbol::PID_PRE_ERR:
					return "PRE/ERR";
				case USB2PacketSymbol::PID_SETUP:
					return "SETUP";
				case USB2PacketSymbol::PID_STALL:
					return "STALL";
				case USB2PacketSymbol::PID_MDATA:
					return "MDATA";

				default:
					return "INVALID PID";
			}
			break;
		}
		case USB2PacketSymbol::TYPE_ADDR:
			return string("Dev ") + to_string(sample.m_data);
		case USB2PacketSymbol::TYPE_NFRAME:
			return string("Frame ") + to_string(sample.m_data);
		case USB2PacketSymbol::TYPE_ENDP:
			return string("EP ") + to_string(sample.m_data);

		case USB2PacketSymbol::TYPE_CRC5_GOOD:
		case USB2PacketSymbol::TYPE_CRC5_BAD:
			snprintf(tmp, sizeof(tmp), "CRC %02x", sample.m_data);
			return string(tmp);

		case USB2PacketSymbol::TYPE_CRC16_GOOD:
		case USB2PacketSymbol::TYPE_CRC16_BAD:
			snprintf(tmp, sizeof(tmp), "CRC %04x", sample.m_data);
			return string(tmp);

		case USB2PacketSymbol::TYPE_DATA:
			snprintf(tmp, sizeof(tmp), "%02x", sample.m_data);
			return string(tmp);
		case USB2PacketSymbol::TYPE_ERROR:
		default:
			return "ERROR";
	}

	return "";
}
