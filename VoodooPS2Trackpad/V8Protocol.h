//
//  V8Protocol.h
//  VoodooPS2Trackpad
//
//  Created by Hades on 12/31/19.
//  Copyright © 2019 rehabman. All rights reserved.
//

#ifndef V8Protocol_h
#define V8Protocol_h

#include "VoodooPS2AlpsTrackpadKonstants.h"

/*
 * enum SS4_PACKET_ID - defines the packet type for V8
 * SS4_PACKET_ID_IDLE: There's no finger and no button activity.
 * SS4_PACKET_ID_ONE: There's one finger on touchpad
 *  or there's button activities.
 * SS4_PACKET_ID_TWO: There's two or more fingers on touchpad
 * SS4_PACKET_ID_MULTI: There's three or more fingers on touchpad
 */
enum SS4_PACKET_ID {
    SS4_PACKET_ID_IDLE = 0,
    SS4_PACKET_ID_ONE,
    SS4_PACKET_ID_TWO,
    SS4_PACKET_ID_MULTI,
    SS4_PACKET_ID_STICK,
};

#define SS4_COUNT_PER_ELECTRODE    256
#define SS4_NUMSENSOR_XOFFSET    7
#define SS4_NUMSENSOR_YOFFSET    7
#define SS4_MIN_PITCH_MM    50

#define SS4_MASK_NORMAL_BUTTONS    0x07

#define SS4_1F_X_V2(_b)    ((_b[0] & 0x0007) |    \
((_b[1] << 3) & 0x0078) |  \
((_b[1] << 2) & 0x0380) |  \
((_b[2] << 5) & 0x1C00)  \
)

#define SS4_1F_Y_V2(_b)    (((_b[2]) & 0x000F) |    \
((_b[3] >> 2) & 0x0030) |  \
((_b[4] << 6) & 0x03C0) |  \
((_b[4] << 5) & 0x0C00)  \
)

#define SS4_1F_Z_V2(_b)    (((_b[5]) & 0x0F) |    \
((_b[5] >> 1) & 0x70) |  \
((_b[4]) & 0x80)    \
)

#define SS4_1F_LFB_V2(_b)  (((_b[2] >> 4) & 0x01) == 0x01)

#define SS4_MF_LF_V2(_b, _i)  ((_b[1 + (_i) * 3] & 0x0004) == 0x0004)

#define SS4_BTN_V2(_b)    ((_b[0] >> 5) & SS4_MASK_NORMAL_BUTTONS)

#define SS4_STD_MF_X_V2(_b, _i)  (((_b[0 + (_i) * 3] << 5) & 0x00E0) |  \
((_b[1 + _i * 3]  << 5) & 0x1F00)  \
)

#define SS4_STD_MF_Y_V2(_b, _i)  (((_b[1 + (_i) * 3] << 3) & 0x0010) |  \
((_b[2 + (_i) * 3] << 5) & 0x01E0) |  \
((_b[2 + (_i) * 3] << 4) & 0x0E00)  \
)

#define SS4_BTL_MF_X_V2(_b, _i)  (SS4_STD_MF_X_V2(_b, _i) |    \
((_b[0 + (_i) * 3] >> 3) & 0x0010)  \
)

#define SS4_BTL_MF_Y_V2(_b, _i)  (SS4_STD_MF_Y_V2(_b, _i) | \
((_b[0 + (_i) * 3] >> 3) & 0x0008)  \
)

#define SS4_MF_Z_V2(_b, _i)  (((_b[1 + (_i) * 3]) & 0x0001) |  \
((_b[1 + (_i) * 3] >> 1) & 0x0002)  \
)

#define SS4_IS_MF_CONTINUE(_b)  ((_b[2] & 0x10) == 0x10)
#define SS4_IS_5F_DETECTED(_b)  ((_b[2] & 0x10) == 0x10)


#define SS4_MFPACKET_NO_AX  8160  /* X-Coordinate value */
#define SS4_MFPACKET_NO_AY  4080  /* Y-Coordinate value */
#define SS4_MFPACKET_NO_AX_BL  8176  /* Buttonless X-Coordinate value */
#define SS4_MFPACKET_NO_AY_BL  4088  /* Buttonless Y-Coordinate value */

class V8Protocol : public Protocol
{
  typedef Protocol super;
public:
  V8Protocol (struct alps_data *priv) : Protocol(priv) {};
private:
  unsigned char alps_get_pkt_id_ss4_v2(UInt8 *byte);
  void decodePacket(struct alps_fields *f, UInt8 *packet);
  
protected:
  struct alps_fields processPacket(UInt8 *packet);

};

#endif /* V8Protocol_h */
