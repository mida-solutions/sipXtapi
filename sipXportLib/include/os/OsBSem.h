//  
// Copyright (C) 2006 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


#ifndef _OsBSem_h_
#define _OsBSem_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "os/OsDefs.h"
#include "os/OsSyncBase.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

/// Binary semaphore
class OsBSemBase : public OsSyncBase
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   enum InitialSemaphoreState
   {
      EMPTY = 0,  ///< semaphore is initially unavailable
      FULL  = 1   ///< semaphore is initially available
   };

   enum QueueOptions
   {
      Q_FIFO     = 0x0, ///< queue blocked tasks on a first-in, first-out basis
      Q_PRIORITY = 0x1  ///< queue blocked tasks based on their priority
   };

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */

     /// Block the task until the semaphore is acquired or the timeout expires
   virtual OsStatus acquire(const OsTime& rTimeout = OsTime::OS_INFINITY) = 0;

     /// Conditionally acquire the semaphore (i.e., don't block)
   virtual OsStatus tryAcquire(void) = 0;
     /**
      * @return OS_BUSY if the semaphore is held by some other task.
      */

     /// Release the semaphore
   virtual OsStatus release(void) = 0;

/* ============================ ACCESSORS ================================= */

     /// Print semaphore information to the console
   virtual void OsBSemShow(void) = 0;

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
   int mOptions;  ///< options specified at time of binary semaphore creation

   int mTaskId;   ///< if OS_SYNC_DEBUG is enabled, ONLY ON WNT, we use this
      ///< variable to store the ID of the task currently holding the semaphore 

     /// Default constructor
   OsBSemBase()  {  };

     /// Destructor
   virtual
      ~OsBSemBase()  {  };

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

     /// Copy constructor (not implemented for this class)
   OsBSemBase(const OsBSemBase& rOsBSemBase);

     /// Assignment operator (not implemented for this class)
   OsBSemBase& operator=(const OsBSemBase& rhs);

};

/* ============================ INLINE METHODS ============================ */

/// Depending on the native OS that we are running on, we include the class
/// declaration for the appropriate lower level implementation and use a
/// "typedef" statement to associate the OS-independent class name (OsBSem)
/// with the OS-dependent realization of that type (e.g., OsBSemWnt).
#if defined(_WIN32)
#  include "os/Wnt/OsBSemWnt.h"
   typedef class OsBSemWnt OsBSem;
#elif defined(_VXWORKS)
#  include "os/Vxw/OsBSemVxw.h"
   typedef class OsBSemVxw OsBSem;
#elif defined(__pingtel_on_posix__)
#  include "os/linux/OsBSemLinux.h"
   typedef class OsBSemLinux OsBSem;
#else
#  error Unsupported target platform.
#endif

#endif  // _OsBSem_h_
