/* Copyright (C) 2003  All rights reserved. */

/* Note - this file is purposefully NOT wrapped in an ifndef. */

/* 
  This module defines a debug macro "BDEBUG" which is enabled
  by placing a 

  #define INCLUDE_TRACE 

  into your source file above the 

  #include "bdebug.h" 

  line.

  BWARN and BERROR macros are also included. They cannot be disabled.

  Limitations: 
    compile into kernel only (uses printk and jiffies).

*/

/* Force the definition that prints */


#ifdef BDEBUG
#undef BDEBUG
#endif

#ifdef BWARN
#undef BWARN
#endif

#ifdef BERROR
#undef BERROR
#endif

/* GNU C has some special macros , e.g. __MODULE__ */

#if !defined (INCLUDE_TRACE)

#define BDEBUG(args...)
#define BWARN(args...)
#define BERROR(args...)

#define BR_READ_LOCK(L)        read_lock(L)
#define BR_READ_UNLOCK(L)      read_unlock(L)
#define BR_READ_LOCK_BH(L)     read_lock_bh(L)
#define BR_READ_UNLOCK_BH(L)   read_unlock_bh(L)
#define BR_WRITE_LOCK(L)       write_lock(L)
#define BR_WRITE_UNLOCK(L)     write_unlock(L)
#define BR_WRITE_LOCK_BH(L)    write_lock_bh(L)
#define BR_WRITE_UNLOCK_BH(L)  write_unlock_bh(L)

#else

#define BR_READ_LOCK(L)                                                     \
   do {                                                                     \
     printk (KERN_DEBUG "BR_READ_LOCK %s %d %x\n",                          \
             __FUNCTION__, __LINE__, L);                                    \
     read_lock(L);                                                          \
   } while (0)

#define BR_READ_LOCK_BH(L)                                                  \
   do {                                                                     \
     printk (KERN_DEBUG "BR_READ_LOCK_BH %s %d %x\n",                       \
             __FUNCTION__, __LINE__, L);                                    \
     read_lock_bh(L);                                                       \
   } while (0)

#define BR_READ_UNLOCK(L)                                                   \
   do {                                                                     \
     printk (KERN_DEBUG "BR_READ_UNLOCK %s %d %x\n",                        \
             __FUNCTION__, __LINE__, L);                                    \
     read_unlock(L);                                                        \
   } while (0)

#define BR_READ_UNLOCK_BH(L)                                                \
   do {                                                                     \
     printk (KERN_DEBUG "BR_READ_UNLOCK_BH %s %d  %x\n",                    \
             __FUNCTION__, __LINE__, L);                                    \
     read_unlock_bh(L);                                                     \
   } while (0)

#define BR_WRITE_LOCK(L)                                                   \
   do {                                                                    \
     printk (KERN_DEBUG "BR_WRITE_LOCK %s %d %x\n",                        \
             __FUNCTION__, __LINE__, L);                                   \
     write_lock(L);                                                        \
   } while (0)

#define BR_WRITE_UNLOCK(L)                                                 \
   do {                                                                    \
     printk (KERN_DEBUG "BR_WRITE_UNLOCK %s %d %x\n",                      \
             __FUNCTION__, __LINE__, L);                                   \
     write_unlock(L);                                                      \
   } while (0)

#define BR_WRITE_LOCK_BH(L)                                                \
   do {                                                                    \
     printk (KERN_DEBUG "BR_WRITE_LOCK_BH %s %d %x\n",                     \
             __FUNCTION__, __LINE__, L);                                   \
     write_lock_bh(L);                                                     \
   } while (0)

#define BR_WRITE_UNLOCK_BH(L)                                              \
   do {                                                                    \
     printk (KERN_DEBUG "BR_WRITE_UNLOCK_BH %s %d %x\n",                   \
             __FUNCTION__, __LINE__, L);                                   \
     write_unlock_bh(L);                                                   \
   } while (0)


#if defined (__GNUC__)

# include <linux/kernel.h>

# define BDEBUG(fmt, args...) \
      printk(KERN_DEBUG " %08ld:%s(%d)[%s]:" fmt, jiffies, __FILE__, __LINE__, __FUNCTION__, ## args)

# define BWARN(fmt, args...) \
      printk(KERN_WARNING " %08ld:%s(%d)[%s]:" fmt, jiffies, __FILE__, __LINE__, __FUNCTION__, ## args)

# define BERROR(fmt, args...) \
      printk(KERN_CRIT " %08ld:%s(%d)[%s]:" fmt, jiffies, __FILE__, __LINE__, __FUNCTION__, ## args)

#else

# define BDEBUG(fmt, args...) \
      printk(KERN_DEBUG " %08ld:%s(%d):" fmt, jiffies, __FILE__, __LINE__, ## args)

# define BWARN(fmt, args...) \
      printk(KERN_WARNING " %08ld:%s(%d):" fmt, jiffies, __FILE__, __LINE__, ## args)

# define BERROR(fmt, args...) \
      printk(KERN_CRIT " %08ld:%s(%d):" fmt, jiffies, __FILE__, __LINE__, ## args)

#endif /* __GNU_C__ */

#endif /* !INCLUDE_TRACE */
