//  
// Copyright (C) 2007-2017 SIPez LLC.  All rights reserved.
//
// Copyright (C) 2007-2008 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// Author: Dan Petrie <dpetrie AT SIPez DOT com>

// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES
#include <os/OsWriteLock.h>
#include <os/OsReadLock.h>
#include <os/OsDateTime.h>
#include <os/OsSysLog.h>
#include <os/OsTask.h>
#include <utl/UtlHashBagIterator.h>
#include <utl/UtlInt.h>
#include <utl/UtlDList.h>
#include <utl/UtlDListIterator.h>
#include <mp/MpInputDeviceManager.h>
#include <mp/MpInputDeviceDriver.h>
#include <mp/MpBuf.h>
#include <mp/MpAudioBuf.h>

#ifdef RTL_ENABLED
#  include <rtl_macro.h>
#else
#  define RTL_BLOCK(x)
#  define RTL_EVENT(x, y)
#endif

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// PRIVATE CLASSES
/**
*  @brief Private class container for input device buffer and related info
*/
class MpInputDeviceFrameData
{
public:
   MpInputDeviceFrameData()
   : mFrameTime(0)
   {};

   virtual ~MpInputDeviceFrameData()
   {};

   MpAudioBufPtr mFrameBuffer; ///< Actual captured data.
   MpFrameTime mFrameTime;
   OsTime mFrameReceivedTime;  ///< This time is used only for driver jitter
                               ///< calculation.

private:
     /// Copy constructor (not implemented for this class)
   MpInputDeviceFrameData(const MpInputDeviceFrameData& rMpInputDeviceFrameData);

     /// Assignment operator (not implemented for this class)
   MpInputDeviceFrameData& operator=(const MpInputDeviceFrameData& rhs);
};

/**
*  @brief Private class container for MpInputDeviceDriver pointer and window of buffers
*/
class MpAudioInputConnection : public UtlInt
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */
///@name Creators
//@{

     /// Default constructor
   MpAudioInputConnection(MpInputDeviceHandle deviceId,
                          MpInputDeviceDriver& deviceDriver,
                          unsigned int frameBufferLength,
                          MpBufPool& bufferPool,
                          uint32_t samplesPerFrame,
                          uint32_t samplesPerSec)
   : UtlInt(deviceId)
   , mLastPushedFrame(frameBufferLength - 1)
   , mFrameBufferLength(frameBufferLength)
   , mFrameBuffersUsed(0)
   , mppFrameBufferArray(NULL)
   , mpInputDeviceDriver(&deviceDriver)
   , mpBufferPool(&bufferPool)
   , mInUse(FALSE)
   , mEnableCounter(0)
   , mSamplesPerFrame(samplesPerFrame)
   , mSamplesPerSec(samplesPerSec)
   {
      assert(mFrameBufferLength > 0);
      mppFrameBufferArray = new MpInputDeviceFrameData[mFrameBufferLength];
   };


     /// Destructor
   virtual
   ~MpAudioInputConnection()
   {
      OsSysLog::add(FAC_MP, PRI_DEBUG,
         "~MpInputDeviceManager start dev: %p id: %d\n",
         mpInputDeviceDriver, getValue());

      if (mppFrameBufferArray)
      {
         delete[] mppFrameBufferArray;
         mppFrameBufferArray = NULL;
      }
      OsSysLog::add(FAC_MP, PRI_DEBUG,"~MpInputDeviceManager end\n");
   }

//@}

/* ============================ MANIPULATORS ============================== */
///@name Manipulators
//@{

   OsStatus setup(uint32_t samplesPerFrame, uint32_t samplesPerSec)
   {
      mSamplesPerFrame = samplesPerFrame;
      mSamplesPerSec = samplesPerSec;
      return OS_SUCCESS;
   }

   OsStatus enable()
   {
      OsStatus status = OS_SUCCESS;
      OsSysLog::add(FAC_MP, PRI_DEBUG,
                    "MpAudioInputConnection::enable() mEnableCounter=%d", mEnableCounter);
      if (mEnableCounter == 0)
      {
         MpInputDeviceDriver *deviceDriver = getDeviceDriver();
         if (deviceDriver)
         {
            status = deviceDriver->enableDevice(mSamplesPerFrame,
                                                mSamplesPerSec);
         }
         else
         {
            status = OS_NOT_FOUND;
         }
      }
      if (status == OS_SUCCESS)
      {
         mEnableCounter++;
      }
      OsSysLog::add(FAC_MP, PRI_DEBUG,
                    "MpAudioInputConnection::enable() counter changed to %d, status is %d",
                    mEnableCounter, status);
      
      return status;
   }

   OsStatus disable()
   {
      OsStatus status = OS_SUCCESS;
      OsSysLog::add(FAC_MP, PRI_DEBUG,
                    "MpAudioInputConnection::disable() mEnableCounter=%d", mEnableCounter);
      if (mEnableCounter == 1)
      {
         MpInputDeviceDriver *deviceDriver = getDeviceDriver();
         if (deviceDriver)
         {
            status = deviceDriver->disableDevice();
         }
         else
         {
            status = OS_NOT_FOUND;
         }
      }
      if (status == OS_SUCCESS)
      {
         mEnableCounter--;
      }

      OsSysLog::add(FAC_MP, PRI_DEBUG,
                    "MpAudioInputConnection::disable() counter changed to %d, status is %d",
                    mEnableCounter, status);

      return status;
   }

   OsStatus pushFrame(unsigned int numSamples,
                      MpAudioSample* samples,
                      MpFrameTime frameTime)
   {
      OsStatus result = OS_FAILED;
      assert(samples);
      assert(mpInputDeviceDriver && mpInputDeviceDriver->isEnabled());

      // figure out the number of samples per second and per frame.
      uint32_t samplesPerFrame = mpInputDeviceDriver->getSamplesPerFrame();
      assert(samplesPerFrame > 0);
      uint32_t samplesPerSec = mpInputDeviceDriver->getSamplesPerSec();
      assert(samplesPerSec > 0);

#ifdef RTL_AUDIO_ENABLED
      RTL_RAW_AUDIO(UtlString("MpAudioInputConnection[")\
                      .append(*mpInputDeviceDriver) \
                      .append("].pushFrameAudio"), \
                    samplesPerSec/samplesPerFrame, \
                    numSamples, samples, mLastPushedFrame);
#endif

      //printf("pushFrame frameTime: %d\n", frameTime);

      // TODO: could support re-framing here.  For now
      // the driver must do the correct framing.
      assert(numSamples == samplesPerFrame);

      // Circular buffer of frames
      int thisFrameIndex;
      if (mFrameBuffersUsed)
      {
         thisFrameIndex = (++mLastPushedFrame) % mFrameBufferLength;
      }
      else
      {
         thisFrameIndex = 0;
         mLastPushedFrame = 0;
      }

      MpInputDeviceFrameData* thisFrameData = &mppFrameBufferArray[thisFrameIndex];

      // Current time to review device driver jitter
      OsDateTime::getCurTime(thisFrameData->mFrameReceivedTime);

      // Frame time slot the driver says this is targeted for
      thisFrameData->mFrameTime = frameTime;

      // Make sure we have someplace we can stuff the data
      if (!thisFrameData->mFrameBuffer.isValid())
      {
         thisFrameData->mFrameBuffer = mpBufferPool->getBuffer();
         mFrameBuffersUsed++;
         if (mFrameBuffersUsed > mFrameBufferLength)
         {
            mFrameBuffersUsed = mFrameBufferLength;
         }
      }

      assert(thisFrameData->mFrameBuffer->getSamplesNumber() >= numSamples);

      // Stuff the data in a buffer
      if (thisFrameData->mFrameBuffer.isValid())
      {
         memcpy(thisFrameData->mFrameBuffer->getSamplesWritePtr(), 
            samples, numSamples * sizeof(MpAudioSample));
         thisFrameData->mFrameBuffer->setSamplesNumber(numSamples);
         thisFrameData->mFrameBuffer->setSpeechType(MP_SPEECH_UNKNOWN);
         result = OS_SUCCESS;
      }
      else
      {
         assert(0);
      }

      RTL_EVENT("MpAudioInputConnection::pushFrame", frameTime);
      return result;
   };

   inline void setInUse() { mInUse = TRUE; }
   inline void clearInUse() { mInUse = FALSE; }

//@}

/* ============================ ACCESSORS ================================= */
///@name Accessors
//@{

   OsStatus getFrame(MpFrameTime &frameTime,
                     MpBufPtr& buffer,
                     unsigned& numFramesBefore,
                     unsigned& numFramesAfter) const
   {
      OsStatus result = OS_INVALID_STATE;
      assert(mpInputDeviceDriver);
      //assert(mpInputDeviceDriver && mpInputDeviceDriver->isEnabled());
      if (mpInputDeviceDriver)
      {
          // If the device is disabled, get out here as there is no frames to be had.
          if(!mpInputDeviceDriver->isEnabled())
          {
              OsSysLog::add(FAC_MP, PRI_ERR, "getFrame - disabled device (%d)\n",
                getValue());
              return(result);
          }

         result = OS_NOT_FOUND;
      }
      else
      {
         // If there is no device driver, or if the device is not enabled,
         // then we cannot get a frame of audio.
         OsSysLog::add(FAC_MP, PRI_ERR, "getFrame - invalid device (%d)\n",
                       getValue());
         return result;
      }

      // figure out the number of samples per second and per frame.
      uint32_t samplesPerFrame = mpInputDeviceDriver->getSamplesPerFrame();
      assert(samplesPerFrame > 0);
      uint32_t samplesPerSec = mpInputDeviceDriver->getSamplesPerSec();
      assert(samplesPerSec > 0);

      // Initialize numFramesBefore and numFramesAfter. They will be advanced
      // inside the loop.
      numFramesBefore = 0;
      numFramesAfter = mFrameBuffersUsed;

      unsigned int lastFrame = mLastPushedFrame;
      int framePeriod = 1000 * samplesPerFrame / samplesPerSec;

      // When requesting a frame we provide the frame that overlaps the
      // given frame time.  The frame time is for the beginning of a frame.
      // So we provide the frame that begins at or less than the requested
      // time, but not more than one frame period older.
      unsigned int frameIndex;
      for (frameIndex = 0; frameIndex < mFrameBuffersUsed; frameIndex++)
      {
         // Walk backwards from the last inserted frame
         MpInputDeviceFrameData* frameData = 
            &mppFrameBufferArray[(lastFrame - frameIndex) % mFrameBufferLength];

         if (frameData->mFrameBuffer.isValid())
         {
            if (frameData->mFrameTime + framePeriod <= frameTime)
            {
               // No need to move forward - we've already gone to too early
               // frame time.

               /*printf("too early: %ums + %ums <= %ums [%u/%u] %u+%u\n",
                      frameData->mFrameTime, framePeriod, frameTime,
                      frameIndex, mFrameBuffersUsed,
                      numFramesBefore, numFramesAfter);*/
               break;
            }
            else if (frameData->mFrameTime <= frameTime)
            {
               // The frame whose time range covers the requested time.

               // Do not count this frame.
               numFramesAfter--;

               // We always make a copy of the frame as we are typically
               // crossing task boundaries here.
               buffer = frameData->mFrameBuffer.clone();

               RTL_EVENT("MpAudioInputConnection::getFrame", frameTime);
               result = OS_SUCCESS;
               break;
            }

            numFramesAfter--;
            numFramesBefore++;
         }
      }
//      printf("[%u] %u+%u=%u\n", frameIndex, numFramesBefore, numFramesAfter, mFrameBuffersUsed);
      return(result);
   };

   MpInputDeviceDriver* getDeviceDriver() const
   {
      return(mpInputDeviceDriver);
   };


   unsigned getTimeDerivatives(unsigned nDerivatives, 
                               double*& derivativeBuf) const
   {
      assert(mpInputDeviceDriver && mpInputDeviceDriver->isEnabled());
      if (mpInputDeviceDriver && mpInputDeviceDriver->isEnabled())
      {
         // Nothing to do here -- just maing if statement more readable.
      }
      else
      {
         // If there is no device driver, or if the device is not enabled,
         // then we cannot get a frame of audio.
         OsSysLog::add(FAC_MP, PRI_ERR, "getTimeDerivatives - "
                       "invalid device (%d)\n", getValue());
         return 0;
      }

      // figure out the number of samples per second and per frame.
      uint32_t samplesPerFrame = mpInputDeviceDriver->getSamplesPerFrame();
      assert(samplesPerFrame > 0);
      uint32_t samplesPerSec = mpInputDeviceDriver->getSamplesPerSec();
      assert(samplesPerSec > 0);

      unsigned nActualDerivs = 0;

      int referenceFramePeriod = 1000 * samplesPerFrame / samplesPerSec;
      unsigned int lastFrame = mLastPushedFrame;

      unsigned int t2FrameIdx;
      for (t2FrameIdx = 0; 
           (t2FrameIdx < mFrameBuffersUsed) && (nActualDerivs < nDerivatives); 
           t2FrameIdx++)
      {
         // in indexes here, higher is older, since we're subtracting before
         // modding to get the actual buffer array index.
         unsigned int t1FrameIdx = t2FrameIdx+1;

         MpInputDeviceFrameData* t2FrameData = 
            &mppFrameBufferArray[(lastFrame - t2FrameIdx) 
                                 % mFrameBufferLength];
         MpInputDeviceFrameData* t1FrameData = 
            &mppFrameBufferArray[(lastFrame - t1FrameIdx) 
                                 % mFrameBufferLength];

         // The first time we find an invalid buffer, break out of the loop.
         // This takes care of the case when only a small amount of data has
         // been pushed to the buffer.
         if (!t2FrameData->mFrameBuffer.isValid() ||
             !t1FrameData->mFrameBuffer.isValid())
         {
            break;
         }

         long t2OsMsecs = t2FrameData->mFrameReceivedTime.cvtToMsecs();
         long t1OsMsecs = t1FrameData->mFrameReceivedTime.cvtToMsecs();
         double curDeriv = (double)(t2OsMsecs - t1OsMsecs) / referenceFramePeriod;
         derivativeBuf[t2FrameIdx] = curDeriv;
         nActualDerivs++;
      }
      return nActualDerivs;
   }

   inline MpFrameTime getCurrentFrameTime() const
   {
      return getDeviceDriver()->getCurrentFrameTime();
   }

//@}

/* ============================ INQUIRY =================================== */
///@name Inquiry
//@{

   inline UtlBoolean isInUse() const { return mInUse; }

//@}

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   unsigned int mLastPushedFrame;    ///< Index of last pushed frame in mppFrameBufferArray.
   unsigned int mFrameBufferLength;  ///< Length of mppFrameBufferArray.
   unsigned int mFrameBuffersUsed;   ///< Actual number of buffers with data.
   MpInputDeviceFrameData* mppFrameBufferArray;
   MpInputDeviceDriver* mpInputDeviceDriver;
   MpBufPool* mpBufferPool;          
   UtlBoolean mInUse;                ///< Use indicator to synchronize disable and remove.
   int mEnableCounter;               ///< Reference counter for enable() and disable() calls.
   uint32_t mSamplesPerFrame;        ///< Samples per frame setting for enable()
   uint32_t mSamplesPerSec;          ///< Samples per second setting for enable()

     /// Copy constructor (not implemented for this class)
   MpAudioInputConnection(const MpAudioInputConnection& rMpAudioInputConnection);

     /// Assignment operator (not implemented for this class)
   MpAudioInputConnection& operator=(const MpAudioInputConnection& rhs);
};


//               MpInputDeviceManager implementation

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
MpInputDeviceManager::MpInputDeviceManager(unsigned defaultSamplesPerFrame, 
                                           unsigned defaultSamplesPerSec,
                                           unsigned defaultNumBufferedFrames,
                                           MpBufPool& bufferPool)
: mRwMutex(OsRWMutex::Q_PRIORITY)
, mLastDeviceId(0)
, mDefaultSamplesPerFrame(defaultSamplesPerFrame)
, mDefaultSamplesPerSecond(defaultSamplesPerSec)
, mDefaultNumBufferedFrames(defaultNumBufferedFrames)
, mpBufferPool(&bufferPool)
{
   assert(defaultSamplesPerFrame > 0);
   assert(defaultSamplesPerSec > 0);
   assert(defaultNumBufferedFrames > 0);

   OsDateTime::getCurTime(mTimeZero);
}


// Destructor
MpInputDeviceManager::~MpInputDeviceManager()
{
   // All devices (and connections, so) should be removed from manager before
   // manager destroyed.
   assert(mConnectionsByDeviceName.entries() == 0);
   assert(mConnectionsByDeviceId.entries() == 0);
}

/* ============================ MANIPULATORS ============================== */
int MpInputDeviceManager::addDevice(MpInputDeviceDriver& newDevice)
{
   OsSysLog::add(FAC_MP, PRI_DEBUG, "MpInputDeviceManager::addDevice");
   OsWriteLock lock(mRwMutex);

   MpInputDeviceHandle newDeviceId = ++mLastDeviceId;
   // Tell the device what id to use when pushing frames to this

   newDevice.setDeviceId(newDeviceId);

   // Create a connection to contain the device and its buffered frames
   MpAudioInputConnection* connection = 
      new MpAudioInputConnection(newDeviceId,
                                 newDevice, 
                                 mDefaultNumBufferedFrames,
                                 *mpBufferPool,
                                 mDefaultSamplesPerFrame,
                                 mDefaultSamplesPerSecond);

   // Map by device name string
   UtlInt* idValue = new UtlInt(newDeviceId);
   OsSysLog::add(FAC_MP, PRI_DEBUG,
                 "MpInputDeviceManager::addDevice dev: %s id: %d", 
                 newDevice.data(), newDeviceId);
   UtlContainable* objInserted = mConnectionsByDeviceName.insertKeyAndValue(&newDevice, idValue);
   if(objInserted == NULL)
   {
      OsSysLog::add(FAC_MP, PRI_ERR, "MpInputDeviceManager::addDevice device not added to mConnectionsByDeviceName samples/frame: %d samples/sec.: %d",
              mDefaultSamplesPerFrame, mDefaultSamplesPerSecond);
   }
   assert(objInserted);

   // Map by device ID
   objInserted = mConnectionsByDeviceId.insert(connection);
   if(objInserted == NULL)
   {
      OsSysLog::add(FAC_MP, PRI_ERR, "MpInputDeviceManager::addDevice device not added to mConnectionsByDeviceId");
   }
   assert(objInserted);

   return(newDeviceId);
}


MpInputDeviceDriver* MpInputDeviceManager::removeDevice(MpInputDeviceHandle deviceId)
{
   // We need the manager lock while we're indicating the connection is in use.
   mRwMutex.acquireWrite();

   MpAudioInputConnection* connectionFound = NULL;
   UtlInt deviceKey(deviceId);
   MpInputDeviceDriver* deviceDriver = NULL;

   // try 10 times (this is a blind guess) -- checking every
   // 10ms over 100ms seems reasonable.
   int checkInUseTries = 10;

   for (int i = 0; i < checkInUseTries; i--)
   {
      connectionFound =
         (MpAudioInputConnection*) mConnectionsByDeviceId.find(&deviceKey);

      // If we couldn't find a connection, or we found the connection
      // and it isn't in use, then no need to continue looping.
      // The loop only continues if a connection was found and in use.
      if (connectionFound == NULL ||
         (!connectionFound->isInUse()))
      {
         break;
      }

      // If the device is found and in use, release the manager lock,
      // wait a small bit, and get the lock again to give the manager a
      // chance to finish what it was doing  with the connection.
      mRwMutex.releaseWrite();
      OsTask::delay(10);
      mRwMutex.acquireWrite();
   }

   if (connectionFound && !connectionFound->isInUse())
   {
      // Remove the connection while we're under the lock.
      // If a disableDevice is waiting to acquire lock,
      // once they receive the lock, the connection will no longer
      // be there, which is the desired behavior.

      // Remove from the id indexed container
      mConnectionsByDeviceId.remove(connectionFound);

      deviceDriver = connectionFound->getDeviceDriver();
      assert(deviceDriver);

      // Get the int value mapped in the hash so we can clean up
      UtlInt* deviceIdInt =
         (UtlInt*) mConnectionsByDeviceName.findValue(deviceDriver);

      // Remove from the name indexed hash
      mConnectionsByDeviceName.remove(deviceDriver);
      if (deviceIdInt)
      {
         OsSysLog::add(FAC_MP, PRI_DEBUG,
                       "MpInputDeviceManager::removeDevice dev: %s id: %d",
                       deviceDriver->data(), deviceIdInt->getValue());
         delete deviceIdInt;
         deviceIdInt = NULL;
      }

      delete connectionFound;
      connectionFound = NULL;
   }

   // Note: we specifically keep the manager lock for the entire duration 
   //       of removal (with exception of waiting while it's in use).
   mRwMutex.releaseWrite();

   // deviceDriver of NULL is returned if:
   //    * The connection is not found.
   //    * The connection is found, but the connection was already in use.
   return(deviceDriver);
}

int MpInputDeviceManager::removeAllDevices()
{
    int numRemoved = 0;
    MpInputDeviceHandle deviceId = MP_INVALID_INPUT_DEVICE_HANDLE;

    while(mConnectionsByDeviceId.entries())
    {
        {
            OsWriteLock lock(mRwMutex);
            MpAudioInputConnection* connection = NULL;
            UtlHashBagIterator iterator(mConnectionsByDeviceId);
            if((connection = (MpAudioInputConnection*) iterator()))
            {
                deviceId = connection->getValue();
            }
            else
            {
                deviceId = MP_INVALID_INPUT_DEVICE_HANDLE;
            }
        }

        if(deviceId > MP_INVALID_INPUT_DEVICE_HANDLE)
        {
            // If device is not disabled, disable it
            if(isDeviceEnabled(deviceId))
            {
                disableDevice(deviceId);
            }

            // Remove device
            MpInputDeviceDriver* deviceDriver = removeDevice(deviceId);

            // Need to delete the device as it is not in removeDevice
            if(deviceDriver)
            {
                delete deviceDriver;
                deviceDriver = NULL;
            }

            numRemoved++;
        }
    }

    return(numRemoved);
}

OsStatus MpInputDeviceManager::setupDevice(MpInputDeviceHandle deviceId,
                                           uint32_t samplesPerFrame,
                                           uint32_t samplesPerSec)
{
   OsStatus status = OS_NOT_FOUND;
   OsWriteLock lock(mRwMutex);

   MpAudioInputConnection* connectionFound = NULL;
   UtlInt deviceKey(deviceId);
   connectionFound =
      (MpAudioInputConnection*) mConnectionsByDeviceId.find(&deviceKey);

   if (connectionFound)
   {
      status = connectionFound->setup(samplesPerFrame, samplesPerSec);
   }
   return(status);
}


OsStatus MpInputDeviceManager::enableDevice(MpInputDeviceHandle deviceId)
{
   OsStatus status = OS_NOT_FOUND;
   OsWriteLock lock(mRwMutex);

   MpAudioInputConnection* connectionFound = NULL;
   UtlInt deviceKey(deviceId);
   connectionFound =
      (MpAudioInputConnection*) mConnectionsByDeviceId.find(&deviceKey);

   if (connectionFound)
   {
      status = connectionFound->enable();

      OsSysLog::add(FAC_MP, PRI_DEBUG,
           "MpInputDeviceManager::enableDevice enableDevice() id: %d (%s) returned: %d",
           deviceId, (connectionFound->getDeviceDriver()->getDeviceName()).data(), status);
   }
   else
   {
      OsSysLog::add(FAC_MP, PRI_ERR, "MpInputDeviceManager::enableDevice deviceId: %d, connection not found",
           deviceId);

   }

   return(status);
}


OsStatus MpInputDeviceManager::disableDevice(MpInputDeviceHandle deviceId)
{
   // We need the manager lock while we're indicating the connection is in use.
   mRwMutex.acquireWrite();

   OsStatus status = OS_NOT_FOUND;
   // We haven't determined if it's ok to disable the device yet.
   UtlBoolean okToDisable = FALSE;  
   MpAudioInputConnection* connectionFound = NULL;
   UtlInt deviceKey(deviceId);

   // try 10 times (this is a blind guess) -- checking every
   // 10ms over 100ms seems reasonable.
   int checkInUseTries = 10;
   
   for (int i = 0; i < checkInUseTries; i--)
   {
      connectionFound =
         (MpAudioInputConnection*) mConnectionsByDeviceId.find(&deviceKey);

      // If we couldn't find a connection, or we found the connection
      // and it isn't in use, then no need to continue looping.
      // The loop only continues if a connection was found and in use.
      if (connectionFound == NULL ||
          (!connectionFound->isInUse()))
      {
         break;
      }

      // If the device is found and in use, release the manager lock,
      // wait a small bit, and get the lock again to give the manager a
      // chance to finish what it was doing  with the connection.
      mRwMutex.releaseWrite();
      OsTask::delay(10);
      mRwMutex.acquireWrite();
   }

   UtlString deviceName;
   if (connectionFound)
   {
       deviceName = connectionFound->getDeviceDriver()->getDeviceName();
      if (connectionFound->isInUse())
      {
         // If the connection is in use by someone else,
         // then we indicate that the connection is busy.
         status = OS_BUSY;
      }
      else
      {
         // It's ok to disable now,
         // Set the connection in use.
         okToDisable = TRUE;
         connectionFound->setInUse();
      }
   }

   // Now we release the mutex and go on to actually disable the device.
   mRwMutex.releaseWrite();

   if (okToDisable)
   {
      status = connectionFound->disable();

      mRwMutex.acquireWrite();
      connectionFound->clearInUse();
      mRwMutex.releaseWrite();
   }

   OsSysLog::add(FAC_MP, PRI_DEBUG,
       "MpInputDeviceManager::disableDevice(%d) (%s) return: %d",
       deviceId, deviceName.data(), status);

   return(status);
}

OsStatus MpInputDeviceManager::disableAllDevicesExcept(int exceptCount, MpInputDeviceHandle exceptDeviceIds[])
{
    OsSysLog::add(FAC_MP, PRI_DEBUG,
        "MpInputDeviceManager::disableAllDevicesExcept(%d, %p)", exceptCount, exceptDeviceIds);
    OsStatus status = OS_SUCCESS;
    UtlDList exceptions;
    for(int i = 0; i < exceptCount; i++)
    {
        exceptions.append(new UtlInt(exceptDeviceIds[i]));
    }

    UtlDList disableList;

    // Lock is taken to get device IDs only.
    {
        OsWriteLock lock(mRwMutex);
        MpAudioInputConnection* connection = NULL;
        UtlHashBagIterator iterator(mConnectionsByDeviceId);
        while((connection = (MpAudioInputConnection*) iterator()))
        {
            // If not in the exception list, add to the list to be disabled
            UtlInt deviceIdInt(connection->getValue());
            if(exceptions.find(&deviceIdInt) == NULL)
            {
                disableList.append(new UtlInt(connection->getValue()));
            }
        }
    }

    // Disable the devices
    UtlDListIterator disableIterator(disableList);
    UtlInt* deviceIdPtr = NULL;
    while((deviceIdPtr = (UtlInt*) disableIterator()))
    {
        // Locking is done in disableDevice
        status = disableDevice(deviceIdPtr->getValue());
    }

    disableList.destroyAll();
    exceptions.destroyAll();

    return(status);
}

OsStatus MpInputDeviceManager::pushFrame(MpInputDeviceHandle deviceId,
                                         unsigned numSamples,
                                         MpAudioSample* samples,
                                         MpFrameTime frameTime)
{
   OsStatus status = OS_NOT_FOUND;
   RTL_EVENT("MpInputDeviceManager.pushFrame", deviceId);

   OsWriteLock lock(mRwMutex);

   MpAudioInputConnection* connectionFound = NULL;
   UtlInt deviceKey(deviceId);
   connectionFound =
      (MpAudioInputConnection*) mConnectionsByDeviceId.find(&deviceKey);

   if (connectionFound)
   {
      status = 
         connectionFound->pushFrame(numSamples, samples, frameTime);
   }

   RTL_EVENT("MpInputDeviceManager.pushFrame", 0);

   return(status);
}


OsStatus MpInputDeviceManager::getFrame(MpInputDeviceHandle deviceId,
                                        MpFrameTime &frameTime,
                                        MpBufPtr& buffer,
                                        unsigned& numFramesBefore,
                                        unsigned& numFramesAfter)
{
   OsStatus status = OS_INVALID_ARGUMENT;
   RTL_EVENT("MpInputDeviceManager.getFrame", deviceId);

   OsWriteLock lock(mRwMutex);

   MpAudioInputConnection* connectionFound = NULL;
   UtlInt deviceKey(deviceId);
   connectionFound =
      (MpAudioInputConnection*) mConnectionsByDeviceId.find(&deviceKey);

   if (connectionFound)
   {
       if(connectionFound->getDeviceDriver()->isEnabled())
       {
            status = 
                connectionFound->getFrame(frameTime,
                                          buffer,
                                          numFramesBefore,
                                          numFramesAfter);
       }
       else
       {
           status = OS_INVALID_STATE;
       }
   }

   RTL_EVENT("MpInputDeviceManager.getFrame", 0);

   return(status);
}

/* ============================ ACCESSORS ================================= */

OsStatus MpInputDeviceManager::getDeviceName(MpInputDeviceHandle deviceId,
                                             UtlString& deviceName) const
{
   OsStatus status = OS_NOT_FOUND;

   OsReadLock lock(mRwMutex);

   MpAudioInputConnection* connectionFound = NULL;
   UtlInt deviceKey(deviceId);
   connectionFound =
      (MpAudioInputConnection*) mConnectionsByDeviceId.find(&deviceKey);
   MpInputDeviceDriver* deviceDriver = NULL;

   if (connectionFound)
   {
      deviceDriver = connectionFound->getDeviceDriver();
      assert(deviceDriver);
      if (deviceDriver)
      {
         status = OS_SUCCESS;
         deviceName = *deviceDriver;
      }
   }
   else
   {
      OsSysLog::add(FAC_MP, PRI_WARNING, "MpInputDeviceManager::getDeviceName deviceId=%d not found, device count: %d",
                    deviceId, (int)mConnectionsByDeviceId.entries());
   }

   return(status);
}


OsStatus MpInputDeviceManager::getDeviceId(const UtlString& deviceName,
                                           MpInputDeviceHandle& deviceId) const
{
   OsStatus status = OS_NOT_FOUND;
   UtlString deviceString(deviceName);

   OsReadLock lock(mRwMutex);

   UtlInt* deviceKey = (UtlInt*) mConnectionsByDeviceName.findValue(&deviceString);

   if (deviceKey != NULL)
   {
      deviceId = deviceKey->getValue();
      status = OS_SUCCESS;
   }
   else
   {
      deviceId = MP_INVALID_INPUT_DEVICE_HANDLE;
   }

   return status;
}


MpFrameTime MpInputDeviceManager::getCurrentFrameTime(MpInputDeviceHandle deviceId) const
{
   MpFrameTime curFrameTime = 0;

   OsReadLock lock(mRwMutex);

   MpAudioInputConnection* connectionFound = NULL;
   UtlInt deviceKey(deviceId);
   connectionFound =
      (MpAudioInputConnection*) mConnectionsByDeviceId.find(&deviceKey);

   if (connectionFound)
   {
      curFrameTime = connectionFound->getCurrentFrameTime();
   }

   return curFrameTime;
}

OsStatus MpInputDeviceManager::getDeviceSamplesPerSec(MpInputDeviceHandle deviceId,
                                                      uint32_t& samplesPerSec) const
{
   OsStatus ret = OS_NOT_FOUND;
   UtlInt deviceKey(deviceId);
   MpAudioInputConnection* connectionFound = 
      (MpAudioInputConnection*) mConnectionsByDeviceId.find(&deviceKey);
   MpInputDeviceDriver* pDevDriver = 
      (connectionFound) ? connectionFound->getDeviceDriver() : NULL;

   if(pDevDriver)
   {
      samplesPerSec = pDevDriver->getSamplesPerSec();
      ret = OS_SUCCESS;
   }
   else
   {
       samplesPerSec = mDefaultSamplesPerSecond;
   }

   return(ret);
}


OsStatus MpInputDeviceManager::getDeviceSamplesPerFrame(MpInputDeviceHandle deviceId,
                                                        uint32_t& samplesPerFrame) const
{
   OsStatus ret = OS_NOT_FOUND;
   UtlInt deviceKey(deviceId);
   MpAudioInputConnection* connectionFound = 
      (MpAudioInputConnection*) mConnectionsByDeviceId.find(&deviceKey);
   MpInputDeviceDriver* pDevDriver = 
      (connectionFound) ? connectionFound->getDeviceDriver() : NULL;

   if(pDevDriver)
   {
      samplesPerFrame = pDevDriver->getSamplesPerFrame();
      ret = OS_SUCCESS;
   }
   return ret;
}

OsStatus MpInputDeviceManager::getTimeDerivatives(MpInputDeviceHandle deviceId,
                                                  unsigned& nDerivatives,
                                                  double*& derivativeBuf) const
{
   OsStatus stat = OS_INVALID_ARGUMENT;
   unsigned nActualDerivs = 0;
   OsReadLock lock(mRwMutex);

   MpAudioInputConnection* connectionFound = NULL;
   UtlInt deviceKey(deviceId);
   connectionFound =
      (MpAudioInputConnection*) mConnectionsByDeviceId.find(&deviceKey);

   if (connectionFound)
   {
      stat = OS_SUCCESS;
      nActualDerivs = connectionFound->getTimeDerivatives(nDerivatives, 
                                                          derivativeBuf);
   }
   else
   {
      printf("MpInputDeviceManager::pushFrame device(%d) not found\n", deviceId);
   }
   nDerivatives = nActualDerivs;
   return(stat);
}

/* ============================ INQUIRY =================================== */

UtlBoolean MpInputDeviceManager::isDeviceEnabled(MpInputDeviceHandle deviceId) const
{
   UtlBoolean enabledState = FALSE;
   OsReadLock lock(mRwMutex);

   MpAudioInputConnection* connectionFound = NULL;
   UtlInt deviceKey(deviceId);
   connectionFound =
      (MpAudioInputConnection*) mConnectionsByDeviceId.find(&deviceKey);
   MpInputDeviceDriver* deviceDriver = NULL;

   if (connectionFound)
   {
      deviceDriver = connectionFound->getDeviceDriver();
      assert(deviceDriver);
      if (deviceDriver)
      {
         enabledState = 
            deviceDriver->isEnabled();
      }
   }
   return(enabledState);
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

