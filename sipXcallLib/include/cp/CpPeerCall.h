//
// Copyright (C) 2006-2017 SIPez LLC.  All rights reserved.
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


#ifndef _CpPeerCall_h_
#define _CpPeerCall_h_

// SYSTEM INCLUDES
//#include <...>

// APPLICATION INCLUDES
//#include <cp/PhoneApplication.h>
#include <cp/CpCall.h>
#include <cp/Connection.h>

#include <os/OsRWMutex.h>
#include <cp/CallManager.h>

#include "tapi/sipXtapiEvents.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS

// Container class to cache pass through address and port info
class MediaStreamPassThroughData
{
public:
    MediaStreamPassThroughData(CpMediaInterface::MEDIA_STREAM_TYPE mediaType,
                               int mediaTypeStreamIndex,
                               UtlString mediaRecieveAddress,
                               int rtpPort,
                               int rtcpPort) :
     mMediaType(mediaType),
     mMediaTypeStreamIndex(mediaTypeStreamIndex),
     mMediaRecieveAddress(mediaRecieveAddress),
     mRtpPort(rtpPort),
     mRtcpPort(rtcpPort)
    {};

    MediaStreamPassThroughData(const MediaStreamPassThroughData& ref) :
     mMediaType(ref.mMediaType),
     mMediaTypeStreamIndex(ref.mMediaTypeStreamIndex),
     mMediaRecieveAddress(ref.mMediaRecieveAddress),
     mRtpPort(ref.mRtpPort),
     mRtcpPort(ref.mRtcpPort)
    {};

    virtual ~MediaStreamPassThroughData(){};

    CpMediaInterface::MEDIA_STREAM_TYPE mMediaType;
    int mMediaTypeStreamIndex;
    UtlString mMediaRecieveAddress;
    int mRtpPort;
    int mRtcpPort;

private:
    MediaStreamPassThroughData();
};

// TYPEDEFS
// FORWARD DECLARATIONS
class Connection;
class SipUserAgent;

//:Class short description which may consist of multiple lines (note the ':')
// Class detailed description which may extend to multiple lines
class CpPeerCall  : public CpCall

{
    /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

    enum callDialingMode
    {
        UNKNOWN = 0,
        ADD_PARTY,
        BLIND_TRANSFER
    };

    /* ============================ CREATORS ================================== */

    CpPeerCall(UtlBoolean isEarlyMediaFor180Enabled = TRUE,
        CpCallManager* callManger = NULL,
        CpMediaInterface* callMediaInterface = NULL,
        int callIndex = -1,
        const char* callId = NULL,
        SipUserAgent* sipUA = NULL,
        int sipSessionReinviteTimer = 0,
        const char* defaultCallExtension = NULL,
        int holdType = CallManager::NEAR_END_HOLD,
        int offeringDelayMilliSeconds = Connection::IMMEDIATE,
        int availableBehavior = Connection::RING,
        const char* forwardUnconditionalUrl = NULL,
        int busyBehavior = Connection::BUSY,
        const char* forwardOnBusyUrl = NULL,
        int forwardOnNoAnswerMilliSeconds = -1,
        const char* forwardOnNoAnswerUrl = NULL,
        int ringingExpireSeconds = CP_MAXIMUM_RINGING_EXPIRE_SECONDS /* = 180 */);
    //:Default constructor
    //! param: callManager - the call processing task manager
    //! param: sipUA - SIP user agent task
    //! param: defaultCallExtension - the local user name/phone extension
    //! param: holdType - NEAR_SIDE_HOLD hold is handled in the media layer to mute the media in both directions, FAR_SIDE_HOLD hold is handled at the call control protocol layer
    //! param: offeringDelayMilliSeconds - the period of time that the call stays in offering before proceeding to the next state (i.e. ALERTING). A value of -1 indicates a delay of forever.  Typically the user or an application acting on behalf of the user will take the call out of offering by accepting or regjecting the call.
    //! param: availableBehavior - defines the behavior of incoming calls after the offering delay times out and the phone (and resources) is available to take a call. This must be set to one of the lineAvailableBehaviors enumerations.
    //! param: forwardUnconditionalUrl - the URL to which the call is unconditionaly forwarded.  This argument is ignored if availableBehavior is not set to FORWARD_UNCONDITIONAL
    //! param: busyBehavior - defines the behavior of incoming calls after the offering delay times out and the phone (and resources) are busy. This must be set to one of the lineBusyBehaviors enumerations.
    //! param: forwardOnBusyUrl - the URL to which the call is forwarded when the phone is busy.  This argument is ignored if busyBehavior is not set to FORWARD_ON_BUSY
    //! param: forwardOnNoAnswerMilliSeconds - after a call rings (RING, RING_SILENT or FAKE_RING) for this period of time, forward the call to the URL in forwardOnNoAnswerUrl. A value of -1 indicates never.
    //! param: forwardOnNoAnswerUrl - the URL to which the call is fowarded on no answer after ringing.
    virtual
        ~CpPeerCall();
    //:Destructor

    /* ============================ MANIPULATORS ============================== */

    virtual void inFocus(int talking = 1);
    virtual void outOfFocus();

    //virtual void blindTransfer();

    //virtual void conferenceAddParty();

    Connection* addParty(const char* partyAddress,
		const char* callController,
        const char* originalCallConnectionAddress,
		const char* pNewCallId,
        SIPX_CONTACT_ID contactId = 0,
        const void* pDisplay = NULL,
        const void* pSecurity = NULL,
        const char* locationHeader = NULL,
        const int bandWidth = AUDIO_CODEC_BW_DEFAULT,
        UtlBoolean bOnHold = false,
		const char* originalCallId = NULL,
        SIPX_TRANSPORT_DATA* pTransport = NULL,
        const RTP_TRANSPORT rtpTransportOptions = RTP_TRANSPORT_UDP);

    OsStatus fireSipXMediaEvent(SIPX_MEDIA_EVENT event,
                                SIPX_MEDIA_CAUSE cause,
                                SIPX_MEDIA_TYPE  type,
                                void*            pEventData = NULL,
                                UtlBoolean fireIfNoConnection = FALSE);

    /* ============================ ACCESSORS ================================= */

    virtual void printCall();

    virtual void getLocalAddress(char* address, int len);

    virtual void getLocalTerminalId(char* terminal, int len);

    void hangUp(const char* callId, const char* toTag,
        const char* fromTag);

    UtlBoolean getConnectionState(const char* callId,
        const char* toTag,
        const char* fromTag,
        int&        state,
        UtlBoolean   strictCompare);
    //:Get the connection for the connection identified by the designated
    //:callId, toTag, and fromTag.  If the connection cannot be found a
    //:UtlBoolean value of false is returned.


    /* ============================ INQUIRY =================================== */

    static UtlBoolean shouldCreateCall(SipUserAgent& sipUa,
        OsMsg& message,
        SdpCodecList& codecFactory);

    virtual UtlBoolean hasCallId(const char* callId);

    virtual enum handleWillingness willHandleMessage(const OsMsg& eventMessage);

    virtual UtlBoolean isQueued();

    virtual UtlBoolean isLocalTerminal(const char* terminalId);

    UtlBoolean isConnection(const char* callId, const char* toTag,
        const char* fromTag);

    virtual UtlBoolean canDisconnectConnection(Connection* pConnection);

    UtlBoolean isConnectionLocallyInitiatedRemoteHold(const char* callId, 
                                                      const char* toTag,
                                                      const char* fromTag) ;


    /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
    virtual UtlBoolean handleCallMessage(OsMsg& eventMessage);
    virtual UtlBoolean handleMiNotificationMessage(MiNotification& notification) ;

    UtlBoolean handleRenegotiateCodecsConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_RENEGOTIATE_CODECS_CONNECTION
    //: message
    UtlBoolean handleRenegotiateCodecsAllConnections(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_RENEGOTIATE_CODECS_ALL_CONNECTIONS
    //: message
    UtlBoolean handleSilentRemoteHold(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_SILENT_REMOTE_HOLD
    //: message
    UtlBoolean handleGetCodecCPULimit(OsMsg& eventMessage);
    //: Handles the processing of a CallManager::CP_GET_CODEC_CPU_LIMIT
    //: message
    UtlBoolean handleSetCodecCPULimit(OsMsg& eventMessage);
    //: Handles the processing of a CallManager::CP_SET_CODEC_CPU_LIMIT
    //: message
    UtlBoolean handleGetCodecCPUCost(OsMsg& eventMessage);
    //: Handles the processing of a CallManager::CP_GET_CODEC_CPU_COST
    //: message
    UtlBoolean handleSetMicGain(OsMsg& eventMessage);
    //: Handles the processing of a CallManager::CP_SET_MIC_GAIN
    //: message

    UtlBoolean handleDialString(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_DIAL_STRING message
    UtlBoolean handleDequeueCall(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_DEQUEUED_CALL message
    UtlBoolean handleTransfer(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_BLIND_TRANSFER and
    //: CallManager::CP_CONSULT_TRANSFER messages
    UtlBoolean handleTransferAddress(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_CONSULT_TRANSFER_ADDRESS 
    //: message

    UtlBoolean handleTransferConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_TRANSFER_CONNECTION
    //: message
    UtlBoolean handleTransfereeConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_TRANSFEREE_CONNECTION
    //: message
    UtlBoolean handleSipMessage(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_SIP_MESSAGE message
    UtlBoolean handleDropConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_DROP_CONNECTION message
    UtlBoolean handleForceDropConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_FORCE_DROP_CONNECTION
    //: message
    UtlBoolean handleGetAddresses(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_GET_CALLED_ADDRESSES and
    //: CallManager::CP_GET_CALLING_ADDRESSES messages
    UtlBoolean handleAcceptConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_ACCEPT_CONNECTION
    //: message
    UtlBoolean handleRejectConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_REJECT_CONNECTION
    //: message
    UtlBoolean handleRedirectConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_REDIRECT_CONNECTION
    //: message
    UtlBoolean handleHoldTermConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_HOLD_TERM_CONNECTION
    //: message
    UtlBoolean handleHoldAllTermConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_HOLD_ALL_TERM_CONNECTIONS
    //: message
    UtlBoolean handleUnholdTermConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_UNHOLD_TERM_CONNECTION
    //: message
    UtlBoolean handleTransferConnectionStatus(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_TRANSFER_CONNECTION_STATUS
    //: message
    UtlBoolean handleTransfereeConnectionStatus(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_TRANSFEREE_CONNECTION_STATUS
    //: message
    UtlBoolean handleGetNumConnections(OsMsg* pEventMessage);
    //: Handles the processing of CallManager::CP_GET_NUM_CONNECTIONS
    //: and CallManager::CP_GET_NUM_TERM_CONNECTIONS messages
    UtlBoolean handleGetConnections(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_GET_CONNECTIONS
    //: message
    UtlBoolean handleGetSession(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_GET_SESSION
    //: message
    UtlBoolean handleGetCallState(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_GET_CALLSTATE
    //: message
    UtlBoolean handleGetConnectionState(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_GET_CONNECTIONSTATE
    //: message
    UtlBoolean handleGetNextCseq(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_GET_NEXT_CSEQ
    //: message
    UtlBoolean handleGetTerminalConnectionState(OsMsg* pEventMessage);
    //: Handles the processing of a
    //: CallManager::CP_GET_TERMINALCONNECTIONSTATE message
    UtlBoolean handleIsLocalTerminalConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_IS_LOCAL_TERM_CONNECTION
    //: message
    UtlBoolean handleCancelTimer(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_CANCEL_TIMER
    //: message
    UtlBoolean handleOfferingExpired(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_OFFERING_EXPIRED
    //: message
    UtlBoolean handleRingingExpired(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_RINGING_EXPIRED
    //: message
    UtlBoolean handleUnholdAllTermConnections(OsMsg* pEventMessage);
    //: Handles the processing of a
    //: CallManager::CP_UNHOLD_ALL_TERM_CONNECTIONS message
    UtlBoolean handleUnholdLocalTermConnection(OsMsg* pEventMessage);
    //: Handles the processing of a
    //: CallManager::CP_UNHOLD_LOCAL_TERM_CONNECTION  message
    UtlBoolean handleHoldLocalTermConnection(OsMsg* pEventMessage);
    //: Handles the processing of a CallManager::CP_HOLD_LOCAL_TERM_CONNECTION
    //: message
    UtlBoolean handleGetLocalContacts(OsMsg* pEventMessage);
    //: Handles the procesing of a CP_GET_LOCAL_CONTACTS message

    UtlBoolean handleSendInfo(OsMsg* pEventMessage);
    //: Handles the processing of a CP_INFO message, and sends an INFO message
    
    UtlBoolean handleGetInviteHeaderValue(OsMsg* pEventMessage);
    //: Handles CP_GET_INVITE_HEADER_VALUE event to retrieve SIP INVITE header value

    UtlBoolean handleGetMediaConnectionId(OsMsg* pEventMessage);
    //: Handles the processing of a CP_GET_MEDIA_CONNECTION_ID message

    UtlBoolean handleLimitCodecs(OsMsg* pEventMessage);
    //: Handles the processing of CP_LIMIT_CODEC message

    UtlBoolean handleLimitCodecPreferences(OsMsg* pEventMessage);
    //: Handles the processing of the CP_LIMIT_CODEC_PREFERENCES message

    UtlBoolean handleGetMediaEnergyLevels(OsMsg* pEventMessage);
    //: Handles the processing of a CP_GET_MEDIA_ENERGY_LEVELS message

    UtlBoolean handleGetCallMediaEnergyLevels(OsMsg* pEventMessage);
    //: Handles the processing of a CP_GET_CALL_MEDIA_ENERGY_LEVELS message

    UtlBoolean handleGetMediaRtpSourceIDs(OsMsg* pEventMessage);
    //: Handles the processing of a CP_GET_MEDIA_RTP_SOURCE_IDS message

    UtlBoolean handleGetCanAddParty(OsMsg* pEventMessage);
    //: Handles the processing of a CP_GET_CAN_ADD_PARTY message
    
    UtlBoolean handleSplitConnection(OsMsg* pEventMessage) ;
    //: Handles the processing of a CP_SPLIT_CONNECTION message
    
    UtlBoolean handleJoinConnection(OsMsg* pEventMessage) ;
    //: Handles the processing of a CP_JOIN_CONNECTION message
    
    UtlBoolean handleSetMediaPassThrough(const OsMsg& eventMessage);
    //: Handles processing of CP_SET_MEDIA_PASS_THROUGH

    UtlBoolean handleTransferOtherPartyHold(OsMsg* pEventMessage) ;
    //: Handles the processing of a CP_TRANSFER_OTHER_PARTY_HOLD message

    UtlBoolean handleTransferOtherPartyJoin(OsMsg* pEventMessage) ;
    //: Handles the processing of a CP_TRANSFER_OTHER_PARTY_JOIN message

    UtlBoolean handleTransferOtherPartyUnhold(OsMsg* pEventMessage) ;
    //: Handles the processing of a CP_TRANSFER_OTHER_PARTY_UNHOLD message

    UtlBoolean handleGetUserAgent(OsMsg* pEventMessage);

    virtual UtlBoolean getConnectionState(const char* remoteAddress, int& state);

    virtual UtlBoolean getTermConnectionState(const char* address,
        const char* terminal,
        int& state);

    virtual void onHook();
    virtual void offHook(const void* hWnd = NULL);

    // Connection manipulators
    Connection* findHandlingConnection(OsMsg& eventMessage);
    Connection* findHandlingConnection(UtlString& remoteAddress);
    Connection* findHandlingConnection(const char* callId,
        const char* toTag,
        const char* fromTag,
        UtlBoolean  strictCompare);

    void addConnection(Connection* connection);
    void removeConnection(Connection* connection);
    Connection* findQueuedConnection();
    UtlBoolean isConnectionLive(int* localConnectionState = NULL);
    void dropIfDead();
    void dropDeadConnections();

    void handleGetTermConnections(OsMsg* pEventMessage);
    void handleSetOutboundLine(OsMsg* pEventMessage);

    void getLocalContactAddresses( SIPX_CONTACT_ADDRESS contacts[],
        size_t nMaxContacts,
        size_t& nActualContacts) ;


    /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

    //void doHold();
    //void doOffHold();

    int offeringDelay;
    int lineAvailableBehavior;
    UtlString forwardUnconditional;
    int lineBusyBehavior;
    UtlString forwardOnBusy;
    int noAnswerTimeout;
    UtlString forwardOnNoAnswer;
    SipUserAgent* sipUserAgent;
    //Connection* connection;
    //UtlString extension;
    int mSipSessionReinviteTimer;
    UtlDList mConnections;
    OsRWMutex mConnectionMutex;
    int mDialMode;
    UtlString mLocalAddress;
    UtlString mLocalTerminalId;
    UtlBoolean mIsEarlyMediaFor180;
    UtlBoolean mbRequestedDrop;      // Have we requested to be dropped by the CallManager
    SIPXTACK_SECURITY_ATTRIBUTES* mpSecurity;

    SIPX_CALLSTATE_EVENT eLastMajor ;
    SIPX_CALLSTATE_CAUSE eLastMinor ; 

    CpPeerCall(const CpPeerCall& rCpPeerCall);
    //:Copy constructor

    CpPeerCall& operator=(const CpPeerCall& rhs);
    //:Assignment operator

   UtlBoolean checkForTag(UtlString &address);

   MediaStreamPassThroughData* mpPassThroughData;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _CpPeerCall_h_
