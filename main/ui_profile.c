#include "FilMachine.h"
#include "ui_profile.h"

#include "ui_profile_480x320.inc"
#include "ui_profile_800x480.inc"

const ui_profile_t *ui_get_profile(void)
{
#if (LCD_H_RES == 800)
    return &ui_profile_800x480;
#else
    return &ui_profile_480x320;
#endif
}
