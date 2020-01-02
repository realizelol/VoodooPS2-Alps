//
//  V8Protocol.cpp
//  VoodooPS2Trackpad
//
//  Created by Hades on 12/31/19.
//  Copyright © 2019 rehabman. All rights reserved.
//

#include "V8Protocol.h"

void V8Protocol::hwInit() {
}

unsigned char V8Protocol::alps_get_pkt_id_ss4_v2(UInt8 *byte) {
  unsigned char pkt_id = SS4_PACKET_ID_IDLE;
  
  switch (byte[3] & 0x30) {
      case 0x00:
          if (byte[0] == 0x18 && byte[1] == 0x10 && byte[2] == 0x00 &&
              (byte[3] & 0x88) == 0x08 && byte[4] == 0x10 &&
              byte[5] == 0x00) {
              pkt_id = SS4_PACKET_ID_IDLE;
          } else {
              pkt_id = SS4_PACKET_ID_ONE;
          }
          break;
      case 0x10:
          /* two-finger finger positions */
          pkt_id = SS4_PACKET_ID_TWO;
          break;
      case 0x20:
          /* stick pointer */
          pkt_id = SS4_PACKET_ID_STICK;
          break;
      case 0x30:
          /* third and fourth finger positions */
          pkt_id = SS4_PACKET_ID_MULTI;
          break;
  }
  
  return pkt_id;
}

void V8Protocol::decodePacket(struct alps_fields *f, UInt8 *packet) {
  unsigned char pkt_id;
  unsigned int no_data_x, no_data_y;
  uint64_t now_abs;
  clock_get_uptime(&now_abs);
  
  pkt_id = alps_get_pkt_id_ss4_v2(packet);
  
  /* Current packet is 1Finger coordinate packet */
  switch (pkt_id) {
      case SS4_PACKET_ID_ONE:
          f->mt[0].x = SS4_1F_X_V2(packet);
          f->mt[0].y = SS4_1F_Y_V2(packet);
          f->pressure = ((SS4_1F_Z_V2(packet)) * 2) & 0x7f;
          /*
           * When a button is held the device will give us events
           * with x, y, and pressure of 0. This causes annoying jumps
           * if a touch is released while the button is held.
           * Handle this by claiming zero contacts.
           */
          f->fingers = f->pressure > 0 ? 1 : 0;
          f->first_mp = 0;
          f->is_mp = 0;
          break;
          
      case SS4_PACKET_ID_TWO:
          if (priv->flags & ALPS_BUTTONPAD) {
              f->mt[0].x = SS4_BTL_MF_X_V2(packet, 0);
              f->mt[0].y = SS4_BTL_MF_Y_V2(packet, 0);
              f->mt[1].x = SS4_BTL_MF_X_V2(packet, 1);
              f->mt[1].y = SS4_BTL_MF_Y_V2(packet, 1);
          } else {
              f->mt[0].x = SS4_STD_MF_X_V2(packet, 0);
              f->mt[0].y = SS4_STD_MF_Y_V2(packet, 0);
              f->mt[1].x = SS4_STD_MF_X_V2(packet, 1);
              f->mt[1].y = SS4_STD_MF_Y_V2(packet, 1);
          }
          f->pressure = SS4_MF_Z_V2(packet, 0) ? 0x30 : 0;
          
          if (SS4_IS_MF_CONTINUE(packet)) {
              f->first_mp = 1;
          } else {
              f->fingers = 2;
              f->first_mp = 0;
          }
          f->is_mp = 0;
          
          break;
          
      case SS4_PACKET_ID_MULTI:
          if (priv->flags & ALPS_BUTTONPAD) {
              f->mt[2].x = SS4_BTL_MF_X_V2(packet, 0);
              f->mt[2].y = SS4_BTL_MF_Y_V2(packet, 0);
              f->mt[3].x = SS4_BTL_MF_X_V2(packet, 1);
              f->mt[3].y = SS4_BTL_MF_Y_V2(packet, 1);
              no_data_x = SS4_MFPACKET_NO_AX_BL;
              no_data_y = SS4_MFPACKET_NO_AY_BL;
          } else {
              f->mt[2].x = SS4_STD_MF_X_V2(packet, 0);
              f->mt[2].y = SS4_STD_MF_Y_V2(packet, 0);
              f->mt[3].x = SS4_STD_MF_X_V2(packet, 1);
              f->mt[3].y = SS4_STD_MF_Y_V2(packet, 1);
              no_data_x = SS4_MFPACKET_NO_AX;
              no_data_y = SS4_MFPACKET_NO_AY;
          }
          
          f->first_mp = 0;
          f->is_mp = 1;
          
          if (SS4_IS_5F_DETECTED(packet)) {
              f->fingers = 5;
          } else if (f->mt[3].x == no_data_x &&
                     f->mt[3].y == no_data_y) {
              f->mt[3].x = 0;
              f->mt[3].y = 0;
              f->fingers = 3;
          } else {
              f->fingers = 4;
          }
          break;
          
      case SS4_PACKET_ID_STICK:
          /*
           * x, y, and pressure are decoded in
           * alps_process_packet_ss4_v2()
           */
          f->first_mp = 0;
          f->is_mp = 0;
          break;
          
      case SS4_PACKET_ID_IDLE:
      default:
          memset(f, 0, sizeof(struct alps_fields));
          break;
  }
  
  /* handle buttons */
  if (pkt_id == SS4_PACKET_ID_STICK) {
      f->ts_left = !!(SS4_BTN_V2(packet) & 0x01);
      if (!(priv->flags & ALPS_BUTTONPAD)) {
          f->ts_right = !!(SS4_BTN_V2(packet) & 0x02);
          f->ts_middle = !!(SS4_BTN_V2(packet) & 0x04);
      }
  } else {
      f->left = !!(SS4_BTN_V2(packet) & 0x01);
      if (!(priv->flags & ALPS_BUTTONPAD)) {
          f->right = !!(SS4_BTN_V2(packet) & 0x02);
          f->middle = !!(SS4_BTN_V2(packet) & 0x04);
      }
  }
}

struct alps_fields V8Protocol::processPacket(UInt8 *packet)
{
  int buttons = 0;
  struct alps_fields f;
  int x, y, pressure;
  
  uint64_t now_abs;
  clock_get_uptime(&now_abs);
  
  memset(&f, 0, sizeof(struct alps_fields));
  decodePacket(&f, packet);
  if (priv->multi_packet) {
      /*
       * Sometimes the first packet will indicate a multi-packet
       * sequence, but sometimes the next multi-packet would not
       * come. Check for this, and when it happens process the
       * position packet as usual.
       */
      if (f.is_mp) {
          /* Now process the 1st packet */
          decodePacket(&f, priv->multi_data);
      } else {
          priv->multi_packet = 0;
      }
  }
  
  /*
   * "f.is_mp" would always be '0' after merging the 1st and 2nd packet.
   * When it is set, it means 2nd packet comes without 1st packet come.
   */
  if (f.is_mp) {
      return f;
  }
  
  /* Save the first packet */
  if (!priv->multi_packet && f.first_mp) {
      priv->multi_packet = 1;
      memcpy(priv->multi_data, packet, sizeof(priv->multi_data));
      return f;
  }
  
  priv->multi_packet = 0;
  
  /* Report trackstick */
  if (alps_get_pkt_id_ss4_v2(packet) == SS4_PACKET_ID_STICK) {
      if (!(priv->flags & ALPS_DUALPOINT)) {
          return f;
      }
      
      x = (((packet[0] & 1) << 7) | (packet[1] & 0x7f));
      y = (((packet[3] & 1) << 7) | (packet[2] & 0x7f));
      pressure = (packet[4] & 0x7f);
      
      buttons |= f.ts_left ? 0x01 : 0;
      buttons |= f.ts_right ? 0x02 : 0;
      buttons |= f.ts_middle ? 0x04 : 0;
      
      if ((abs(x) >= 0x7f) || (abs(y) >= 0x7f)) {
          return f;
      }
      
      //TODO: V8 Trackstick: Someone with the hardware needs to debug this.
      //IOLog("ALPS: Trackstick report: X=%d, Y=%d, Z=%d, buttons=%d\n", x, y, pressure, buttons);
      //dispatchRelativePointerEventX(x, y, 0, now_abs);
      return f;
  }
  
  /* Report touchpad */
  buttons |= f.left ? 0x01 : 0;
  buttons |= f.right ? 0x02 : 0;
  buttons |= f.middle ? 0x04 : 0;
  
  /* Reverse y co-ordinates to have 0 at bottom for gestures to work */
  f.mt[0].y = priv->y_max - f.mt[0].y;
  f.mt[1].y = priv->y_max - f.mt[1].y;
  
  return f;
}
