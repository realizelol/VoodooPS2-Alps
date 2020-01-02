//
//  VoodooPS2AlpsTrackpadKonstants.h
//  VoodooPS2Trackpad
//
//  Created by Hades on 12/31/19.
//  Copyright © 2019 rehabman. All rights reserved.
//

#ifndef VoodooPS2AlpsTrackpadKonstants_h
#define VoodooPS2AlpsTrackpadKonstants_h

#include <IOKit/hidsystem/IOHIPointing.h>
#include <IOKit/IOCommandGate.h>

#define ALPS_PROTO_V1  0x100
#define ALPS_PROTO_V2  0x200
#define ALPS_PROTO_V3  0x300
#define ALPS_PROTO_V3_RUSHMORE  0x310
#define ALPS_PROTO_V4  0x400
#define ALPS_PROTO_V5  0x500
#define ALPS_PROTO_V6  0x600
#define ALPS_PROTO_V7    0x700  /* t3btl t4s */
#define ALPS_PROTO_V8    0x800  /* SS4btl SS4s */

#define MAX_TOUCHES     4

#define DOLPHIN_COUNT_PER_ELECTRODE  64
#define DOLPHIN_PROFILE_XOFFSET    8  /* x-electrode offset */
#define DOLPHIN_PROFILE_YOFFSET    1  /* y-electrode offset */

// Device Capabilities
#define ALPS_DUALPOINT          0x02    /* touchpad has trackstick */
#define ALPS_PASS               0x04    /* device has a pass-through port */

#define ALPS_WHEEL              0x08    /* hardware wheel present */
#define ALPS_FW_BK_1            0x10    /* front & back buttons present */
#define ALPS_FW_BK_2            0x20    /* front & back buttons present */
#define ALPS_FOUR_BUTTONS       0x40    /* 4 direction button present */
#define ALPS_PS2_INTERLEAVED    0x80    /* 3-byte PS/2 packet interleaved with
6-byte ALPS packet */
#define ALPS_STICK_BITS        0x100  /* separate stick button bits */
#define ALPS_BUTTONPAD        0x200  /* device is a clickpad */
#define ALPS_DUALPOINT_WITH_PRESSURE  0x400  /* device can report trackpoint pressure */

#define kPacketLengthSmall  3
#define kPacketLengthLarge  6
#define kPacketLengthMax    6
#define kDP_CommandNibble10 0xf2
#define BITS_PER_BYTE 8

#define ALPS_CMD_NIBBLE_10  0x01f2

#define ALPS_QUIRK_TRACKSTICK_BUTTONS  1 /* trakcstick buttons in trackstick packet */

#define ARRAY_SIZE(x)    (sizeof(x)/sizeof(x[0]))
#define MAX(X,Y)         ((X) > (Y) ? (X) : (Y))
#define abs(x) ((x) < 0 ? -(x) : (x))
#define BIT(x) (1 << (x))


struct alps_bitmap_point {
    int start_bit;
    int num_bits;
};

struct input_mt_pos {
    UInt32 x;
    UInt32 y;
};

/**
 * struct alps_fields - decoded version of the report packet
 * @x_map: Bitmap of active X positions for MT.
 * @y_map: Bitmap of active Y positions for MT.
 * @fingers: Number of fingers for MT.
 * @pressure: Pressure.
 * @st: position for ST.
 * @mt: position for MT.
 * @first_mp: Packet is the first of a multi-packet report.
 * @is_mp: Packet is part of a multi-packet report.
 * @left: Left touchpad button is active.
 * @right: Right touchpad button is active.
 * @middle: Middle touchpad button is active.
 * @ts_left: Left trackstick button is active.
 * @ts_right: Right trackstick button is active.
 * @ts_middle: Middle trackstick button is active.
 */
struct alps_fields {
    UInt32 x_map;
    UInt32 y_map;
    UInt32 fingers;
    
    int pressure;
    struct input_mt_pos st;
    struct input_mt_pos mt[MAX_TOUCHES];
    
    UInt32 first_mp:1;
    UInt32 is_mp:1;
    
    UInt32 left:1;
    UInt32 right:1;
    UInt32 middle:1;
    
    UInt32 ts_left:1;
    UInt32 ts_right:1;
    UInt32 ts_middle:1;
};

/**
 * struct alps_data - private data structure for the ALPS driver
 * @nibble_commands: Command mapping used for touchpad register accesses.
 * @addr_command: Command used to tell the touchpad that a register address
 *   follows.
 * @proto_version: Indicates V1/V2/V3/...
 * @byte0: Helps figure out whether a position report packet matches the
 *   known format for this model.  The first byte of the report, ANDed with
 *   mask0, should match byte0.
 * @mask0: The mask used to check the first byte of the report.
 * @fw_ver: cached copy of firmware version (EC report)
 * @flags: Additional device capabilities (passthrough port, trackstick, etc.).
 * @x_max: Largest possible X position value.
 * @y_max: Largest possible Y position value.
 * @x_bits: Number of X bits in the MT bitmap.
 * @y_bits: Number of Y bits in the MT bitmap.
 * @prev_fin: Finger bit from previous packet.
 * @multi_packet: Multi-packet data in progress.
 * @multi_data: Saved multi-packet data.
 * @f: Decoded packet data fields.
 * @quirks: Bitmap of ALPS_QUIRK_*.
 */
struct alps_data {
    /* these are autodetected when the device is identified */
    const struct alps_nibble_commands *nibble_commands;
    SInt32 addr_command;
    UInt16 proto_version;
    UInt8 byte0, mask0;
    UInt8 fw_ver[3];
    int flags;
    SInt32 x_max;
    SInt32 y_max;
    SInt32 x_bits;
    SInt32 y_bits;
    unsigned int x_res;
    unsigned int y_res;
    
    SInt32 prev_fin;
    SInt32 multi_packet;
    int second_touch;
    UInt8 multi_data[6];
    struct alps_fields f;
    UInt8 quirks;
    bool PSMOUSE_BAD_DATA;
    
    int pktsize = 6;
};

class Protocol {
    // struct alps_fields f;
    // memset(&f, 0, sizeof(f));
protected:
    
    UInt32 lastButtons;
    struct alps_data *priv;
    Protocol (struct alps_data *priv) { this->priv = priv; };
    int process_bitmap(struct alps_data *priv,
                       struct alps_fields *fields);
    virtual struct alps_fields processPacket(struct alps_field *f, UInt8 *packet);
  

};

#endif /* VoodooPS2AlpsTrackpadKonstants_h */
