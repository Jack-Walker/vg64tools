#ifndef __ZELDA64_ACTOR_H__
#define __ZELDA64_ACTOR_H__

#include "mips.h"

struct actor_spec_t
{
    u16 actor_number;
    u8 actor_type;
    u8 actor_status;
    u32 _NULL;
    u16 object_number;
    u16 _NULL_;
    u32 _NULL__;
    u32 init;
    u32 func_1;
    u32 func_2;
    u32 func_3;
};

struct z64_actor_t
{
    u16 actor_number;           /* actor number                         */
    u8  actor_type;             /* 2 = link 4 = npcs etc.               */
    u8  actor_status;           /* unknown (Z-Target related, perhaps?) */
    u32 unknown_;
    f32 x_unknown;              /* Does not affect actual x position, but is constantly written to */
    f32 y_unknown;              /* Same, but for Y                      */
    
    /*0x10*/
    f32 z_unknown;              /* Same, but for Z                      */
    s16 initial_x_rot;          /* Needs to be confirmed - But I think it is only written to upon loading the actor */
    s16 initial_y_rot;          /* Same, but for Y                      */
    s16 initial_z_rot;          /* Same, but for Z                      */
    u16 buffer;                 /* I *think* this is a buffer so the next part has 4 byte alignment */
    u16 variable;               /* Actor Variable                       */
    u16 unknown_2;              /* Unknown                              */
    
    /*0x20*/
    u32 unknown_3;              /* Unknown                              */
    f32 x_rw;                   /* Same as x_unknown, but when written to it /does/ affect the actor's x pos */
    f32 y_rw;                   /* Same, but for Y                      */
    f32 z_rw;                   /* Same, but for Z                      */
    
    /*0x30*/
    s16 x_rot_unknown_1;        /* Copy of initial_x rot?               */
    s16 y_rot_unknown_1;        /* Same, but for Y                      */
    s16 z_rot_unknown_1;        /* Same, but for Z                      */
    u16 unknown_4;              /* Buffer?                              */
    f32 unknown_5;              /* X pos + small rand? constantly changing */
    f32 unknown_6;              /* Same? (for Y)                        */
    
    /*0x40*/
    f32 unknown_7;              /* Same? (for Z)                        */
    s16 x_rot_unknown_2;        /* Copy of initial_x rot?               */
    s16 y_rot_unknown_2;        /* Same, but for Y                      */
    s16 z_rot_unknown_2;        /* Same, but for Z                      */
    u16 unknown_8;              /* Buffer?                              */
    u32 unknown_9;              /* Unknown                              */
    
    /*0x50*/
    f32 x_scale;                 /* X scale for model, if applicable    */
    f32 y_scale;                 /* Same, but for Y                     */
    f32 z_scale;                 /* Same, but for Z                     */
    
    f32 x_accel;
    f32 y_accel;
    f32 z_accel;
    
    int _0x0068;
    int _0x006C;
    /* 0x70 */
    int _0x0070;
    int _0x0074;
    int _0x0078;
    int _0x007C;
    /* 0x80 */
    int _0x0080;
    int _0x0084;
    int _0x0088;
    int _0x008C;
    /* 0x90 */
    int _0x0090;
    int _0x0094;
    int _0x0098;
    int _0x009C;
    /* 0xA0 */
    int _0x00A0;
    int _0x00A4;
    int _0x00A8;
    u16 _0x00AC;
    u8  _0x00AE;
    u8  _0x00AF;
    /* 0xB0 */
    int _0x00B0;
    int _0x00B4;
    int _0x00B8;
    f32 _0x00BC;
    /* 0xC0 */
    int _0x00C0;
    int _0x00C4;
    int _0x00C8;
    int _0x00CC;
    /* 0xD0 */
    int _0x00D0;
    int _0x00D4;
    int _0x00D8;
    int _0x00DC;
    /* 0xE0 */
    int _0x00E0;
    int _0x00E4;
    int _0x00E8;
    int _0x00EC;
    /* 0xF0 */
    int _0x00F0;
    int _0x00F4;
    int _0x00F8;
    int _0x00FC;
    /* 0x100 */
    int _0x0100;
    int _0x0104;
    int _0x0108;
    int _0x010C;
    /* 0x110 */
    int _0x0110;
    int _0x0114;
    int _0x0118;
    int _0x011C;
    /* 0x120 */
    
    /* Previous actor in list */
    struct z64_actor_t * previous;
    
    /* Next actor in list */
    struct z64_actor_t * next;
    
    u32 actor_init;
    u32 actor_rotuine_1;
    /* 0x130 */
    u32 actor_routine_2;
    u32 actor_routine_3;
    u32 actor_code_entry;
    u32 v013C;
    /* 0x140 */
    u32 v0140;
    u32 v0144;
    u32 v0148;
    u32 v014C;
    /* 0x150 */
    u32 v0150;
    u32 v0154;
    u32 v0158;
    u32 v015C;
    /* 0x160 */
    u32 v0160;
    u32 v0164;
    f32 v0168;
    u32 v016C;
    /* 0x170 */
    u32 v0170;
    u32 v0174;
    u32 v0178;
    u32 v017C;
    /* 0x180 */
    u32 v0180;
    u32 v0184;
    u32 v0188;
    u32 v018C;
    /* 0x190 */
    u32 next_routine;
    u32 v0194;
    u32 v0198;
    u16 v019C;
    u16 v019E;
    /* 0x1A0 */
    f32 v01A0;
    f32 v01A4;
    f32 v01A8;
    f32 v01AC;
    /* 0x1B0 */
    f32 v01B0;
    f32 v01B4;
    f32 v01B8;
    f32 v01BC;
    /* 0x1C0 */
    u16 v01C0;
    u16 v01C2;
    u32 v01C4;
    u32 v01C8;
    u32 v01CC;
    /* 0x1D0 */
    u32 v01D0;
    u32 v01D4;
    u32 v01D8;
    u32 v01DC;
    /* 0x1E0 */
    u32 v01E0;
    u32 v01E4;
    u32 v01E8;
    u32 v01EC;
    /* 0x1F0 */
    u32 v01F0;
    u32 v01F4;
    u32 v01F8;
    u32 v01FC;
    /* 0x200 */
};


#endif /* __ZELDA64_ACTOR_H__ */

