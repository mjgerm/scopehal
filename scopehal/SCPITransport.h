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
	@brief Declaration of SCPITransport
 */

#ifndef SCPITransport_h
#define SCPITransport_h

/**
	@brief Abstraction of a transport layer for moving SCPI data between endpoints
 */
class SCPITransport
{
public:
	SCPITransport();
	virtual ~SCPITransport();

	virtual std::string GetConnectionString() =0;
	virtual std::string GetName() =0;

	//Queued command API
	void SendCommandQueued(const std::string& cmd);
	std::string SendCommandQueuedWithReply(std::string cmd, bool endOnSemicolon = true);
	void SendCommandImmediate(std::string cmd);
	std::string SendCommandImmediateWithReply(std::string cmd, bool endOnSemicolon = true);
	void* SendCommandImmediateWithRawBlockReply(std::string cmd, size_t& len);
	bool FlushCommandQueue();

	//Manual mutex locking for ReadRawData() etc
	std::recursive_mutex& GetMutex()
	{ return m_netMutex; }

	//Immediate command API
	virtual void FlushRXBuffer(void);
	virtual bool SendCommand(const std::string& cmd) =0;
	virtual std::string ReadReply(bool endOnSemicolon = true) =0;
	virtual size_t ReadRawData(size_t len, unsigned char* buf) =0;
	virtual void SendRawData(size_t len, const unsigned char* buf) =0;

	virtual bool IsCommandBatchingSupported() =0;
	virtual bool IsConnected() =0;

public:
	typedef SCPITransport* (*CreateProcType)(const std::string& args);
	static void DoAddTransportClass(std::string name, CreateProcType proc);

	static void EnumTransports(std::vector<std::string>& names);
	static SCPITransport* CreateTransport(const std::string& transport, const std::string& args);

protected:

	//Class enumeration
	typedef std::map< std::string, CreateProcType > CreateMapType;
	static CreateMapType m_createprocs;

	//Queued commands waiting to be sent
	std::mutex m_queueMutex;
	std::recursive_mutex m_netMutex;
	std::list<std::string> m_txQueue;
};

#define TRANSPORT_INITPROC(T) \
	static SCPITransport* CreateInstance(const std::string& args) \
	{ \
		return new T(args); \
	} \
	virtual std::string GetName() \
	{ return GetTransportName(); }

#define AddTransportClass(T) SCPITransport::DoAddTransportClass(T::GetTransportName(), T::CreateInstance)

#endif
