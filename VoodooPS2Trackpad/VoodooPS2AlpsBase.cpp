/*
 * Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.2 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License
 * http://www.apple.com/publicsource and read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#include "VoodooPS2AlpsBase.h"

enum {
    kTapEnabled = 0x01
};

/*
 * Definitions for ALPS version 3 and 4 command mode protocol
 */
#define ALPS_CMD_NIBBLE_10  0x01f2

#define ALPS_REG_BASE_RUSHMORE  0xc2c0
#define ALPS_REG_BASE_V7	0xc2c0
#define ALPS_REG_BASE_PINNACLE  0x0000

static const struct alps_nibble_commands alps_v3_nibble_commands[] = {
    { kDP_MouseSetPoll,                 0x00 }, /* 0 no send/recv */
    { kDP_SetDefaults,                  0x00 }, /* 1 no send/recv */
    { kDP_SetMouseScaling2To1,          0x00 }, /* 2 no send/recv */
    { kDP_SetMouseSampleRate | 0x1000,  0x0a }, /* 3 send=1 recv=0 */
    { kDP_SetMouseSampleRate | 0x1000,  0x14 }, /* 4 ..*/
    { kDP_SetMouseSampleRate | 0x1000,  0x28 }, /* 5 ..*/
    { kDP_SetMouseSampleRate | 0x1000,  0x3c }, /* 6 ..*/
    { kDP_SetMouseSampleRate | 0x1000,  0x50 }, /* 7 ..*/
    { kDP_SetMouseSampleRate | 0x1000,  0x64 }, /* 8 ..*/
    { kDP_SetMouseSampleRate | 0x1000,  0xc8 }, /* 9 ..*/
    { kDP_CommandNibble10    | 0x0100,  0x00 }, /* a send=0 recv=1 */
    { kDP_SetMouseResolution | 0x1000,  0x00 }, /* b send=1 recv=0 */
    { kDP_SetMouseResolution | 0x1000,  0x01 }, /* c ..*/
    { kDP_SetMouseResolution | 0x1000,  0x02 }, /* d ..*/
    { kDP_SetMouseResolution | 0x1000,  0x03 }, /* e ..*/
    { kDP_SetMouseScaling1To1,          0x00 }, /* f no send/recv */
};

static const struct alps_nibble_commands alps_v4_nibble_commands[] = {
    { kDP_Enable,                       0x00 }, /* 0 no send/recv */
    { kDP_SetDefaults,                  0x00 }, /* 1 no send/recv */
    { kDP_SetMouseScaling2To1,          0x00 }, /* 2 no send/recv */
    { kDP_SetMouseSampleRate | 0x1000,  0x0a }, /* 3 send=1 recv=0 */
    { kDP_SetMouseSampleRate | 0x1000,  0x14 }, /* 4 ..*/
    { kDP_SetMouseSampleRate | 0x1000,  0x28 }, /* 5 ..*/
    { kDP_SetMouseSampleRate | 0x1000,  0x3c }, /* 6 ..*/
    { kDP_SetMouseSampleRate | 0x1000,  0x50 }, /* 7 ..*/
    { kDP_SetMouseSampleRate | 0x1000,  0x64 }, /* 8 ..*/
    { kDP_SetMouseSampleRate | 0x1000,  0xc8 }, /* 9 ..*/
    { kDP_CommandNibble10    | 0x0100,  0x00 }, /* a send=0 recv=1 */
    { kDP_SetMouseResolution | 0x1000,  0x00 }, /* b send=1 recv=0 */
    { kDP_SetMouseResolution | 0x1000,  0x01 }, /* c ..*/
    { kDP_SetMouseResolution | 0x1000,  0x02 }, /* d ..*/
    { kDP_SetMouseResolution | 0x1000,  0x03 }, /* e ..*/
    { kDP_SetMouseScaling1To1,          0x00 }, /* f no send/recv */
};

static const struct alps_nibble_commands alps_v6_nibble_commands[] = {
    { kDP_Enable,		            0x00 }, /* 0 */
    { kDP_SetMouseSampleRate,		0x0a }, /* 1 */
    { kDP_SetMouseSampleRate,		0x14 }, /* 2 */
    { kDP_SetMouseSampleRate,		0x28 }, /* 3 */
    { kDP_SetMouseSampleRate,		0x3c }, /* 4 */
    { kDP_SetMouseSampleRate,		0x50 }, /* 5 */
    { kDP_SetMouseSampleRate,		0x64 }, /* 6 */
    { kDP_SetMouseSampleRate,		0xc8 }, /* 7 */
    { kDP_GetId,		            0x00 }, /* 8 */
    { kDP_GetMouseInformation,		0x00 }, /* 9 */
    { kDP_SetMouseResolution,		0x00 }, /* a */
    { kDP_SetMouseResolution,		0x01 }, /* b */
    { kDP_SetMouseResolution,		0x02 }, /* c */
    { kDP_SetMouseResolution,		0x03 }, /* d */
    { kDP_SetMouseScaling2To1,	    0x00 }, /* e */
    { kDP_SetMouseScaling1To1,	    0x00 }, /* f */
};

// =============================================================================
// ALPS Class Implementation  //////////////////////////////////////////////////
// =============================================================================

OSDefineMetaClassAndAbstractStructors(VoodooPS2AlpsBase, VoodooPS2TouchPad);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

VoodooPS2AlpsBase *VoodooPS2AlpsBase::probe(IOService *provider, SInt32 *score) {
    bool success;
    
    //
    // The driver has been instructed to verify the presence of the actual
    // hardware we represent. We are guaranteed by the controller that the
    // mouse clock is enabled and the mouse itself is disabled (thus it
    // won't send any asynchronous mouse data that may mess up the
    // responses expected by the commands we send it).
    //
    
    _device = (ApplePS2MouseDevice *) provider;
    
    _device->lock();
    resetMouse();
    
    if (identify() != 0) {
        success = false;
    } else {
        success = true;
        IOLog("ALPS: TouchPad driver started...\n");
    }
    _device->unlock();
    
    _device = 0;
    
    return success ? this : 0;
}

void VoodooPS2AlpsBase::restart() {
    
    _device->lock();
    
    resetMouse();
    
    identify();
    
    _device->unlock();
    
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool VoodooPS2AlpsBase::deviceSpecificInit() {
    
    // Setup expected packet size
    priv.pktsize = priv.proto_version == ALPS_PROTO_V4 ? 8 : 6;
    
    if (!deviceDevice()) {
        goto init_fail;
    }
    
    return true;
    
init_fail:
    IOLog("ALPS: Hardware initialization failed. TouchPad probably won't work\n");
    resetMouse();
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/* Link with Base Driver */
bool VoodooPS2AlpsBase::init(OSDictionary *dict) {
    if (!super::init(dict)) {
        return false;
    }
    
    memset(&inputEvent, 0, sizeof(VoodooInputEvent));
    
    // Intialize Variables
    lastx=0;
    lasty=0;
    last_fingers=0;
    xrest=0;
    yrest=0;
    lastbuttons=0;
    
    // Default Configuration
    clicking=true;
    rtap=true;
    dragging=true;
    scroll=true;
    hscroll=true;
    momentumscroll=true;
    outzone_wt=palm=palm_wt=true;
    
    return true;
}

void VoodooPS2AlpsBase::stop(IOService *provider) {
    
    resetMouse();
    
    super::stop(provider);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool VoodooPS2AlpsBase::resetMouse() {
    TPS2Request<3> request;
    
    // Reset mouse
    request.commands[0].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[0].inOrOut = kDP_Reset;
    request.commands[1].command = kPS2C_ReadDataPort;
    request.commands[1].inOrOut = 0;
    request.commands[2].command = kPS2C_ReadDataPort;
    request.commands[2].inOrOut = 0;
    request.commandsCount = 3;
    assert(request.commandsCount <= countof(request.commands));
    _device->submitRequestAndBlock(&request);
    
    // Verify the result
    if (request.commands[1].inOrOut != kSC_Reset && request.commands[2].inOrOut != kSC_ID) {
        IOLog("ALPS: Failed to reset mouse, return values did not match. [0x%02x, 0x%02x]\n", request.commands[1].inOrOut, request.commands[2].inOrOut);
        return false;
    }
    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/* ============================================================================================== */
/* ===============================||\\alps.c from linux 4.4//||================================== */
/* ============================================================================================== */


static void alps_get_bitmap_points(unsigned int map,
                                   struct alps_bitmap_point *low,
                                   struct alps_bitmap_point *high,
                                   int *fingers)
{
    struct alps_bitmap_point *point;
    int i, bit, prev_bit = 0;
    
    point = low;
    for (i = 0; map != 0; i++, map >>= 1) {
        bit = map & 1;
        if (bit) {
            if (!prev_bit) {
                point->start_bit = i;
                point->num_bits = 0;
                (*fingers)++;
            }
            point->num_bits++;
        } else {
            if (prev_bit)
                point = high;
        }
        prev_bit = bit;
    }
}

/*
 * Process bitmap data from semi-mt protocols. Returns the number of
 * fingers detected. A return value of 0 means at least one of the
 * bitmaps was empty.
 *
 * The bitmaps don't have enough data to track fingers, so this function
 * only generates points representing a bounding box of all contacts.
 * These points are returned in fields->mt when the return value
 * is greater than 0.
 */
int ALPS::alps_process_bitmap(struct alps_data *priv,
                              struct alps_fields *fields)
{
    
    int i, fingers_x = 0, fingers_y = 0, fingers, closest;
    struct alps_bitmap_point x_low = {0,}, x_high = {0,};
    struct alps_bitmap_point y_low = {0,}, y_high = {0,};
    struct input_mt_pos corner[4];
    
    
    if (!fields->x_map || !fields->y_map) {
        return 0;
    }
    
    alps_get_bitmap_points(fields->x_map, &x_low, &x_high, &fingers_x);
    alps_get_bitmap_points(fields->y_map, &y_low, &y_high, &fingers_y);
    
    /*
     * Fingers can overlap, so we use the maximum count of fingers
     * on either axis as the finger count.
     */
    fingers = max(fingers_x, fingers_y);
    
    /*
     * If an axis reports only a single contact, we have overlapping or
     * adjacent fingers. Divide the single contact between the two points.
     */
    if (fingers_x == 1) {
        i = x_low.num_bits / 2;
        x_low.num_bits = x_low.num_bits - i;
        x_high.start_bit = x_low.start_bit + i;
        x_high.num_bits = max(i, 1);
    }
    
    if (fingers_y == 1) {
        i = y_low.num_bits / 2;
        y_low.num_bits = y_low.num_bits - i;
        y_high.start_bit = y_low.start_bit + i;
        y_high.num_bits = max(i, 1);
    }
    
    /* top-left corner */
    corner[0].x = (priv->x_max * (2 * x_low.start_bit + x_low.num_bits - 1)) /
    (2 * (priv->x_bits - 1));
    corner[0].y = (priv->y_max * (2 * y_low.start_bit + y_low.num_bits - 1)) /
    (2 * (priv->y_bits - 1));
    
    /* top-right corner */
    corner[1].x = (priv->x_max * (2 * x_high.start_bit + x_high.num_bits - 1)) /
    (2 * (priv->x_bits - 1));
    corner[1].y = (priv->y_max * (2 * y_low.start_bit + y_low.num_bits - 1)) /
    (2 * (priv->y_bits - 1));
    
    /* bottom-right corner */
    corner[2].x = (priv->x_max * (2 * x_high.start_bit + x_high.num_bits - 1)) /
    (2 * (priv->x_bits - 1));
    corner[2].y = (priv->y_max * (2 * y_high.start_bit + y_high.num_bits - 1)) /
    (2 * (priv->y_bits - 1));
    
    /* bottom-left corner */
    corner[3].x = (priv->x_max * (2 * x_low.start_bit + x_low.num_bits - 1)) /
    (2 * (priv->x_bits - 1));
    corner[3].y = (priv->y_max * (2 * y_high.start_bit + y_high.num_bits - 1)) /
    (2 * (priv->y_bits - 1));
    
    /* x-bitmap order is reversed on v5 touchpads  */
    if (priv->proto_version == ALPS_PROTO_V5) {
        for (i = 0; i < 4; i++)
            corner[i].x = priv->x_max - corner[i].x;
    }
    
    /* y-bitmap order is reversed on v3 and v4 touchpads  */
    if (priv->proto_version == ALPS_PROTO_V3 || priv->proto_version == ALPS_PROTO_V4) {
        for (i = 0; i < 4; i++)
            corner[i].y = priv->y_max - corner[i].y;
    }
    
    /*
     * We only select a corner for the second touch once per 2 finger
     * touch sequence to avoid the chosen corner (and thus the coordinates)
     * jumping around when the first touch is in the middle.
     */
    if (priv->second_touch == -1) {
        /* Find corner closest to our st coordinates */
        closest = 0x7fffffff;
        for (i = 0; i < 4; i++) {
            int dx = fields->st.x - corner[i].x;
            int dy = fields->st.y - corner[i].y;
            int distance = dx * dx + dy * dy;
            
            if (distance < closest) {
                priv->second_touch = i;
                closest = distance;
            }
        }
        /* And select the opposite corner to use for the 2nd touch */
        priv->second_touch = (priv->second_touch + 2) % 4;
    }
    
    fields->mt[0] = fields->st;
    fields->mt[1] = corner[priv->second_touch];
  
#if DEBUG
    IOLog("ALPS: BITMAP\n");
    
    unsigned int ymap = fields->y_map;
    
    for (int i = 0; ymap != 0; i++, ymap >>= 1) {
      unsigned int xmap = fields->x_map;
      char bitLog[160];
      strlcpy(bitLog, "ALPS: ", sizeof("ALPS: ") + 1);
      
      for (int j = 0; xmap != 0; j++, xmap >>= 1) {
        strcat(bitLog, (ymap & 1 && xmap & 1) ? "1 " : "0 ");
      }
      
      IOLog("%s\n", bitLog);
    }
  
    IOLog("ALPS: Process Bitmap, Corner=%d, Fingers=%d, x1=%d, x2=%d, y1=%d, y2=%d xmap=%d ymap=%d\n", priv->second_touch, fingers, fields->mt[0].x, fields->mt[1].x, fields->mt[0].y, fields->mt[1].y, fields->x_map, fields->y_map);
#endif // DEBUG
  return fingers;
}


void ALPS::alps_process_packet_v6(UInt8 *packet)
{
    int x, y, z, left, right, middle;
    int fingers = 0;
    int buttons = 0;
    
    uint64_t now_abs;
    clock_get_uptime(&now_abs);
    
    /*
     * We can use Byte5 to distinguish if the packet is from Touchpad
     * or Trackpoint.
     * Touchpad:	0 - 0x7E
     * Trackpoint:	0x7F
     */
    if (packet[5] == 0x7F) {
        /* It should be a DualPoint when received Trackpoint packet */
        if (!(priv.flags & ALPS_DUALPOINT)) {
            return;
        }
        
        /* Trackpoint packet */
        x = packet[1] | ((packet[3] & 0x20) << 2);
        y = packet[2] | ((packet[3] & 0x40) << 1);
        z = packet[4];
        left = packet[3] & 0x01;
        right = packet[3] & 0x02;
        middle = packet[3] & 0x04;
        
        buttons |= left ? 0x01 : 0;
        buttons |= right ? 0x02 : 0;
        buttons |= middle ? 0x04 : 0;
        
        /* To prevent the cursor jump when finger lifted */
        if (x == 0x7F && y == 0x7F && z == 0x7F)
            x = y = z = 0;
        
        /* Divide 4 since trackpoint's speed is too fast */
        dispatchRelativePointerEventX(x / 4, y / 4, buttons, now_abs);
        return;
    }
    
    /* Touchpad packet */
    x = packet[1] | ((packet[3] & 0x78) << 4);
    y = packet[2] | ((packet[4] & 0x78) << 4);
    z = packet[5];
    left = packet[3] & 0x01;
    right = packet[3] & 0x02;
    
    fingers = z > 0 ? 1 : 0;
    
    buttons |= left ? 0x01 : 0;
    buttons |= right ? 0x02 : 0;
    
    dispatchEventsWithInfo(x, y, z, fingers, buttons);
}

int ALPS::alps_get_mt_count(struct input_mt_pos *mt)
{
    int i, fingers = 0;
    
    for (i = 0; i < MAX_TOUCHES; i++) {
        if (mt[i].x != 0 || mt[i].y != 0)
            fingers++;
    }
    
    return fingers;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ALPS::setTouchPadEnable(bool enable) {
    DEBUG_LOG("setTouchpadEnable enter\n");
    //
    // Instructs the trackpad to start or stop the reporting of data packets.
    // It is safe to issue this request from the interrupt/completion context.
    //
    
    if (enable) {
        initTouchPad();
    } else {
        // to disable just reset the mouse
        resetMouse();
    }
}

PS2InterruptResult ALPS::interruptOccurred(UInt8 data) {
    //
    // This will be invoked automatically from our device when asynchronous
    // events need to be delivered. Process the trackpad data. Do NOT issue
    // any BLOCKING commands to our device in this context.
    //
    
    UInt8 *packet = _ringBuffer.head();
    
    /* Save first packet */
    if (0 == _packetByteCount) {
        packet[0] = data;
    }
    
    /* Reset PSMOUSE_BAD_DATA flag */
    priv.PSMOUSE_BAD_DATA = false;
    
    /*
     * Check if we are dealing with a bare PS/2 packet, presumably from
     * a device connected to the external PS/2 port. Because bare PS/2
     * protocol does not have enough constant bits to self-synchronize
     * properly we only do this if the device is fully synchronized.
     * Can not distinguish V8's first byte from PS/2 packet's
     */
    if (priv.proto_version != ALPS_PROTO_V8 &&
        (packet[0] & 0xc8) == 0x08) {
        if (_packetByteCount == 3) {
            //dispatchRelativePointerEventWithPacket(packet, kPacketLengthSmall); //Dr Hurt: allow this?
            priv.PSMOUSE_BAD_DATA = true;
            _ringBuffer.advanceHead(priv.pktsize);
            return kPS2IR_packetReady;
        }
        packet[_packetByteCount++] = data;
        return kPS2IR_packetBuffering;
    }
    
    /* Check for PS/2 packet stuffed in the middle of ALPS packet. */
    if ((priv.flags & ALPS_PS2_INTERLEAVED) &&
        _packetByteCount >= 4 && (packet[3] & 0x0f) == 0x0f) {
        priv.PSMOUSE_BAD_DATA = true;
        _ringBuffer.advanceHead(priv.pktsize);
        return kPS2IR_packetReady;
    }
    
    /* alps_is_valid_first_byte */
    if ((packet[0] & priv.mask0) != priv.byte0) {
        priv.PSMOUSE_BAD_DATA = true;
        _ringBuffer.advanceHead(priv.pktsize);
        return kPS2IR_packetReady;
    }
    
    /* Bytes 2 - pktsize should have 0 in the highest bit */
    if (priv.proto_version < ALPS_PROTO_V5 &&
        _packetByteCount >= 2 && _packetByteCount <= priv.pktsize &&
        (packet[_packetByteCount - 1] & 0x80)) {
        priv.PSMOUSE_BAD_DATA = true;
        _ringBuffer.advanceHead(priv.pktsize);
        return kPS2IR_packetReady;
    }
    
    /* alps_is_valid_package_v7 */
    if (priv.proto_version == ALPS_PROTO_V7 &&
        (((_packetByteCount == 3) && ((packet[2] & 0x40) != 0x40)) ||
         ((_packetByteCount == 4) && ((packet[3] & 0x48) != 0x48)) ||
         ((_packetByteCount == 6) && ((packet[5] & 0x40) != 0x0)))) {
            priv.PSMOUSE_BAD_DATA = true;
            _ringBuffer.advanceHead(priv.pktsize);
            return kPS2IR_packetReady;
        }
    
    /* alps_is_valid_package_ss4_v2 */
    if (priv.proto_version == ALPS_PROTO_V8 &&
        ((_packetByteCount == 4 && ((packet[3] & 0x08) != 0x08)) ||
         (_packetByteCount == 6 && ((packet[5] & 0x10) != 0x0)))) {
            priv.PSMOUSE_BAD_DATA = true;
            _ringBuffer.advanceHead(priv.pktsize);
            return kPS2IR_packetReady;
        }
    
    packet[_packetByteCount++] = data;
    if (_packetByteCount == priv.pktsize)
    {
        _ringBuffer.advanceHead(priv.pktsize);
        return kPS2IR_packetReady;
    }
    return kPS2IR_packetBuffering;
}

void VoodooPS2AlpsBase::packetReady() {
    // empty the ring buffer, dispatching each packet...
    while (_ringBuffer.count() >= priv.pktsize) {
        UInt8 *packet = _ringBuffer.tail();
        if (priv.PSMOUSE_BAD_DATA == false) {
            processPacket(packet);
        } else {
            IOLog("ALPS: an invalid or bare packet has been dropped...\n");
            /* Might need to perform a full HW reset here if we keep receiving bad packets (consecutively) */
        }
        _packetByteCount = 0;
        _ringBuffer.advanceTail(priv.pktsize);
    }
}

bool VoodooPS2AlpsBase::alps_command_mode_send_nibble(int nibble) {
    SInt32 command;
    // The largest amount of requests we will have is 2 right now
    // 1 for the initial command, and 1 for sending data OR 1 for receiving data
    // If the nibble commands at the top change then this will need to change as
    // well. For now we will just validate that the request will not overload
    // this object.
    TPS2Request<2> request;
    int cmdCount = 0, send = 0, receive = 0, i;
    
    if (nibble > 0xf) {
        IOLog("%s::alps_command_mode_send_nibble ERROR: nibble value is greater than 0xf, command may fail\n", getName());
    }
    
    request.commands[cmdCount].command = kPS2C_SendMouseCommandAndCompareAck;
    command = priv.nibble_commands[nibble].command;
    request.commands[cmdCount++].inOrOut = command & 0xff;
    
    send = (command >> 12 & 0xf);
    receive = (command >> 8 & 0xf);
    
    if ((send > 1) || ((send + receive + 1) > 2)) {
        return false;
    }
    
    if (send > 0) {
        request.commands[cmdCount].command = kPS2C_SendMouseCommandAndCompareAck;
        request.commands[cmdCount++].inOrOut = priv.nibble_commands[nibble].data;
    }
    
    for (i = 0; i < receive; i++) {
        request.commands[cmdCount].command = kPS2C_ReadDataPort;
        request.commands[cmdCount++].inOrOut = 0;
    }
    
    request.commandsCount = cmdCount;
    assert(request.commandsCount <= countof(request.commands));
    
    _device->submitRequestAndBlock(&request);
    
    return request.commandsCount == cmdCount;
}

bool VoodooPS2AlpsBase::alps_command_mode_set_addr(int addr) {
    
    TPS2Request<1> request;
    int i, nibble;
    
    //    DEBUG_LOG("command mode set addr with addr command: 0x%02x\n", priv.addr_command);
    request.commands[0].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[0].inOrOut = priv.addr_command;
    request.commandsCount = 1;
    _device->submitRequestAndBlock(&request);
    
    if (request.commandsCount != 1) {
        return false;
    }
    
    for (i = 12; i >= 0; i -= 4) {
        nibble = (addr >> i) & 0xf;
        if (!alps_command_mode_send_nibble(nibble)) {
            return false;
        }
    }
    
    return true;
}

int VoodooPS2AlpsBase::alps_command_mode_read_reg(int addr) {
    TPS2Request<4> request;
    ALPSStatus_t status;
    
    if (!alps_command_mode_set_addr(addr)) {
        DEBUG_LOG("Failed to set addr to read register\n");
        return -1;
    }
    
    request.commands[0].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[0].inOrOut = kDP_GetMouseInformation; //sync..
    request.commands[1].command = kPS2C_ReadDataPort;
    request.commands[1].inOrOut = 0;
    request.commands[2].command = kPS2C_ReadDataPort;
    request.commands[2].inOrOut = 0;
    request.commands[3].command = kPS2C_ReadDataPort;
    request.commands[3].inOrOut = 0;
    request.commandsCount = 4;
    assert(request.commandsCount <= countof(request.commands));
    _device->submitRequestAndBlock(&request);
    
    if (request.commandsCount != 4) {
        return -1;
    }
    
    status.bytes[0] = request.commands[1].inOrOut;
    status.bytes[1] = request.commands[2].inOrOut;
    status.bytes[2] = request.commands[3].inOrOut;
    
    //IOLog("ALPS read reg result: { 0x%02x, 0x%02x, 0x%02x }\n", status.bytes[0], status.bytes[1], status.bytes[2]);
    
    /* The address being read is returned in the first 2 bytes
     * of the result. Check that the address matches the expected
     * address.
     */
    if (addr != ((status.bytes[0] << 8) | status.bytes[1])) {
        DEBUG_LOG("ALPS ERROR: read wrong registry value, expected: %x\n", addr);
        return -1;
    }
    
    return status.bytes[2];
}

bool VoodooPS2AlpsBase::alps_command_mode_write_reg(int addr, UInt8 value) {
    
    if (!alps_command_mode_set_addr(addr)) {
        return false;
    }
    
    return alps_command_mode_write_reg(value);
}

bool VoodooPS2AlpsBase::alps_command_mode_write_reg(UInt8 value) {
    if (!alps_command_mode_send_nibble((value >> 4) & 0xf)) {
        return false;
    }
    if (!alps_command_mode_send_nibble(value & 0xf)) {
        return false;
    }
    
    return true;
}

bool VoodooPS2AlpsBase::alps_rpt_cmd(SInt32 init_command, SInt32 init_arg, SInt32 repeated_command, ALPSStatus_t *report) {
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

bool VoodooPS2AlpsBase::alps_enter_command_mode() {
    DEBUG_LOG("enter command mode\n");
    TPS2Request<4> request;
    ALPSStatus_t status;
    
    if (!alps_rpt_cmd(NULL, NULL, kDP_MouseResetWrap, &status)) {
        IOLog("ALPS: Failed to enter command mode!\n");
        return false;
    }
    return true;
}

bool VoodooPS2AlpsBase::alps_exit_command_mode() {
    DEBUG_LOG("exit command mode\n");
    TPS2Request<1> request;
    
    request.commands[0].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[0].inOrOut = kDP_SetMouseStreamMode;
    request.commandsCount = 1;
    assert(request.commandsCount <= countof(request.commands));
    _device->submitRequestAndBlock(&request);
    
    return true;
}

bool VoodooPS2AlpsBase::alps_passthrough_mode_v2(bool enable) {
    int cmd = enable ? kDP_SetMouseScaling2To1 : kDP_SetMouseScaling1To1;
    TPS2Request<4> request;
    
    request.commands[0].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[0].inOrOut = cmd;
    request.commands[1].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[1].inOrOut = cmd;
    request.commands[2].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[2].inOrOut = cmd;
    request.commands[3].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[3].inOrOut = kDP_SetDefaultsAndDisable;
    request.commandsCount = 4;
    assert(request.commandsCount <= countof(request.commands));
    _device->submitRequestAndBlock(&request);
    
    return request.commandsCount == 4;
}

int ALPS::alps_monitor_mode_send_word(int word)
{
    int i, nibble;
    
    for (i = 0; i <= 8; i += 4) {
        nibble = (word >> i) & 0xf;
        alps_command_mode_send_nibble(nibble);
    }
    
    return 0;
}

int ALPS::alps_monitor_mode_write_reg(int addr, int value)
{
    ps2_command_short(kDP_Enable);
    alps_monitor_mode_send_word(0x0A0);
    alps_monitor_mode_send_word(addr);
    alps_monitor_mode_send_word(value);
    ps2_command_short(kDP_SetDefaultsAndDisable);
    
    return 0;
}

int ALPS::alps_monitor_mode(bool enable)
{
    TPS2Request<4> request;
    int cmd = 0;
    
    if (enable) {
        
        ps2_command_short(kDP_MouseResetWrap);
        request.commands[cmd].command = kPS2C_SendMouseCommandAndCompareAck;
        request.commands[cmd++].inOrOut = kDP_GetMouseInformation;
        request.commands[cmd].command = kPS2C_ReadDataPort;
        request.commands[cmd++].inOrOut = 0;
        request.commands[cmd].command = kPS2C_ReadDataPort;
        request.commands[cmd++].inOrOut = 0;
        request.commands[cmd].command = kPS2C_ReadDataPort;
        request.commands[cmd++].inOrOut = 0;
        request.commandsCount = cmd;
        assert(request.commandsCount <= countof(request.commands));
        _device->submitRequestAndBlock(&request);
        
        ps2_command_short(kDP_SetDefaultsAndDisable);
        ps2_command_short(kDP_SetDefaultsAndDisable);
        ps2_command_short(kDP_SetMouseScaling2To1);
        ps2_command_short(kDP_SetMouseScaling1To1);
        ps2_command_short(kDP_SetMouseScaling2To1);
        
        /* Get Info */
        request.commands[cmd].command = kPS2C_SendMouseCommandAndCompareAck;
        request.commands[cmd++].inOrOut = kDP_GetMouseInformation;
        request.commands[cmd].command = kPS2C_ReadDataPort;
        request.commands[cmd++].inOrOut = 0;
        request.commands[cmd].command = kPS2C_ReadDataPort;
        request.commands[cmd++].inOrOut = 0;
        request.commands[cmd].command = kPS2C_ReadDataPort;
        request.commands[cmd++].inOrOut = 0;
        request.commandsCount = cmd;
        assert(request.commandsCount <= countof(request.commands));
        _device->submitRequestAndBlock(&request);
    } else {
        ps2_command_short(kDP_MouseResetWrap);
    }
    
    return 0;
}

void ALPS::alps_absolute_mode_v6()
{
    // enter monitor mode, to write the register /
    alps_monitor_mode(true);
    alps_monitor_mode_write_reg(0x000, 0x181);
    alps_monitor_mode(false);
}

bool ALPS::alps_get_status(ALPSStatus_t *status) {
    return alps_rpt_cmd(NULL, NULL, kDP_SetDefaultsAndDisable, status);
}

/*
 * Turn touchpad tapping on or off. The sequences are:
 * 0xE9 0xF5 0xF5 0xF3 0x0A to enable,
 * 0xE9 0xF5 0xF5 0xE8 0x00 to disable.
 * My guess that 0xE9 (GetInfo) is here as a sync point.
 * For models that also have stickpointer (DualPoints) its tapping
 * is controlled separately (0xE6 0xE6 0xE6 0xF3 0x14|0x0A) but
 * we don't fiddle with it.
 */
bool ALPS::alps_tap_mode(bool enable) {
    int cmd = enable ? kDP_SetMouseSampleRate : kDP_SetMouseResolution;
    UInt8 tapArg = enable ? 0x0A : 0x00;
    TPS2Request<8> request;
    ALPSStatus_t result;
    
    request.commands[0].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[0].inOrOut = kDP_GetMouseInformation;
    request.commands[1].command = kPS2C_ReadDataPort;
    request.commands[1].inOrOut = 0;
    request.commands[2].command = kPS2C_ReadDataPort;
    request.commands[2].inOrOut = 0;
    request.commands[3].command = kPS2C_ReadDataPort;
    request.commands[3].inOrOut = 0;
    request.commands[4].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[4].inOrOut = kDP_SetDefaultsAndDisable;
    request.commands[5].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[5].inOrOut = kDP_SetDefaultsAndDisable;
    request.commands[6].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[6].inOrOut = cmd;
    request.commands[7].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[7].inOrOut = tapArg;
    request.commandsCount = 8;
    _device->submitRequestAndBlock(&request);
    
    if (request.commandsCount != 8) {
        DEBUG_LOG("Enabling tap mode failed before getStatus call, command count=%d\n",
                  request.commandsCount);
        return false;
    }
    
    return alps_get_status(&result);
}

bool ALPS::alps_hw_init_v6()
{
    //TODO: V6 is not yet fully implemented.
    //unsigned char param[2] = {0xC8, 0x14};
    
    /* Enter passthrough mode to let trackpoint enter 6byte raw mode */
    alps_passthrough_mode_v2(true);
    
    ps2_command_short(kDP_SetMouseScaling1To1);
    ps2_command_short(kDP_SetMouseScaling1To1);
    ps2_command_short(kDP_SetMouseScaling1To1);
    ps2_command(0xC8, kDP_SetMouseSampleRate);
    ps2_command(0x14, kDP_SetMouseSampleRate);
    
    alps_passthrough_mode_v2(false);
    
    alps_absolute_mode_v6();
    
    return true;
}

IOReturn ALPS::alps_probe_trackstick_v3_v7(int regBase) {
    int ret = kIOReturnIOError, regVal;
    
    if (!alps_enter_command_mode()) {
        goto error;
    }
    
    regVal = alps_command_mode_read_reg(regBase + 0x08);
    
    if (regVal == -1) {
        goto error;
    }
    
    /* bit 7: trackstick is present */
    ret = regVal & 0x80 ? 0 : kIOReturnNoDevice;
    
error:
    alps_exit_command_mode();
    return ret;
}


bool ALPS::alps_get_v3_v7_resolution(int reg_pitch)
{
    int reg, x_pitch, y_pitch, x_electrode, y_electrode, x_phys, y_phys;
    
    reg = alps_command_mode_read_reg(reg_pitch);
    if (reg < 0)
        return reg;
    
    x_pitch = (char)(reg << 4) >> 4; /* sign extend lower 4 bits */
    x_pitch = 50 + 2 * x_pitch; /* In 0.1 mm units */
    
    y_pitch = (char)reg >> 4; /* sign extend upper 4 bits */
    y_pitch = 36 + 2 * y_pitch; /* In 0.1 mm units */
    
    reg = alps_command_mode_read_reg(reg_pitch + 1);
    if (reg < 0)
        return reg;
    
    x_electrode = (char)(reg << 4) >> 4; /* sign extend lower 4 bits */
    x_electrode = 17 + x_electrode;
    
    y_electrode = (char)reg >> 4; /* sign extend upper 4 bits */
    y_electrode = 13 + y_electrode;
    
    x_phys = x_pitch * (x_electrode - 1); /* In 0.1 mm units */
    y_phys = y_pitch * (y_electrode - 1); /* In 0.1 mm units */
    
    priv.x_res = priv.x_max * 10 / x_phys; /* units / mm */
    priv.y_res = priv.y_max * 10 / y_phys; /* units / mm */
    
    /*IOLog("pitch %dx%d num-electrodes %dx%d physical size %dx%d mm res %dx%d\n",
     x_pitch, y_pitch, x_electrode, y_electrode,
     x_phys / 10, y_phys / 10, priv.x_res, priv.y_res);*/
    
    return true;
}




void ALPS::ps2_command(unsigned char value, UInt8 command)
{
    TPS2Request<2> request;
    int cmdCount = 0;
    
    request.commands[cmdCount].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[cmdCount++].inOrOut = command;
    request.commands[cmdCount].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[cmdCount++].inOrOut = value;
    request.commandsCount = cmdCount;
    assert(request.commandsCount <= countof(request.commands));
    _device->submitRequestAndBlock(&request);
    
    //return request.commandsCount = cmdCount;
}

void ALPS::ps2_command_short(UInt8 command)
{
    TPS2Request<1> request;
    int cmdCount = 0;
    
    request.commands[cmdCount].command = kPS2C_SendMouseCommandAndCompareAck;
    request.commands[cmdCount++].inOrOut = command;
    request.commandsCount = cmdCount;
    assert(request.commandsCount <= countof(request.commands));
    _device->submitRequestAndBlock(&request);
    
    //return request.commandsCount = cmdCount;
}

void ALPS::set_protocol() {
    priv.byte0 = 0x8f;
    priv.mask0 = 0x8f;
    priv.flags = ALPS_DUALPOINT;
    
    priv.x_max = 2000;
    priv.y_max = 1400;
    priv.x_bits = 15;
    priv.y_bits = 11;
    
    switch (priv.proto_version) {
        case ALPS_PROTO_V1:
        case ALPS_PROTO_V2:
            hw_init = &ALPS::alps_hw_init_v1_v2;
            process_packet = &ALPS::alps_process_packet_v1_v2;
            priv.x_max = 1023;
            priv.y_max = 767;
            //            set_abs_params = alps_set_abs_params_st;
            break;
            
        case ALPS_PROTO_V3:
            hw_init = &ALPS::alps_hw_init_v3;
            process_packet = &ALPS::alps_process_packet_v3;
            //            set_abs_params = alps_set_abs_params_mt;
            decode_fields = &ALPS::alps_decode_pinnacle;
            priv.nibble_commands = alps_v3_nibble_commands;
            priv.addr_command = kDP_MouseResetWrap;
            
            if (alps_probe_trackstick_v3_v7(ALPS_REG_BASE_PINNACLE)) {
                priv.flags &= ~ALPS_DUALPOINT;
            } else {
                IOLog("ALPS: TrackStick detected...\n");
            }
            
            break;
            
        case ALPS_PROTO_V3_RUSHMORE:
            hw_init = &ALPS::alps_hw_init_rushmore_v3;
            process_packet = &ALPS::alps_process_packet_v3;
            //            set_abs_params = alps_set_abs_params_mt;
            decode_fields = &ALPS::alps_decode_rushmore;
            priv.nibble_commands = alps_v3_nibble_commands;
            priv.addr_command = kDP_MouseResetWrap;
            priv.x_bits = 16;
            priv.y_bits = 12;
            
            if (alps_probe_trackstick_v3_v7(ALPS_REG_BASE_RUSHMORE)) {
                priv.flags &= ~ALPS_DUALPOINT;
            } else {
                IOLog("ALPS: TrackStick detected...\n");
            }
            
            break;
            
        case ALPS_PROTO_V4:
            hw_init = &ALPS::alps_hw_init_v4;
            process_packet = &ALPS::alps_process_packet_v4;
            //            set_abs_params = alps_set_abs_params_mt;
            priv.nibble_commands = alps_v4_nibble_commands;
            priv.addr_command = kDP_SetDefaultsAndDisable;
            break;
            
        case ALPS_PROTO_V5:
            hw_init = &ALPS::alps_hw_init_dolphin_v1;
//            process_packet = &ALPS::alps_process_touchpad_packet_v3_v5;
//            decode_fields = &ALPS::alps_decode_dolphin;
            //            set_abs_params = alps_set_abs_params_mt;
            priv.nibble_commands = alps_v3_nibble_commands;
            priv.addr_command = kDP_MouseResetWrap;
            priv.byte0 = 0xc8;
            priv.mask0 = 0xc8;
            priv.flags = 0;
            //priv.x_max = 1360;
            //priv.y_max = 660;
            priv.x_bits = 23;
            priv.y_bits = 12;
            
            alps_dolphin_get_device_area(&priv);
            
            break;
            
        case ALPS_PROTO_V6:
            //hw_init = &ApplePS2ALPSGlidePoint::hwInitV6_version2;
            //process_packet = &ApplePS2ALPSGlidePoint::processPacketV6;
            priv.nibble_commands = alps_v6_nibble_commands;
            priv.addr_command = kDP_MouseResetWrap;
            priv.byte0 = 0xc8;
            priv.mask0 = 0xc8;
            priv.flags = 0;
            priv.x_max = 2047;
            priv.y_max = 1535;
            //decode_fields = &ApplePS2ALPSGlidePoint::decodePacketV6;
            break;
            
        case ALPS_PROTO_V7:
            hw_init = &ALPS::alps_hw_init_v7;
            process_packet = &ALPS::alps_process_packet_v7;
            decode_fields = &ALPS::alps_decode_packet_v7;
            priv.nibble_commands = alps_v3_nibble_commands;
            priv.addr_command = kDP_MouseResetWrap;
            priv.byte0 = 0x48;
            priv.mask0 = 0x48;
            
            priv.x_max = 0xfff;
            priv.y_max = 0x7ff;
            
            if (priv.fw_ver[1] != 0xba){
                priv.flags |= ALPS_BUTTONPAD;
                IOLog("ALPS: ButtonPad Detected...\n");
            }
            
            if (alps_probe_trackstick_v3_v7(ALPS_REG_BASE_V7)){
                priv.flags &= ~ALPS_DUALPOINT;
            } else {
                IOLog("ALPS: TrackStick detected...\n");
            }
            
            break;
            
        case ALPS_PROTO_V8:
            hw_init = &ALPS::alps_hw_init_ss4_v2;
            process_packet = &ALPS::alps_process_packet_ss4_v2;
            decode_fields = &ALPS::alps_decode_ss4_v2;
            priv.nibble_commands = alps_v3_nibble_commands;
            priv.addr_command = kDP_MouseResetWrap;
            priv.byte0 = 0x18;
            priv.mask0 = 0x18;
            priv.flags = 0;
            
            alps_set_defaults_ss4_v2(&priv);
            
            //TODO: V8: add detection of tarckstick using the "alps_set_defaults_ss4_v2(&priv)" funtcion
            if (priv.fw_ver[1] == 0x1) {
                // buttons and trackpad
                priv.x_max = 8160;
                priv.y_max = 4080;
                priv.flags |= ALPS_DUALPOINT |
                ALPS_DUALPOINT_WITH_PRESSURE;
                IOLog("ALPS: TrackStick detected... (WARNING: V8 TrackStick disabled)\n");
            } else {
                // buttonless
                priv.x_max = 8176;
                priv.y_max = 4088;
                priv.flags |= ALPS_BUTTONPAD;
                IOLog("ALPS: ButtonPad Detected...\n");
            }
            break;
    }
}

bool ALPS::matchTable(ALPSStatus_t *e7, ALPSStatus_t *ec) {
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
                }else if (priv.proto_version == ALPS_PROTO_V6) {
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

IOReturn ALPS::identify() {
    ALPSStatus_t e6, e7, ec;
    
    /*
     * First try "E6 report".
     * ALPS should return 0,0,10 or 0,0,100 if no buttons are pressed.
     * The bits 0-2 of the first byte will be 1s if some buttons are
     * pressed.
     */
    
    if (!alps_rpt_cmd(kDP_SetMouseResolution, NULL, kDP_SetMouseScaling1To1, &e6)) {
        IOLog("ALPS: identify: not an ALPS device. Error getting E6 report\n");
        return kIOReturnIOError;
    }
    
    if ((e6.bytes[0] & 0xf8) != 0 || e6.bytes[1] != 0 || (e6.bytes[2] != 10 && e6.bytes[2] != 100)) {
        IOLog("ALPS: identify: not an ALPS device. Invalid E6 report\n");
        return kIOReturnInvalid;
    }
    
    /*
     * Now get the "E7" and "EC" reports.  These will uniquely identify
     * most ALPS touchpads.
     */
    if (!(alps_rpt_cmd(kDP_SetMouseResolution, NULL, kDP_SetMouseScaling2To1, &e7) &&
          alps_rpt_cmd(kDP_SetMouseResolution, NULL, kDP_MouseResetWrap, &ec) &&
          alps_exit_command_mode())) {
        IOLog("ALPS: identify: not an ALPS device. Error getting E7/EC report\n");
        return kIOReturnIOError;
    }
    
    if (matchTable(&e7, &ec)) {
        return 0;
        
    } else if (e7.bytes[0] == 0x73 && e7.bytes[1] == 0x03 && e7.bytes[2] == 0x50 &&
               ec.bytes[0] == 0x73 && (ec.bytes[1] == 0x01 || ec.bytes[1] == 0x02)) {
        priv.proto_version = ALPS_PROTO_V5;
        IOLog("ALPS: Found a V5 Dolphin TouchPad with ID: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x\n", e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
        
    } else if (ec.bytes[0] == 0x88 &&
               ((ec.bytes[1] & 0xf0) == 0xb0 || (ec.bytes[1] & 0xf0) == 0xc0)) {
        priv.proto_version = ALPS_PROTO_V7;
        IOLog("ALPS: Found a V7 TouchPad with ID: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x\n", e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
        
    } else if (ec.bytes[0] == 0x88 && ec.bytes[1] == 0x08) {
        priv.proto_version = ALPS_PROTO_V3_RUSHMORE;
        IOLog("ALPS: Found a V3 Rushmore TouchPad with ID: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x\n", e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
        
    } else if (ec.bytes[0] == 0x88 && ec.bytes[1] == 0x07 &&
               ec.bytes[2] >= 0x90 && ec.bytes[2] <= 0x9d) {
        priv.proto_version = ALPS_PROTO_V3;
        IOLog("ALPS: Found a V3 Pinnacle TouchPad with ID: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x\n", e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
        
    } else if (e7.bytes[0] == 0x73 && e7.bytes[1] == 0x03 &&
               e7.bytes[2] == 0x14 && ec.bytes[1] == 0x02) {
        priv.proto_version = ALPS_PROTO_V8;
        IOLog("ALPS: Found a V8 TouchPad with ID: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x\n", e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
        
        
    } else if (e7.bytes[0] == 0x73 && e7.bytes[1] == 0x03 &&
               e7.bytes[2] == 0x28 && ec.bytes[1] == 0x01) {
        priv.proto_version = ALPS_PROTO_V8;
        IOLog("ALPS: Found a V8 Flare TouchPad with ID: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x\n", e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
        
        
    } else {
        IOLog("ALPS DRIVER: TouchPad didn't match any known IDs: E7=0x%02x 0x%02x 0x%02x, EC=0x%02x 0x%02x 0x%02x ... driver will now exit\n",
              e7.bytes[0], e7.bytes[1], e7.bytes[2], ec.bytes[0], ec.bytes[1], ec.bytes[2]);
        return kIOReturnInvalid;
    }
    
    /* Save the Firmware version */
    memcpy(priv.fw_ver, ec.bytes, 3);
    set_protocol();
    return 0;
}

/* ============================================================================================== */
/* ===========================||\\PROCESS AND DISPATCH TO macOS//||============================== */
/* ============================================================================================== */

void ALPS::dispatchEventsWithInfo(int xraw, int yraw, int z, int fingers, UInt32 buttonsraw) {
    uint64_t now_abs;
    clock_get_uptime(&now_abs);
    uint64_t now_ns;
    absolutetime_to_nanoseconds(now_abs, &now_ns);
    
    // scale x & y to the axis which has the most resolution
    if (xupmm < yupmm) {
        xraw = xraw * yupmm / xupmm;
    } else if (xupmm > yupmm) {
        yraw = yraw * xupmm / yupmm;
    }
    
    /* Dr Hurt: Scale all touchpads' axes to 6000 to be able to the same divisors for all models */
    xraw *= (6000 / ((priv.x_max + priv.y_max)/2));
    yraw *= (6000 / ((priv.x_max + priv.y_max)/2));
    
    int x = xraw;
    int y = yraw;
    
    fingers = z > z_finger ? fingers : 0;
    
    // allow middle click to be simulated the other two physical buttons
    UInt32 buttons = buttonsraw;
    lastbuttons = buttons;
    
    // allow middle button to be simulated with two buttons down
    if (!clickpadtype || fingers == 3) {
        buttons = middleButton(buttons, now_abs, fingers == 3 ? fromPassthru : fromTrackpad);
        DEBUG_LOG("New buttons value after check for middle click: %d\n", buttons);
    }
    
    // recalc middle buttons if finger is going down
    if (0 == last_fingers && fingers > 0) {
        buttons = middleButton(buttonsraw | passbuttons, now_abs, fromCancel);
    }
    
    if (last_fingers > 0 && fingers > 0 && last_fingers != fingers) {
        // ignore deltas for a while after finger change
        ignoredeltas = ignoredeltasstart;
    }
    
    if (last_fingers != fingers) {
        DEBUG_LOG("Finger change, reset averages\n");
        // reset averages after finger change
        x_undo.reset();
        y_undo.reset();
        x_avg.reset();
        y_avg.reset();
    }
    
    // unsmooth input (probably just for testing)
    // by default the trackpad itself does a simple decaying average (1/2 each)
    // we can undo it here
    if (unsmoothinput) {
        x = x_undo.filter(x);
        y = y_undo.filter(y);
    }
    
    // smooth input by unweighted average
    if (smoothinput) {
        x = x_avg.filter(x);
        y = y_avg.filter(y);
    }
    
    if (ignoredeltas) {
        DEBUG_LOG("ALPS: Still ignoring deltas. Value=%d\n", ignoredeltas);
        lastx = x;
        lasty = y;
        if (--ignoredeltas == 0) {
            x_undo.reset();
            y_undo.reset();
            x_avg.reset();
            y_avg.reset();
        }
    }
    
    // deal with "OutsidezoneNoAction When Typing"
    if (outzone_wt && z > z_finger && now_ns - keytime < maxaftertyping &&
        (x < zonel || x > zoner || y < zoneb || y > zonet)) {
        DEBUG_LOG("Ignore touch input after typing\n");
        // touch input was shortly after typing and outside the "zone"
        // ignore it...
        return;
    }
    
    // if trackpad input is supposed to be ignored, then don't do anything
    if (ignoreall) {
        DEBUG_LOG("ignoreall is set, returning\n");
        return;
    }
    
#ifdef DEBUG
    int tm1 = touchmode;
#endif
    DEBUG_LOG("VoodooPS2::Mode: %d\n", touchmode);
    if (z < z_finger && isTouchMode()) {
        // Finger has been lifted
        DEBUG_LOG("finger lifted after touch\n");
        xrest = yrest = scrollrest = 0;
        inSwipeLeft = inSwipeRight = inSwipeUp = inSwipeDown = 0;
        inSwipe4Left = inSwipe4Right = inSwipe4Up = inSwipe4Down = 0;
        xmoved = ymoved = 0;
        untouchtime = now_ns;
        
        DEBUG_LOG("finger lifted -> touchmode: %d history: %d", touchmode, dy_history.count());
        DEBUG_LOG("PS2: wastriple: %d wasdouble: %d touchtime: %llu", wastriple, wasdouble, touchtime);
        
        // check for scroll momentum start
        if ((MODE_MTOUCH == touchmode || MODE_VSCROLL == touchmode) && momentumscroll && momentumscrolltimer) {
            // releasing when we were in touchmode -- check for momentum scroll
            if (dy_history.count() > momentumscrollsamplesmin &&
                (momentumscrollinterval = time_history.newest() - time_history.oldest())) {
                momentumscrollsum = dy_history.sum();
                momentumscrollcurrent = momentumscrolltimer * momentumscrollsum;
                momentumscrollrest1 = 0;
                momentumscrollrest2 = 0;
                setTimerTimeout(scrollTimer, momentumscrolltimer);
            }
        }
        time_history.reset();
        dy_history.reset();
        
        if (now_ns - touchtime < maxtaptime && clicking) {
            switch (touchmode) {
                case MODE_DRAG:
                    if (!immediateclick) {
                        buttons &= ~0x7;
                        dispatchRelativePointerEventX(0, 0, buttons | 0x1, now_abs);
                        dispatchRelativePointerEventX(0, 0, buttons, now_abs);
                    }
                    if (wastriple && rtap) {
                        buttons |= !swapdoubletriple ? 0x4 : 0x02;
                    } else if (wasdouble && rtap) {
                        buttons |= !swapdoubletriple ? 0x2 : 0x04;
                    } else {
                        buttons |= 0x1;
                    }
                    touchmode = MODE_NOTOUCH;
                    break;
                    
                case MODE_DRAGLOCK:
                    touchmode = MODE_NOTOUCH;
                    break;
                    
                default: //dispatch taps
                    if (wastriple && rtap)
                    {
                        buttons |= !swapdoubletriple ? 0x4 : 0x02;
                        touchmode=MODE_NOTOUCH;
                    }
                    else if (wasdouble && rtap)
                    {
                        buttons |= !swapdoubletriple ? 0x2 : 0x04;
                        touchmode=MODE_NOTOUCH;
                    }
                    else
                    {
                        buttons |= 0x1;
                        touchmode=dragging ? MODE_PREDRAG : MODE_NOTOUCH;
                    }
                    break;
            }
        }
        else {
            if ((touchmode==MODE_DRAG || touchmode==MODE_DRAGLOCK)
                && (draglock || draglocktemp || (dragTimer && dragexitdelay)))
            {
                touchmode=MODE_DRAGNOTOUCH;
                if (!draglock && !draglocktemp)
                {
                    cancelTimer(dragTimer);
                    setTimerTimeout(dragTimer, dragexitdelay);
                }
            } else {
                touchmode = MODE_NOTOUCH;
                draglocktemp = 0;
            }
        }
        wasdouble = false;
        wastriple = false;
    }
    
    // cancel pre-drag mode if second tap takes too long
    if (touchmode == MODE_PREDRAG && now_ns - untouchtime >= maxdragtime) {
        DEBUG_LOG("cancel pre-drag since second tap took too long\n");
        touchmode = MODE_NOTOUCH;
    }
    
    // Note: This test should probably be done somewhere else, especially if to
    // implement more gestures in the future, because this information we are
    // erasing here (time of touch) might be useful for certain gestures...
    
    // cancel tap if touch point moves too far
    if (isTouchMode() && isFingerTouch(z) && last_fingers == fingers) {
        int dy = abs(touchy-y);
        int dx = abs(touchx-x);
        DEBUG_LOG("PS2: Cancel DX: %d Cancel DY: %d", dx, dy);
        if (!wasdouble && !wastriple && (dx > tapthreshx || dy > tapthreshy)) {
            touchtime = 0;
        }
        else if (dx > dblthreshx || dy > dblthreshy) {
            touchtime = 0;
        }
    }
    
#ifdef DEBUG
    int tm2 = touchmode;
#endif
    int dx = 0, dy = 0;
    
    switch (touchmode) {
        case MODE_DRAG:
        case MODE_DRAGLOCK:
            if (MODE_DRAGLOCK == touchmode || (!immediateclick || now_ns - touchtime > maxdbltaptime)) {
                buttons |= 0x1;
            }
            // fall through
        case MODE_MOVE:
            if (last_fingers == fingers && z<=zlimit)
            {
                if (now_ns - touchtime > 100000000) {
                    if(wasScroll) {
                        wasScroll = false;
                        ignoredeltas = ignoredeltasstart;
                        break;
                    }
                    dx = x-lastx+xrest;
                    dy = lasty-y+yrest;
                    xrest = dx % divisorx;
                    yrest = dy % divisory;
                    if (abs(dx) > bogusdxthresh || abs(dy) > bogusdythresh)
                        dx = dy = xrest = yrest = 0;
                }
            }
            break;
            
        case MODE_MTOUCH:
            switch (fingers) {
                case 1:
                    if (last_fingers != fingers) break;
                    
                    // transition from multitouch to single touch
                    // user could be letting go - ignore single for a few
                    // packets to see if they completely let go before
                    // starting to move w/ single finger
                    if (!wsticky && !scrolldebounce && !ignoresingle)
                    {
                        cancelTimer(scrollDebounceTIMER);
                        setTimerTimeout(scrollDebounceTIMER, scrollexitdelay);
                        scrolldebounce = true;
                        wasScroll = true;
                        dy_history.reset();
                        time_history.reset();
                        touchmode=MODE_MOVE;
                        break;
                    }
                    
                    // Decrement ignore single counter
                    if (ignoresingle)
                        ignoresingle--;
                    
                    break;
                case 2: // two finger
                    if (last_fingers != fingers) {
                        break;
                    }
                    if (palm && z > zlimit) {
                        break;
                    }
                    if (palm_wt && now_ns - keytime < maxaftertyping) {
                        break;
                    }
                    dy = (wvdivisor) ? (y-lasty+yrest) : 0;
                    dx = (whdivisor&&hscroll) ? (x-lastx+xrest) : 0;
                    yrest = (wvdivisor) ? dy % wvdivisor : 0;
                    xrest = (whdivisor&&hscroll) ? dx % whdivisor : 0;
                    // check for stopping or changing direction
                    DEBUG_LOG("fingers dy: %d", dy);
                    if ((dy < 0) != (dy_history.newest() < 0) || dy == 0) {
                        // stopped or changed direction, clear history
                        dy_history.reset();
                        time_history.reset();
                    }
                    // put movement and time in history for later
                    dy_history.filter(dy);
                    time_history.filter(now_ns);
                    //REVIEW: filter out small movements (Mavericks issue)
                    if (abs(dx) < scrolldxthresh)
                    {
                        xrest = dx;
                        dx = 0;
                    }
                    if (abs(dy) < scrolldythresh)
                    {
                        yrest = dy;
                        dy = 0;
                    }
                    if (0 != dy || 0 != dx)
                    {
                        // Don't move unless user is moved fingers far enough to know this wasn't a two finger tap
                        // Gets rid of scrolling while trying to tap 
                        if (!touchtime)
                            dispatchScrollWheelEventX(wvdivisor ? dy / wvdivisor : 0, (whdivisor && hscroll) ? -dx / whdivisor : 0, 0, now_abs);
                        dx = dy = 0;
                        ignoresingle = 3;
                    }
                    break;
                    
                case 3: // three finger
                    if (last_fingers != fingers) {
                        break;
                    }
                    
                    if (threefingerhorizswipe || threefingervertswipe) {
                        // Now calculate total movement since 3 fingers down (add to total)
                        xmoved += lastx-x;
                        ymoved += y-lasty;
                        
                        // dispatching 3 finger movement
                        if (ymoved > swipedy && !inSwipeUp && !inSwipe4Up && threefingervertswipe) {
                            inSwipeUp = 1;
                            inSwipeDown = 0;
                            ymoved = 0;
                            _device->dispatchKeyboardMessage(kPS2M_swipeUp, &now_abs);
                            break;
                        }
                        if (ymoved < -swipedy && !inSwipeDown && !inSwipe4Down && threefingervertswipe) {
                            inSwipeDown = 1;
                            inSwipeUp = 0;
                            ymoved = 0;
                            _device->dispatchKeyboardMessage(kPS2M_swipeDown, &now_abs);
                            break;
                        }
                        if (xmoved < -swipedx && !inSwipeRight && !inSwipe4Right && threefingerhorizswipe) {
                            inSwipeRight = 1;
                            inSwipeLeft = 0;
                            xmoved = 0;
                            _device->dispatchKeyboardMessage(kPS2M_swipeRight, &now_abs);
                            break;
                        }
                        if (xmoved > swipedx && !inSwipeLeft && !inSwipe4Left && threefingerhorizswipe) {
                            inSwipeLeft = 1;
                            inSwipeRight = 0;
                            xmoved = 0;
                            _device->dispatchKeyboardMessage(kPS2M_swipeLeft, &now_abs);
                            break;
                        }
                    }
                    break;
                    
                case 4: // four fingers
                    if (last_fingers != fingers) {
                        break;
                    }
                    
                    // Now calculate total movement since 4 fingers down (add to total)
                    xmoved += lastx-x;
                    ymoved += y-lasty;
                    
                    // dispatching 4 finger movement
                    if (ymoved > swipedy && !inSwipe4Up) {
                        inSwipe4Up = 1; inSwipeUp = 0;
                        inSwipe4Down = 0;
                        ymoved = 0;
                        _device->dispatchKeyboardMessage(kPS2M_swipe4Up, &now_abs);
                        break;
                    }
                    if (ymoved < -swipedy && !inSwipe4Down) {
                        inSwipe4Down = 1; inSwipeDown = 0;
                        inSwipe4Up = 0;
                        ymoved = 0;
                        _device->dispatchKeyboardMessage(kPS2M_swipe4Down, &now_abs);
                        break;
                    }
                    if (xmoved < -swipedx && !inSwipe4Right) {
                        inSwipe4Right = 1; inSwipeRight = 0;
                        inSwipe4Left = 0;
                        xmoved = 0;
                        _device->dispatchKeyboardMessage(kPS2M_swipe4Right, &now_abs);
                        break;
                    }
                    if (xmoved > swipedx && !inSwipe4Left) {
                        inSwipe4Left = 1; inSwipeLeft = 0;
                        inSwipe4Right = 0;
                        xmoved = 0;
                        _device->dispatchKeyboardMessage(kPS2M_swipe4Left, &now_abs);
                        break;
                    }
            }
            break;
        case MODE_DRAGNOTOUCH:
            buttons |= 0x1;
            // fall through
        case MODE_PREDRAG:
            if (!immediateclick && (!palm_wt || now_ns - keytime >= maxaftertyping)) {
                buttons |= 0x1;
            }
        case MODE_NOTOUCH:
            break;
            
        default:
            ; // nothing
    }
    
    // capture time of tap, and watch for double/triple tap
    if (isFingerTouch(z)) {
        // taps don't count if too close to typing or if currently in momentum scroll
        if ((!palm_wt || now_ns - keytime >= maxaftertyping) && !momentumscrollcurrent) {
            
            if (!isTouchMode()) {
                touchtime = now_ns;
            }
            
            if (last_fingers < fingers) {
                touchx = x;
                touchy = y;
            }
            
            DEBUG_LOG("PS2:Checking Fingers");
            wasdouble = fingers == 2 || (wasdouble && last_fingers != fingers);// && !scrolldebounce;
            wastriple = fingers == 3 || (wastriple && last_fingers != fingers);// && !scrolldebounce;
        }
        
        if(!scrolldebounce && momentumscrollcurrent){
            // any touch cancels momentum scroll
            momentumscrollcurrent = 0;
            setTimerTimeout(scrollDebounceTIMER,scrollexitdelay);
            scrolldebounce = true;
        }
    }
    // switch modes, depending on input
    if (touchmode == MODE_PREDRAG && isFingerTouch(z)) {
        touchmode = MODE_DRAG;
        draglocktemp = _modifierdown & draglocktempmask;
    }
    if (touchmode == MODE_DRAGNOTOUCH && isFingerTouch(z)) {
        if (dragTimer)
            cancelTimer(dragTimer);
        touchmode=MODE_DRAGLOCK;
    }
    if (MODE_MTOUCH != touchmode && fingers > 1 && isFingerTouch(z)) {
        touchmode = MODE_MTOUCH;
    }
    
    if (touchmode == MODE_NOTOUCH && z > z_finger && !scrolldebounce) {
        touchmode = MODE_MOVE;
    }
    
    // dispatch dx/dy and current button status
    dispatchRelativePointerEventX(dx / divisorx, dy / divisory, buttons, now_abs);
    
    // always save last seen position for calculating deltas later
    lastx = x;
    lasty = y;
    //b4last = last_fingers;
    last_fingers = fingers;
    
#ifdef DEBUG
    DEBUG_LOG("ps2: fingers=%d, dx=%d, dy=%d (%d,%d) z=%d mode=(%d,%d,%d) buttons=%d wasdouble=%d wastriple=%d\n", fingers, dx, dy, x, y, z, tm1, tm2, touchmode, buttons, wasdouble, wastriple);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ALPS::
dispatchRelativePointerEventWithPacket(UInt8 *packet,
                                       UInt32 packetSize) {
    //
    // Process the three byte relative format packet that was retrieved from the
    // trackpad. The format of the bytes is as follows:
    //
    //  7  6  5  4  3  2  1  0
    // -----------------------
    // YO XO YS XS  1  M  R  L
    // X7 X6 X5 X4 X3 X3 X1 X0  (X delta)
    // Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0  (Y delta)
    //
    
    UInt32 buttons = 0;
    SInt32 dx, dy;
    
    if ((packet[0] & 0x1)) buttons |= 0x1;  // left button   (bit 0 in packet)
    if ((packet[0] & 0x2)) buttons |= 0x2;  // right button  (bit 1 in packet)
    if ((packet[0] & 0x4)) buttons |= 0x4;  // middle button (bit 2 in packet)
    
    dx = packet[1];
    if (dx) {
        dx = packet[1] - ((packet[0] << 4) & 0x100);
    }
    
    dy = packet[2];
    if (dy) {
        dy = ((packet[0] << 3) & 0x100) - packet[2];
    }
    
    uint64_t now_abs;
    clock_get_uptime(&now_abs);
    IOLog("ALPS: Dispatch relative PS2 packet: dx=%d, dy=%d, buttons=%d\n", dx, dy, buttons);
    dispatchRelativePointerEventX(dx, dy, buttons, now_abs);
}
