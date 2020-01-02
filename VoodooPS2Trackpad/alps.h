/*
 * Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.2 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
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

#include "VoodooPS2TrackpadBase.h"
#include "VoodooPS2AlpsTrackpadKonstants.h"

/*
 * enum V7_PACKET_ID - defines the packet type for V7
 * V7_PACKET_ID_IDLE: There's no finger and no button activity.
 * V7_PACKET_ID_TWO: There's one or two non-resting fingers on touchpad
 *  or there's button activities.
 * V7_PACKET_ID_MULTI: There are at least three non-resting fingers.
 * V7_PACKET_ID_NEW: The finger position in slot is not continues from
 *  previous packet.
 */
enum V7_PACKET_ID {
    V7_PACKET_ID_IDLE,
    V7_PACKET_ID_TWO,
    V7_PACKET_ID_MULTI,
    V7_PACKET_ID_NEW,
    V7_PACKET_ID_UNKNOWN,
};

class ALPS;


// Pulled out of alps_data, now saved as vars on class
// makes invoking a little easier
typedef bool (ALPS::*hw_init)();
typedef bool (ALPS::*decode_fields)(struct alps_fields *f, UInt8 *p);
typedef void (ALPS::*process_packet)(UInt8 *packet);
//typedef void (ALPS::*set_abs_params)();


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ALPS Class Declaration
//



// predeclure stuff
struct alps_data;

class EXPORT ALPS : public VoodooPS2TouchPadBase {
    typedef VoodooPS2TouchPadBase super;
    OSDeclareDefaultStructors( ALPS );
    
private:
    hw_init hw_init;
    decode_fields decode_fields;
    process_packet process_packet;
    
    IOService *voodooInputInstance;
    VoodooInputEvent inputEvent;
    //    set_abs_params set_abs_params;
    
public:
    virtual ALPS * probe(IOService *provider, SInt32 *score);
    
    bool init(OSDictionary * dict);
    
    void stop(IOService *provider);
    
protected:
    int _multiPacket;
    
    UInt8 _multiData[6];
    
    IOGBounds _bounds;
    
    virtual bool deviceSpecificInit();
    
    bool resetMouse();
    
    void alps_process_packet_v1_v2(UInt8 *packet);
    
    int alps_process_bitmap(struct alps_data *priv, struct alps_fields *f);
    
    void alps_process_trackstick_packet_v3(UInt8 * packet);
    
    bool alps_decode_buttons_v3(struct alps_fields *f, UInt8 *p);
    
    bool alps_decode_pinnacle(struct alps_fields *f, UInt8 *p);
    
    bool alps_decode_rushmore(struct alps_fields *f, UInt8 *p);
    
    bool alps_decode_dolphin(struct alps_fields *f, UInt8 *p);
    
    void alps_process_touchpad_packet_v3_v5(UInt8 * packet);
    
    void alps_process_packet_v3(UInt8 *packet);
    
    void alps_process_packet_v6(UInt8 *packet);
    
    void alps_process_packet_v4(UInt8 *packet);
    
    unsigned char alps_get_packet_id_v7(UInt8 *byte);
    
    void alps_get_finger_coordinate_v7(struct input_mt_pos *mt, UInt8 *pkt, UInt8 pkt_id);
    
    int alps_get_mt_count(struct input_mt_pos *mt);
    
    bool alps_decode_packet_v7(struct alps_fields *f, UInt8 *p);
    
    void alps_process_trackstick_packet_v7(UInt8 *packet);
    
    void alps_process_touchpad_packet_v7(UInt8 *packet);
    
    void alps_process_packet_v7(UInt8 *packet);
    
    void dispatchEventsWithInfo(int xraw, int yraw, int z, int fingers, UInt32 buttonsraw);
    
    virtual void dispatchRelativePointerEventWithPacket(UInt8 *packet, UInt32 packetSize);
    
    void setTouchPadEnable(bool enable);
    
    PS2InterruptResult interruptOccurred(UInt8 data);
    
    void packetReady();
    
    bool alps_command_mode_send_nibble(int value);
    
    bool alps_command_mode_set_addr(int addr);
    
    int alps_command_mode_read_reg(int addr);
    
    bool alps_command_mode_write_reg(int addr, UInt8 value);
    
    bool alps_command_mode_write_reg(UInt8 value);
    
    bool alps_rpt_cmd(SInt32 init_command, SInt32 init_arg, SInt32 repeated_command, ALPSStatus_t *report);
    
    bool alps_enter_command_mode();
    
    bool alps_exit_command_mode();
    
    bool alps_passthrough_mode_v2(bool enable);
        
    bool alps_absolute_mode_v1_v2();
    
    int alps_monitor_mode_send_word(int word);
    
    int alps_monitor_mode_write_reg(int addr, int value);
    
    int alps_monitor_mode(bool enable);
    
    void alps_absolute_mode_v6();
    
    bool alps_get_status(ALPSStatus_t *status);
    
    bool alps_tap_mode(bool enable);
    
    bool alps_hw_init_v1_v2();
    
    bool alps_hw_init_v6();
    
    bool alps_passthrough_mode_v3(int regBase, bool enable);
    
    bool alps_absolute_mode_v3();
    
    IOReturn alps_probe_trackstick_v3_v7(int regBase);
    
    IOReturn alps_setup_trackstick_v3(int regBase);
    
    bool alps_hw_init_v3();
    
    bool alps_get_v3_v7_resolution(int reg_pitch);
    
    bool alps_hw_init_rushmore_v3();
    
    bool alps_absolute_mode_v4();
    
    bool alps_hw_init_v4();
    
    void alps_get_otp_values_ss4_v2(unsigned char index);
    
    void alps_set_defaults_ss4_v2(struct alps_data *priv);
    
    int alps_dolphin_get_device_area(struct alps_data *priv);
    
    bool alps_hw_init_dolphin_v1();
    
    bool alps_hw_init_v7();
    
    void ps2_command_short(UInt8 command);
    
    void ps2_command(unsigned char value, UInt8 command);
        
    void set_protocol();
    
    bool matchTable(ALPSStatus_t *e7, ALPSStatus_t *ec);
  
    bool alps_hw_init_ss4_v2();
  
    IOReturn identify();
    
    void restart();
};
