§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Michael Kraus, Thomas Riedmaier, Pascal Eckmann
§§*/
§§
§§#include "stdafx.h"
§§#include "CppUnitTest.h"
§§#include "Util.h"
§§#include "CommPartnerManager.h"
§§
§§using namespace Microsoft::VisualStudio::CppUnitTestFramework;
§§
§§namespace FluffiTester
§§{
§§	TEST_CLASS(CommPartnerManagerTest)
§§	{
§§	public:
§§
§§		CommPartnerManager* cPManager = nullptr;
§§
§§		TEST_METHOD_INITIALIZE(ModuleInitialize)
§§		{
§§			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
§§			cPManager = new CommPartnerManager();
§§		}
§§
§§		TEST_METHOD_CLEANUP(ModuleCleanup)
§§		{
§§			//Freeing the testcase Queue
§§			delete cPManager;
§§			cPManager = nullptr;
§§		}
§§
§§		TEST_METHOD(CommPartnerManager_getNextPartner)
§§		{
§§			// Proove GetPartner from empty CommPartnerManager
§§			// Test Method
§§			std::string generatorTargetIP = cPManager->getNextPartner();
§§			Assert::AreEqual(generatorTargetIP, std::string(""), L"Error calculating number of inserted CommPartners into CommPartnerManager (getNextPartner)");
§§
§§			SetTGsAndTEsRequest* setTGsAndTEsReqest = new SetTGsAndTEsRequest();
§§
§§			// Configure:
§§			const int numberOfPartner = 10;
§§			uint32_t weightOfEachPartner = 10;
§§			int numberOfPartnerCalculations = 10000;
§§			int resultedPartnerRatio[numberOfPartner] = {};
§§
§§			for (int i = 0; i < numberOfPartner; i++) {
§§				resultedPartnerRatio[i] = 0;
§§			}
§§
§§			// Update with a number of CommPartners
§§			for (int i = 0; i < numberOfPartner; i++) {
§§				FluffiServiceDescriptor serviceDescriptor{ "testHostAndPort" + std::to_string(i), "testGUID" + std::to_string(i) };
§§				FluffiServiceAndWeight serviceAndWeight{ serviceDescriptor, weightOfEachPartner };
§§
§§				ServiceAndWeigth* serviceAndWeightOfTE = setTGsAndTEsReqest->add_tgs();
§§				serviceAndWeightOfTE->CopyFrom(serviceAndWeight.getProtobuf());
§§			}
§§
§§			google::protobuf::RepeatedPtrField<ServiceAndWeigth> tgs = setTGsAndTEsReqest->tgs();
§§			Assert::AreEqual(cPManager->updateCommPartners(&tgs), numberOfPartner, L"Error calculating number of inserted CommPartners into CommPartnerManager (updateCommPartners)");
§§
§§			// Run with a big number of getNextPartner calls
§§			for (int u = 0; u < numberOfPartnerCalculations; u++) {
§§				// Test Method
§§				std::string generatorTargetIP = cPManager->getNextPartner();
§§				std::string descriptorPartA = generatorTargetIP.substr(0, 15);
§§				Assert::AreEqual(descriptorPartA, std::string("testHostAndPort"), L"Error checking CommPartner -> corrupted or wrong Partner (updateCommPartners)");
§§
§§				int specificPartner = atoi((generatorTargetIP.substr(15)).c_str());
§§				Assert::IsTrue(specificPartner < numberOfPartner, L"Error calculating the number of the CommPartner (updateCommPartners)");
§§
§§				resultedPartnerRatio[specificPartner]++;
§§			}
§§
§§			// Proove that every CommPartner is used in correct ratio!
§§			for (int i = 0; i < numberOfPartner; i++) {
§§				Assert::IsTrue(resultedPartnerRatio[i] > (0.09 * numberOfPartnerCalculations), L"Error in resulted CommPartner Ratio!, Proove RNG, weights and logic! (updateCommPartners)");
§§			}
§§		}
§§
§§		TEST_METHOD(CommPartnerManager_updateCommPartners)
§§		{
§§			// Update with empy Partner list
§§			for (int i = 0; i < 5; i++) {
§§				google::protobuf::RepeatedPtrField<ServiceAndWeigth> tes = google::protobuf::RepeatedPtrField<ServiceAndWeigth>();
§§				// Test Method
§§				Assert::AreEqual(cPManager->updateCommPartners(&tes), 0, L"Error updating CommPartners in CommPartnerManager with empty list (updateCommPartners)");
§§			}
§§
§§			const int numberOfPartners = 10;
§§			bool commPartnerArrived[numberOfPartners] = { false };
§§
§§			for (int i = 0; i < 10; i++) {
§§				commPartnerArrived[i] = false;
§§			}
§§
§§			// Update with a number of CommPartners
§§			SetTGsAndTEsRequest* setTGsAndTEsReqest = new SetTGsAndTEsRequest();
§§			for (int i = 0; i < numberOfPartners; i++) {
§§				FluffiServiceDescriptor serviceDescriptor{ "testHostAndPort" + std::to_string(i), "testGUID" + std::to_string(i) };
§§				FluffiServiceAndWeight serviceAndWeight{ serviceDescriptor, 10 };
§§
§§				ServiceAndWeigth* serviceAndWeightOfTE = setTGsAndTEsReqest->add_tgs();
§§				serviceAndWeightOfTE->CopyFrom(serviceAndWeight.getProtobuf());
§§			}
§§			google::protobuf::RepeatedPtrField<ServiceAndWeigth> tgs = setTGsAndTEsReqest->tgs();
§§			// Test Method
§§			Assert::AreEqual(cPManager->updateCommPartners(&tgs), 10, L"Error calculating number of inserted CommPartners into CommPartnerManager (updateCommPartners)");
§§
§§			// Run with a big number of getNextPartner calls
§§			for (int u = 0; u < 100; u++) {
§§				std::string generatorTargetIP = cPManager->getNextPartner();
§§				std::string descriptorPartA = generatorTargetIP.substr(0, 15);
§§				Assert::AreEqual(descriptorPartA, std::string("testHostAndPort"), L"Error checking data of CommPartner -> corrupted or wrong Partner (updateCommPartners)");
§§
§§				int specificPartner = atoi((generatorTargetIP.substr(15)).c_str());
§§				Assert::IsTrue(specificPartner < numberOfPartners, L"Error corrupt number of the next CommPartner (updateCommPartners)");
§§
§§				commPartnerArrived[specificPartner] = true;
§§			}
§§
§§			// Check if every CommPartner was reached in getNextPartner Method
§§			for (int i = 0; i < numberOfPartners; i++) {
§§				Assert::IsTrue(commPartnerArrived[i], L"Error not all CommPartners got inserted OR not all reached with getNextPartner() (updateCommPartners)");
§§			}
§§
§§			// Another update with empty CommPartner list
§§			google::protobuf::RepeatedPtrField<ServiceAndWeigth> tes2 = google::protobuf::RepeatedPtrField<ServiceAndWeigth>();
§§			// Test Method
§§			Assert::AreEqual(cPManager->updateCommPartners(&tes2), 0, L"Error updating CommPartners in CommPartnerManager with empty list (updateCommPartners)");
§§
§§			// Proove GetPartner from empty CommPartnerManager
§§			for (int i = 0; i < 5; i++) {
§§				std::string generatorTargetIP = cPManager->getNextPartner();
§§				Assert::AreEqual(generatorTargetIP, std::string(""), L"Error getting corrupt data from getNextPartner, list should be empty -> empty String should be returned (updateCommPartners)");
§§			}
§§		}
§§	};
§§}
