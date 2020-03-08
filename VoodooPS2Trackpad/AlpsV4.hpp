//
//  AlpsV4.hpp
//  VoodooPS2Trackpad
//
//  Created by Hades on 2/29/20.
//  Copyright © 2020 rehabman. All rights reserved.
//

#ifndef AlpsV4_hpp
#define AlpsV4_hpp

#include "VoodooPS2AlpsBase.h"

class EXPORT AlpsV4Touchpad : public VoodooPS2TouchPadBase {
    typedef VoodooPS2TouchPadBase super;
    OSDeclareDefaultStructors( AlpsV4Touchpad );

public:
    virtual AlpsV4Touchpad * probe(IOService *provider, SInt32 *score);
    bool init(OSDictionary * dict);
    
private:
    bool alps_absolute_mode_v4();
    
    bool alps_hw_init_v4();
    
    void alps_process_packet_v4(UInt8 *packet);
};

#endif /* AlpsV4_hpp */
