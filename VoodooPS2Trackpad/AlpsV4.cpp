//
//  AlpsV4.cpp
//  VoodooPS2Trackpad
//
//  Created by Hades on 2/29/20.
//  Copyright © 2020 rehabman. All rights reserved.
//

#include "AlpsV4.hpp"

/*
 * Used during both passthrough mode initialization and touchpad enablement
 */


/* Must be in command mode when calling this function */
bool ALPS::alps_absolute_mode_v4() {
    int regVal;
    
    regVal = alps_command_mode_read_reg(0x0004);
    if (regVal == -1) {
        return false;
    }
    
    regVal |= 0x02;
    if (!alps_command_mode_write_reg(regVal)) {
        return false;
    }
    
    return true;
}

bool ALPS::alps_hw_init_v4() {
    
    if (!alps_enter_command_mode()) {
        goto error;
    }
    
    if (!alps_absolute_mode_v4()) {
        IOLog("ALPS: Failed to enter absolute mode\n");
        goto error;
    }
    
    if (!alps_command_mode_write_reg(0x0007, 0x8c)) {
        goto error;
    }
    
    if (!alps_command_mode_write_reg(0x0149, 0x03)) {
        goto error;
    }
    
    if (!alps_command_mode_write_reg(0x0160, 0x03)) {
        goto error;
    }
    
    if (!alps_command_mode_write_reg(0x017f, 0x15)) {
        goto error;
    }
    
    if (!alps_command_mode_write_reg(0x0151, 0x01)) {
        goto error;
    }
    
    if (!alps_command_mode_write_reg(0x0168, 0x03)) {
        goto error;
    }
    
    if (!alps_command_mode_write_reg(0x014a, 0x03)) {
        goto error;
    }
    
    if (!alps_command_mode_write_reg(0x0161, 0x03)) {
        goto error;
    }
    
    alps_exit_command_mode();
    
    /*
     * This sequence changes the output from a 9-byte to an
     * 8-byte format. All the same data seems to be present,
     * just in a more compact format.
     */
    ps2_command(0xc8, kDP_SetMouseSampleRate);
    ps2_command(0x64, kDP_SetMouseSampleRate);
    ps2_command(0x50, kDP_SetMouseSampleRate);
    ps2_command_short(kDP_GetId);
    
    /* Set rate and enable data reporting */
    ps2_command(0x64, kDP_SetMouseSampleRate);
    ps2_command_short(kDP_Enable);
    return true;
    
error:
    alps_exit_command_mode();
    return false;
}


void ALPS::alps_process_packet_v4(UInt8 *packet) {
    SInt32 offset;
    SInt32 fingers = 0;
    UInt32 buttons = 0;
    struct alps_fields f;
    
    /*
     * v4 has a 6-byte encoding for bitmap data, but this data is
     * broken up between 3 normal packets. Use priv.multi_packet to
     * track our position in the bitmap packet.
     */
    if (packet[6] & 0x40) {
        /* sync, reset position */
        priv.multi_packet = 0;
    }
    
    if (priv.multi_packet > 2) {
        return;
    }
    
    offset = 2 * priv.multi_packet;
    priv.multi_data[offset] = packet[6];
    priv.multi_data[offset + 1] = packet[7];
    
    f.left = packet[4] & 0x01;
    f.right = packet[4] & 0x02;
    
    f.st.x = ((packet[1] & 0x7f) << 4) | ((packet[3] & 0x30) >> 2) |
    ((packet[0] & 0x30) >> 4);
    f.st.y = ((packet[2] & 0x7f) << 4) | (packet[3] & 0x0f);
    f.pressure = packet[5] & 0x7f;
    
    if (++priv.multi_packet > 2) {
        priv.multi_packet = 0;
        
        f.x_map = ((priv.multi_data[2] & 0x1f) << 10) |
        ((priv.multi_data[3] & 0x60) << 3) |
        ((priv.multi_data[0] & 0x3f) << 2) |
        ((priv.multi_data[1] & 0x60) >> 5);
        f.y_map = ((priv.multi_data[5] & 0x01) << 10) |
        ((priv.multi_data[3] & 0x1f) << 5) |
        (priv.multi_data[1] & 0x1f);
        
        fingers = alps_process_bitmap(&priv, &f);
        
    }
    
    buttons |= f.left ? 0x01 : 0;
    buttons |= f.right ? 0x02 : 0;
    
    dispatchEventsWithInfo(f.st.x, f.st.y, f.pressure, fingers, buttons);
}

