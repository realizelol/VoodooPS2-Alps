//
//  V35Protocol.cpp
//  VoodooPS2Trackpad
//
//  Created by Hades on 1/1/20.
//  Copyright © 2020 rehabman. All rights reserved.
//

#include "V35Protocol.h"

// =============================================================================
// ALPS Class Implementation  //////////////////////////////////////////////////
// =============================================================================

OSDefineMetaClassAndStructors(V35Protocol, VoodooPS2TouchPadBase);

/* ===============================||\\alps.c from linux 4.4//||================================== */

bool V35Protocol::decode_buttons_v3(struct alps_fields *f, unsigned char *p) {
    f->left = !!(p[3] & 0x01);
    f->right = !!(p[3] & 0x02);
    f->middle = !!(p[3] & 0x04);
    
    f->ts_left = !!(p[3] & 0x10);
    f->ts_right = !!(p[3] & 0x20);
    f->ts_middle = !!(p[3] & 0x40);
    return true;
}

bool V35Protocol::decode_pinnacle(struct alps_fields *f, UInt8 *p) {
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
        
        decode_buttons_v3(f, p);
    }
    return true;
}

bool V35Protocol::decode_rushmore(struct alps_fields *f, UInt8 *p) {
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
        
        decode_buttons_v3(f, p);
    }
    return true;
}

bool V35Protocol::decode_dolphin(struct alps_fields *f, UInt8 *p) {
    uint64_t palm_data = 0;
    
    f->first_mp = !!(p[0] & 0x02);
    f->is_mp = !!(p[0] & 0x20);
    
    if (!f->is_mp) {
        f->st.x = ((p[1] & 0x7f) | ((p[4] & 0x0f) << 7));
        f->st.y = ((p[2] & 0x7f) | ((p[4] & 0xf0) << 3));
        f->pressure = (p[0] & 4) ? 0 : p[5] & 0x7f;
        decode_buttons_v3(f, p);
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
        f->y_map = palm_data & (BIT(priv->y_bits) - 1);
        
        /* X-profile is stored in p(n) to p(n+m-1), m = x_bits; */
        f->x_map = (palm_data >> priv->y_bits) &
        (BIT(priv->x_bits) - 1);
    }
    return true;
}

struct alps_fields* V35Protocol::processPacketTouchpad(struct alps_fields* f, UInt8 *packet) {
    //ffff
    int fingers = 0, buttons = 0;
    
    switch (priv->proto_version) {
        case ALPS_PROTO_V3:
            decode_pinnacle(f, packet);
            break;
            
        case ALPS_PROTO_V3_RUSHMORE:
            decode_rushmore(f, packet);
            break;
            
        case ALPS_PROTO_V5:
            decode_dolphin(f, packet);
            break;
    }
    
    /*
     * There's no single feature of touchpad position and bitmap packets
     * that can be used to distinguish between them. We rely on the fact
     * that a bitmap packet should always follow a position packet with
     * bit 6 of packet[4] set.
     */
    if (priv->multi_packet) {
        /*
         * Sometimes a position packet will indicate a multi-packet
         * sequence, but then what follows is another position
         * packet. Check for this, and when it happens process the
         * position packet as usual.
         */
        if (f->is_mp) {
            fingers = f->fingers;
            /*
             * Bitmap processing uses position packet's coordinate
             * data, so we need to do decode it first.
             */

            switch (priv->proto_version) {
                case ALPS_PROTO_V3:
                    decode_pinnacle(f, priv->multi_data);
                    break;
                    
                case ALPS_PROTO_V3_RUSHMORE:
                    decode_rushmore(f, priv->multi_data);
                    break;
                    
                case ALPS_PROTO_V5:
                    decode_dolphin(f, priv->multi_data);
                    break;
            }
            
            if (process_bitmap(priv, f) == 0) {
                fingers = 0; /* Use st data */
            }
        } else {
            priv->multi_packet = 0;
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
    if (f->is_mp) {
        return NULL;
    }
    
    if (!priv->multi_packet && (f->first_mp)) {
        priv->multi_packet = 1;
        memcpy(priv->multi_data, packet, sizeof(priv->multi_data));
        return NULL;
    }
    
    priv->multi_packet = 0;
    
    /*
     * Sometimes the hardware sends a single packet with z = 0
     * in the middle of a stream. Real releases generate packets
     * with x, y, and z all zero, so these seem to be flukes.
     * Ignore them.
     */
    if (f->st.x && f->st.y && !f->pressure) {
        //return; //Dr Hurt: This causes jitter
    }
    
    /* Use st data when we don't have mt data */
    if (fingers < 2) {
        f->mt[0].x = f->st.x;
        f->mt[0].y = f->st.y;
        fingers = f->pressure > 0 ? 1 : 0;
        priv->second_touch = -1;
    }
    
    buttons |= f->left ? 0x01 : 0;
    buttons |= f->right ? 0x02 : 0;
    buttons |= f->middle ? 0x04 : 0;
    
    if ((priv->flags & ALPS_DUALPOINT) &&
        !(priv->quirks & ALPS_QUIRK_TRACKSTICK_BUTTONS)) {
        buttons |= f->ts_left ? 0x01 : 0;
        buttons |= f->ts_right ? 0x02 : 0;
        buttons |= f->ts_middle ? 0x04 : 0;
    }
    
    /* Reverse y co-ordinates to have 0 at bottom for gestures to work */
    f->mt[0].y = priv->y_max - f->mt[0].y;
    f->mt[1].y = priv->y_max - f->mt[1].y;
    
    
    return f;
  
}

void V35Protocol::process_trackstick_packet_v3(UInt8 *packet) {
    int x, y, z, left, right, middle;
    uint64_t now_abs;
    UInt32 buttons = 0, raw_buttons = 0;
    
    /* It should be a DualPoint when received trackstick packet */
    if (!(priv->flags & ALPS_DUALPOINT)) {
        return;
    }
    
    /* Sanity check packet */
    if (!(packet[0] & 0x40)) {
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
    
    if (!(priv->quirks & ALPS_QUIRK_TRACKSTICK_BUTTONS) &&
        (left || middle || right)) {
        priv->quirks |= ALPS_QUIRK_TRACKSTICK_BUTTONS;
    }
    
    if (priv->quirks & ALPS_QUIRK_TRACKSTICK_BUTTONS) {
        raw_buttons |= left ? 0x01 : 0;
        raw_buttons |= right ? 0x02 : 0;
        raw_buttons |= middle ? 0x04 : 0;
    }
    
    /* Button status can appear in normal packet */
    if (raw_buttons == 0) {
        buttons = lastButtons;
    } else {
        buttons = raw_buttons;
        lastButtons = buttons;
    }
    
//    /* If middle button is pressed, switch to scroll mode. Else, move pointer normally */
//    if (0 == (buttons & 0x04)) {
//        dispatchRelativePointerEventX(x, y, buttons, now_abs);
//    } else {
//        dispatchScrollWheelEventX(-y, -x, 0, now_abs);
//    }
}

struct alps_fields* V35Protocol::processPacket(struct alps_fields *f, UInt8 *packet) {
    /*
     * v3 protocol packets come in three types, two representing
     * touchpad data and one representing trackstick data.
     * Trackstick packets seem to be distinguished by always
     * having 0x3f in the last byte. This value has never been
     * observed in the last byte of either of the other types
     * of packets.
     */

    // V5 has no trackstick packet
    if (priv->proto_version != ALPS_PROTO_V5 && packet[5] == 0x3f) {
        process_trackstick_packet_v3(packet);
        return NULL;
    }
    
    return processPacketTouchpad(f, packet);
}
