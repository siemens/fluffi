/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

§§Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "FluffiLMConfiguration.h"

FluffiLMConfiguration::FluffiLMConfiguration(const std::string dbHost, const std::string dbUser, const std::string dbPassword, const std::string dbName, const std::string fuzzJobName)
	: m_dbHost(dbHost),
	m_dbUser(dbUser),
	m_dbPassword(dbPassword),
	m_dbName(dbName),
	m_fuzzJobName(fuzzJobName)
{}

FluffiLMConfiguration::FluffiLMConfiguration(const LMConfiguration& lmConfiguration)
{
	m_dbHost = lmConfiguration.dbhost();
	m_dbUser = lmConfiguration.dbuser();
	m_dbPassword = lmConfiguration.dbpassword();
	m_dbName = lmConfiguration.dbname();
	m_fuzzJobName = lmConfiguration.fuzzjobname();
}

FluffiLMConfiguration::~FluffiLMConfiguration()
{}

LMConfiguration FluffiLMConfiguration::getProtobuf() const
{
	LMConfiguration lmConfiguration{};

	lmConfiguration.set_dbhost(m_dbHost);
	lmConfiguration.set_dbuser(m_dbUser);
	lmConfiguration.set_dbpassword(m_dbPassword);
	lmConfiguration.set_dbname(m_dbName);
	lmConfiguration.set_fuzzjobname(m_fuzzJobName);

	return lmConfiguration;
}
