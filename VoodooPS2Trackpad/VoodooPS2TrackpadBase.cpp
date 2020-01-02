//
// Created by Brandon Pedersen on 5/1/13.
//

#include <IOKit/IOLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOTimerEventSource.h>
#include "VoodooPS2Controller.h"
#include "VoodooPS2TrackpadBase.h"

// =============================================================================
// VoodooPS2TouchPadBase Class Implementation
//

OSDefineMetaClassAndAbstractStructors(VoodooPS2TouchPadBase, IOHIPointing);

UInt32 VoodooPS2TouchPadBase::deviceType()
{ return NX_EVS_DEVICE_TYPE_MOUSE; };

UInt32 VoodooPS2TouchPadBase::interfaceID()
{ return NX_EVS_DEVICE_INTERFACE_BUS_ACE; };

IOItemCount VoodooPS2TouchPadBase::buttonCount() { return _buttonCount; };
IOFixed     VoodooPS2TouchPadBase::resolution()  { return _resolution << 16; };

#define abs(x) ((x) < 0 ? -(x) : (x))

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/* ===============================||\\alps.c from linux 4.4//||================================== */

bool VoodooPS2TouchPadBase::rpt_cmd(SInt32 init_command, SInt32 init_arg, SInt32 repeated_command, ALPSStatus_t *report)
{
    TPS2Request<9> request;
    int byte0, cmd;
    cmd = byte0 = 0;
    
    if (init_command) {
        request.commands[cmd].command = kPS2C_SendMouseCommandAndCompareAck;
        request.commands[cmd++].inOrOut = kDP_SetMouseResolution;
        request.commands[cmd].command = kPS2C_SendMouseCommandAndCompareAck;
        request.commands[cmd++].inOrOut = init_arg;
    }
    
    
    // 3X run command
    request.commands[cmd].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[cmd++].inOrOut = repeated_command;
    request.commands[cmd].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[cmd++].inOrOut = repeated_command;
    request.commands[cmd].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[cmd++].inOrOut = repeated_command;
    
    // Get info/result
    request.commands[cmd].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[cmd++].inOrOut = kDP_GetMouseInformation;
    byte0 = cmd;
    request.commands[cmd].command = kPS2C_ReadDataPort;
    request.commands[cmd++].inOrOut = 0;
    request.commands[cmd].command = kPS2C_ReadDataPort;
    request.commands[cmd++].inOrOut = 0;
    request.commands[cmd].command = kPS2C_ReadDataPort;
    request.commands[cmd++].inOrOut = 0;
    request.commandsCount = cmd;
    assert(request.commandsCount <= countof(request.commands));
    _device->submitRequestAndBlock(&request);
    
    report->bytes[0] = request.commands[byte0].inOrOut;
    report->bytes[1] = request.commands[byte0+1].inOrOut;
    report->bytes[2] = request.commands[byte0+2].inOrOut;
    
    DEBUG_LOG("%02x report: [0x%02x 0x%02x 0x%02x]\n",
              repeated_command,
              report->bytes[0],
              report->bytes[1],
              report->bytes[2]);
    
    return request.commandsCount == cmd;
}

bool VoodooPS2TouchPadBase::matchTable(ALPSStatus_t *e7, ALPSStatus_t *ec) {
    const struct alps_model_info *model;
    int i;
  
    IOLog("ALPS: Touchpad with Signature { %d, %d, %d }", e7->bytes[0], e7->bytes[1], e7->bytes[2]);
  
    for (i = 0; i < ARRAY_SIZE(alps_model_data); i++) {
        model = &alps_model_data[i];
        
        if (!memcmp(e7->bytes, model->signature, sizeof(model->signature)) &&
            (!model->command_mode_resp ||
             model->command_mode_resp == ec->bytes[2])) {
                
                priv.proto_version = model->proto_version;
                
                // log model version:
                if (priv.proto_version == ALPS_PROTO_V1) {
                    IOLog("ALPS: Found an ALPS V1 TouchPad\n");
                } else if (priv.proto_version == ALPS_PROTO_V2) {
                    IOLog("ALPS: Found an ALPS V2 TouchPad\n");
                } else if (priv.proto_version == ALPS_PROTO_V3_RUSHMORE) {
                    IOLog("ALPS: Found an ALPS V3 Rushmore TouchPad\n!");
                } else if (priv.proto_version == ALPS_PROTO_V4) {
                    IOLog("ALPS: Found an ALPS V4 TouchPad\n");
                } else if (priv.proto_version == ALPS_PROTO_V6) {
                    IOLog("ALPS: Found an ALPS V6 TouchPad\n");
                }
                
                set_protocol();
                
                priv.flags = model->flags;
                priv.byte0 = model->byte0;
                priv.mask0 = model->mask0;
                
                return true;
            }
    }
    
    return false;
}

IOReturn VoodooPS2TouchPadBase::identify(UInt16 modelToLookFor) {
    ALPSStatus_t e6, e7, ec;
    
    /*
     * First try "E6 report".
     * ALPS should return 0,0,10 or 0,0,100 if no buttons are pressed.
     * The bits 0-2 of the first byte will be 1s if some buttons are
     * pressed.
     */
    
    if (!rpt_cmd(kDP_SetMouseResolution, NULL, kDP_SetMouseScaling1To1, &e6)) {
        IOLog("ALPS: identify: not an ALPS device. Error getting E6 report\n");
        //return kIOReturnIOError;
    }
    
    if ((e6.bytes[0] & 0xf8) != 0 || e6.bytes[1] != 0 || (e6.bytes[2] != 10 && e6.bytes[2] != 100)) {
        IOLog("ALPS: identify: not an ALPS device. Invalid E6 report\n");
        //return kIOReturnInvalid;
    }
    
    /*
     * Now get the "E7" and "EC" reports.  These will uniquely identify
     * most ALPS touchpads.
     */
    if (!(rpt_cmd(kDP_SetMouseResolution, NULL, kDP_SetMouseScaling2To1, &e7) &&
          rpt_cmd(kDP_SetMouseResolution, NULL, kDP_MouseResetWrap, &ec) &&
          exit_command_mode())) {
        IOLog("ALPS: identify: not an ALPS device. Error getting E7/EC report\n");
        return kIOReturnIOError;
    }
    
    if (matchTable(&e7, &ec)) {
        if(priv.proto_version != modelToLookFor)
            return kIOReturnInvalid;
        else
            return 0;
    } // ELSE
    
    switch(modelToLookFor) {
        // V3, V3-Rushmore, and V5 are basically the same
        case ALPS_PROTO_V3:
            if (ec.bytes[0] == 0x88 && ec.bytes[1] == 0x08) {
                
                priv.proto_version = ALPS_PROTO_V3_RUSHMORE;
                IOLog("ALPS: Found a V3 Rushmore TouchPad with ID: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x\n", e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
                
            } else if (e7.bytes[0] == 0x73 && e7.bytes[1] == 0x03 && e7.bytes[2] == 0x50 &&
                ec.bytes[0] == 0x73 && (ec.bytes[1] == 0x01 || ec.bytes[1] == 0x02)) {
                
                priv.proto_version = ALPS_PROTO_V5;
                IOLog("ALPS: Found a V5 Dolphin TouchPad with ID: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x\n", e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
            } else if (ec.bytes[0] == 0x88 && ec.bytes[1] == 0x07 &&
                ec.bytes[2] >= 0x90 && ec.bytes[2] <= 0x9d) {
                
                priv.proto_version = ALPS_PROTO_V3;
                IOLog("ALPS: Found a V3 Pinnacle TouchPad with ID: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x\n", e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
                
            } else return kIOReturnInvalid;
            break;
        case ALPS_PROTO_V7:
            if (ec.bytes[0] == 0x88 &&
                ((ec.bytes[1] & 0xf0) == 0xb0 || (ec.bytes[1] & 0xf0) == 0xc0)) {
            
                priv.proto_version = ALPS_PROTO_V7;
                    IOLog("ALPS: Found a V7 TouchPad with ID: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x\n", e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
            } else return kIOReturnInvalid;
            break;
        case ALPS_PROTO_V8:
            if (e7.bytes[0] == 0x73 && e7.bytes[1] == 0x03 &&
                e7.bytes[2] == 0x14 && ec.bytes[1] == 0x02) {
                
                priv.proto_version = ALPS_PROTO_V8;
                IOLog("ALPS: Found a V8 TouchPad with ID: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x\n", e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
            } else if (e7.bytes[0] == 0x73 && e7.bytes[1] == 0x03 &&
                       e7.bytes[2] == 0x28 && ec.bytes[1] == 0x01) {
                priv.proto_version = ALPS_PROTO_V8;
                IOLog("ALPS: Found a V8 Flare TouchPad with ID: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x\n", e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
                
                
            } else return kIOReturnInvalid;
            break;
        default:
            return kIOReturnInvalid;
    }
    
    /* Save the Firmware version */
    memcpy(priv.fw_ver, ec.bytes, 3);
    set_protocol();
    return 0;
}


bool VoodooPS2TouchPadBase::init(OSDictionary * dict)
{
    //
    // Initialize this object's minimal state. This is invoked right after this
    // object is instantiated.
    //
    
    if (!super::init(dict))
        return false;

    // find config specific to Platform Profile
    OSDictionary* list = OSDynamicCast(OSDictionary, dict->getObject(kPlatformProfile));
    OSDictionary* config = ApplePS2Controller::makeConfigurationNode(list);
    if (config)
    {
        // if DisableDevice is Yes, then do not load at all...
        OSBoolean* disable = OSDynamicCast(OSBoolean, config->getObject(kDisableDevice));
        if (disable && disable->isTrue())
        {
            config->release();
            return false;
        }
#ifdef DEBUG
        // save configuration for later/diagnostics...
        setProperty(kMergedConfiguration, config);
#endif
    }
    
    // initialize state...
    _device = NULL;
    _interruptHandlerInstalled = false;
    _powerControlHandlerInstalled = false;
    _messageHandlerInstalled = false;
    _packetByteCount = 0;
    _lastdata = 0;
    _cmdGate = 0;
        
    memset(&inputEvent, 0, sizeof(VoodooInputEvent));
    
	IOLog("VoodooPS2TouchPad Base Driver loaded...\n");
    
	setProperty("Revision", 24, 32);
    
    //
    // Load settings specific to Platform Profile
    //
    
    setParamPropertiesGated(config);
    OSSafeReleaseNULL(config);
    
    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool VoodooPS2TouchPadBase::start( IOService * provider )
{
    //
    // The driver has been instructed to start. This is called after a
    // successful probe and match.
    //

    if (!super::start(provider))
        return false;

    // Maintain a pointer to and retain the provider object.
    _device = (ApplePS2MouseDevice *) provider;
    _device->retain();
    
    //
    // Advertise the current state of the tapping feature.
    //
    // Must add this property to let our superclass know that it should handle
    // trackpad acceleration settings from user space.  Without this, tracking
    // speed adjustments from the mouse prefs panel have no effect.
    //

    setProperty(kIOHIDPointerAccelerationTypeKey, kIOHIDTrackpadAccelerationType);
    setProperty(kIOHIDScrollAccelerationTypeKey, kIOHIDTrackpadScrollAccelerationKey);
	setProperty(kIOHIDScrollResolutionKey, _scrollresolution << 16, 32);
    setProperty("HIDScrollResolutionX", _scrollresolution << 16, 32);
    setProperty("HIDScrollResolutionY", _scrollresolution << 16, 32);
    
    // Setup workloop with command gate for thread synchronization...
    IOWorkLoop* pWorkLoop = getWorkLoop();
    _cmdGate = IOCommandGate::commandGate(this);
    if (!pWorkLoop || !_cmdGate)
    {
        _device->release();
        return false;
    }
    
    // Setup button timer event source
    if (_buttonCount >= 3)
    {
        if (!_buttonTimer)
        {
            _device->release();
            return false;
        }
        pWorkLoop->addEventSource(_buttonTimer);
    }
    
    // Lock the controller during initialization
    _device->lock();

    // Perform any implementation specific device initialization
    if (!deviceSpecificInit()) {
        _device->unlock();
        _device->release();
        // TODO: any other cleanup?
        return false;
    }

    // Install our driver's interrupt handler, for asynchronous data delivery.
    _device->installInterruptAction(this,
                                    OSMemberFunctionCast(PS2InterruptAction,this,&VoodooPS2TouchPadBase::interruptOccurred),
                                    OSMemberFunctionCast(PS2PacketAction, this, &VoodooPS2TouchPadBase::packetReady));
    _interruptHandlerInstalled = true;
    
    // now safe to allow other threads
    _device->unlock();
    
	// Install our power control handler.
	_device->installPowerControlAction( this,
        OSMemberFunctionCast(PS2PowerControlAction, this, &VoodooPS2TouchPadBase::setDevicePowerState) );
	_powerControlHandlerInstalled = true;
    
    //
    // Install message hook for keyboard to trackpad communication
    //
    
    _device->installMessageAction( this,
        OSMemberFunctionCast(PS2MessageAction, this, &VoodooPS2TouchPadBase::receiveMessage));
    _messageHandlerInstalled = true;

    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void VoodooPS2TouchPadBase::stop( IOService * provider )
{
    DEBUG_LOG("%s: stop called\n", getName());
    
    //
    // The driver has been instructed to stop.  Note that we must break all
    // connections to other service objects now (ie. no registered actions,
    // no pointers and retains to objects, etc), if any.
    //

    assert(_device == provider);

    // free up timer for scroll momentum
    IOWorkLoop* pWorkLoop = getWorkLoop();
    if (pWorkLoop)
    {
        if (_buttonTimer)
        {
            pWorkLoop->removeEventSource(_buttonTimer);
            _buttonTimer->release();
            _buttonTimer = 0;
        }
        if (_cmdGate)
        {
            pWorkLoop->removeEventSource(_cmdGate);
            _cmdGate->release();
            _cmdGate = 0;
        }
    }
    
    //
    // Uninstall the interrupt handler.
    //

    if (_interruptHandlerInstalled)
    {
        _device->uninstallInterruptAction();
        _interruptHandlerInstalled = false;
    }

    //
    // Uninstall the power control handler.
    //

    if (_powerControlHandlerInstalled)
    {
        _device->uninstallPowerControlAction();
        _powerControlHandlerInstalled = false;
    }
    
    //
    // Uinstall message handler.
    //
    if (_messageHandlerInstalled)
    {
        _device->uninstallMessageAction();
        _messageHandlerInstalled = false;
    }

    //
    // Release the pointer to the provider object.
    //
    
    OSSafeReleaseNULL(_device);
    
	super::stop(provider);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void VoodooPS2TouchPadBase::onButtonTimer(void)
{
	uint64_t now_abs;
	clock_get_uptime(&now_abs);
    
    middleButton(lastbuttons, now_abs, fromTimer);
}

UInt32 VoodooPS2TouchPadBase::middleButton(UInt32 buttons, uint64_t now_abs, MBComingFrom from)
{
    if (!_fakemiddlebutton || _buttonCount <= 2 || (ignoreall && fromTrackpad == from))
        return buttons;
    
    // cancel timer if we see input before timeout has fired, but after expired
    bool timeout = false;
    uint64_t now_ns;
    absolutetime_to_nanoseconds(now_abs, &now_ns);
    if (fromTimer == from || fromCancel == from || now_ns - _buttontime > _maxmiddleclicktime)
        timeout = true;

    //
    // A state machine to simulate middle buttons with two buttons pressed
    // together.
    //
    switch (_mbuttonstate)
    {
        // no buttons down, waiting for something to happen
        case STATE_NOBUTTONS:
            if (fromCancel != from)
            {
                if (buttons & 0x4)
                    _mbuttonstate = STATE_NOOP;
                else if (0x3 == buttons)
                    _mbuttonstate = STATE_MIDDLE;
                else if (0x0 != buttons)
                {
                    // only single button, so delay this for a bit
                    _pendingbuttons = buttons;
                    _buttontime = now_ns;
                    setTimerTimeout(_buttonTimer, _maxmiddleclicktime);
                    _mbuttonstate = STATE_WAIT4TWO;
                }
            }
            break;
            
        // waiting for second button to come down or timeout
        case STATE_WAIT4TWO:
            if (!timeout && 0x3 == buttons)
            {
                _pendingbuttons = 0;
                cancelTimer(_buttonTimer);
                _mbuttonstate = STATE_MIDDLE;
            }
            else if (timeout || buttons != _pendingbuttons)
            {
                if (fromTimer == from || !(buttons & _pendingbuttons))
                    dispatchRelativePointerEventX(0, 0, buttons|_pendingbuttons, now_abs);
                _pendingbuttons = 0;
                cancelTimer(_buttonTimer);
                if (0x0 == buttons)
                    _mbuttonstate = STATE_NOBUTTONS;
                else
                    _mbuttonstate = STATE_NOOP;
            }
            break;
            
        // both buttons down and delivering middle button
        case STATE_MIDDLE:
            if (0x0 == buttons)
                _mbuttonstate = STATE_NOBUTTONS;
            else if (0x3 != (buttons & 0x3))
            {
                // only single button, so delay to see if we get to none
                _pendingbuttons = buttons;
                _buttontime = now_ns;
                setTimerTimeout(_buttonTimer, _maxmiddleclicktime);
                _mbuttonstate = STATE_WAIT4NONE;
            }
            break;
            
        // was middle button, but one button now up, waiting for second to go up
        case STATE_WAIT4NONE:
            if (!timeout && 0x0 == buttons)
            {
                _pendingbuttons = 0;
                cancelTimer(_buttonTimer);
                _mbuttonstate = STATE_NOBUTTONS;
            }
            else if (timeout || buttons != _pendingbuttons)
            {
                if (fromTimer == from)
                    dispatchRelativePointerEventX(0, 0, buttons|_pendingbuttons, now_abs);
                _pendingbuttons = 0;
                cancelTimer(_buttonTimer);
                if (0x0 == buttons)
                    _mbuttonstate = STATE_NOBUTTONS;
                else
                    _mbuttonstate = STATE_NOOP;
            }
            break;
            
        case STATE_NOOP:
            if (0x0 == buttons)
                _mbuttonstate = STATE_NOBUTTONS;
            break;
    }
    
    // modify buttons after new state set
    switch (_mbuttonstate)
    {
        case STATE_MIDDLE:
            buttons = 0x4;
            break;
            
        case STATE_WAIT4NONE:
        case STATE_WAIT4TWO:
            buttons &= ~0x3;
            break;
            
        case STATE_NOBUTTONS:
        case STATE_NOOP:
            break;
    }
    
    // return modified buttons
    return buttons;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void VoodooPS2TouchPadBase::initTouchPad()
{
    //
    // Clear packet buffer pointer to avoid issues caused by
    // stale packet fragments.
    //
    
    _packetByteCount = 0;
    _ringBuffer.reset();
    
    // clear passbuttons, just in case buttons were down when system
    // went to sleep (now just assume they are up)
    passbuttons = 0;
    _clickbuttons = 0;
    tracksecondary=false;
    
    // clear state of control key cache
    _modifierdown = 0;
    
    // initialize the touchpad
    deviceSpecificInit();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void VoodooPS2TouchPadBase::setParamPropertiesGated(OSDictionary * config)
{
	if (NULL == config)
		return;
    
	const struct {const char *name; int *var;} int32vars[]={
		{"FingerZ",							&z_finger},
		{"DivisorX",						&divisorx},
		{"DivisorY",						&divisory},
		{"EdgeRight",						&redge},
		{"EdgeLeft",						&ledge},
		{"EdgeTop",							&tedge},
		{"EdgeBottom",						&bedge},
		{"VerticalScrollDivisor",			&vscrolldivisor},
		{"HorizontalScrollDivisor",			&hscrolldivisor},
		{"CircularScrollDivisor",			&cscrolldivisor},
		{"CenterX",							&centerx},
		{"CenterY",							&centery},
		{"CircularScrollTrigger",			&ctrigger},
		{"MultiFingerWLimit",				&wlimit},
		{"MultiFingerVerticalDivisor",		&wvdivisor},
		{"MultiFingerHorizontalDivisor",	&whdivisor},
        {"ZLimit",                          &zlimit},
        {"MouseMultiplierX",                &mousemultiplierx},
        {"MouseMultiplierY",                &mousemultipliery},
        {"MouseScrollMultiplierX",          &mousescrollmultiplierx},
        {"MouseScrollMultiplierY",          &mousescrollmultipliery},
        {"WakeDelay",                       &wakedelay},
        {"TapThresholdX",                   &tapthreshx},
        {"TapThresholdY",                   &tapthreshy},
        {"DoubleTapThresholdX",             &dblthreshx},
        {"DoubleTapThresholdY",             &dblthreshy},
        {"ZoneLeft",                        &zonel},
        {"ZoneRight",                       &zoner},
        {"ZoneTop",                         &zonet},
        {"ZoneBottom",                      &zoneb},
        {"DisableZoneLeft",                 &diszl},
        {"DisableZoneRight",                &diszr},
        {"DisableZoneTop",                  &diszt},
        {"DisableZoneBottom",               &diszb},
        {"DisableZoneControl",              &diszctrl},
        {"Resolution",                      &_resolution},
        {"ScrollResolution",                &_scrollresolution},
        {"SwipeDeltaX",                     &swipedx},
        {"SwipeDeltaY",                     &swipedy},
        {"MouseCount",                      &mousecount},
        {"RightClickZoneLeft",              &rczl},
        {"RightClickZoneRight",             &rczr},
        {"RightClickZoneTop",               &rczt},
        {"RightClickZoneBottom",            &rczb},
        {"HIDScrollZoomModifierMask",       &scrollzoommask},
        {"ButtonCount",                     &_buttonCount},
        {"DragLockTempMask",                &draglocktempmask},
        {"FingerChangeIgnoreDeltas",        &ignoredeltasstart},
        {"BogusDeltaThreshX",               &bogusdxthresh},
        {"BogusDeltaThreshY",               &bogusdythresh},
        {"UnitsPerMMX",                     &xupmm},
        {"UnitsPerMMY",                     &yupmm},
        {"ScrollDeltaThreshX",              &scrolldxthresh},
        {"ScrollDeltaThreshY",              &scrolldythresh},
	};
	const struct {const char *name; int *var;} boolvars[]={
		{"StickyHorizontalScrolling",		&hsticky},
		{"StickyVerticalScrolling",			&vsticky},
		{"StickyMultiFingerScrolling",		&wsticky},
		{"StabilizeTapping",				&tapstable},
        {"DisableLEDUpdate",                &noled},
        {"SmoothInput",                     &smoothinput},
        {"UnsmoothInput",                   &unsmoothinput},
        {"SkipPassThrough",                 &skippassthru},
        {"SwapDoubleTriple",                &swapdoubletriple},
        {"ClickPadTrackBoth",               &clickpadtrackboth},
        {"ImmediateClick",                  &immediateclick},
        {"MouseMiddleScroll",               &mousemiddlescroll},
        {"FakeMiddleButton",                &_fakemiddlebutton},
	};
    const struct {const char* name; bool* var;} lowbitvars[]={
        {"Clicking",                        &clicking},
        {"Dragging",                        &dragging},
        {"TrackpadRightClick",              &rtap},
        {"DragLock",                        &draglock},
        {"OutsidezoneNoAction When Typing", &outzone_wt},
        {"PalmNoAction Permanent",          &palm},
        {"PalmNoAction When Typing",        &palm_wt},
        {"USBMouseStopsTrackpad",           &usb_mouse_stops_trackpad},
        {"TrackpadMomentumScroll",          &momentumscroll},
    };
    const struct {const char* name; uint64_t* var; } int64vars[]={
        {"MaxDragTime",                     &maxdragtime},
        {"MaxTapTime",                      &maxtaptime},
        {"HIDClickTime",                    &maxdbltaptime},
        {"QuietTimeAfterTyping",            &maxaftertyping},
        {"MomentumScrollTimer",             &momentumscrolltimer},
        {"ClickPadClickTime",               &clickpadclicktime},
        {"MiddleClickTime",                 &_maxmiddleclicktime},
    };
    
    int oldmousecount = mousecount;
    bool old_usb_mouse_stops_trackpad = usb_mouse_stops_trackpad;

    OSBoolean *bl;
    OSNumber *num;
    // 64-bit config items
    for (int i = 0; i < countof(int64vars); i++) {
        if ((num=OSDynamicCast(OSNumber, config->getObject(int64vars[i].name))))
        {
            *int64vars[i].var = num->unsigned64BitValue();
            ////DEBUG_LOG("%s::setProperty64(%s, %llu)\n", getName(), int64vars[i].name, *int64vars[i].var);
            setProperty(int64vars[i].name, *int64vars[i].var, 64);
        }
    }
    // boolean config items
	for (int i = 0; i < countof(boolvars); i++) {
		if ((bl=OSDynamicCast (OSBoolean,config->getObject (boolvars[i].name))))
        {
			*boolvars[i].var = bl->isTrue();
            ////DEBUG_LOG("%s::setPropertyBool(%s, %d)\n", getName(), boolvars[i].name, *boolvars[i].var);
            setProperty(boolvars[i].name, *boolvars[i].var ? kOSBooleanTrue : kOSBooleanFalse);
        }
    }
    // 32-bit config items
	for (int i = 0; i < countof(int32vars);i++) {
		if ((num=OSDynamicCast (OSNumber,config->getObject (int32vars[i].name))))
        {
			*int32vars[i].var = num->unsigned32BitValue();
            ////DEBUG_LOG("%s::setProperty32(%s, %d)\n", getName(), int32vars[i].name, *int32vars[i].var);
            setProperty(int32vars[i].name, *int32vars[i].var, 32);
        }
    }
    // lowbit config items
	for (int i = 0; i < countof(lowbitvars); i++) {
		if ((num=OSDynamicCast (OSNumber,config->getObject(lowbitvars[i].name))))
        {
			*lowbitvars[i].var = (num->unsigned32BitValue()&0x1)?true:false;
            ////DEBUG_LOG("%s::setPropertyLowBit(%s, %d)\n", getName(), lowbitvars[i].name, *lowbitvars[i].var);
            setProperty(lowbitvars[i].name, *lowbitvars[i].var ? 1 : 0, 32);
        }
    }
    
    // special case for MaxDragTime (which is really max time for a double-click)
    // we can let it go no more than 230ms because otherwise taps on
    // the menu bar take too long if drag mode is enabled.  The code in that case
    // has to "hold button 1 down" for the duration of maxdragtime because if
    // it didn't then dragging on the caption of a window will not work
    // (some other apps too) because these apps will see a double tap+hold as
    // a single click, then double click and they don't go into drag mode when
    // initiated with a double click.
    //
    // same thing going on with the forward/back buttons in Finder, except the
    // timeout OS X is using is different (shorter)
    //
    // this all happens during MODE_PREDRAG
    //
    // summary:
    //  if the code releases button 1 after a tap, then dragging windows
    //    breaks
    //  if the maxdragtime is too large (200ms is small enough, 500ms is too large)
    //    then clicking on menus breaks because the system sees it as a long
    //    press and hold
    //
    // fyi:
    //  also tried to allow release of button 1 during MODE_PREDRAG, and then when
    //   attempting to initiate the drag (in the case the second touch comes soon
    //   enough), modifying the time such that it is not seen as a double tap.
    //  unfortunately, that destroys double tap as well, probably because the
    //   system is confused seeing input "out of order"
    
    //if (maxdragtime > 230000000)
    //    maxdragtime = 230000000;
    
    // DivisorX and DivisorY cannot be zero, but don't crash if they are...
    if (!divisorx)
        divisorx = 1;
    if (!divisory)
        divisory = 1;

    // bogusdeltathreshx/y = 0 is MAX_INT
    if (!bogusdxthresh)
        bogusdxthresh = 0x7FFFFFFF;
    if (!bogusdythresh)
        bogusdythresh = 0x7FFFFFFF;

    // check for special terminating sequence from PS2Daemon
    if (-1 == mousecount)
    {
        DEBUG_LOG("Shutdown touchpad, mousecount=%d\n", mousecount);
        touchpadShutdown();
        mousecount = oldmousecount;
    }

    // disable trackpad when USB mouse is plugged in
    // check for mouse count changing...
    if ((oldmousecount != 0) != (mousecount != 0) || old_usb_mouse_stops_trackpad != usb_mouse_stops_trackpad)
    {
        // either last mouse removed or first mouse added
        ignoreall = (mousecount != 0) && usb_mouse_stops_trackpad;
        touchpadToggled();
    }
}

IOReturn VoodooPS2TouchPadBase::setParamProperties(OSDictionary* dict)
{
    ////IOReturn result = super::IOHIDevice::setParamProperties(dict);
    if (_cmdGate)
    {
        // syncronize through workloop...
        ////_cmdGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &VooodooPS2TouchPadBase::setParamPropertiesGated), dict);
        setParamPropertiesGated(dict);
    }
    
    return super::setParamProperties(dict);
    ////return result;
}

IOReturn VoodooPS2TouchPadBase::setProperties(OSObject *props)
{
	OSDictionary *dict = OSDynamicCast(OSDictionary, props);
    if (dict && _cmdGate)
    {
        // syncronize through workloop...
        _cmdGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &VoodooPS2TouchPadBase::setParamPropertiesGated), dict);
    }
    
	return super::setProperties(props);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void VoodooPS2TouchPadBase::setDevicePowerState( UInt32 whatToDo )
{
    switch ( whatToDo )
    {
        case kPS2C_DisableDevice:
            //
            // Disable touchpad (synchronous).
            //

            setTouchPadEnable( false );
            break;

        case kPS2C_EnableDevice:
            //
            // Must not issue any commands before the device has
            // completed its power-on self-test and calibration.
            //

            IOSleep(wakedelay);
            
            // Reset and enable the touchpad.
            initTouchPad();
            break;
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void VoodooPS2TouchPadBase::receiveMessage(int message, void* data)
{
    //
    // Here is where we receive messages from the keyboard driver
    //
    // This allows for the keyboard driver to enable/disable the trackpad
    // when a certain keycode is pressed.
    //
    // It also allows the trackpad driver to learn the last time a key
    //  has been pressed, so it can implement various "ignore trackpad
    //  input while typing" options.
    //
    switch (message)
    {
        case kPS2M_getDisableTouchpad:
        {
            bool* pResult = (bool*)data;
            *pResult = !ignoreall;
            break;
        }
            
        case kPS2M_setDisableTouchpad:
        {
            bool enable = *((bool*)data);
            // ignoreall is true when trackpad has been disabled
            if (enable == ignoreall)
            {
                // save state, and update LED
                ignoreall = !enable;
                touchpadToggled();
            }
            break;
        }
            
        case kPS2M_notifyKeyPressed:
        {
            // just remember last time key pressed... this can be used in
            // interrupt handler to detect unintended input while typing
            PS2KeyInfo* pInfo = (PS2KeyInfo*)data;
            static const int masks[] =
            {
                0x10,       // 0x36
                0x100000,   // 0x37
                0,          // 0x38
                0,          // 0x39
                0x080000,   // 0x3a
                0x040000,   // 0x3b
                0,          // 0x3c
                0x08,       // 0x3d
                0x04,       // 0x3e
                0x200000,   // 0x3f
            };
#ifdef SIMULATE_PASSTHRU
            static int buttons = 0;
            int button;
            switch (pInfo->adbKeyCode)
            {
                // make right Alt,Menu,Ctrl into three button passthru
                case 0x36:
                    button = 0x1;
                    goto dispatch_it;
                case 0x3f:
                    button = 0x4;
                    goto dispatch_it;
                case 0x3e:
                    button = 0x2;
                    // fall through...
                dispatch_it:
                    if (pInfo->goingDown)
                        buttons |= button;
                    else
                        buttons &= ~button;
                    UInt8 packet[6];
                    packet[0] = 0x84 | trackbuttons;
                    packet[1] = 0x08 | buttons;
                    packet[2] = 0;
                    packet[3] = 0xC4 | trackbuttons;
                    packet[4] = 0;
                    packet[5] = 0;
                    dispatchEventsWithPacket(packet, 6);
                    pInfo->eatKey = true;
            }
#endif
            switch (pInfo->adbKeyCode)
            {
                // don't store key time for modifier keys going down
                // track modifiers for scrollzoom feature...
                // (note: it turns out we didn't need to do this, but leaving this code in for now in case it is useful)
                case 0x38:  // left shift
                case 0x3c:  // right shift
                case 0x3b:  // left control
                case 0x3e:  // right control
                case 0x3a:  // left windows (option)
                case 0x3d:  // right windows
                case 0x37:  // left alt (command)
                case 0x36:  // right alt
                case 0x3f:  // osx fn (function)
                    if (pInfo->goingDown)
                    {
                        _modifierdown |= masks[pInfo->adbKeyCode-0x36];
                        break;
                    }
                    _modifierdown &= ~masks[pInfo->adbKeyCode-0x36];
                    keytime = pInfo->time;
                    break;
                    
                default:
                    momentumscrollcurrent = 0;  // keys cancel momentum scroll
                    keytime = pInfo->time;
            }
            break;
        }
    }
}
