//  
// Copyright (C) 2007-2013 SIPez LLC.  All rights reserved.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// Author: Keith Kyzivat <kkyzivat AT SIPez DOT com>

#include <os/OsIntTypes.h>
#include <sipxunittests.h>
#include <mp/MpBufPool.h>
#include <mp/MpArrayBuf.h>
#include <mp/MpAudioBuf.h>
#include <mp/MpInputDeviceManager.h>
#ifdef WIN32
#include <mp/MpidWinMM.h>
#elif defined ANDROID
#   include <mp/MpidAndroid.h>
#   include <mp/MpAndroidAudioTrack.h>
#elif defined __linux__
#include <mp/MpidOss.h>
#elif defined __APPLE__
#include <mp/MpidCoreAudio.h>
#else
#include <mp/MpSineWaveGeneratorDeviceDriver.h>
#endif
#include <os/OsTask.h>
#include <utl/UtlString.h>

#define MIDDT_SAMPLES_PER_FRAME 80
#define MIDDT_NBUFS 20

class MpInputDeviceDriverTest : public SIPX_UNIT_BASE_CLASS
{
   CPPUNIT_TEST_SUITE(MpInputDeviceDriverTest);
   CPPUNIT_TEST(testSetup);
   CPPUNIT_TEST_SUITE_END();

private:
   MpBufPool* mpBufPool;
   MpBufPool* mpHeadersPool;

   int mNumBufferedFrames;
   unsigned int mSamplesPerSecond;
   unsigned int mFramePeriodMSecs;

public:
   void setUp()
   {
#ifdef ANDROID
      OsStatus stat = MpAndroidAudioBindingInterface::spGetAndroidAudioBinding()->getStatus();
#endif

      mpBufPool = 
         new MpBufPool(MIDDT_SAMPLES_PER_FRAME * sizeof(MpAudioSample)
                       + MpArrayBuf::getHeaderSize(), 
                       MIDDT_NBUFS, "MpInputDeviceDriverTest");
      CPPUNIT_ASSERT(mpBufPool != NULL);

      // Create pool for buffer headers
      mpHeadersPool = new MpBufPool(sizeof(MpAudioBuf), MIDDT_NBUFS, "MpInputDeviceDriverTestHeaders");
      CPPUNIT_ASSERT(mpHeadersPool != NULL);

      // Set mpHeadersPool as default pool for audio and data pools.
      MpAudioBuf::smpDefaultPool = mpHeadersPool;
      MpDataBuf::smpDefaultPool = mpHeadersPool;

      mNumBufferedFrames = 5;
      mSamplesPerSecond = 8000;
      mFramePeriodMSecs = MIDDT_SAMPLES_PER_FRAME * 1000 / mSamplesPerSecond;
   }



   void testSetup()
   {
      MpInputDeviceManager inDevMgr(MIDDT_SAMPLES_PER_FRAME, 
                                    mSamplesPerSecond,
                                    mNumBufferedFrames, 
                                    *mpBufPool);

      // Buffer for recorded data.
      MpAudioSample* pRecordBuffer = new MpAudioSample[mNumBufferedFrames* MIDDT_SAMPLES_PER_FRAME];
      int pRecordBufferPointer = 0;


      MpInputDeviceDriver* pInDevDriver = 
#ifdef WIN32
         new MpidWinMM(MpidWinMM::getDefaultDeviceName(), inDevMgr);
#elif defined ANDROID
         new MpidAndroid(MpidAndroid::AUDIO_SOURCE_DEFAULT, inDevMgr);
#elif defined __linux__
         new MpidOss("/dev/dsp", inDevMgr);
#elif defined __APPLE__
         new MpidCoreAudio("[default]", inDevMgr);
#else
         new MpSineWaveGeneratorDeviceDriver("SineWaveDriver", inDevMgr,
                                             3000, 3000, 0);
#endif
      if (pInDevDriver != NULL)
      {
         // Verify that our device is indeed valid and, if not using the test
         // driver, is indeed pointing at an actual device in the OS.
         CPPUNIT_ASSERT(pInDevDriver->isDeviceValid());

         // Since we've only just created this device, it shouldn't be enabled.
         CPPUNIT_ASSERT(!pInDevDriver->isEnabled());
         // And shouldn't have a valid device handle/ID.
         CPPUNIT_ASSERT(pInDevDriver->getDeviceId() < 0);

         // Try to enable the device when it isn't added to a manager..
         // SHOULDN'T DO THIS - Only the manager should be able to do this..
         // perhaps enabling should be protected, and manager be friended?
         //CPPUNIT_ASSERT(iDrv->enableDevice(10,10,10) != OS_SUCCESS);

         // Add the device to an input manager.
         MpInputDeviceHandle iDrvHnd = inDevMgr.addDevice(*pInDevDriver);

         // Verify it has a valid handle/ID.
         CPPUNIT_ASSERT(iDrvHnd > 0);

         // Try to disable it -- this should fail, since it isn't enabled yet.
         // Also note that one should be disabling/enabling via the manager..
         // I'm just verifying that disabling the device itself when it isn't
         // set up doesn't kill things.
         CPPUNIT_ASSERT(pInDevDriver->disableDevice() != OS_SUCCESS);

         // Now enable it via the manager -- this should succeed.
         CPPUNIT_ASSERT(inDevMgr.enableDevice(iDrvHnd) == OS_SUCCESS);

         int nMSPerBuffer = mNumBufferedFrames * mFramePeriodMSecs;
         unsigned nMSecsToRecord = 5000;
         double* derivs = new double[(mNumBufferedFrames-1)*(nMSecsToRecord/nMSPerBuffer)];
         // Round nMSecsToRecord to nMSPerBuffer boundary.
         nMSecsToRecord = (nMSecsToRecord/nMSPerBuffer) * nMSPerBuffer;

         UtlString derivPlotStr;
         derivPlotStr.capacity((nMSecsToRecord/mFramePeriodMSecs) << 2);
         UtlString derivWAvgStr;

         unsigned i;
         for(i=0;i<(mNumBufferedFrames-1)*(nMSecsToRecord/nMSPerBuffer);i++)
            derivs[i] = -1;

         unsigned derivBufPos;
         unsigned derivBufSz = 0;
         unsigned nDerivsPerBuf = mNumBufferedFrames-1;
         for(i = 0, derivBufPos = 0;
             i < nMSecsToRecord; 
             i = i+nMSPerBuffer, derivBufPos += nDerivsPerBuf)
         {
            // Reset nDerivsPerBuf, as getting time derivs could have changed it.
            nDerivsPerBuf = mNumBufferedFrames-1;
            
            // Sleep till when the input buffer should be full
            OsTask::delay(nMSPerBuffer);

            // Grab time derivative statistics..
            double* curDerivFramePtr = (double*)(derivs + derivBufPos);
            CPPUNIT_ASSERT_EQUAL(OS_SUCCESS,
                                 inDevMgr.getTimeDerivatives(iDrvHnd, 
                                                             nDerivsPerBuf, 
                                                             curDerivFramePtr));
            derivBufSz += nDerivsPerBuf;
         }

         // Ok, now disable it via the manager -- this time it should succeed.
         CPPUNIT_ASSERT(inDevMgr.disableDevice(iDrvHnd) == OS_SUCCESS);

         // Define weighted average accumulator and period.
         double derivWeightedAverage = 0;
         int derivWAvgPeriod = 5;

         // Now that we have all the derivatives, 
         // make a string out of em..
         for(i = 0; i < derivBufSz; i++)
         {
            // Prepare the derivative line to print.
#           define NUMSTRSZ 32
            char tmpBuf[NUMSTRSZ];

            // Add derivative to our big-long string that can be used for plotting.
            snprintf(tmpBuf, NUMSTRSZ, "%.2f", derivs[i]);
            derivPlotStr.append(tmpBuf);
            if(i < derivBufSz-1) // While there's still one more, put a comma
               derivPlotStr.append(", ");

            if ((i != 0) && (i % derivWAvgPeriod) == 0)
            {
               // Now that we have derivWAvgPeriod samples,
               // calculate and assign the actual weighted average.
               derivWeightedAverage = derivWeightedAverage / derivWAvgPeriod;

               // Now append this to our weighted average string.
               snprintf(tmpBuf, NUMSTRSZ, "%.2f", derivWeightedAverage);
               derivWAvgStr.append(tmpBuf);
               derivWAvgStr.append(", ");

               // reset the weighted average collector.
               derivWeightedAverage = 0;
            }

            derivWeightedAverage += derivs[i];

            CPPUNIT_ASSERT(derivs[i] <= 4);
         }

         // Remove the device from the manager explicitly, 
         // Otherwise the manager will assert fail if there are devices
         // still present when the manager is destroyed
         inDevMgr.removeDevice(iDrvHnd);

         // Now print out our derivative results.
         printf(" derivatives: %s\n", derivPlotStr.data());
         printf("weighted avg: %s\n", derivWAvgStr.data());
      }  // if pInDevDriver != NULL
   }

   void tearDown()
   {
      if (mpBufPool != NULL)
      {
         delete mpBufPool;
      }
      if (mpHeadersPool != NULL)
      {
         delete mpHeadersPool;
      }
   }
};

CPPUNIT_TEST_SUITE_REGISTRATION(MpInputDeviceDriverTest);

