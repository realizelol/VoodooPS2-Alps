//
//  AlpsV7.hpp
//  VoodooPS2Trackpad
//
//  Created by Hades on 2/29/20.
//  Copyright © 2020 rehabman. All rights reserved.
//

#ifndef AlpsV7_hpp
#define AlpsV7_hpp

#include "VoodooPS2AlpsBase.h"

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

class EXPORT AlpsV7Touchpad : public VoodooPS2AlpsBase {
    typedef VoodooPS2AlpsBase super;
    OSDeclareDefaultStructors( AlpsV7Touchpad );

public:
    virtual AlpsV7Touchpad * probe(IOService *provider, SInt32 *score);
    bool init(OSDictionary * dict);
    
private:

    unsigned char alps_get_packet_id_v7(UInt8 *byte);

    void alps_get_finger_coordinate_v7(struct input_mt_pos *mt, UInt8 *pkt, UInt8 pkt_id);

    int alps_get_mt_count(struct input_mt_pos *mt);

    bool alps_decode_packet_v7(struct alps_fields *f, UInt8 *p);

    void alps_process_trackstick_packet_v7(UInt8 *packet);

    void alps_process_touchpad_packet_v7(UInt8 *packet);

    void alps_process_packet_v7(UInt8 *packet);
    
    bool alps_hw_init_v7();
};

#endif /* AlpsV7_hpp */
