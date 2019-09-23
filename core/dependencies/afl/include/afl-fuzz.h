/*
   Copyright 2017-2019 Siemens AG

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 
 Author(s): Thomas Riedmaier, Pascal Eckmann
*/

#ifndef AFL_FLUFFI_INCLUDE
#define AFL_FLUFFI_INCLUDE

uint32_t SWAP32(uint32_t _x);
uint16_t SWAP16(uint16_t _x);
uint32_t UR(uint32_t limit);
std::vector<char> mutateWithAFLHavoc(const std::vector<char> input);

#endif /* AFL_FLUFFI_INCLUDE */
