/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _FP_H
#define _FP_H

/*                        Fixed point format. 
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Integral Part                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Fractional Part                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct long_fp
{
  union {
    u_int32_t ui;
    s_int32_t si;
  } integral;
  union {
    u_int32_t uf;
    s_int32_t sf;
  } fractional;
};

#define uintegral    integral.ui      /* unsigned integral part. */
#define sintegral    integral.si      /* signed integral part. */
#define ufractional  fractional.uf    /* unsigned fractional part. */
#define sfractional  fractional.sf    /* signed fractional part. */

#define LFP_NEGATE(v)          /* a = -a */ \
  do { \
    if ((v).ufractional == 0) \
      (v).uintegral = -((s_int32_t)(v).uintegral); \
    else \
     { \
       (v).ufractional = -((s_int32_t)(v).ufractional); \
       (v).uintegral = ~(v).uintegral; \
     } \
  } while(0)

#define LFP_LSHIFT(v)         /* v << 1 */ \
   do { \
     (v).uintegral <<= 1; \
     if ((v).ufractional & 0x80000000) \
       (v).uintegral |= 0x1; \
     (v).ufractional <<= 1; \
   } while (0)

#define LFP_ADD(i,j)           /* i += j */ \
   do { \
     u_int32_t lo; \
     u_int32_t hi; \
     \
     lo = ((i).ufractional & 0xffff) + ((j).ufractional & 0xffff); \
     hi = (((i).ufractional >> 16) & 0xffff) + (((j).ufractional >> 16) & 0xffff); \
     if (lo & 0x10000) \
       hi++; \
     (i).ufractional = ((hi & 0xffff) << 16) | (lo & 0xffff); \
     \
     (i).uintegral += (j).uintegral; \
     if (hi & 0x10000) \
       (i).uintegral++; \
   } while (0)

#define LFP_SUB(i,j)          /* i -= j */ \
  do { \
    u_int32_t lo; \
    u_int32_t hi; \
    \
    if((j).ufractional == 0) \
     { \
       (i).uintegral = (j).uintegral; \
     } \
    else \
     { \
       lo = ((i).ufractional 0xffff) + ((-((s_int32_t)((j).ufractional))) & 0xffff); \
       hi = (((i).ufractional >> 16) & 0xffff) \
             + (((-((s_int32_t)((j).ufractional))) >> 16) & 0xffff); \
       if (lo & 0x10000) \
         hi++; \
       (i).ufractional = ((hi & 0xffff) << 16) | (lo & 0xffff); \
       \
       (i).uintegral += ~((j).uintegral; \
       if (hi & 0x10000) \
         (i).uintegral++; \
     } \
  } while (0) 

/* Ascii to long floating point. Input string has only digits. */
int atolongfp(const char *str, struct long_fp *lfp);

/* Hex string to long floating point. Input string canbe in hex or decimals.*/
int
hextolongfp (const char *str, struct long_fp *lfp);

#endif /* _FP_TYPE_H */
