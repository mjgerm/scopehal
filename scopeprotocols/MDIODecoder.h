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
	@brief Declaration of MDIODecoder
 */

#ifndef MDIODecoder_h
#define MDIODecoder_h

#include "../scopehal/PacketDecoder.h"

class MDIOSymbol
{
public:
	enum stype
	{
		TYPE_PREAMBLE,
		TYPE_START,
		TYPE_OP,
		TYPE_PHYADDR,
		TYPE_REGADDR,
		TYPE_TURN,
		TYPE_DATA,
		TYPE_ERROR,
	};

	MDIOSymbol()
	{}

	MDIOSymbol(stype t, uint16_t d = 0)
	 : m_stype(t)
	 , m_data(d)
	{}

	stype m_stype;
	uint16_t m_data;

	bool operator== (const MDIOSymbol& s) const
	{
		return (m_stype == s.m_stype) && (m_data == s.m_data);
	}
};

typedef Waveform<MDIOSymbol> MDIOWaveform;

class MDIODecoder : public PacketDecoder
{
public:
	MDIODecoder(const std::string& color);

	virtual std::string GetText(int i);
	virtual Gdk::Color GetColor(int i);

	virtual void Refresh();
	virtual bool NeedsConfig();

	static std::string GetProtocolName();
	virtual void SetDefaultName();

	enum PhyTypes
	{
		PHY_TYPE_GENERIC,	//IEEE registers only
		PHY_TYPE_KSZ9031
	};

	virtual std::vector<std::string> GetHeaders();

	virtual bool CanMerge(Packet* first, Packet* cur, Packet* next);
	virtual Packet* CreateMergedHeader(Packet* pack, size_t i);

	virtual bool ValidateChannel(size_t i, StreamDescriptor stream);

	PROTOCOL_DECODER_INITPROC(MDIODecoder)

protected:
	std::string m_typename;
};

#endif
