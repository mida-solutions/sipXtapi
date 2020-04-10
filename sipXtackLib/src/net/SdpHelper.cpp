//
// Copyright (C) 2008-2020 SIPez LLC.  All rights reserved.
//
// Copyright (C) 2007 Plantronics
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2007-2008 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// $$
///////////////////////////////////////////////////////////////////////////////
// Author: Scott Godin (sgodin AT SipSpectrum DOT com)

#ifndef EXCLUDE_SIPX_SDP_HELPER

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include <net/NetBase64Codec.h>
#include <sdp/SdpCodec.h>
#include <net/SdpHelper.h>
#include <net/SdpBody.h>
#include <sdp/Sdp.h>
#include <sdp/SdpMediaLine.h>
#include <sdp/SdpCandidate.h>


// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define MAXIMUM_MEDIA_TYPES 30 // should match values from SdpBody.cpp
#define MAXIMUM_VIDEO_SIZES 6
#define MAXIMUM_CANDIDATES 20

// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */

SdpMediaLine::SdpCryptoSuiteType 
SdpHelper::convertCryptoSuiteType(int sdpBodyType)
{
   switch(sdpBodyType)
   { 
   case AES_CM_128_HMAC_SHA1_80:
      return SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_80;
   case AES_CM_128_HMAC_SHA1_32:
      return SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_32;
   case F8_128_HMAC_SHA1_80:
      return SdpMediaLine::CRYPTO_SUITE_TYPE_F8_128_HMAC_SHA1_80;
   default:
      return SdpMediaLine::CRYPTO_SUITE_TYPE_NONE;
   }
}

Sdp* SdpHelper::createSdpFromSdpBody(const SdpBody& sdpBody, const SdpCodecList* codecFactory)
{
   bool rtcpEnabled = true;
   Sdp* sdp = new Sdp();

   // !slg! Note:  the current implementation of SdpBody does not allow access to all of the data in the SDP body
   // in the future codec similar to that present in SdpHelperResip should be implemented

   // Get Bandwidth - !slg! Current SdpBody implementation does not support getting non CT bandwidths or to get medialine specific bandwidths
   int bandwidth;
   if(sdpBody.getBandwidthField(bandwidth))
   {
      sdp->addBandwidth(Sdp::BANDWIDTH_TYPE_CT, (unsigned int) bandwidth);
   }

   // Iterate through the m= lines
   int i;
   for(i = 0; i < sdpBody.getMediaSetCount(); i++)
   {
      SdpMediaLine* mediaLine = new SdpMediaLine();

      // Get data out of SdpBody and stuff it into SdpMediaLine for the i'th
      // media line in the SdpBody.
      getMediaLine(sdpBody, i, *mediaLine, codecFactory);

      // Add the media line to the sdp
      sdp->addMediaLine(mediaLine);
   }

   return sdp;
}

UtlBoolean SdpHelper::getMediaLine(const SdpBody& sdpBody, int mediaLineIndex, SdpMediaLine& mediaLine,
                                   const SdpCodecList* codecFactory)
{
    UtlString utlString;  // Temp String
    UtlString mediaType;
    int mediaPort=0;
    int mediaNumPorts=0;
    UtlString mediaTransportType;
    int numPayloadTypes=0;
    int payloadTypes[MAXIMUM_MEDIA_TYPES];
    UtlBoolean foundMline =
        sdpBody.getMediaData(mediaLineIndex, &mediaType, &mediaPort, &mediaNumPorts, 
                &mediaTransportType, MAXIMUM_MEDIA_TYPES, &numPayloadTypes, payloadTypes);

    mediaLine.setMediaType(SdpMediaLine::getMediaTypeFromString(mediaType.data()));
    mediaLine.setTransportProtocolType(SdpMediaLine::getTransportProtocolTypeFromString(mediaTransportType.data()));

    // Get PTime for media line to assign to codecs
    int mediaLinePtime=0;
    sdpBody.getPtime(mediaLineIndex, mediaLinePtime);

    // Iterate Through Codecs
    {
        int typeIndex;

        for(typeIndex = 0; typeIndex < numPayloadTypes; typeIndex++)
        {
            UtlString mimeSubType;
            UtlString payloadFormat;
            int sampleRate = 0;
            int numChannels = 0;
            const SdpCodec* matchingCodec = NULL;
            int ptime = 0;
            int numVideoSizes = MAXIMUM_VIDEO_SIZES;
            int videoSizes[MAXIMUM_VIDEO_SIZES];
            sdpBody.getPayloadFormat(mediaLineIndex, payloadTypes[typeIndex], payloadFormat);
            SdpCodec::getVideoSizes(payloadFormat, MAXIMUM_VIDEO_SIZES, numVideoSizes, videoSizes);

            if(!sdpBody.getPayloadRtpMap(mediaLineIndex, payloadTypes[typeIndex], mimeSubType, sampleRate, numChannels))
            {

               if(codecFactory == NULL)
               {
                   // static codecs as defined in RFC 3551
                   switch(payloadTypes[typeIndex])
                   {
                   case 0:
                      mimeSubType = "PCMU";
                      sampleRate = 8000;
                      break;
                   case 3:
                      mimeSubType = "GSM";
                      sampleRate = 8000;
                      break;
                   case 4:
                      mimeSubType = "G723";
                      sampleRate = 8000;
                      ptime = 30;
                      break;
                   case 5:
                      mimeSubType = "DVI4";
                      sampleRate = 8000;
                      break;
                   case 6:
                      mimeSubType = "DVI4";
                      sampleRate = 16000;
                      break;
                   case 7:
                      mimeSubType = "LPC";
                      sampleRate = 8000;
                      break;
                   case 8:
                      mimeSubType = "PCMA";
                      sampleRate = 8000;
                      break;
                   case 9:
                      mimeSubType = "G722";
                      sampleRate = 16000;
                      break;
                   case 10:
                      mimeSubType = "L16";
                      sampleRate = 44100;
                      numChannels = 2;
                      break;
                   case 11:
                      mimeSubType = "L16";
                      sampleRate = 44100;
                      break;
                   case 12:
                      mimeSubType = "QCELP";
                      sampleRate = 8000;
                      break;
                   case 13:
                      mimeSubType = "CN";
                      sampleRate = 8000;
                      break;
                   case 14:
                      mimeSubType = "MPA";
                      sampleRate = 90000;
                      break;
                   case 15:
                      mimeSubType = "G728";
                      sampleRate = 8000;
                      break;
                   case 16:
                      mimeSubType = "DVI4";
                      sampleRate = 11025;
                      break;
                   case 17:
                      mimeSubType = "DVI4";
                      sampleRate = 22050;
                      break;
                   case 18:
                      mimeSubType = "G729";
                      sampleRate = 8000;
                      break;
                   case 25:
                      mimeSubType = "CelB";
                      sampleRate = 90000;
                      break;
                   case 26:
                      mimeSubType = "JPEG";
                      sampleRate = 90000;
                      break;
                   case 28:
                      mimeSubType = "nv";
                      sampleRate = 90000;
                      break;
                   case 31:
                      mimeSubType = "H261";
                      sampleRate = 90000;
                      break;
                   case 32:
                      mimeSubType = "MPV";
                      sampleRate = 90000;
                      break;
                   case 33:
                      mimeSubType = "MP2T";
                      sampleRate = 90000;
                      break;
                   case 34:
                      mimeSubType = "H263";
                      sampleRate = 90000;
                      break;
                   }
               }
               else
               {
                   if(payloadTypes[typeIndex] <= SdpCodec::SDP_CODEC_MAXIMUM_STATIC_CODEC)
                   {
                      matchingCodec = codecFactory->getCodecByType(payloadTypes[typeIndex]);
                   }
               }
            }
            else
            {
                // TODO: A lot of this should probably go in a codec factory method

                // Workaround RFC bug with G.722 samplerate.
                // Read RFC 3551 Section 4.5.2 "G722" for details.
                if (mimeSubType.compareTo(MIME_SUBTYPE_G722, UtlString::ignoreCase) == 0)
                {
                   sampleRate = 16000;
                }

                else if(mimeSubType.compareTo(MIME_SUBTYPE_ILBC, UtlString::ignoreCase) == 0)
                {
                    // TODO: move this to SdpCodec
                    /*
                    if(fmtpMode == 0)
                    {
                        fmtpMode = 20;
                    }
                    if(fmtpMode == 20 || fmtpMode == 30)
                    {
                        ptime = fmtpMode;
                    }

                    else if(codecFactory)
                    {
                        // Other iLBC framing not supported, prevent match
                        mimeSubType = "";
                    }
                    */
                }

                if(numChannels == -1) // Note:  SdpBody returns -1 if no numChannels is specified - default should be one
                {
                    numChannels = 1;
                }

                if(codecFactory)
                {
                    // Find a match in the factory to the given codec parameters
                    matchingCodec = codecFactory->getCodec(mediaType.data(),
                                                       mimeSubType.data(),
                                                       sampleRate,
                                                       numChannels,
                                                       payloadFormat);
                }

            }

            if(codecFactory == NULL)
            {
                SdpCodec* codec = new SdpCodec(payloadTypes[typeIndex], 
                                               mediaType.data(), 
                                               mimeSubType.data(), 
                                               sampleRate, 
                                               ptime * 1000, 
                                               numChannels, 
                                               payloadFormat);
                mediaLine.addCodec(codec);
            }
            else
            {
                if(matchingCodec)
                {
                    SdpCodec* codecToAdd = new SdpCodec(*matchingCodec);
                    // Need to use payload ID of remote side, not that set in codec factory
                    codecToAdd->setCodecPayloadFormat(payloadTypes[typeIndex]);
                    codecToAdd->setPacketSize((ptime ? ptime : mediaLinePtime) * 1000);
                    mediaLine.addCodec(codecToAdd);
                }
            }
         }
      }

      // Add Connection
      UtlString mediaAddress;
      sdpBody.getMediaAddress(mediaLineIndex, &mediaAddress);
      // TODO:
      //sdpBody.getMediaNetworkType(mediaLineIndex, &utlString);  !slg! not implemented in SdpBody
      //Sdp::SdpAddressType addrType = Sdp::getAddressTypeFromString(utlString.data());
      Sdp::SdpAddressType addrType = Sdp::ADDRESS_TYPE_IP4;
      if(mediaPort != 0)
      {
         int j;
         for(j = 0; j < mediaNumPorts; j++)
         {
            mediaLine.addConnection(Sdp::NET_TYPE_IN, addrType, mediaAddress.data(), mediaPort + (2 * j));
         }
      }

      // Add Rtcp Connection
      int rtcpPort = 0;
      sdpBody.getMediaRtcpPort(mediaLineIndex, &rtcpPort);
      if(rtcpPort != 0)
      {
         int j;
         for(j = 0; j < mediaNumPorts; j++)
         {
            mediaLine.addRtcpConnection(Sdp::NET_TYPE_IN, addrType, mediaAddress.data(), rtcpPort + (2 * j));
         }
      }      

      // Set direction, a=sendrecv, a=sendonly, a=recvonly, a=inactive
      // !slg! Note: SdpBody does not currenlty support reading attributes from a particular media line
      SdpMediaLine::SdpDirectionType direction = SdpMediaLine::DIRECTION_TYPE_SENDRECV; // default
      if(sdpBody.findValueInField("a", "sendrecv"))
      {
         direction = SdpMediaLine::DIRECTION_TYPE_SENDRECV;
      }
      else if(sdpBody.findValueInField("a", "sendonly"))
      {
         direction = SdpMediaLine::DIRECTION_TYPE_SENDONLY;
      }
      else if(sdpBody.findValueInField("a", "recvonly"))
      {
         direction = SdpMediaLine::DIRECTION_TYPE_RECVONLY;
      }
      else if(sdpBody.findValueInField("a", "inactive"))
      {
         direction = SdpMediaLine::DIRECTION_TYPE_INACTIVE;
      }
      mediaLine.setDirection(direction);

      UtlString trackId;
      if(sdpBody.getControlTrackId(mediaLineIndex, trackId))
      {
          mediaLine.setControlTrackId(trackId);
      }

      int frameRate;
      if(sdpBody.getFramerateField(mediaLineIndex, frameRate))
      {
         mediaLine.setFrameRate((unsigned int)frameRate);
      }

      // TCP Setup Attribute - !slg! SdpBody currently does not support getting this for a particular media line
      mediaLine.setTcpSetupAttribute(SdpMediaLine::getTcpSetupAttributeFromString(sdpBody.getRtpTcpRole().data()));

      // Get the SRTP Crypto Settings
      SdpSrtpParameters srtpParameters;
      int index=1;
      while(sdpBody.getSrtpCryptoField(mediaLineIndex, index, srtpParameters))
      {
         SdpMediaLine::SdpCrypto* sdpCrypto = new SdpMediaLine::SdpCrypto;
         sdpCrypto->setTag(index);
         sdpCrypto->setSuite(convertCryptoSuiteType(srtpParameters.cipherType));
         UtlString encodedKey;  // Note:  Key returned from SdpBody is Base64 decoded, but Sdp container expects encoded key
         NetBase64Codec::encode(sizeof(srtpParameters.masterKey)-1, (const char*)srtpParameters.masterKey, encodedKey);
         sdpCrypto->addCryptoKeyParam(SdpMediaLine::CRYPTO_KEY_METHOD_INLINE, encodedKey.data()); 
         sdpCrypto->setEncryptedSrtp((srtpParameters.securityLevel & SRTP_ENCRYPTION) != 0);
         sdpCrypto->setAuthenticatedSrtp((srtpParameters.securityLevel & SRTP_AUTHENTICATION) != 0);
         mediaLine.addCryptoSettings(sdpCrypto); 
         index++;
      }

      // Get Ice Candidate(s) - !slg! note: ice candidate support in SdpBody is old and should be updated - at which time the following code
      // should also be updated      
      int candidateIds[MAXIMUM_CANDIDATES];
      UtlString transportIds[MAXIMUM_CANDIDATES];
      UtlString transportTypes[MAXIMUM_CANDIDATES];
      uint64_t qvalues[MAXIMUM_CANDIDATES];
      UtlString candidateIps[MAXIMUM_CANDIDATES];
      int candidatePorts[MAXIMUM_CANDIDATES];
      int numCandidates;

      if(sdpBody.getCandidateAttributes(mediaLineIndex, MAXIMUM_CANDIDATES, candidateIds, transportIds, transportTypes, qvalues, candidateIps, candidatePorts, numCandidates))
      {
         UtlString userFrag;  // !slg! Currently no way to retrieve these
         UtlString password;

         // TODO:
         //mediaLine.setIceUserFrag(userFrag.data());
         //mediaLine.setIcePassword(password.data());
         int idx;
         for(idx = 0; idx < numCandidates; idx++)
         {
            mediaLine.addCandidate(transportIds[idx].data(), 
                                    candidateIds[idx], 
                                    SdpCandidate::getCandidateTransportTypeFromString(transportTypes[idx].data()), 
                                    qvalues[idx], 
                                    candidateIps[idx].data(), 
                                    candidatePorts[idx], 
                                    SdpCandidate::CANDIDATE_TYPE_NONE); 
         }
      }

    return(foundMline);
}


/* ============================ ACCESSORS ================================= */


/* ============================ INQUIRY =================================== */


/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

#endif

