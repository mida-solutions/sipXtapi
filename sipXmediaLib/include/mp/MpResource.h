//  
// Copyright (C) 2022 SIP Spectrum, Inc.  All rights reserved.
//  
// Copyright (C) 2006-2013 SIPez LLC.  All rights reserved.
//
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


#ifndef _MpResource_h_
#define _MpResource_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "os/OsDefs.h"
#include "os/OsRWMutex.h"
#include "os/OsStatus.h"
#include "os/OsMsgQ.h"
#include "utl/UtlContainable.h"
#include "utl/UtlString.h"
#include "mp/MpBuf.h"
#include "mp/MpResNotificationMsg.h"
#include "rtcp/RtcpConfig.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class MpFlowGraphBase;
class MpFlowGraphMsg;
class MpResourceMsg;

/// Abstract base class for all media processing objects.
/**
*  Each resource has zero or more input ports and zero or more output ports.
*  Each frame processing interval, the <i>processFrame()</i> method is
*  invoked to process one interval's worth of media.
* 
*  Substantive changes to a resource can only be made:
*  1) when the resource is not part of flow graph, or
*  2) at the start of a frame processing interval
*
*  MpResource inherits from UtlString and stores the resource name
*  in the base class.  The resource name is used to uniquely identify
*  the resource within the scope of a flowgraph.
*
*  @nosubgrouping
*/
class MpResource : public UtlString
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   friend class MpFlowGraphBase;

   /// @brief Graph traversal states that are used when running a topological 
   /// sort to order resources within a flow graph.
   typedef enum
   {
      NOT_VISITED,
      IN_PROGRESS,
      FINISHED
   } VisitState;

   enum
   {
      ASSOCIATED_LATENCY=-1, ///< Get input or output latency, associated with a resource.
      INF_LATENCY=-1 ///< Infinite latency, i.e. data from the input is not sent to the output.
   };

static const UtlContainableType TYPE; ///< Class name, used for run-time checks.

/* ============================ CREATORS ================================== */
///@name Creators
//@{

   /// Constructor
   MpResource(const UtlString& rName, int minInputs, int maxInputs,
              int minOutputs, int maxOutputs);

   /// Destructor
   virtual ~MpResource();

//@}

/* ============================ MANIPULATORS ============================== */
///@name Manipulators
//@{

     /// Disable this resource.
   virtual UtlBoolean disable();
     /**<
     *  If a resource is disabled, it will perform only minimal processing
     *  typically. For example, passing the input straight through
     *  to the output in the case of a one input / one output resource.
     *  @see enable()
     *
     *  @note This is an asynchronous operation, if this resource is a part
     *        of flowgraph. In this case status returned does not indicate that
     *        operation succeed - only that it was properly queued. If resource
     *        is not part of flowgraph, then operation will be processed
     *        synchronously.
     *
     *  @retval TRUE if successful.
     *  @retval FALSE otherwise.
     */

     /// Post a message to disable the resource named.
   static OsStatus disable(const UtlString& namedResource,
                           OsMsgQ& fgQ);
     /**<
     *  Post a disable message for the named resource to the flowgraph queue
     *  supplied.
     *  @see disable() for more information.
     *
     *  @note This is an asynchronous operation.
     *        The status returned does not indicate that the disable
     *        happened - only that it was properly queued.
     *
     *  @param[in] namedResource - the name of the resource to disable.
     *  @param[in] fgQ - The flowgraph message queue to post the message to.
     *
     *  @retval OS_SUCCESS if the message was successfully queued
     *          to the message queue.
     *  @retval OS_FAILED if the message could not be added to the
     *          message queue.
     */

     /// Enable this resource.
   virtual UtlBoolean enable();
     /**<
     *  If a resource is enabled, it will perform full featured processing
     *  typically. For example, apply gain, mix several frames, remove noise,
     *  etc. However resources such as MprFromFile should be further started
     *  to do what they supposed to.
     *  @see disable()
     *
     *  @note This is an asynchronous operation, if this resource is a part
     *        of flowgraph. In this case status returned does not indicate that
     *        operation succeed - only that it was properly queued. If resource
     *        is not part of flowgraph, then operation will be processed
     *        synchronously.
     *
     *  @retval TRUE if successful.
     *  @retval FALSE otherwise.
     */

     /// Post a message to enable the resource named.
   static OsStatus enable(const UtlString& namedResource,
                          OsMsgQ& fgQ);
     /**<
     *  Post an enable message for the named resource to the flowgraph queue
     *  supplied.
     *  @see enable() for more information.
     *
     *  @note This is an asynchronous operation.
     *        The status returned does not indicate that the enable
     *        happened - only that it was properly queued.
     *
     *  @param[in] namedResource - the name of the resource to enable.
     *  @param[in] fgQ - The flowgraph message queue to post the message to.
     *
     *  @retval OS_SUCCESS if the message was successfully queued
     *          to the message queue.
     *  @retval OS_FAILED if the message could not be added to the
     *          message queue.
     */

     /// @brief This method is invoked for resources that care about stream discontinuities
   virtual void reset();

     /// @brief Post a message to enable or disable resource notifications on 
     /// the named resource.
   static OsStatus setNotificationsEnabled(UtlBoolean enable,
                                           const UtlString& namedResource,
                                           OsMsgQ& fgQ);
     /**<
     *  Post a message to either enable or disable sending all notifications for 
     *  the named resource to the flowgraph queue supplied.
     *
     *  @note This is an asynchronous operation.
     *        The status returned does not indicate that notifications
     *        are enabled or disabled - only that it was properly queued.
     *
     *  @param[in] enable - whether enabling or disabling is requested.
     *  @param[in] namedResource - the name of the resource to operate on.
     *  @param[in] fgQ - The flowgraph message queue to post the message to.
     *
     *  @retval OS_SUCCESS if the message was successfully queued
     *          to the message queue.
     *  @retval OS_FAILED if the message could not be added to the
     *          message queue.
     */


     /// @brief Handles a queue full of incoming messages for this media
     /// processing object.
   UtlBoolean handleMessages(OsMsgQ& msgQ);
     /**<
     *  This is intended to handle messages directly on a resource, 
     *  circumventing a flowgraph's queue, and allowing things like the
     *  application to get resources to process some operations directly.
     *  (usually before a flowgraph is set up, but perhaps else-when too.
     *
     *  @note This makes an assumption that the destination of these 
     *        messages is this resource.
     *
     *  @param[in] msgQ - a message queue full of messages intended for
     *             this resource.
     *
     *  @returns TRUE if all the messages were handled, otherwise FALSE. 
     */

     /// This method is called in every flowgraph processing cycle.
   virtual UtlBoolean processFrame() = 0;
     /**<
     *  This method is called for each resource during frame processing cycle
     *  to perform data processing and, hence, it should be implemented in all
     *  child classes. Note, that this method is called regardless of enabled
     *  or disabled state of resource. Resource should handle enabled flag
     *  on its own.
     *
     *  @retval TRUE if successful
     *  @retval FALSE otherwise.
     */

     /// Sets the visit state for this resource.
   void setVisitState(int newState);
     /**<
     *  Used in performing a topological sort on the resources contained within
     *  a flow graph.
     */

     /// Send a notification with the given message type if notifications are enabled.
   OsStatus sendNotification(MpResNotificationMsg::RNMsgType msgType);
     /**<
     *  
     *  @param msgType - the type of message to send.
     *  @retval OS_SUCCESS if message send succeeded.
     */

     /// Send the given notification message if notifications are enabled.
   OsStatus sendNotification(MpResNotificationMsg& msg);
     /**<
     *  @NOTE Use this variant if you need to provide a message that is a 
     *        child of MpResNotificationMsg.
     *  @param msg - the Notification Message to send.
     *  @retval OS_SUCCESS if message send succeeded.
     */

     /// Set the ID of a connection this resource belongs to.
   virtual void setConnectionId(MpConnectionID connectionId);
     /**<
     *  @warning This method directly modifies resource structure.
     */

     /// Set the ID of a stream inside of the connection this resource belongs to.
   virtual void setStreamId(int streamId);
     /**<
     *  @warning This method directly modifies resource structure.
     */

     /// Get the ID of a stream within the connection this resource belongs to.
   virtual int getStreamId(void);

   /// Receive buffer asynchronously from resource at given input port
   virtual OsStatus pushBuffer(int inputPort, MpBufPtr& inputBuffer);

   /// Broadcast announcement that we are changing our SSRC
   virtual void reassignSSRC(ssrc_t newSSRC) {}  // Base class, do nothing

//@}

/* ============================ ACCESSORS ================================= */
///@name Accessors
//@{

     /// Displays information on the console about the specified resource.
   static void resourceInfo(MpResource* pResource, int index);

     /// Returns parent flowgraph.
   MpFlowGraphBase* getFlowGraph() const;
     /**<
     *  @returns the flow graph that contains this resource or NULL if the 
     *           resource is not presently part of any flow graph.
     */

     /// Returns information about the upstream end of a connection.
   void getInputInfo(int inPortIdx, MpResource*& rpUpstreamResource,
                     int& rUpstreamPortIdx);
     /**<
     *  Returns information about the upstream end of a connection to the
     *  \p inPortIdx input on this resource.  If \p inPortIdx is
     *  invalid or there is no connection, then \p rpUpstreamResource
     *  will be set to NULL.
     *
     *  @note This method locks to avoid contention over port allocation.
     *        For this reason it SHOULD NOT be used in process frame.
     */

     /// Returns the name associated with this resource.
   const UtlString &getName() const;

     /// Returns information about the downstream end of a connection.
   void getOutputInfo(int outPortIdx, MpResource*& rpDownstreamResource,
                      int& rDownstreamPortIdx);
     /**<
     *  Returns information about the downstream end of a connection to the
     *  \p outPortIdx output on this resource.  If \p outPortIdx is
     *  invalid or there is no connection, then \p rpDownstreamResource 
     *  will be set to NULL.
     *
     *  @note This method locks to avoid contention over port allocation.
     *        For this reason it SHOULD NOT be used in process frame.
     */

     /// Returns the current visit state for this resource
   int getVisitState();
     /**<
     *  Used in performing a topological sort on the resources contained within
     *  a flow graph.
     */

     /// Returns the maximum number of inputs supported by this resource.
   int maxInputs() const;

     /// Returns the maximum number of outputs supported by this resource.
   int maxOutputs() const;

     /// Returns the minimum number of inputs required by this resource.
   int minInputs() const;

     /// Returns the minimum number of outputs required by this resource.
   int minOutputs() const;

     /// Returns the number of resource inputs that are currently connected.
   int numInputs() const;

     /// Returns the number of resource outputs that are currently connected.
   int numOutputs() const;

     /// Get the ID of a connection this resource belongs to.
   virtual MpConnectionID getConnectionId(void) const;

     /// Find the first unconnected input port and reserve it
   int reserveFirstUnconnectedInput();
     /**<
     *  Reserving a port does not prevent someone from connecting to that port.
     *
     *  @returns -1 if no free ports
     *
     *  @note This method locks to avoid contention over port allocation.
     *        It SHOULD NOT be used in process frame.
     */

     /// Find the first unconnected output port and reserve it
   int reserveFirstUnconnectedOutput();
     /**<
     * Reserving a port does not prevent someone from connecting to that port.
     *
     * @returns -1 if no free ports
     *
     *  @note This method locks to avoid contention over port allocation.
     *        It SHOULD NOT be used in process frame.
     */

     /// Get the ContainableType for a UtlContainable derived class.
   UtlContainableType getContainableType() const;

     /// Get current input to output latency (in samples)
   virtual OsStatus getCurrentLatency(int &latency, int input=0, int output=0) const;
     /**<
     *  Get given input to given output latency in samples.
     *
     *  This method is called from media processing loop and thus should not
     *  block.
     *
     *  @note Resource may cache latency from the last processed frame interval
     *        or retrieve/calculate it at every call to this method. I think
     *        this freedom shouldn't affect precision in a considerable way.
     *        But it is not recommended to return average latency here.
     *
     *  @param[out] latency - value of latency returned. Set to INF_LATENCY if
     *              this input does not send data to this output currently.
     *  @param[in] input - input for data to return latency for. If set to
     *             ASSOCIATED_LATENCY, then input latency, associated with this
     *             resource to be returned. E.g. driver latency to be returned
     *             for MprFromInputDevice.
     *  @param[in] output - output for data to return latency for. If set to
     *             ASSOCIATED_LATENCY, then output latency, associated with this
     *             resource to be returned. E.g. driver latency to be returned
     *             for MprToOutputDevice.
     *
     *  @retval OS_SUCCESS if latency has been returned successfully.
     *  @retval OS_NOT_FOUND if input or output are not connected.
     */

//@}

/* ============================ INQUIRY =================================== */
///@name Inquiry
//@{

     /// Returns TRUE is this resource is currently enabled, FALSE otherwise.
   UtlBoolean isEnabled() const;

     /// Returns TRUE if portIdx is valid and the indicated input is connected, FALSE otherwise.
   UtlBoolean isInputConnected(int portIdx);
     /**<
     *  @note This method locks to avoid contention over port allocation.
     *        It SHOULD NOT be used in process frame.
     */

     /// Returns TRUE if portIdx is valid and the indicated input is not connected, FALSE otherwise.
   UtlBoolean isInputUnconnected(int portIdx);
     /**<
     *  @note This method locks to avoid contention over port allocation.
     *        It SHOULD NOT be used in process frame.
     */

     /// Returns TRUE if portIdx is valid and the indicated output is connected, FALSE otherwise.
   UtlBoolean isOutputConnected(int portIdx);
     /**<
     *  @note This method locks to avoid contention over port allocation.
     *        It SHOULD NOT be used in process frame.
     */

     /// Returns TRUE if portIdx is valid and the indicated output is not connected, FALSE otherwise.
   UtlBoolean isOutputUnconnected(int portIdx);
     /**<
     *  @note This method locks to avoid contention over port allocation.
     *        It SHOULD NOT be used in process frame.
     */

     /// Returns TRUE if notification sending is enabled on this resource.
   UtlBoolean areNotificationsEnabled() const;
     /**<
     *  @see setAllNotificationsEnabled()
     *  @see MpFlowGraphBase::setNotificationsEnabled()
     */

   /// Takes asynchronous input (pushBuffer).
   virtual UtlBoolean isAsynchInput(int inputIndex);

//@}

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   // Conn is a local class definition

   /// The Conn object maintains information about the "far end" of a connection.
   struct Conn
   {
      MpResource* pResource;    ///< Other end of the connection.
      int         portIndex;    ///< Port number on the other end of the connection.
      UtlBoolean  reserved;     ///< this port is reserved to be used
   };

   MpFlowGraphBase* mpFlowGraph;   ///< flow graph this resource belongs to
   MpConnectionID mConnectionId;   ///< The ID of connection this resource belongs to.
   int            mStreamId;       ///< The ID of the stream inside the connection
                                   ///< this resource belongs to.
   UtlBoolean    mIsEnabled;       ///< TRUE if resource is enabled, FALSE otherwise

   OsRWMutex    mRWMutex;          ///< reader/writer lock for synchronization

   MpBufPtr*    mpInBufs;          ///< input buffers for this resource
   MpBufPtr*    mpOutBufs;         ///< output buffers for this resource
   Conn*        mpInConns;         ///< input connections for this resource
   Conn*        mpOutConns;        ///< output connections for this resource
   int          mMaxInputs;        ///< maximum number of inputs
   int          mMaxOutputs;       ///< maximum number of outputs
   int          mMinInputs;        ///< number of required inputs
   int          mMinOutputs;       ///< number of required outputs
   int          mNumActualInputs;  ///< actual number of connected inputs
   int          mNumActualOutputs; ///< actual number of connected outputs
   int          mVisitState;       ///< (used by flow graph topological sort alg.)
   UtlBoolean   mNotificationsEnabled; ///< Whether we should send notifications or not.
   OsBSem       mLock;             ///< used mainly to make safe changes to ports

   static const OsTime sOperationQueueTimeout;
     ///< The timeout for message operations for all resources when posting to the flowgraph queue.

     /// @brief Handles an incoming flowgraph message for this media processing object.
   virtual UtlBoolean handleMessage(MpFlowGraphMsg& fgMsg);
     /**< @returns TRUE if the message was handled, otherwise FALSE. */

     /// @brief Handles an incoming resource message for this media processing object.
   virtual UtlBoolean handleMessage(MpResourceMsg& rMsg);
     /**< @returns TRUE if the message was handled, otherwise FALSE. */

     /// @brief perform the enable operation on the resource
   virtual UtlBoolean handleEnable();

     /// @brief perform the disable operation on the resource
   virtual UtlBoolean handleDisable();

     /// @brief If there already is a buffer stored for this input port,
     /// delete it. Then store \p pBuf for the indicated input port.
   void setInputBuffer(int inPortIdx, const MpBufPtr &pBuf);

     /// Post a message from this resource.
   OsStatus postMessage(MpFlowGraphMsg& rMsg);
     /**<
     *  If this resource is not part of a flow graph, then \p rMsg is
     *  immediately passed to the handleMessage() method for this
     *  resource.  If this resource is part of a flow graph, then
     *  \p rMsg will be sent to the message queue for the flow graph
     *  that this resource belongs to.  The handleMessage() method
     *  for <i>destination</i> resource will be invoked at the start of the next
     *  frame processing interval.
     *
     *  @warning Feel the difference in method behaviour if resource in the
     *           flow graph and if it is not.
     */

     /// Post a message for this resource.
   OsStatus postMessage(MpResourceMsg& rMsg);
     /**<
     *  @see postMessage(MpFlowGraphMsg&) for details.
     */

     /// @brief Makes \p pBuf available to resource connected to the
     /// \p outPortIdx output port of this resource.
   UtlBoolean pushBufferDownsream(int outPortIdx, const MpBufPtr &pBuf);
     /**<
     *  @returns TRUE if there is a resource connected to the specified output
     *  port, FALSE otherwise.
     */

     /// @brief Associates this resource with the indicated flow graph.
   virtual OsStatus setFlowGraph(MpFlowGraphBase* pFlowGraph);
     /**<
     *  @param[in] pFlowGraph - pointer to a flowgraph owning this resource.
     *
     *  @retval OS_SUCCESS - for now, this method always returns success
     */

     /// @brief Sets whether or not this resource should send notifications
   virtual OsStatus setNotificationsEnabled(UtlBoolean enable);
     /**<
     *  @param[in] enable - TRUE to enable notifications, FALSE to disable.
     *  
     *  @retval OS_SUCCESS if setting worked.
     *  @retval OS_FAILURe if setting failed.
     */

     /// @brief Connects the \p toPortIdx input port on this resource to the 
     /// \p fromPortIdx output port of the \p rFrom resource.
   virtual UtlBoolean connectInput(MpResource& rFrom, int fromPortIdx, int toPortIdx);
     /**<
     *  @note This method locks to avoid contention over port allocation.
     *        It SHOULD NOT be used in process frame.
     *
     *  @returns TRUE if successful, FALSE otherwise.
     */

     /// @brief Connects the \p fromPortIdx output port on this resource to the 
     /// \p toPortIdx input port of the \p rTo resource.
   virtual UtlBoolean connectOutput(MpResource& rTo, int toPortIdx, int fromPortIdx);
     /**<
     *  @note This method locks to avoid contention over port allocation.
     *        It SHOULD NOT be used in process frame.
     *
     *  @returns TRUE if successful, FALSE otherwise.
     */

     /// @brief Removes the connection to the \p inPortIdx input port
     /// of this resource.
   virtual UtlBoolean disconnectInput(int inPortIdx);
     /**<
     *  @note This method locks to avoid contention over port allocation.
     *        It SHOULD NOT be used in process frame.
     *
     *  @returns TRUE if successful, FALSE otherwise.
     */

     /// @brief Removes the connection to the \p outPortIdx output port
     /// of this resource.
   virtual UtlBoolean disconnectOutput(int outPortIdx);
     /**<
     *  @note This method locks to avoid contention over port allocation.
     *        It SHOULD NOT be used in process frame.
     *
     *  @returns TRUE if successful, FALSE otherwise.
     */

     /// Sets the name that is associated with this resource.
   void setName(const UtlString& rName);

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

     /// Copy constructor (not implemented for this class)
   MpResource(const MpResource& rMpResource);

     /// Assignment operator (not implemented for this class)
   MpResource& operator=(const MpResource& rhs);

};

/* ============================ INLINE METHODS ============================ */

#endif  // _MpResource_h_
