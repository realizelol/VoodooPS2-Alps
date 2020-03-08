//
//  AlpsV8.cpp
//  VoodooPS2Trackpad
//
//  Created by Hades on 2/29/20.
//  Copyright © 2020 rehabman. All rights reserved.
//

#include "AlpsV8.hpp"

void ALPS::alps_get_otp_values_ss4_v2(unsigned char index)
{
    int cmd = 0;
    TPS2Request<4> request;
    
    switch (index) {
        case 0:
            ps2_command_short(kDP_SetMouseStreamMode);
            ps2_command_short(kDP_SetMouseStreamMode);
            
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
            
            break;
            
        case 1:
            ps2_command_short(kDP_MouseSetPoll);
            ps2_command_short(kDP_MouseSetPoll);
            
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
            
            break;
    }
}

void ALPS::alps_set_defaults_ss4_v2(struct alps_data *priv)
{
    alps_get_otp_values_ss4_v2(0);
    alps_get_otp_values_ss4_v2(1);
    
}


bool ALPS::alps_hw_init_ss4_v2()
{
    /* enter absolute mode */
    ps2_command_short(kDP_SetMouseStreamMode);
    ps2_command_short(kDP_SetMouseStreamMode);
    ps2_command(0x64, kDP_SetMouseSampleRate);
    ps2_command(0x28, kDP_SetMouseSampleRate);
    
    /* T.B.D. Decread noise packet number, delete in the future */
    alps_exit_command_mode();
    alps_enter_command_mode();
    alps_command_mode_write_reg(0x001D, 0x20);
    alps_exit_command_mode();
    
    /* final init */
    ps2_command_short(kDP_Enable);
    
    return true;
    
}

unsigned char ALPS::alps_get_pkt_id_ss4_v2(UInt8 *byte)
{
    unsigned char pkt_id = SS4_PACKET_ID_IDLE;
    
    switch (byte[3] & 0x30) {
        case 0x00:
            if (byte[0] == 0x18 && byte[1] == 0x10 && byte[2] == 0x00 &&
                (byte[3] & 0x88) == 0x08 && byte[4] == 0x10 &&
                byte[5] == 0x00) {
                pkt_id = SS4_PACKET_ID_IDLE;
            } else {
                pkt_id = SS4_PACKET_ID_ONE;
            }
            break;
        case 0x10:
            /* two-finger finger positions */
            pkt_id = SS4_PACKET_ID_TWO;
            break;
        case 0x20:
            /* stick pointer */
            pkt_id = SS4_PACKET_ID_STICK;
            break;
        case 0x30:
            /* third and fourth finger positions */
            pkt_id = SS4_PACKET_ID_MULTI;
            break;
    }
    
    return pkt_id;
}

bool ALPS::alps_decode_ss4_v2(struct alps_fields *f, UInt8 *p){
    
    //struct alps_data *priv;
    unsigned char pkt_id;
    unsigned int no_data_x, no_data_y;
    uint64_t now_abs;
    clock_get_uptime(&now_abs);
    
    pkt_id = alps_get_pkt_id_ss4_v2(p);
    
    /* Current packet is 1Finger coordinate packet */
    switch (pkt_id) {
        case SS4_PACKET_ID_ONE:
            f->mt[0].x = SS4_1F_X_V2(p);
            f->mt[0].y = SS4_1F_Y_V2(p);
            f->pressure = ((SS4_1F_Z_V2(p)) * 2) & 0x7f;
            /*
             * When a button is held the device will give us events
             * with x, y, and pressure of 0. This causes annoying jumps
             * if a touch is released while the button is held.
             * Handle this by claiming zero contacts.
             */
            f->fingers = f->pressure > 0 ? 1 : 0;
            f->first_mp = 0;
            f->is_mp = 0;
            break;
            
        case SS4_PACKET_ID_TWO:
            if (priv.flags & ALPS_BUTTONPAD) {
                f->mt[0].x = SS4_BTL_MF_X_V2(p, 0);
                f->mt[0].y = SS4_BTL_MF_Y_V2(p, 0);
                f->mt[1].x = SS4_BTL_MF_X_V2(p, 1);
                f->mt[1].y = SS4_BTL_MF_Y_V2(p, 1);
            } else {
                f->mt[0].x = SS4_STD_MF_X_V2(p, 0);
                f->mt[0].y = SS4_STD_MF_Y_V2(p, 0);
                f->mt[1].x = SS4_STD_MF_X_V2(p, 1);
                f->mt[1].y = SS4_STD_MF_Y_V2(p, 1);
            }
            f->pressure = SS4_MF_Z_V2(p, 0) ? 0x30 : 0;
            
            if (SS4_IS_MF_CONTINUE(p)) {
                f->first_mp = 1;
            } else {
                f->fingers = 2;
                f->first_mp = 0;
            }
            f->is_mp = 0;
            
            break;
            
        case SS4_PACKET_ID_MULTI:
            if (priv.flags & ALPS_BUTTONPAD) {
                f->mt[2].x = SS4_BTL_MF_X_V2(p, 0);
                f->mt[2].y = SS4_BTL_MF_Y_V2(p, 0);
                f->mt[3].x = SS4_BTL_MF_X_V2(p, 1);
                f->mt[3].y = SS4_BTL_MF_Y_V2(p, 1);
                no_data_x = SS4_MFPACKET_NO_AX_BL;
                no_data_y = SS4_MFPACKET_NO_AY_BL;
            } else {
                f->mt[2].x = SS4_STD_MF_X_V2(p, 0);
                f->mt[2].y = SS4_STD_MF_Y_V2(p, 0);
                f->mt[3].x = SS4_STD_MF_X_V2(p, 1);
                f->mt[3].y = SS4_STD_MF_Y_V2(p, 1);
                no_data_x = SS4_MFPACKET_NO_AX;
                no_data_y = SS4_MFPACKET_NO_AY;
            }
            
            f->first_mp = 0;
            f->is_mp = 1;
            
            if (SS4_IS_5F_DETECTED(p)) {
                f->fingers = 5;
            } else if (f->mt[3].x == no_data_x &&
                       f->mt[3].y == no_data_y) {
                f->mt[3].x = 0;
                f->mt[3].y = 0;
                f->fingers = 3;
            } else {
                f->fingers = 4;
            }
            break;
            
        case SS4_PACKET_ID_STICK:
            /*
             * x, y, and pressure are decoded in
             * alps_process_packet_ss4_v2()
             */
            f->first_mp = 0;
            f->is_mp = 0;
            break;
            
        case SS4_PACKET_ID_IDLE:
        default:
            memset(f, 0, sizeof(struct alps_fields));
            break;
    }
    
    /* handle buttons */
    if (pkt_id == SS4_PACKET_ID_STICK) {
        f->ts_left = !!(SS4_BTN_V2(p) & 0x01);
        if (!(priv.flags & ALPS_BUTTONPAD)) {
            f->ts_right = !!(SS4_BTN_V2(p) & 0x02);
            f->ts_middle = !!(SS4_BTN_V2(p) & 0x04);
        }
    } else {
        f->left = !!(SS4_BTN_V2(p) & 0x01);
        if (!(priv.flags & ALPS_BUTTONPAD)) {
            f->right = !!(SS4_BTN_V2(p) & 0x02);
            f->middle = !!(SS4_BTN_V2(p) & 0x04);
        }
    }
    return true;
}

void ALPS::alps_process_packet_ss4_v2(UInt8 *packet) {
    int buttons = 0;
    struct alps_fields f;
    int x, y, pressure;
    
    uint64_t now_abs;
    clock_get_uptime(&now_abs);
    
    memset(&f, 0, sizeof(struct alps_fields));
    (this->*decode_fields)(&f, packet);
    if (priv.multi_packet) {
        /*
         * Sometimes the first packet will indicate a multi-packet
         * sequence, but sometimes the next multi-packet would not
         * come. Check for this, and when it happens process the
         * position packet as usual.
         */
        if (f.is_mp) {
            /* Now process the 1st packet */
            (this->*decode_fields)(&f, priv.multi_data);
        } else {
            priv.multi_packet = 0;
        }
    }
    
    /*
     * "f.is_mp" would always be '0' after merging the 1st and 2nd packet.
     * When it is set, it means 2nd packet comes without 1st packet come.
     */
    if (f.is_mp) {
        return;
    }
    
    /* Save the first packet */
    if (!priv.multi_packet && f.first_mp) {
        priv.multi_packet = 1;
        memcpy(priv.multi_data, packet, sizeof(priv.multi_data));
        return;
    }
    
    priv.multi_packet = 0;
    
    /* Report trackstick */
    if (alps_get_pkt_id_ss4_v2(packet) == SS4_PACKET_ID_STICK) {
        if (!(priv.flags & ALPS_DUALPOINT)) {
            IOLog("ALPS: Rejected trackstick packet from non DualPoint device\n");
            return;
        }
        
        x = (((packet[0] & 1) << 7) | (packet[1] & 0x7f));
        y = (((packet[3] & 1) << 7) | (packet[2] & 0x7f));
        pressure = (packet[4] & 0x7f);
        
        buttons |= f.ts_left ? 0x01 : 0;
        buttons |= f.ts_right ? 0x02 : 0;
        buttons |= f.ts_middle ? 0x04 : 0;
        
        if ((abs(x) >= 0x7f) || (abs(y) >= 0x7f)) {
            return;
        }
        
        //TODO: V8 Trackstick: Someone with the hardware needs to debug this.
        //IOLog("ALPS: Trackstick report: X=%d, Y=%d, Z=%d, buttons=%d\n", x, y, pressure, buttons);
        //dispatchRelativePointerEventX(x, y, 0, now_abs);
        return;
    }
    
    /* Report touchpad */
    buttons |= f.left ? 0x01 : 0;
    buttons |= f.right ? 0x02 : 0;
    buttons |= f.middle ? 0x04 : 0;
    
    /* Reverse y co-ordinates to have 0 at bottom for gestures to work */
    f.mt[0].y = priv.y_max - f.mt[0].y;
    f.mt[1].y = priv.y_max - f.mt[1].y;
    
    dispatchEventsWithInfo(f.mt[0].x, f.mt[0].y, f.pressure, f.fingers, buttons);
}
