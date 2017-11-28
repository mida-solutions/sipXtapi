//
// Copyright (C) 2007-2017 SIPez LLC  All rights reserved.
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#include <os/OsTime.h>
#include <os/OsEvent.h>
#include <os/OsServerTask.h>
#include <os/OsRpcMsg.h>
#include <os/OsDateTime.h>
#include <sipxunittests.h>


class RightEventThread : public OsServerTask
{
    public:
   
    int mNumEvents;
    int mMaxEvents;
    int* mpDeletedEvent;

    RightEventThread(int* eventOutcome, int eventsSize)
    {
        mNumEvents = -1;
        mMaxEvents = eventsSize;
        mpDeletedEvent = eventOutcome;
    }

    UtlBoolean handleMessage(OsMsg& rMsg)
    {
        int waitMsec = rand();
        delay((waitMsec % 3 ) * 50);

        OsEvent* event = ((OsRpcMsg&)rMsg).getEvent();
        CPPUNIT_ASSERT(event);
   
        mNumEvents++;
        intptr_t eventIndex =-1;
        event->getUserData(eventIndex);
        //CPPUNIT_ASSERT(mNumEvents == eventIndex);
        CPPUNIT_ASSERT(mNumEvents < mMaxEvents);

        OsStatus eventStat = event->signal(mNumEvents);
        if(eventStat == OS_ALREADY_SIGNALED)
        {
           // The Right side lost, the Left side is done
           // we delete on this side
           delete event;
           event = NULL;
           mpDeletedEvent[mNumEvents] = TRUE;
        }
        else
        {
           // This/Right side won. we do nothing
           mpDeletedEvent[mNumEvents] = FALSE;
           //osPrintf("Right: %d\n", eventStat);
        }
        return(TRUE);
    }
};

/// This thread begin firing passed notification with specified delay.
class MultipleFireThread : public OsTask
{
public:

     /**
     *  @param delay - (in) - delay between fires. Pass -1 to get no-delay firing.
     *  @param notification - (in) Notification to fire
     */
   MultipleFireThread(int delay, OsNotification *notification)
   : OsTask("MultipleFireThread")
   , mDelay(delay)
   , mpNotification(notification)
   {
      CPPUNIT_ASSERT(mpNotification != NULL);
   }

   int run(void* pArg)
   {
      while(!isShuttingDown())
      {
         if (mDelay > 0)
         {
            delay(mDelay);
         }
         OsStatus status = mpNotification->signal(0);
         CPPUNIT_ASSERT(status == OS_ALREADY_SIGNALED || status == OS_SUCCESS);
      }

      return 0;
   }


protected:
   int mDelay;
   OsNotification *mpNotification;

};

class OsEventTest : public SIPX_UNIT_BASE_CLASS
{
    CPPUNIT_TEST_SUITE(OsEventTest);
    CPPUNIT_TEST(testTimedEvent);
    CPPUNIT_TEST(testThreadedEvent);
    CPPUNIT_TEST(testThreadedMultipleFire);
    CPPUNIT_TEST_SUITE_END();


public:
    void testTimedEvent()
    {
        OsTime   eventTimeout(2,0);
        OsEvent* pEvent;

        pEvent = new OsEvent(12345);
        time_t epochTime = time(NULL);
        CPPUNIT_ASSERT(pEvent->wait(eventTimeout) != OS_SUCCESS);
        pEvent->signal(67890);
        CPPUNIT_ASSERT_EQUAL(OS_SUCCESS, pEvent->wait(eventTimeout));
        pEvent->reset();
        CPPUNIT_ASSERT(pEvent->wait(eventTimeout) != OS_SUCCESS);
        epochTime = time(NULL) - epochTime;
   
        // Make sure we waited (approximately) 2 seconds each time.
        CPPUNIT_ASSERT(epochTime > 2 && epochTime < 6);
   
        delete pEvent;
    }

    void testThreadedEvent()
    {
        // Seed the random number generator
        srand(OsDateTime::getSecsSinceEpoch());

        int numTries = 100;
        int* rightResults = new int[numTries];
        int* leftResults = new int[numTries];

        // Create the Right thread.  This context will be the
        // Left thread.
        RightEventThread rightThread(rightResults, numTries);
        rightThread.start();

        int index;
        for(index = 0; index < numTries; index++)
        {
            OsEvent* event = new OsEvent(index);
            OsRpcMsg eventMsg(OsMsg::USER_START,0,*event);
            rightThread.postMessage(eventMsg);

            int waitTimeMsec = (rand() % 3) * 110;
            OsTime time(0, waitTimeMsec * 1000);
            event->wait(time);

            OsStatus eventStat = event->signal(index);
            if(eventStat == OS_ALREADY_SIGNALED)
            {
                // We (Left) lost the other side is done
                intptr_t eventData;
                event->getEventData(eventData);
                CPPUNIT_ASSERT(eventData == index);

                // This/Left side deletes the event
                delete event;
                event = NULL;
               leftResults[index] = TRUE;
            }
            else
            {
                // The other/Right side lost
                // Do nothing
                leftResults[index] = FALSE;
                //osPrintf("Left: %d\n", eventStat);
            }
        }

        OsTask::delay(1000);

        int leftDeletes = 0;
        int rightDeletes = 0;
        for(index = 0; index < numTries; index++)
        {
            if(leftResults[index] == TRUE)
            {
                leftDeletes++;
            }
            if(rightResults[index] == TRUE)
            {
                rightDeletes++;
            }
            if(rightResults[index] == leftResults[index])
            {
               //osPrintf("Left deleted: %d Right deleted: %d\n",
               //           leftDeletes, rightDeletes);
               //osPrintf("[%d]: Both sides %s\n", index, 
               //        rightResults[index] ? "Deleted" : "Did not delete");
            }
            CPPUNIT_ASSERT(rightResults[index] != leftResults[index]);
        }

        //osPrintf("Left deleted: %d Right deleted: %d\n",
        //        leftDeletes, rightDeletes);

        CPPUNIT_ASSERT(leftDeletes + rightDeletes == numTries);
    }

    void testThreadedMultipleFire()
    {
        OsEvent event;
        MultipleFireThread fireThread(-1, &event);

        fireThread.start();

        for (int i=0; i<10000; i++)
        {
           CPPUNIT_ASSERT_EQUAL(OS_SUCCESS, event.wait(500));
           CPPUNIT_ASSERT_EQUAL(OS_SUCCESS, event.reset());
        }

        fireThread.requestShutdown();
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(OsEventTest);

