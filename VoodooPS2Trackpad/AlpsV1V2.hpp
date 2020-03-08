//
//  AlpsV1V2.hpp
//  VoodooPS2Trackpad
//
//  Created by Hades on 2/29/20.
//  Copyright © 2020 rehabman. All rights reserved.
//

#ifndef AlpsV1V2_hpp
#define AlpsV1V2_hpp

#include "VoodooPS2AlpsBase.h"

class EXPORT AlpsV1V2Touchpad : public VoodooPS2AlpsBase {
    typedef VoodooPS2AlpBase super;
    OSDeclareDefaultStructors( AlpsV1V2Touchpad );

public:
    virtual AlpsV1V2Touchpad * probe(IOService *provider, SInt32 *score);
    bool init(OSDictionary * dict);
    
private:
    void alps_process_packet_v1_v2(UInt8 *packet);
    bool alps_passthrough_mode_v2(bool enable);
        
    bool alps_absolute_mode_v1_v2();
    bool alps_hw_init_v1_v2();
    
};

#endif /* AlpsV1V2_hpp */
