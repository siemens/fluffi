/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

§§Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "BlockCoverageCache.h"
#include "FluffiBasicBlock.h"
#include "Util.h"

BlockCoverageCache::BlockCoverageCache()
{
}

BlockCoverageCache::~BlockCoverageCache()
{
}

void BlockCoverageCache::addBlockToCache(const FluffiBasicBlock newBlock) {
	std::unique_lock<std::mutex> mlock(m_mutex_);

	m_blockCoverageCache.insert(newBlock);
}

§§void  BlockCoverageCache::addBlocksToCache(const std::set<FluffiBasicBlock>* newBlocks) {
	std::unique_lock<std::mutex> mlock(m_mutex_);

§§	for (auto& newBlock : *newBlocks) {
§§		m_blockCoverageCache.insert(newBlock);
	}
}

bool BlockCoverageCache::isBlockInCache(const FluffiBasicBlock theBlock) {
	return m_blockCoverageCache.count(theBlock) > 0;
}

bool BlockCoverageCache::isBlockInCacheAndAddItIfNot(const  FluffiBasicBlock theBlock) {
	bool wasAlreadyInSet = (m_blockCoverageCache.count(theBlock) > 0);
	if (!wasAlreadyInSet) {
		std::unique_lock<std::mutex> mlock(m_mutex_);
		m_blockCoverageCache.insert(theBlock);
	}
	return wasAlreadyInSet;
}

size_t BlockCoverageCache::getSizeOfCache() const {
	return m_blockCoverageCache.size();
}
