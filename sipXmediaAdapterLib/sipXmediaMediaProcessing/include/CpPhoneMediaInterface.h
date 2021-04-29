// 
// Copyright (C) 2005-2015 SIPez LLC.  All rights reserved.
// 
// Copyright (C) 2004-2009 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
//////////////////////////////////////////////////////////////////////////////

// Author: Dan Petrie (dpetrie AT SIPez DOT com)

#ifndef _CpPhoneMediaInterface_h_
#define _CpPhoneMediaInterface_h_

// SYSTEM INCLUDES
//#include <>

// APPLICATION INCLUDES
#include <os/OsStatus.h>
#include <os/OsDefs.h>
#include <net/QoS.h>
#include <sdp/SdpCodecList.h>
#include "mi/CpMediaInterface.h"
#include "MaNotfTranslatorDispatcher.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class MpCallFlowGraph;
class SdpCodec;
class OsDatagramSocket;
class CpPhoneMediaConnection;
class CircularBufferPtr;

/**
*  An older media interface
*/
class CpPhoneMediaInterface : public CpMediaInterface
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */

     /// @brief Constructor
   CpPhoneMediaInterface(CpMediaInterfaceFactoryImpl* pFactoryImpl,
                         uint32_t samplesPerFrame = 0,
                         uint32_t samplesPerSec = 0,
                         const char* publicAddress = NULL, 
                         const char* localAddress = NULL,
                         int numCodecs = 0, 
                         SdpCodec* sdpCodecArray[] = NULL,
                         const char* pLocale = "",
                         int expeditedIpTos = QOS_LAYER3_LOW_DELAY_IP_TOS,
                         const char* szStunServer = NULL,
                         int iStunPort = PORT_NONE,
                         int iStunKeepAlivePeriodSecs = 28,
                         const char* szTurnServer = NULL,
                         int iTurnPort = PORT_NONE,
                         const char* szTurnUsername = NULL,
                         const char* szTurnPassword = NULL,
                         int iTurnKeepAlivePeriodSecs = 28,
                         UtlBoolean bEnableICE = FALSE,
                         OsMsgDispatcher* pDispatcher = NULL
                        );

protected:
     /// @brief Destructor
   virtual ~CpPhoneMediaInterface();

public:

     /// @brief Public interface for destroying this media interface
   void release();

/* ============================ MANIPULATORS ============================== */


   virtual OsStatus createConnection(int& connectionId,
                                     const char* szLocalAddress,
                                     int localPort = 0,
                                     void* videoWindowHandle = NULL,
                                     void* const pSecurityAttributes = NULL,
                                     const RtpTransportOptions rtpTransportOptions=RTP_TRANSPORT_UDP);

     /// @copydoc CpMediaInterface::setPlcMethod()
   virtual OsStatus setPlcMethod(int connectionId,
                                 const UtlString &methodName="");
   
   virtual OsStatus getCapabilities(int connectionId, 
                                    UtlString& rtpHostAddress, 
                                    int& rtpAudioPort,
                                    int& rtcpAudioPort,
                                    int& rtpVideoPort,
                                    int& rtcpVideoPort,
                                    SdpCodecList& supportedCodecs,
                                    SdpSrtpParameters& srtpParams,
                                    int bandWidth,
                                    int& videoBandwidth,
                                    int& videoFramerate);

   virtual OsStatus getCapabilitiesEx(int connectionId, 
                                      int nMaxAddresses,
                                      UtlString rtpHostAddresses[], 
                                      int rtpAudioPorts[],
                                      int rtcpAudioPorts[],
                                      int rtpVideoPorts[],
                                      int rtcpVideoPorts[],
                                      RTP_TRANSPORT transportTypes[],
                                      int& nActualAddresses,
                                      SdpCodecList& supportedCodecs,
                                      SdpSrtpParameters& srtpParameters,
                                      int bandWidth,
                                      int& videoBandwidth,
                                      int& videoFramerate);

   virtual OsMsgDispatcher*
   setNotificationDispatcher(OsMsgDispatcher* pNotificationDispatcher);

   virtual OsStatus setNotificationsEnabled(bool enabled, 
                                            const UtlString& resourceName);

   virtual OsStatus setConnectionDestination(int connectionId,
                                             const char* rtpHostAddress, 
                                             int rtpAudioPort,
                                             int rtcpAudioPort,
                                             int rtpVideoPort,
                                             int rtcpVideoPort);

   virtual OsStatus startRtpSend(int connectionId, 
                                 int numCodecs,
                                 SdpCodec* sendCodec[]);

   virtual OsStatus startRtpReceive(int connectionId,
                                    int numCodecs,
                                    SdpCodec* sendCodec[]);

   virtual OsStatus stopRtpSend(int connectionId);
   virtual OsStatus stopRtpReceive(int connectionId);

   virtual OsStatus deleteConnection(int connectionId);

   virtual OsStatus startTone(int toneId, UtlBoolean local, UtlBoolean remote);
   virtual OsStatus stopTone();

     /// @copydoc CpMediaInterface::setRtcpTimeOffset()
   virtual OsStatus setRtcpTimeOffset(int connectionId,
                                      CpMediaInterface::MEDIA_STREAM_TYPE mediaType,
                                      int streamIndex,
                                      int timeOffset);

   virtual OsStatus startChannelTone(int connectionId, int toneId, UtlBoolean local, UtlBoolean remote);
   virtual OsStatus stopChannelTone(int connectionId);

   virtual OsStatus playAudio(const char* url, 
                              UtlBoolean repeat,
                              UtlBoolean local, 
                              UtlBoolean remote,
                              UtlBoolean mixWithMic = false,
                              int downScaling = 100,
                              UtlBoolean autoStopOnFinish = TRUE);


   virtual OsStatus playBuffer(char* buf, 
                               unsigned long bufSize,
                               uint32_t bufRate, 
                               int type, 
                               UtlBoolean repeat,
                               UtlBoolean local, 
                               UtlBoolean remote,
                               OsProtectedEvent* event = NULL,
                               UtlBoolean mixWithMic = false,
                               int downScaling = 100,
                               UtlBoolean autoStopOnFinish = TRUE);

     /// @copydoc CpMediaInterface::pauseAudio()
   virtual OsStatus pauseAudio();

     /// @copydoc CpMediaInterface::resumeAudio()
   virtual OsStatus resumeAudio();

     /// @copydoc CpMediaInterface::stopAudio()
   virtual OsStatus stopAudio();

   virtual OsStatus playChannelAudio(int connectionId,
                                    const char* url,
                                    UtlBoolean repeat,
                                    UtlBoolean local,
                                    UtlBoolean remote,
                                    UtlBoolean mixWithMic = false,
                                    int downScaling = 100,
                                    UtlBoolean autoStopOnFinish = TRUE);


   virtual OsStatus stopChannelAudio(int connectionId);


   virtual OsStatus recordChannelAudio(int connectionId,
                                       const char* szFile);

   virtual OsStatus stopRecordChannelAudio(int connectionId);

   virtual OsStatus recordBufferChannelAudio(int connectionId,
                                             char* pBuffer,
                                             int bufferSize,
                                             int maxRecordTime = -1,
                                             int maxSilence = -1) ;

   virtual OsStatus stopRecordBufferChannelAudio(int connectionId) ;

   virtual OsStatus recordCircularBufferChannelAudio(int connectionId,
                                                     CircularBufferPtr & buffer,
                                                     CpMediaInterface::CpAudioFileFormat recordingFormat,
                                                     unsigned long recordingBufferNotificationWatermark);

   virtual OsStatus stopRecordCircularBufferChannelAudio(int connectionId);

   virtual OsStatus createPlayer(MpStreamPlayer** ppPlayer, 
                                 const char* szStream, 
                                 int flags, 
                                 OsMsgQ *pMsgQ = NULL, 
                                 const char* szTarget = NULL) ;
   virtual OsStatus destroyPlayer(MpStreamPlayer* pPlayer);
   virtual OsStatus createPlaylistPlayer(MpStreamPlaylistPlayer** 
                                         ppPlayer, 
                                         OsMsgQ *pMsgQ = NULL, 
                                         const char* szTarget = NULL);
   virtual OsStatus destroyPlaylistPlayer(MpStreamPlaylistPlayer* pPlayer);
   virtual OsStatus createQueuePlayer(MpStreamQueuePlayer** ppPlayer, 
                                      OsMsgQ *pMsgQ = NULL, 
                                      const char* szTarget = NULL);
   virtual OsStatus destroyQueuePlayer(MpStreamQueuePlayer* pPlayer);

   virtual OsStatus giveFocus();
   virtual OsStatus defocus();

     /// @brief Limits the available codecs to only those within the designated limit
   virtual void setCodecCPULimit(int iLimit);

     /// @copydoc CpMediaInterface::setMicWeightOnBridge()
   virtual OsStatus setMicGain(float gain) {return OS_NOT_YET_IMPLEMENTED;} ;

     /// @copydoc CpMediaInterface::recordMic(int,int16_t*,int)
   virtual OsStatus recordMic(int ms, int16_t* pAudioBuf, int bufferSize);

     /// @copydoc CpMediaInterface::recordMic(int, int, const char*)
   virtual OsStatus recordMic(int ms,
                              int silenceLength,
                              const char* fileName); 

     /// @brief Set the contact type for this Phone media interface
   virtual void setContactType(int connectionId, SIPX_CONTACT_TYPE eType, SIPX_CONTACT_ID contactId);
     /**<
     *  It is important to set the contact type BEFORE creating the connection
     *  -- setting after the connection has been created is essentially a NOP.
     */

     /// @brief Rebuild the codec factory on the fly
   virtual OsStatus setAudioCodecBandwidth(int connectionId, int bandWidth);

   virtual OsStatus rebuildCodecFactory(int connectionId, 
                                        int audioBandwidth, 
                                        int videoBandwidth, 
                                        UtlString& videoCodec);

     /// @brief Set connection bitrate on the fly
   virtual OsStatus setConnectionBitrate(int connectionId, int bitrate);

     /// @brief Set connection framerate on the fly
   virtual OsStatus setConnectionFramerate(int connectionId, int framerate);

   virtual OsStatus setSecurityAttributes(const void* security);

   virtual OsStatus addAudioRtpConnectionDestination(int connectionId,
                                                     int iPriority,
                                                     const char* candidateIp, 
                                                     int candidatePort);

   virtual OsStatus addAudioRtcpConnectionDestination(int connectionId,
                                                      int iPriority,
                                                      const char* candidateIp, 
                                                      int candidatePort);

   virtual OsStatus addVideoRtpConnectionDestination(int connectionId,
                                                     int iPriority,
                                                     const char* candidateIp, 
                                                     int candidatePort);

   virtual OsStatus addVideoRtcpConnectionDestination(int connectionId,
                                                      int iPriority,
                                                      const char* candidateIp, 
                                                      int candidatePort);
    
   virtual void setConnectionTcpRole(const int connectionId, const RtpTcpRoles role)
   {
      // NOT IMPLEMENTED
   }

   virtual OsStatus generateVoiceQualityReport(int connectiond,
                                               const char* callId,
                                               UtlString& report);

   /// @copydoc CpMediaInterface::setMixWeightsForOutput()
   virtual OsStatus setMixWeightsForOutput(int bridgeOutputPort, int numWeights, float weights[]);

/* ============================ ACCESSORS ================================= */

     /// @brief Calculate the current cost for our sending/receiving codecs
   virtual int getCodecCPUCost();

     /// @brief Calculate the worst cost for our sending/receiving codecs
   virtual int getCodecCPULimit();

     /// @copydoc CpMediaInterface::getSamplesPerSec()
   virtual uint32_t getSamplesPerSec();

     /// @copydoc CpMediaInterface::getSamplesPerFrame()
   virtual uint32_t getSamplesPerFrame();

     /// @brief Returns the flowgraph's message queue
   virtual OsMsgQ* getMsgQ();

     /// @copydoc CpMediaInterface::getNotificationDispatcher()
   virtual OsMsgDispatcher* getNotificationDispatcher();

   virtual OsStatus getVideoQuality(int& quality);
   virtual OsStatus getVideoBitRate(int& bitRate);
   virtual OsStatus getVideoFrameRate(int& frameRate);

     /// @brief Returns primary codec for the connection
   virtual OsStatus setVideoWindowDisplay(const void* hWnd);
   virtual const void* getVideoWindowDisplay();

     /// @brief Set a media property on the media interface
   virtual OsStatus setMediaProperty(const UtlString& propertyName,
                                     const UtlString& propertyValue);

     /// @brief Get a media property on the media interface
   virtual OsStatus getMediaProperty(const UtlString& propertyName,
                                     UtlString& propertyValue);

     /// @brief Set a media property associated with a connection
   virtual OsStatus setMediaProperty(int connectionId,
                                     const UtlString& propertyName,
                                     const UtlString& propertyValue);

     /// @brief Get a media property associated with a connection
   virtual OsStatus getMediaProperty(int connectionId,
                                     const UtlString& propertyName,
                                     UtlString& propertyValue);



   virtual OsStatus setVideoQuality(int quality);
   virtual OsStatus setVideoParameters(int bitRate, int frameRate);

     /// @brief Returns the primary codec for the connection
   virtual OsStatus getPrimaryCodec(int connectionId, 
                                    UtlString& audioCodec,
                                    UtlString& videoCodec,
                                    int* audiopPayloadType,
                                    int* videoPayloadType,
                                    bool& isEncrypted);

   virtual UtlString getType() { return "CpPhoneMediaInterface"; };

/* ============================ INQUIRY =================================== */

   virtual UtlBoolean isSendingRtpAudio(int connectionId);
   virtual UtlBoolean isReceivingRtpAudio(int connectionId);
   virtual UtlBoolean isSendingRtpVideo(int connectionId);
   virtual UtlBoolean isReceivingRtpVideo(int connectionId);
   virtual UtlBoolean isDestinationSet(int connectionId);   
   virtual UtlBoolean canAddParty();
   virtual UtlBoolean isVideoInitialized(int connectionId);
   virtual UtlBoolean isAudioInitialized(int connectionId);
   virtual UtlBoolean isAudioAvailable();
   virtual UtlBoolean isVideoConferencing() { return false; };


/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   UtlBoolean getLocalAddresses(int connectionId,
                                UtlString& hostIp,
                                int& rtpAudioPort,
                                int& rtcpAudioPort,
                                int& rtpVideoPort,
                                int& rtcpVideoPort);

   UtlBoolean getNatedAddresses(int connectionId,
                                UtlString& hostIp,
                                int& rtpAudioPort,
                                int& rtcpAudioPort,
                                int& rtpVideoPort,
                                int& rtcpVideoPort);


   UtlBoolean getRelayAddresses(int connectionId,
                                UtlString& hostIp,
                                int& rtpAudioPort,
                                int& rtcpAudioPort,
                                int& rtpVideoPort,
                                int& rtcpVideoPort);


   OsStatus addLocalContacts(int connectionId, 
                             int nMaxAddresses,
                             UtlString rtpHostAddresses[], 
                             int rtpAudioPorts[],
                             int rtcpAudioPorts[],
                             int rtpVideoPorts[],
                             int rtcpVideoPorts[],
                             int& nActualAddresses);

   OsStatus addNatedContacts(int connectionId, 
                             int nMaxAddresses,
                             UtlString rtpHostAddresses[], 
                             int rtpAudioPorts[],
                             int rtcpAudioPorts[],
                             int rtpVideoPorts[],
                             int rtcpVideoPorts[],
                             int& nActualAddresses);

   OsStatus addRelayContacts(int connectionId, 
                             int nMaxAddresses,
                             UtlString rtpHostAddresses[], 
                             int rtpAudioPorts[],
                             int rtcpAudioPorts[],
                             int rtpVideoPorts[],
                             int rtcpVideoPorts[],
                             int& nActualAddresses);

   void applyAlternateDestinations(int connectionId);

     /// @brief Create socket pair for RTP/RTCP streams.
   OsStatus createRtpSocketPair(UtlString localAddress,
                                int localPort,
                                SIPX_CONTACT_TYPE contactType,
                                OsDatagramSocket* &rtpSocket,
                                OsDatagramSocket* &rtcpSocket);
     /**<
     *  For RTP/RTCP port pair will be set next free port pair.
     *  
     *  @param[in] localAddress - Address to bind to (for multihomed hosts).
     *  @param[in] localPort - Local port to bind to (0 for auto select).
     *  @param[in] contactType - Contact type (see SIPX_CONTACT_TYPE).
     *  @param[out] rtpSocket - Created socket for RTP stream.
     *  @param[out] rtcpSocket - Created socket for RTCP stream.
     */


/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   CpPhoneMediaConnection* getMediaConnection(int connectionId);
   CpPhoneMediaConnection* removeMediaConnection(int connectionId);
   OsStatus doDeleteConnection(CpPhoneMediaConnection* mediaConnection);

   UtlString mRtpReceiveHostAddress; ///< Advertised as place to send RTP/RTCP
   UtlString mLocalAddress;          ///< Address on which ports are bound
   MpCallFlowGraph* mpFlowGraph;     ///< Flowgraph for audio part of call
   UtlBoolean mRingToneFromFile;
   SdpCodecList mSupportedCodecs;
   UtlDList mMediaConnections;
   int mDefaultMaxMcastRtpStreams;
   int mExpeditedIpTos;
   UtlString mStunServer ;
   int mStunPort;
   int mStunRefreshPeriodSecs ;
   UtlString mTurnServer ;
   int mTurnPort ;
   int mTurnRefreshPeriodSecs ;
   UtlString mTurnUsername ;
   UtlString mTurnPassword ;
   UtlBoolean mEnableIce ;
   UtlHashMap mInterfaceProperties;
   MaNotfTranslatorDispatcher mTranslatorDispatcher;  ///< Dispatcher for translating
      ///< mediaLib notification messages into abstract MediaAdapter ones.
      ///< Only used if a dispatcher is set on this interface.

     /// @brief Disabled copy constructor
   CpPhoneMediaInterface(const CpPhoneMediaInterface& rCpPhoneMediaInterface);

     /// @brief Disabled equals operator
   CpPhoneMediaInterface& operator=(const CpPhoneMediaInterface& rhs);

};

/* ============================ INLINE METHODS ============================ */
#endif  // _CpPhoneMediaInterface_h_
