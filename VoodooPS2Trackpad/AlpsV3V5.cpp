//
//  AlpsV3V5.cpp
//  VoodooPS2Trackpad
//
//  Created by Hades on 2/29/20.
//  Copyright © 2020 rehabman. All rights reserved.
//

#include "AlpsV3V5.h"

void AlpsV3V5Touchpad::alps_process_trackstick_packet_v3(UInt8 *packet) {
    int x, y, z, left, right, middle;
    uint64_t now_abs;
    UInt32 buttons = 0, raw_buttons = 0;
    
    /* It should be a DualPoint when received trackstick packet */
    if (!(priv.flags & ALPS_DUALPOINT)) {
        return;
    }
    
    /* Sanity check packet */
    if (!(packet[0] & 0x40)) {
        DEBUG_LOG("ps2: bad trackstick packet, disregarding...\n");
        return;
    }
    
    /* There is a special packet that seems to indicate the end
     * of a stream of trackstick data. Filter these out
     */
    if (packet[1] == 0x7f && packet[2] == 0x7f && packet[3] == 0x7f) {
        return;
    }
    
    x = (SInt8) (((packet[0] & 0x20) << 2) | (packet[1] & 0x7f));
    y = (SInt8) (((packet[0] & 0x10) << 3) | (packet[2] & 0x7f));
    z = (packet[4] & 0x7c) >> 2;
    
    /* Prevent pointer jump on finger lift */
    if ((abs(x) >= 0x7f) && (abs(y) >= 0x7f)) {
        x = y = 0;
    }
    
    /*
     * The x and y values tend to be quite large, and when used
     * alone the trackstick is difficult to use. Scale them down
     * to compensate.
     */
    x /= 3;
    y /= 3;
    
    /* To get proper movement direction */
    y = -y;
    
    clock_get_uptime(&now_abs);
    
    /*
     * Most ALPS models report the trackstick buttons in the touchpad
     * packets, but a few report them here. No reliable way has been
     * found to differentiate between the models upfront, so we enable
     * the quirk in response to seeing a button press in the trackstick
     * packet.
     */
    left = packet[3] & 0x01;
    right = packet[3] & 0x02;
    middle = packet[3] & 0x04;
    
    if (!(priv.quirks & ALPS_QUIRK_TRACKSTICK_BUTTONS) &&
        (left || middle || right)) {
        priv.quirks |= ALPS_QUIRK_TRACKSTICK_BUTTONS;
    }
    
    if (priv.quirks & ALPS_QUIRK_TRACKSTICK_BUTTONS) {
        raw_buttons |= left ? 0x01 : 0;
        raw_buttons |= right ? 0x02 : 0;
        raw_buttons |= middle ? 0x04 : 0;
    }
    
    /* Button status can appear in normal packet */
    if (0 == raw_buttons) {
        buttons = lastbuttons;
    } else {
        buttons = raw_buttons;
        lastbuttons = buttons;
    }
    
    /* If middle button is pressed, switch to scroll mode. Else, move pointer normally */
    if (0 == (buttons & 0x04)) {
        dispatchRelativePointerEventX(x, y, buttons, now_abs);
    } else {
        dispatchScrollWheelEventX(-y, -x, 0, now_abs);
    }
}

bool AlpsV3V5Touchpad::alps_decode_buttons_v3(struct alps_fields *f, unsigned char *p) {
    f->left = !!(p[3] & 0x01);
    f->right = !!(p[3] & 0x02);
    f->middle = !!(p[3] & 0x04);
    
    f->ts_left = !!(p[3] & 0x10);
    f->ts_right = !!(p[3] & 0x20);
    f->ts_middle = !!(p[3] & 0x40);
    return true;
}

bool AlpsV3V5Touchpad::alps_decode_pinnacle(struct alps_fields *f, UInt8 *p) {
    f->first_mp = !!(p[4] & 0x40);
    f->is_mp = !!(p[0] & 0x40);
    
    if (f->is_mp) {
        f->fingers = (p[5] & 0x3) + 1;
        f->x_map = ((p[4] & 0x7e) << 8) |
        ((p[1] & 0x7f) << 2) |
        ((p[0] & 0x30) >> 4);
        f->y_map = ((p[3] & 0x70) << 4) |
        ((p[2] & 0x7f) << 1) |
        (p[4] & 0x01);
    } else {
        f->st.x = ((p[1] & 0x7f) << 4) | ((p[4] & 0x30) >> 2) |
        ((p[0] & 0x30) >> 4);
        f->st.y = ((p[2] & 0x7f) << 4) | (p[4] & 0x0f);
        f->pressure = p[5] & 0x7f;
        
        alps_decode_buttons_v3(f, p);
    }
    return true;
}

bool AlpsV3V5Touchpad::alps_decode_rushmore(struct alps_fields *f, UInt8 *p) {
    f->first_mp = !!(p[4] & 0x40);
    f->is_mp = !!(p[5] & 0x40);
    
    if (f->is_mp) {
        f->fingers = max((p[5] & 0x3), ((p[5] >> 2) & 0x3)) + 1;
        f->x_map = ((p[5] & 0x10) << 11) |
        ((p[4] & 0x7e) << 8) |
        ((p[1] & 0x7f) << 2) |
        ((p[0] & 0x30) >> 4);
        f->y_map = ((p[5] & 0x20) << 6) |
        ((p[3] & 0x70) << 4) |
        ((p[2] & 0x7f) << 1) |
        (p[4] & 0x01);
    } else {
        f->st.x = ((p[1] & 0x7f) << 4) | ((p[4] & 0x30) >> 2) |
        ((p[0] & 0x30) >> 4);
        f->st.y = ((p[2] & 0x7f) << 4) | (p[4] & 0x0f);
        f->pressure = p[5] & 0x7f;
        
        alps_decode_buttons_v3(f, p);
    }
    return true;
}

bool AlpsV3V5Touchpad::alps_decode_dolphin(struct alps_fields *f, UInt8 *p) {
    uint64_t palm_data = 0;
    
    f->first_mp = !!(p[0] & 0x02);
    f->is_mp = !!(p[0] & 0x20);
    
    if (!f->is_mp) {
        f->st.x = ((p[1] & 0x7f) | ((p[4] & 0x0f) << 7));
        f->st.y = ((p[2] & 0x7f) | ((p[4] & 0xf0) << 3));
        f->pressure = (p[0] & 4) ? 0 : p[5] & 0x7f;
        alps_decode_buttons_v3(f, p);
    } else {
        f->fingers = ((p[0] & 0x6) >> 1 |
                      (p[0] & 0x10) >> 2);
        
        palm_data = (p[1] & 0x7f) |
        ((p[2] & 0x7f) << 7) |
        ((p[4] & 0x7f) << 14) |
        ((p[5] & 0x7f) << 21) |
        ((p[3] & 0x07) << 28) |
        (((uint64_t)p[3] & 0x70) << 27) |
        (((uint64_t)p[0] & 0x01) << 34);
        
        /* Y-profile is stored in P(0) to p(n-1), n = y_bits; */
        f->y_map = palm_data & (BIT(priv.y_bits) - 1);
        
        /* X-profile is stored in p(n) to p(n+m-1), m = x_bits; */
        f->x_map = (palm_data >> priv.y_bits) &
        (BIT(priv.x_bits) - 1);
    }
    return true;
}

void AlpsV3V5Touchpad::alps_process_touchpad_packet_v3_v5(UInt8 *packet) {
    //ffff
    int fingers = 0, buttons = 0;
    struct alps_fields f;
    
    memset(&f, 0, sizeof(f));
    
    (this->*decode_fields)(&f, packet);
    /*
     * There's no single feature of touchpad position and bitmap packets
     * that can be used to distinguish between them. We rely on the fact
     * that a bitmap packet should always follow a position packet with
     * bit 6 of packet[4] set.
     */
    if (priv.multi_packet) {
        /*
         * Sometimes a position packet will indicate a multi-packet
         * sequence, but then what follows is another position
         * packet. Check for this, and when it happens process the
         * position packet as usual.
         */
        if (f.is_mp) {
            fingers = f.fingers;
            /*
             * Bitmap processing uses position packet's coordinate
             * data, so we need to do decode it first.
             */
            (this->*decode_fields)(&f, priv.multi_data);
            if (alps_process_bitmap(&priv, &f) == 0) {
                fingers = 0; /* Use st data */
            }
        } else {
            priv.multi_packet = 0;
        }
    }
    
    /*
     * Bit 6 of byte 0 is not usually set in position packets. The only
     * times it seems to be set is in situations where the data is
     * suspect anyway, e.g. a palm resting flat on the touchpad. Given
     * this combined with the fact that this bit is useful for filtering
     * out misidentified bitmap packets, we reject anything with this
     * bit set.
     */
    if (f.is_mp) {
        return;
    }
    
    if (!priv.multi_packet && (f.first_mp)) {
        priv.multi_packet = 1;
        memcpy(priv.multi_data, packet, sizeof(priv.multi_data));
        return;
    }
    
    priv.multi_packet = 0;
    
    /*
     * Sometimes the hardware sends a single packet with z = 0
     * in the middle of a stream. Real releases generate packets
     * with x, y, and z all zero, so these seem to be flukes.
     * Ignore them.
     */
    if (f.st.x && f.st.y && !f.pressure) {
        //return; //Dr Hurt: This causes jitter
    }
    
    /* Use st data when we don't have mt data */
    if (fingers < 2) {
        f.mt[0].x = f.st.x;
        f.mt[0].y = f.st.y;
        fingers = f.pressure > 0 ? 1 : 0;
        priv.second_touch = -1;
    }
    
    buttons |= f.left ? 0x01 : 0;
    buttons |= f.right ? 0x02 : 0;
    buttons |= f.middle ? 0x04 : 0;
    
    if ((priv.flags & ALPS_DUALPOINT) &&
        !(priv.quirks & ALPS_QUIRK_TRACKSTICK_BUTTONS)) {
        buttons |= f.ts_left ? 0x01 : 0;
        buttons |= f.ts_right ? 0x02 : 0;
        buttons |= f.ts_middle ? 0x04 : 0;
    }
    
    /* Reverse y co-ordinates to have 0 at bottom for gestures to work */
    f.mt[0].y = priv.y_max - f.mt[0].y;
    f.mt[1].y = priv.y_max - f.mt[1].y;
    
    /* Ignore 1 finger events after 2 finger scroll to prevent jitter */
    if (last_fingers == 2 && fingers == 1 && scrolldebounce) {
        //fingers = 2;
    }
    
    dispatchEventsWithInfo(f.mt[0].x, f.mt[0].y, f.pressure, fingers, buttons);
}

void AlpsV3V5Touchpad::alps_process_packet_v3(UInt8 *packet) {
    /*
     * v3 protocol packets come in three types, two representing
     * touchpad data and one representing trackstick data.
     * Trackstick packets seem to be distinguished by always
     * having 0x3f in the last byte. This value has never been
     * observed in the last byte of either of the other types
     * of packets.
     */
    if (packet[5] == 0x3f) {
        alps_process_trackstick_packet_v3(packet);
        return;
    }
    
    alps_process_touchpad_packet_v3_v5(packet);
}

int ALPS::alps_dolphin_get_device_area(struct alps_data *priv)
{
    int num_x_electrode, num_y_electrode;
    TPS2Request<4> request;
    int cmd = 0;
    ALPSStatus_t status;
    
    alps_enter_command_mode();
    
    ps2_command_short(kDP_MouseResetWrap);
    ps2_command_short(kDP_MouseSetPoll);
    ps2_command_short(kDP_MouseSetPoll);
    ps2_command(0x0a, kDP_SetMouseSampleRate);
    ps2_command(0x0a, kDP_SetMouseSampleRate);
    
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
    
    /* results */
    status.bytes[0] = request.commands[1].inOrOut;
    status.bytes[1] = request.commands[2].inOrOut;
    status.bytes[2] = request.commands[3].inOrOut;
    
    num_x_electrode = DOLPHIN_PROFILE_XOFFSET + (status.bytes[2] & 0x0F);
    num_y_electrode = DOLPHIN_PROFILE_YOFFSET + ((status.bytes[2] >> 4) & 0x0F);
    priv->x_bits = num_x_electrode;
    priv->y_bits = num_y_electrode;
    priv->x_max = (num_x_electrode - 1) * DOLPHIN_COUNT_PER_ELECTRODE;
    priv->y_max = (num_y_electrode - 1) * DOLPHIN_COUNT_PER_ELECTRODE;
    
    alps_exit_command_mode();
    
    return 0;
}

bool ALPS::alps_hw_init_dolphin_v1() {
    
    ps2_command_short(kDP_SetMouseStreamMode);
    ps2_command(0x64, kDP_SetMouseSampleRate);
    ps2_command(0x28, kDP_SetMouseSampleRate);
    ps2_command_short(kDP_Enable);
    
    return true;
}

bool ALPS::alps_hw_init_rushmore_v3() {
    int regVal;
    
    if (priv.flags & ALPS_DUALPOINT) {
        regVal = alps_setup_trackstick_v3(ALPS_REG_BASE_RUSHMORE);
        if (regVal == kIOReturnIOError) {
            goto error;
        }
    }
    
    if (!alps_enter_command_mode() ||
        alps_command_mode_read_reg(0xc2d9) == -1 ||
        !alps_command_mode_write_reg(0xc2cb, 0x00)) {
        goto error;
    }
    
    regVal = alps_command_mode_read_reg(0xc2c6);
    if (regVal == -1)
        goto error;
    if (!alps_command_mode_write_reg(regVal & 0xfd))
        goto error;
    
    if (!alps_command_mode_write_reg(0xc2c9, 0x64))
        goto error;
    
    /* enter absolute mode */
    regVal = alps_command_mode_read_reg(0xc2c4);
    if (regVal == -1)
        goto error;
    if (!alps_command_mode_write_reg(regVal | 0x02))
        goto error;
    
    alps_exit_command_mode();
    
    /* Enable data reporting */
    ps2_command_short(kDP_Enable);
    
    return true;
    
error:
    alps_exit_command_mode();
    return false;
}

bool ALPS::alps_hw_init_v3() {
    int regVal;
    
    if ((priv.flags & ALPS_DUALPOINT) &&
        alps_setup_trackstick_v3(ALPS_REG_BASE_PINNACLE) == kIOReturnIOError)
        goto error;
    
    if (!(alps_enter_command_mode() &&
          alps_absolute_mode_v3())) {
        IOLog("ALPS: Failed to enter absolute mode\n");
        goto error;
    }
    
    regVal = alps_command_mode_read_reg(0x0006);
    if (regVal == -1)
        goto error;
    if (!alps_command_mode_write_reg(regVal | 0x01))
        goto error;
    
    regVal = alps_command_mode_read_reg(0x0007);
    if (regVal == -1)
        goto error;
    if (!alps_command_mode_write_reg(regVal | 0x01))
        goto error;
    
    if (alps_command_mode_read_reg(0x0144) == -1)
        goto error;
    if (!alps_command_mode_write_reg(0x04))
        goto error;
    
    if (alps_command_mode_read_reg(0x0159) == -1)
        goto error;
    if (!alps_command_mode_write_reg(0x03))
        goto error;
    
    if (alps_command_mode_read_reg(0x0163) == -1)
        goto error;
    if (!alps_command_mode_write_reg(0x0163, 0x03))
        goto error;
    
    if (alps_command_mode_read_reg(0x0162) == -1)
        goto error;
    if (!alps_command_mode_write_reg(0x0162, 0x04))
        goto error;
    
    alps_exit_command_mode();
    
    /* Set rate and enable data reporting */
    ps2_command(0x28, kDP_SetMouseSampleRate);
    ps2_command_short(kDP_Enable);
    
    return true;
    
error:
    alps_exit_command_mode();
    return false;
}

IOReturn ALPS::alps_setup_trackstick_v3(int regBase) {
    IOReturn ret = 0;
    ALPSStatus_t report;
    TPS2Request<3> request;
    
    if (!alps_passthrough_mode_v3(regBase, true)) {
        return kIOReturnIOError;
    }
    
    /*
     * E7 report for the trackstick
     *
     * There have been reports of failures to seem to trace back
     * to the above trackstick check failing. When these occur
     * this E7 report fails, so when that happens we continue
     * with the assumption that there isn't a trackstick after
     * all.
     */
    if (!alps_rpt_cmd(NULL, NULL, kDP_SetMouseScaling2To1, &report)) {
        IOLog("ALPS: trackstick E7 report failed\n");
        ret = kIOReturnNoDevice;
    } else {
        /*
         * Not sure what this does, but it is absolutely
         * essential. Without it, the touchpad does not
         * work at all and the trackstick just emits normal
         * PS/2 packets.
         */
        request.commands[0].command = kPS2C_SendMouseCommandAndCompareAck;
        request.commands[0].inOrOut = kDP_SetMouseScaling1To1;
        request.commands[1].command = kPS2C_SendMouseCommandAndCompareAck;
        request.commands[1].inOrOut = kDP_SetMouseScaling1To1;
        request.commands[2].command = kPS2C_SendMouseCommandAndCompareAck;
        request.commands[2].inOrOut = kDP_SetMouseScaling1To1;
        request.commandsCount = 3;
        assert(request.commandsCount <= countof(request.commands));
        _device->submitRequestAndBlock(&request);
        if (request.commandsCount != 3) {
            IOLog("ALPS: error sending magic E6 scaling sequence\n");
            ret = kIOReturnIOError;
            goto error;
        }
        if (!(alps_command_mode_send_nibble(0x9) && alps_command_mode_send_nibble(0x4))) {
            IOLog("ALPS: error sending magic E6 nibble sequence\n");
            ret = kIOReturnIOError;
            goto error;
        }
        DEBUG_LOG("Sent magic E6 sequence\n");
        
        /*
         * This ensures the trackstick packets are in the format
         * supported by this driver. If bit 1 isn't set the packet
         * format is different.
         */
        if (!(alps_enter_command_mode() &&
              alps_command_mode_write_reg(regBase + 0x0008, 0x82) &&
              alps_exit_command_mode())) {
            ret = -kIOReturnIOError;
            //goto error;
        }
    }
error:
    if (!alps_passthrough_mode_v3(regBase, false)) {
        ret = kIOReturnIOError;
    }
    
    return ret;
}

bool ALPS::alps_passthrough_mode_v3(int regBase, bool enable) {
    int regVal;
    bool ret = false;
    
    DEBUG_LOG("passthrough mode enable=%d\n", enable);
    
    if (!alps_enter_command_mode()) {
        IOLog("ALPS: Failed to enter command mode while enabling passthrough mode\n");
        return false;
    }
    
    regVal = alps_command_mode_read_reg(regBase + 0x0008);
    if (regVal == -1) {
        IOLog("ALPS: Failed to read register while setting up passthrough mode\n");
        goto error;
    }
    
    if (enable) {
        regVal |= 0x01;
    } else {
        regVal &= ~0x01;
    }
    
    ret = alps_command_mode_write_reg(regVal);
    
error:
    if (!alps_exit_command_mode()) {
        IOLog("ALPS: failed to exit command mode while enabling passthrough mode v3\n");
        return false;
    }
    
    return ret;
}

bool ALPS::alps_absolute_mode_v3() {
    
    int regVal;
    
    regVal = alps_command_mode_read_reg(0x0004);
    if (regVal == -1) {
        return false;
    }
    
    regVal |= 0x06;
    if (!alps_command_mode_write_reg(regVal)) {
        return false;
    }
    
    return true;
}
