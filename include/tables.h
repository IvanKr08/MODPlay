#pragma once
#include "utils.h"

extern int8 int4                 [16];
extern uint16 notePeriods        [84];
extern uint16 tunedPeriods       [1344];
extern cstr noteNames            [86];
extern uint16 tunedPeriodsCompact[192];

extern int8 vibrSine    [64];
extern int8 vibrRampDown[64];
extern int8 vibrSquare  [64];
extern int8 vibrRandom  [64];