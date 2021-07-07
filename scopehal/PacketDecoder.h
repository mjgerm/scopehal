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

#ifndef PacketDecoder_h
#define PacketDecoder_h

#include "Filter.h"

/**
	@class
	@brief Generic display representation for arbitrary packetized data
 */
class Packet
{
public:
	Packet();
	virtual ~Packet();

	///Offset of the packet from the start of the capture (femtoseconds)
	int64_t m_offset;

	///Duration time of the packet (femtoseconds)
	int64_t m_len;

	//Arbitrary header properties (human readable)
	std::map<std::string, std::string> m_headers;

	//Packet bytes
	std::vector<uint8_t> m_data;

	//Text color of the packet
	Gdk::Color m_displayForegroundColor;

	//Background color of the packet
	Gdk::Color m_displayBackgroundColor;
};

/**
	@class
	@brief A protocol decoder that outputs packetized data
 */
class PacketDecoder : public Filter
{
public:
	PacketDecoder(OscilloscopeChannel::ChannelType type, const std::string& color, Filter::Category cat);
	virtual ~PacketDecoder();

	const std::vector<Packet*>& GetPackets()
	{ return m_packets; }

	virtual std::vector<std::string> GetHeaders() =0;

	virtual bool GetShowDataColumn();
	virtual bool GetShowImageColumn();

	virtual Packet* CreateMergedHeader(Packet* pack, size_t i);
	virtual bool CanMerge(Packet* first, Packet* cur, Packet* next);

	/**
		@brief Standard colors for protocol analyzer lines.

		Do not change ordering, add new items to the end only.
	 */
	enum PacketColor
	{
		PROTO_COLOR_DEFAULT,		//Default color if not otherwise specified
		PROTO_COLOR_ERROR,			//Malformed packets, or packets indicating an error condition
		PROTO_COLOR_STATUS,			//Reading or writing status registers
		PROTO_COLOR_CONTROL,		//Reading or writing control registers
		PROTO_COLOR_DATA_READ,		//Reading unspecified data
		PROTO_COLOR_DATA_WRITE,		//Writing unspecified data
		PROTO_COLOR_COMMAND,		//Executing commands of some sort

		PROTO_STANDARD_COLOR_COUNT
	};

	static Gdk::Color m_backgroundColors[PROTO_STANDARD_COLOR_COUNT];

protected:
	void ClearPackets();

	std::vector<Packet*> m_packets;
};

#endif
