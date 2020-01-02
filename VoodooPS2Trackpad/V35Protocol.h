//
//  V35Protocol.hpp
//  VoodooPS2Trackpad
//
//  Created by Hades on 1/1/20.
//  Copyright © 2020 rehabman. All rights reserved.
//

#ifndef V35Protocol_hpp
#define V35Protocol_hpp

#include "VoodooPS2AlpsTrackpadKonstants.h"
#include "VoodooPS2TrackpadBase.h"

class EXPORT V35Protocol : public VoodooPS2TouchPadBase
{
    typedef VoodooPS2TouchPadBase super;
    OSDeclareDefaultStructors( V35Protocol );

public:
  
    virtual V35Protocol * probe(IOService *provider, SInt32 *score);
//    V35Protocol (struct alps_data *priv) : Protocol(priv) {};

private:
  
    bool decode_buttons_v3(struct alps_fields *f, unsigned char *p);
    bool decode_pinnacle(struct alps_fields *f, UInt8 *p);
    bool decode_rushmore(struct alps_fields *f, UInt8 *p);
    bool decode_dolphin(struct alps_fields *f, UInt8 *p);
    void process_trackstick_packet_v3(UInt8 *packet);
    struct alps_fields* processPacketTouchpad(struct alps_fields *f, UInt8 *packet);
    
protected:
  
    struct alps_fields* processPacket(struct alps_fields *f, UInt8 *packet);

};

#endif /* V35Protocol_hpp */
