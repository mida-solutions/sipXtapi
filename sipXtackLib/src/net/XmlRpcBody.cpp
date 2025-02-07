//
// Copyright (C) 2005-2019 SIPez LLC.  All rights reserved.
// 
// Copyright (C) 2005 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include <os/OsIntTypes.h>
#include <os/OsSysLog.h>
#include <os/OsDateTime.h>
#include <utl/UtlInt.h>
#include <utl/UtlLongLongInt.h>
#include <utl/UtlBool.h>
#include <utl/UtlDateTime.h>
#include <utl/UtlSList.h>
#include <utl/UtlSListIterator.h>
#include <utl/UtlHashMapIterator.h>
#include <utl/XmlContent.h>
#include <net/XmlRpcBody.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
XmlRpcBody::XmlRpcBody()
{
   mBody = XML_VERSION_1_0;
}

// Destructor
XmlRpcBody::~XmlRpcBody()
{
}

/* ============================ MANIPULATORS ============================== */


void XmlRpcBody::append(const char* string)
{
   mBody.append(string);
}


/* ============================ ACCESSORS ================================= */

int XmlRpcBody::getLength() const
{
   return (mBody.length());
}

void XmlRpcBody::getBytes(const char** bytes, int* length) const
{
   // This version of getBytes exists so that a caller who is
   // calling this method through an HttpBody will get the right
   // thing - we fill in the mBody string and then return that.
   UtlString tempBody;
   getBytes( &tempBody, length );
   ((XmlRpcBody*)this)->mBody = tempBody.data();
   *bytes = mBody.data();
}

void XmlRpcBody::getBytes(UtlString* bytes, int* length) const
{
   *bytes = mBody;
   *length = bytes->length();
}

bool XmlRpcBody::addValue(UtlContainable* value)
{
   bool result = false;

   UtlString paramValue; 

   // UtlInt
   if (value->isInstanceOf(UtlInt::TYPE))
   {
      UtlInt* pValue = (UtlInt *)value;
      // allow room for the widest possible value, INT_MIN = -2147483648
      char temp[11];
      sprintf(temp, "%d", pValue->getValue());
      paramValue.append(BEGIN_INT);
      paramValue.append(temp);
      paramValue.append(END_INT);
      result = true;
   }
   // UtlLongLongInt
   else if (value->isInstanceOf(UtlLongLongInt::TYPE))
   {
      UtlLongLongInt* pValue = (UtlLongLongInt *)value;
      // always encode these in hex - more readable for values this big
      char temp[19];
      sprintf(temp, "%0#16" PRIx64, pValue->getValue());
      paramValue.append(BEGIN_I8);
      paramValue.append(temp);
      paramValue.append(END_I8);
      result = true;
   }
   else if (value->isInstanceOf(UtlBool::TYPE))
   {
      UtlBool* pValue = (UtlBool *)value;
      paramValue.append(BEGIN_BOOLEAN);
      paramValue.append(pValue->getValue() ? "1" : "0");
      paramValue.append(END_BOOLEAN);
      result = true;
   }
   else if (value->isInstanceOf(UtlString::TYPE))
   {
      UtlString* pValue = (UtlString *)value;

      // Fix XSL-116: XML-RPC must escape special chars in string values
      result = XmlEscape(paramValue, *pValue);

      paramValue.insert(0, BEGIN_STRING);
      paramValue.append(END_STRING);
   }
   else if (value->isInstanceOf(UtlDateTime::TYPE))
   {
      UtlDateTime* pTime = (UtlDateTime *)value;
      OsDateTime time;
      pTime->getTime(time);
      UtlString isoTime;
      time.getIsoTimeStringZ(isoTime);               
      paramValue = BEGIN_TIME + isoTime + END_TIME;
      result = true;
   }
   else if (value->isInstanceOf(UtlHashMap::TYPE))
   {
      result = addStruct((UtlHashMap *)value);
   }
   else if (value->isInstanceOf(UtlSList::TYPE))
   {
      result = addArray((UtlSList *)value);
   }
   else
   {
      assert(false); // unsupported type
   }                     
            
   mBody.append(paramValue);
   return result;
}


bool XmlRpcBody::addArray(UtlSList* array)
{
   bool result = false;
   mBody.append(BEGIN_ARRAY);
   
   UtlSListIterator iterator(*array);
   UtlContainable* pObject;
   while (   (pObject = iterator())
          && (result = addValue(pObject))
          )
   {
   }
   mBody.append(END_ARRAY);
   return result;
}

bool XmlRpcBody::addStruct(UtlHashMap* members)
{
   bool result = true;
   mBody.append(BEGIN_STRUCT);
   
   UtlHashMapIterator iterator(*members);
   UtlString* pName;
   UtlContainable* pObject;
   UtlString structName;
   while (result && (pName = (UtlString *)iterator()))
   {
      mBody.append(BEGIN_MEMBER);

      structName = BEGIN_NAME + *pName + END_NAME;
      mBody.append(structName); 
      
      pObject = members->findValue(pName);
      result = addValue(pObject);
      mBody.append(END_MEMBER);
   }
   
   mBody.append(END_STRUCT);
   return result;
}


/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */


/* ============================ FUNCTIONS ================================= */
