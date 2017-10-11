/* Copyright (C) 2003   All Rights Reserved.  */
#include "pal.h"
#include "fp.h"

/* Ascii to long floating point. Input string has only digits. */
int
atolongfp (const char *str, struct long_fp *lfp)
{
  register const char *cp;
  register unsigned long dec_i;
  register unsigned long dec_f;
  char *indicator = NULL;
  int ndec;
  int isneg;
  struct long_fp tfp;
  const char *digits = "0123456789";
  unsigned long ten_to_the_n[10] = 
    {
      0,
      10,
      100,
      1000,
      10000,
      100000,
      1000000,
      10000000,
      100000000,
      1000000000,
    };

  pal_assert (lfp != NULL);
                                                                           
  isneg = 0;
  dec_i = dec_f = 0;
  ndec = 0;
  cp = str;

  /* Input format is 
     [spaces][-|+][digits][.][digits][spaces|\n|\0]  */
  while (pal_char_isspace((int)*cp))
    cp++;
                                                                               
  if (*cp == '-') 
    {
      cp++;
      isneg = 1;
    }
                                                                               
  if (*cp == '+')
    cp++;
                                                                               
  if (*cp != '.' && !pal_char_isdigit((int)*cp))
    return -1;
  while (*cp != '\0' && (indicator = pal_strchr(digits, *cp)) != NULL) 
    {
      dec_i = (dec_i << 3) + (dec_i << 1);    /* multiply by 10 */
      dec_i += (indicator - digits);
      cp++;
    }
                                                                               
  if (*cp != '\0' && !pal_char_isspace((int)*cp)) 
    {
      if (*cp++ != '.')
        return -1;
      
      while (ndec < 9 && *cp != '\0'
             && (indicator = pal_strchr(digits, *cp)) != NULL) 
        {
          ndec++;
          dec_f = (dec_f << 3) + (dec_f << 1);    /* *10 */
          dec_f += (indicator - digits);
          cp++;
        }
                                                                               
      while (pal_char_isdigit((int)*cp))
        cp++;
                                                                               
      if (*cp != '\0' && !pal_char_isspace((int)*cp))
        return -1;
    }
                                                                               
  if (ndec > 0)
    {
      register unsigned long tmp;
      register unsigned long bit;
      register unsigned long ten_fact;
                                                                               
      ten_fact = ten_to_the_n[ndec];

      tmp = 0;
      bit = 0x80000000;
      while (bit != 0) 
        {
          dec_f <<= 1;
          if (dec_f >= ten_fact) 
            {
              tmp |= bit;
              dec_f -= ten_fact;
            }
          bit >>= 1;
        }
      if ((dec_f << 1) > ten_fact)
        tmp++;
      dec_f = tmp;
    }
                                                                               
  if (isneg)
    {
      tfp.uintegral = dec_i;
      tfp.ufractional = dec_f;
      LFP_NEGATE(tfp);
      dec_i = tfp.uintegral;
      dec_f = tfp.ufractional;
    }
                                                                               
  lfp->uintegral = dec_i;
  lfp->ufractional = dec_f;

  return 0;
}

/* Hex string to long floating point. Input string canbe in hex or decimals.*/
int
hextolongfp (const char *str, struct long_fp *lfp)
{
  register const char *cp, *cpstart;
  register u_int32_t dec_i;
  register u_int32_t dec_f;
  static const char *digits = "0123456789abcdefABCDEF";
  char *indicator = NULL;

  pal_assert (lfp != NULL);

  dec_i = dec_f = 0;

  cp = str;
  while (pal_char_isspace((int)*cp))
    cp++;

  /* String starts with 0x */
  if (*str == '0' && (*(str+1) == 'x' || *(str+1) == 'X'))
    {
      /* Numbers of the form. 
         [spaces]8_hex_digits[.]8_hex_digits[spaces|\n|\0] */

      cp = str + 2;
      while (pal_char_isspace((int)*cp))
        cp++;

      cpstart = cp;
      while (*cp != '\0' 
             && ((cp - cpstart) < 8)
             && (indicator = pal_strchr (digits, *cp)) != NULL)
        {
          dec_i = dec_i << 4; /* Multiply by 16. */
          dec_i += ((indicator - digits) > 15) ? (indicator - digits) - 6 : (indicator - digits);
          cp++;
        }

      if ((cp - cpstart) < 8 || (indicator == NULL))
        return -1;
 
      if (*cp == '.')
        cp++;

      cpstart = cp;
      while (*cp != '\0' 
             && ((cp - cpstart) < 8)
             && (indicator = pal_strchr (digits, *cp)) != NULL)
        {
          dec_f = dec_f << 4; /* Multiply by 16. */
          dec_f += ((indicator - digits) > 15) ? (indicator - digits) - 6 : (indicator - digits);
          cp++;
        }

      if ((cp - cpstart) < 8 || (indicator == NULL))
        return -1;

      if (*cp != '\0' && !pal_char_isspace ((int)*cp))
        return -1;

      lfp->uintegral = dec_i;
      lfp->ufractional = dec_f;

      return 0; 
    }

  /* Treat it like decimal. */
  return atolongfp (str, lfp);
}








             
      
