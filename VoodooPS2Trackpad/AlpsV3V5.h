//
//  AlpsV3V5.h
//  VoodooPS2Controller
//
//  Created by Gwy on 2/29/20.
//  Copyright © 2020 Gwy. All rights reserved.
//

#ifndef AlpsV3V5_h
#define AlpsV3V5_h

#include "VoodooPS2AlpsBase.h"

#define DOLPHIN_COUNT_PER_ELECTRODE    64
#define DOLPHIN_PROFILE_XOFFSET        8    /* x-electrode offset */
#define DOLPHIN_PROFILE_YOFFSET        1    /* y-electrode offset */

class EXPORT AlpsV3V5Touchpad : public VoodooPS2AlpsBase {
    typedef VoodooPS2AlpsBase super;
    OSDeclareDefaultStructors( AlpsV3V5Touchpad );

public:
    virtual AlpsV3V5Touchpad * probe(IOService *provider, SInt32 *score);
    bool init(OSDictionary * dict);
    
private:
    void alps_process_trackstick_packet_v3(UInt8 * packet);
    
    bool alps_decode_buttons_v3(struct alps_fields *f, UInt8 *p);
    
    bool alps_decode_pinnacle(struct alps_fields *f, UInt8 *p);
    
    bool alps_decode_rushmore(struct alps_fields *f, UInt8 *p);
    
    bool alps_decode_dolphin(struct alps_fields *f, UInt8 *p);
    
    void alps_process_touchpad_packet_v3_v5(UInt8 * packet);
    
    void alps_process_packet_v3(UInt8 *packet);
    
    bool alps_hw_init_dolphin_v1();
    
    int alps_dolphin_get_device_area(struct alps_data *priv);
    
    bool alps_hw_init_rushmore_v3();
    
    IOReturn alps_setup_trackstick_v3(int regBase);
    
    bool alps_hw_init_v3();
    
    bool alps_passthrough_mode_v3(int regBase, bool enable);
    
    bool alps_absolute_mode_v3();
};

#endif /* AlpsV3V5_h */
