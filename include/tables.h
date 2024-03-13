#pragma once
#include "utils.h"

extern uint16 notePeriods        [84];
extern uint16 tunedPeriods       [1344];
extern uint16 tunedPeriodsCompact[192];
extern uint16 s3mFreq            [12];

extern cstr   noteNames          [256];
extern int8   int4               [16];

extern int8 vibrSine    [64];
extern int8 vibrRampDown[64];
extern int8 vibrSquare  [64];
extern int8 vibrRandom  [64];