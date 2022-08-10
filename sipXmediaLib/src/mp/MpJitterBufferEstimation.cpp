//
// Copyright (C) 2022 SIP Spectrum, Inc.  All rights reserved.
//  
// Copyright (C) 2008-2013 SIPez LLC. All rights reserved.
//
// Copyright (C) 2008 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// Author: Alexander Chemeris <Alexander.Chemeris AT SIPez DOT com>

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "mp/MpJitterBufferEstimation.h"
#include "mp/MpJbeFixed.h"
#include "os/OsSysLog.h"

// EXTERNAL FUNCTIONS
#ifdef EXTERNAL_JB_ESTIMATION // [
   extern "C" MpJitterBufferEstimation *createJbe(const char *name);
#endif // !EXTERNAL_JB_ESTIMATION ]

// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
UtlString MpJitterBufferEstimation::smDefaultAlgorithm;

/* //////////////////////////////// PUBLIC //////////////////////////////// */

/* =============================== CREATORS =============================== */

MpJitterBufferEstimation *MpJitterBufferEstimation::createJbe(const UtlString &algName)
{
   const UtlString &name = (algName.length() == 0) ? smDefaultAlgorithm : algName;

   if (name == MpJbeFixed::name)
   {
      return new MpJbeFixed();
   } 
   else
   {
#ifdef EXTERNAL_JB_ESTIMATION // [
       OsSysLog::add(FAC_MP, PRI_DEBUG, "MpJitterBufferEstimation::createJbe(%s) using external JB", algName.data());

      return ::createJbe(name.data());

#else // EXTERNAL_VAD ][
       OsSysLogPriority logPri = PRI_WARNING;
       if (algName.length() == 0)
       {
           logPri = PRI_DEBUG;
       }
       OsSysLog::add(FAC_MP, logPri,
                    "MpJitterBufferEstimation::createJbe(): "
                    "Could not find JBE algorithm with name \"%s\"!"
                    " Using default JBE algorithm instead.",
                    name.data());
      // Default VAD for unknown names is simple VAD.
      return new MpJbeFixed();

#endif // EXTERNAL_VAD ]
   }

}

/* ============================= MANIPULATORS ============================= */

void MpJitterBufferEstimation::setDefaultAlgorithm(const UtlString& name)
{
   smDefaultAlgorithm = name;
}

/* ============================== ACCESSORS =============================== */

/* =============================== INQUIRY ================================ */


/* ////////////////////////////// PROTECTED /////////////////////////////// */


/* /////////////////////////////// PRIVATE //////////////////////////////// */


/* ============================== FUNCTIONS =============================== */

