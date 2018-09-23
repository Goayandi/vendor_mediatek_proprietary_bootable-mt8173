#ifndef __MT_EMI_MPU_H
#define __MT_EMI_MPU_H

#include "mt8173.h"

#define UINT32P unsigned int *

/* EMI_MPU */
#define EMI_MPUA ((volatile UINT32P)(EMI_BASE+0x0160))
#define EMI_MPUB ((volatile UINT32P)(EMI_BASE+0x0168))
#define EMI_MPUC ((volatile UINT32P)(EMI_BASE+0x0170))
#define EMI_MPUD ((volatile UINT32P)(EMI_BASE+0x0178))
#define EMI_MPUE ((volatile UINT32P)(EMI_BASE+0x0180))
#define EMI_MPUF ((volatile UINT32P)(EMI_BASE+0x0188))
#define EMI_MPUG ((volatile UINT32P)(EMI_BASE+0x0190))
#define EMI_MPUH ((volatile UINT32P)(EMI_BASE+0x0198))
#define EMI_MPUI ((volatile UINT32P)(EMI_BASE+0x01A0))
#define EMI_MPUJ ((volatile UINT32P)(EMI_BASE+0x01A8))
#define EMI_MPUK ((volatile UINT32P)(EMI_BASE+0x01B0))
#define EMI_MPUL ((volatile UINT32P)(EMI_BASE+0x01B8))
#define EMI_MPUM ((volatile UINT32P)(EMI_BASE+0x01C0))
#define EMI_MPUN ((volatile UINT32P)(EMI_BASE+0x01C8))
#define EMI_MPUO ((volatile UINT32P)(EMI_BASE+0x01D0))
#define EMI_MPUP ((volatile UINT32P)(EMI_BASE+0x01D8))
#define EMI_MPUQ ((volatile UINT32P)(EMI_BASE+0x01E0))
#define EMI_MPUR ((volatile UINT32P)(EMI_BASE+0x01E8))
#define EMI_MPUS ((volatile UINT32P)(EMI_BASE+0x01F0))
#define EMI_MPUT ((volatile UINT32P)(EMI_BASE+0x01F8))

#define EMI_MPUU ((volatile UINT32P)(EMI_BASE+0x0200))
#define EMI_MPUY ((volatile UINT32P)(EMI_BASE+0x0220))

#define NO_PROTECTION 0
#define SEC_RW 1
#define SEC_RW_NSEC_R 2
#define SEC_RW_NSEC_W 3
#define SEC_R_NSEC_R 4
#define FORBIDDEN 5
#define SEC_R_NSEC_RW 6


/*EMI memory protection align 64K*/
#define EMI_MPU_ALIGNMENT 0x10000
#define OOR_VIO 0x00000200

#define SET_ACCESS_PERMISSON(d3, d2, d1, d0) (((d3) << 9) | ((d2) << 6) | ((d1) << 3) | (d0))

extern int emi_mpu_set_region_protection(unsigned int start_addr, unsigned int end_addr, int region, unsigned int access_permission);

#endif  /* !__MT_EMI_MPU_H */
