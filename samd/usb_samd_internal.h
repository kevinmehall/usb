#pragma once

#define USB_EP_size_to_gc(x)  ((x <= 8   )?0:\
                               (x <= 16  )?1:\
                               (x <= 32  )?2:\
                               (x <= 64  )?3:\
                               (x <= 128 )?4:\
                               (x <= 256 )?5:\
                               (x <= 512 )?6:\
                                           7)
