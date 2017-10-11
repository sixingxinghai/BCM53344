/* Copyright (C) 2003   All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_NTP
#include "pal_file.h"

#include "snprintf.h"
#include "fp.h"

#include "imi.h"
#include "imi_ntp.h"
#include "pal_ntp.h"

/* Packet data. */
static long ntp_pktdata[NTP_DATASIZE/sizeof(long)];

/* NTP debug. */
static int ntp_debug = 0;

/* Sequence numbers for requests. */
static unsigned int sequence = 1;

/* NTP variables. */
struct ntp_variables ntpvar[] = 
  {
    {"version",  NTP_VAR_VERSION},
    {"processor", NTP_VAR_PROCESSOR},
    {"system", NTP_VAR_SYSTEM},
    {"leap", NTP_VAR_LEAP},
    {"stratum", NTP_VAR_STRATUM},
    {"precision", NTP_VAR_PRECISION},
    {"rootdelay", NTP_VAR_ROOTDELAY},
    {"rootdispersion", NTP_VAR_ROOTDISPERSION},
    {"peer", NTP_VAR_PEER},
    {"refid", NTP_VAR_REFID},
    {"reftime", NTP_VAR_REFTIME},
    {"poll", NTP_VAR_POLL},
    {"clock", NTP_VAR_CLOCK},
    {"state", NTP_VAR_STATE},
    {"offset", NTP_VAR_OFFSET},
    {"frequency", NTP_VAR_FREQUENCY},
    {"jitter", NTP_VAR_JITTER},
    {"stability", NTP_VAR_STABILITY},
    {"config", NTP_VAR_CONFIG},
    {"authenable", NTP_VAR_AUTHENABLE},
    {"authentic", NTP_VAR_AUTHENTIC},
    {"srcadr", NTP_VAR_SRCADR},
    {"srcport", NTP_VAR_SRCPORT},
    {"dstadr", NTP_VAR_DSTADR},
    {"dstport", NTP_VAR_DSTPORT},
    {"hmode", NTP_VAR_HMODE},
    {"ppoll", NTP_VAR_PPOLL},
    {"hpoll", NTP_VAR_HPOLL},
    {"org", NTP_VAR_ORG},
    {"rec", NTP_VAR_REC},
    {"xmt", NTP_VAR_XMT},
    {"reach", NTP_VAR_REACH},
    {"valid", NTP_VAR_VALID},
    {"timer", NTP_VAR_TIMER},
    {"delay", NTP_VAR_DELAY},
    {"dispersion", NTP_VAR_DISPERSION},
    {"keyid", NTP_VAR_KEYID},
    {"filtdelay", NTP_VAR_FILTDELAY},
    {"filtoffset", NTP_VAR_FILTOFFSET},
    {"pmode", NTP_VAR_PMODE},
    {"received", NTP_VAR_RECEIVED},
    {"sent", NTP_VAR_SENT},
    {"filtdisp", NTP_VAR_FILTDISP},
    {"flash", NTP_VAR_FLASH},
    {"ttl", NTP_VAR_TTL},
    {"ttlmax", NTP_VAR_TTLMAX},
    {"", NTP_VAR_NULL}
  };

static int
ntp_restart()
{
  int ret=0;
  ret = system (NTP_RH_RESTART_STR);
  if (ret != 0)
    system (NTP_MV_RESTART_STR);

  return 0;
}

static int
ntp_start()
{
  int ret=0;
  ret = system (NTP_RH_START_STR);
  if (ret != 0)
    system (NTP_MV_START_STR);

  return 0;
}

static int
ntp_stop()
{
  int ret=0;
  ret = system (NTP_RH_STOP_STR);
  if (ret != 0)
    system (NTP_MV_STOP_STR);

  return 0;
}

static int
ntp_init_socket (void)
{
  struct sockaddr_in sockaddr;
  int ntp_sock;

  memset (&sockaddr, 0, sizeof(struct sockaddr_in));

  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons (NTP_PORT);
  sockaddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

  ntp_sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (ntp_sock < 0)
    return ntp_sock;

  if (connect (ntp_sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
    {
      close (ntp_sock);
      return -1;
    }

  return ntp_sock;
}

static int
ntp_send_query (int sock, u_char opcode, 
                int associd, 
                char *data, int length)
{
  int ret;
  struct ntp_control ntppkt;
  int size;

  memset (&ntppkt, 0, sizeof (struct ntp_control));

  ntppkt.li_vn_mode = NTP_PACKET_LI_VN_MODE(0, NTP_VERSION_2, NTP_MODE_CONTROL);
  ntppkt.r_m_e_op = (u_char)opcode & NTP_CONTROL_OP_MASK;
  ntppkt.sequence = htons(sequence);
  ntppkt.status = 0;
  ntppkt.associd = htons((u_short)associd);
  ntppkt.offset = 0;
  ntppkt.count = htons((u_short)length);

  /* If we have data, copy it in and pad it out to a 64 bit boundary. */
  if (length > 0)
    {
      pal_assert (data != NULL);

      memmove((char *)ntppkt.data, data, (unsigned)length);
      size = length + NTP_CONTROL_HDR_LEN;
      while (size & (sizeof(u_long) - 1)) 
        {
          ntppkt.data[length++] = 0;
          size++;
        }
    } 
  else 
    {
      size = NTP_CONTROL_HDR_LEN;
    }

  ret = send(sock, &ntppkt, (size_t)size, 0);
  if (ret < 0)
    return ret;

  return ret;
}

static int
ntp_recv_packet (int sock, int opcode, int associd,
                 char **rdata, int *rsize, 
                 unsigned short *rstatus)
{
  struct timeval tvout = { DEFAULTTIMEOUT, 0 };       /* time out for reads */
  struct timeval tvsout = { DEFAULTSECONDARYTIMEOUT, 0 };     /* secondary time out */
  struct ntp_control rpkt;
  struct timeval tvo;
  u_short offsets[NTP_MAXFRAGS + 1];
  u_short counts[NTP_MAXFRAGS + 1];
  u_short offset;
  u_short count;
  int numfrags;
  int seenlastfrag;
  fd_set fds;
  int n;

  /*
   * This is pretty tricky.  We may get between 1 and MAXFRAG packets
   * back in response to the request.  We peel the data out of
   * each packet and collect it in one long block.  When the last
   * packet in the sequence is received we'll know how much data we
   * should have had.  Note we use one long time out, should reconsider.
   */
  *rsize = 0;
  if (rstatus)
    *rstatus = 0;
  memset (ntp_pktdata, 0, sizeof(ntp_pktdata));
  *rdata = (char *)ntp_pktdata;

  numfrags = 0;
  seenlastfrag = 0;

  FD_ZERO(&fds);

 again:
  if (numfrags == 0)
    tvo = tvout;
  else
    tvo = tvsout;
        
  FD_SET(sock, &fds);
  n = select(sock+1, &fds, (fd_set *)0, (fd_set *)0, &tvo);

  if (n == -1) 
    return -1;

  if (n == 0) 
    {
      /*
       * Timed out.  Return what we have
       */
      if (numfrags == 0)
        return -1;
      else 
        return -1;
    }

  memset (&rpkt, 0, sizeof(rpkt));
  n = recv(sock, (char *)&rpkt, sizeof(rpkt), 0);
  if (n == -1)
    {
      return -1;
    }

  /*
   * Check for format errors.  Bug proofing.
   */
  if (n < NTP_CONTROL_HDR_LEN) 
    {
      if (ntp_debug)
        printf("Short (%d byte) packet received\n", n);
      goto again;
    }

  if (NTP_PACKET_VERSION(rpkt.li_vn_mode) > NTP_VERSION_4
      || NTP_PACKET_VERSION(rpkt.li_vn_mode) < NTP_VERSION_1) 
    {
      if (ntp_debug)
        printf("Packet received with version %d\n",
               NTP_PACKET_VERSION(rpkt.li_vn_mode));
      goto again;
    }

  if (NTP_PACKET_MODE(rpkt.li_vn_mode) != NTP_MODE_CONTROL) 
    {
      if (ntp_debug)
        printf("Packet received with mode %d\n",
               NTP_PACKET_MODE(rpkt.li_vn_mode));
      goto again;
    }

  if (!NTP_CONTROL_ISRESPONSE(rpkt.r_m_e_op))
    {
      if (ntp_debug)
        printf("Received request packet, wanted response\n");
      goto again;
    }

  /*
   * Check opcode and sequence number for a match.
   * Could be old data getting to us.
   */
  if (ntohs(rpkt.sequence) != sequence) 
    {
      if (ntp_debug)
        printf(
               "Received sequnce number %d, wanted %d\n",
               ntohs(rpkt.sequence), sequence);
      goto again;
    }

  if (NTP_CONTROL_OP(rpkt.r_m_e_op) != opcode) 
    {
      if (ntp_debug)
        printf(
               "Received opcode %d, wanted %d (sequence number okay)\n",
               NTP_CONTROL_OP(rpkt.r_m_e_op), opcode);
      goto again;
    }

  /*
   * Check the error code.  If non-zero, return it.
   */
  if (NTP_CONTROL_ISERROR(rpkt.r_m_e_op))
    {
      int errcode;

      errcode = (ntohs(rpkt.status) >> 8) & 0xff;
      if (ntp_debug && NTP_CONTROL_ISMORE(rpkt.r_m_e_op))
        {
          printf("Error code %d received on not-final packet\n",
                 errcode);
        }
      if (errcode == NTP_CERR_UNSPEC)
        return -1;
      return errcode;
    }

  /*
   * Check the association ID to make sure it matches what
   * we sent.
   */
  if (ntohs(rpkt.associd) != associd)
    {
      if (ntp_debug)
        printf("Association ID %d doesn't match expected %d\n",
               ntohs(rpkt.associd), associd);
      /*
       * Hack for silly fuzzballs which, at the time of writing,
       * return an assID of sys.peer when queried for system variables.
       */
#ifdef notdef
      goto again;
#endif
    }

  /*
   * Collect offset and count.  Make sure they make sense.
   */
  offset = ntohs(rpkt.offset);
  count = ntohs(rpkt.count);

  if (count > (u_short)(n - NTP_CONTROL_HDR_LEN)) 
    {
      if (ntp_debug)
        printf(
               "Received count of %d octets, data in packet is %d\n",
               count, n - NTP_CONTROL_HDR_LEN);
      goto again;
    }

  if (count == 0 && NTP_CONTROL_ISMORE(rpkt.r_m_e_op))
    {
      if (ntp_debug)
        printf("Received count of 0 in non-final fragment\n");
      goto again;
    }

  if (offset + count > sizeof(ntp_pktdata)) 
    {
      if (ntp_debug)
        printf("Offset %d, count %d, too big for buffer\n",
               offset, count);
      return -1;
    }

  if (seenlastfrag && !NTP_CONTROL_ISMORE(rpkt.r_m_e_op)) 
    {
      if (ntp_debug)
        printf("Received second last fragment packet\n");
      goto again;
    }

  /*
   * So far, so good.  Record this fragment, making sure it doesn't
   * overlap anything.
   */
  if (numfrags == NTP_MAXFRAGS) 
    {
      if (ntp_debug)
        printf("Number of fragments exceeds maximum\n");
      return -1;
    }
        
  for (n = 0; n < numfrags; n++) 
    {
      if (offset == offsets[n])
        goto again;     /* duplicate */
      if (offset < offsets[n])
        break;
    }
        
  if ((u_short)(n > 0 && offsets[n-1] + counts[n-1]) > offset)
    goto overlap;
  if (n < numfrags && (u_short)(offset + count) > offsets[n])
    goto overlap;
        
  {
    register int i;
                
    for (i = numfrags; i > n; i--) {
      offsets[i] = offsets[i-1];
      counts[i] = counts[i-1];
    }
  }
  offsets[n] = offset;
  counts[n] = count;
  numfrags++;

  /*
   * Got that stuffed in right.  Figure out if this was the last.
   * Record status info out of the last packet.
   */
  if (!NTP_CONTROL_ISMORE(rpkt.r_m_e_op)) {
    seenlastfrag = 1;
    if (rstatus != 0)
      *rstatus = ntohs(rpkt.status);
  }

  /*
   * Copy the data into the data buffer.
   */
  memmove((char *)ntp_pktdata + offset, (char *)rpkt.data, count);

  /*
   * If we've seen the last fragment, look for holes in the sequence.
   * If there aren't any, we're done.
   */
  if (seenlastfrag && offsets[0] == 0) {
    for (n = 1; n < numfrags; n++) {
      if (offsets[n-1] + counts[n-1] != offsets[n])
        break;
    }
    if (n == numfrags) {
      *rsize = offsets[numfrags-1] + counts[numfrags-1];
      return 0;
    }
  }
  goto again;

 overlap:
  /*
   * Print debugging message about overlapping fragments
   */
  goto again;
 
}

static int
ntp_get_variable (int *size, char **data,
                  char **var, char **ovalue)
{
#define MAXVARLEN        400
  char *cp;
  char *np;
  char *cpend;
  char *npend;  /* character after last */
  int quoted = 0;
  static char name[MAXVARLEN];
  static char value[MAXVARLEN];

  cp = *data;
  cpend = cp + *size;

  /*
   * Space past commas and white space
   */
  while (cp < cpend && (*cp == ',' || isspace((int)*cp)))
    cp++;
  if (cp == cpend)
    return 0;
        
  /*
   * Copy name until we hit a ',', an '=', a '\r' or a '\n'.  Backspace
   * over any white space and terminate it.
   */
  np = name;
  npend = &name[MAXVARLEN];
  while (cp < cpend && np < npend && *cp != ',' && *cp != '='
         && *cp != '\r' && *cp != '\n')
    *np++ = *cp++;
  /*
   * Check if we ran out of name space, without reaching the end or a
   * terminating character
   */
  if (np == npend && !(cp == cpend || *cp == ',' || *cp == '=' ||
                       *cp == '\r' || *cp == '\n'))
    return 0;
  while (isspace((int)(*(np-1))))
    np--;
  *np = '\0';
  *var = name;

  /*
   * Check if we hit the end of the buffer or a ','.  If so we are done.
   */
  if (cp == cpend || *cp == ',' || *cp == '\r' || *cp == '\n') 
    {
      if (cp != cpend)
        cp++;
      *data = cp;
      *size = cpend - cp;
      *ovalue = (char *)0;
      return 1;
    }

  /*
   * So far, so good.  Copy out the value
   */
  cp++; /* past '=' */
  while (cp < cpend && (isspace((int)*cp) && *cp != '\r' && *cp != '\n'))
    cp++;
  np = value;
  npend = &value[MAXVARLEN];
  while (cp < cpend && np < npend && ((*cp != ',') || quoted))
    {
      quoted ^= ((*np++ = *cp++) == '"');
    }

  /*
   * Check if we overran the value buffer while still in a quoted string
   * or without finding a comma
   */
  if (np == npend && (quoted || *cp != ','))
    return 0;
  /*
   * Trim off any trailing whitespace
   */
  while (np > value && isspace((int)(*(np-1))))
    np--;
  *np = '\0';

  /*
   * Return this.  All done.
   */
  if (cp != cpend)
    cp++;
  *data = cp;
  *size = cpend - cp;
  *ovalue = value;
  return 1;
}

static void
ntp_get_sys_time (struct long_fp *now)
{
  struct timeval tv;
  double temp;

  gettimeofday (&tv, 0);
  now->sintegral = tv.tv_sec + JAN_1970;
 
  temp = tv.tv_usec * FRAC / 1e6;
  if (temp >= FRAC)
    now->sintegral++;

  now->ufractional = (u_int32_t)temp;

  return;
}

static u_int32_t
ntp_get_last_pkt (struct long_fp *ts, 
                  struct long_fp *rec, 
                  struct long_fp *reftime)
{
  struct long_fp *lasttime;

  if (rec->uintegral != 0)
    lasttime = rec;
  else if (reftime->uintegral != 0)
    lasttime = reftime;
  else
    return 0;

  return (ts->uintegral - lasttime->uintegral);
}

enum ntpvar_operand
ntp_find_variable (char *var)
{
  int i = 0;

  while (pal_strcmp (ntpvar[i].string, "") != 0)
    {
      if (!pal_strcmp (ntpvar[i].string, var))
        return ntpvar[i].operand;
      i++;
    }

  return NTP_VAR_NULL;
}

/* Utility routine to get array of long FPs from a buffer. */
int
ntp_decode_longfp_arr (char *str, int *num, struct long_fp *lfparr)
{
  char *cp, *bp;
  struct long_fp *lfp;
  char buf[60];
                                                                                
  lfp = lfparr;
  cp = str;
  *num = 0;
                                                                                
  while (*num < 8) 
    {
      while (isspace((int)*cp))
        cp++;
      
      /* If done. */
      if (*cp == '\0')
        break;

      bp = buf;
      while (!isspace((int)*cp) && *cp != '\0')
        *bp++ = *cp++;
      *bp++ = '\0';

      if (atolongfp (buf, lfp) < 0)
        return -1;
      
      (*num)++;
      lfp++;
    }

  return 0;
} 

/* Octal to unsigned integer. */
static int
octtoint (const char *str, u_int32_t *ival)
{
  register u_int32_t u;
  register const char *cp;

  cp = str;

  if (*cp == '\0')
    return -1;

  u = 0;
  while (*cp != '\0') {
    if (!isdigit((int)*cp) || *cp == '8' || *cp == '9')
      return 0;
    if (u >= 0x20000000)
      return 0;   /* overflow */

    u <<= 3;
    u += *cp++ - '0';       /* ascii dependent */
  }
  *ival = u;
  return 0;
}

/* Hex to unsigned integer. */
static int
hextoint(const char *str, u_int32_t *ival)
{
  register u_int32_t u;
  register const char *cp;

  cp = str;

  if (*cp == '\0')
    return -1;
  u = 0;
  while (*cp != '\0') 
    {
      if (!isxdigit((int)*cp))
        return 0;
      if (u >= 0x10000000)
        return -1;   /* overflow */
      u <<= 4;
      if (*cp <= '9')         /* very ascii dependent */
        u += *cp++ - '0';
      else if (*cp >= 'a')
        u += *cp++ - 'a' + 10;
      else
        u += *cp++ - 'A' + 10;
    }
  *ival = u;
  return 0;
}

/* Decode a unsigned integer. */
int
decodeuint (char *str, u_int32_t *val)
{
  if (*str == '0') 
    {
      if (*(str + 1) == 'x' 
          || *(str + 1) == 'X')
        return (hextoint(str + 2, val));
      return (octtoint(str, val));
    }

  *val = atoi (str);
  return  0;
}

/* Milliseconds to long fp. */
int
millisectolongfp (const char *str, struct long_fp *lfp)
{
  register const char *cp;
  register char *bp;
  register const char *cpdec;
  char buf[100];

  bp = buf;
  cp = str;
  while (isspace((int)*cp))
    cp++;

  /* Move decimal three places to convert in seconds format. */
  
  if (*cp == '-') 
    {
      *bp++ = '-';
      cp++;
    }
  
  if (*cp != '.' && !isdigit((int)*cp))
    return -1;

  /* Search forward for the decimal point or the end of the string.  */
  cpdec = cp;
  while (isdigit((int)*cpdec))
    cpdec++;
  
  /* Found something.  If we have more than three digits copy the  over, 
     else insert a leading 0. */
  if ((cpdec - cp) > 3) 
    {
      do 
        {
          *bp++ = (char)*cp++;
        } while ((cpdec - cp) > 3);
    } 
  else 
    {
      *bp++ = '0';
    }

  /* Stick the decimal in.  If we've got less than three digits in
     front of the millisecond decimal we insert the appropriate number
     of zeros.  */
  *bp++ = '.';
  if ((cpdec - cp) < 3) 
    {
      register int i = 3 - (cpdec - cp);
      
      do 
        {
          *bp++ = '0';
        } while (--i > 0);
    }

  /* Copy the remainder up to the millisecond decimal.  If cpdec
     is pointing at a decimal point, copy in the trailing number too.  */
  while (cp < cpdec)
    *bp++ = (char)*cp++;
  
  if (*cp == '.') 
    {
      cp++;
      while (isdigit((int)*cp))
        *bp++ = (char)*cp++;
    }
  *bp = '\0';
  
  /* Check to make sure the string is properly terminated.  If so, give the
     buffer to the decoding routine.  */
  if (*cp != '\0' && !isspace((int)*cp))
    return 0;
  return atolongfp (buf, lfp);
}

int
pal_ntp_update_status(struct ntp_sys_stats *stats)
{
  char data[NTP_MAX_DATA_LEN], *datap;
  int size, sizep;
  int ntp_sock;
  u_char opcode;
  u_int16_t status;
  char *name, *value;
  int operand;

  ntp_sock = ntp_init_socket ();
  if (ntp_sock < 0)
    return -1;

  /* Currently no variables are being sent. */
  size = 0;

  /* Set opcode. */
  opcode = NTP_CONTROL_OP_READVAR;

  ntp_send_query (ntp_sock, opcode, 0 /* associd */, 
                  data, size);

  ntp_recv_packet (ntp_sock, opcode, 0 /* associd */,
                   &datap, &sizep, 
                   &status);

  stats->status = status;

  /* Scan all variables and populate. */
  while (ntp_get_variable (&sizep, &datap, &name, &value))
    {
      operand =  ntp_find_variable (name);
      if (operand == 0)
        continue;

      switch (operand)
        {
        case NTP_VAR_VERSION:
          /* Ignore. */
          break;
        case NTP_VAR_PROCESSOR:
          /* Ignore. */
          break;
        case NTP_VAR_SYSTEM:
          /* Ignore. */
          break;
        case NTP_VAR_LEAP:
          stats->leap = (u_char)atoi(value);
          break;
        case NTP_VAR_STRATUM:
          stats->stratum = (u_char)atoi(value);
          break;
        case NTP_VAR_PRECISION:
          stats->precision = atoi(value);
          break;
        case NTP_VAR_ROOTDELAY:
          atolongfp(value, &stats->rootdelay);
          break;
        case NTP_VAR_ROOTDISPERSION:
          atolongfp(value, &stats->rootdispersion);
          break;
        case NTP_VAR_PEER:
          stats->peer = atoi(value);
          break;
        case NTP_VAR_REFID:
          stats->refid = pal_strdup (MTYPE_TMP, value);
          break;
        case NTP_VAR_REFTIME:
          hextolongfp(value, &stats->reftime);
          break;
        case NTP_VAR_POLL:
          stats->poll = atoi(value);
          break;
        case NTP_VAR_CLOCK:
          atolongfp(value, &stats->clock);
          break;
        case NTP_VAR_STATE:
          stats->state = atoi(value);
          break;
        case NTP_VAR_OFFSET:
          millisectolongfp (value, &stats->offset);
          break;
        case NTP_VAR_FREQUENCY:
          atolongfp(value, &stats->frequency);
          break;
        case NTP_VAR_JITTER:
          millisectolongfp (value, &stats->jitter);
          break;
        case NTP_VAR_STABILITY:
          atolongfp(value, &stats->stability);
          break;
        }
    }

  /* For next control packet. */
  sequence++;
  
  /* Close control socket. */
  close (ntp_sock);

  return 0;
}

int
pal_ntp_update_associations(struct ntp_all_peer_stats *peerstats)
{
  char data[NTP_MAX_DATA_LEN]; 
  u_int16_t *dataq;
  char *datap;
  char *name, *value;
  int size, sizep;
  int ntp_sock;
  u_char opcode;
  u_int16_t status;
  struct ntp_associations assoc[MAX_NTP_ASSOCIATIONS];
  u_int16_t num;
  int i, ret;
  struct ntp_peer_stats *stats = NULL;
  int narr;
  enum ntpvar_operand operand;
  struct long_fp now;

  ntp_sock = ntp_init_socket ();
  if (ntp_sock < 0)
    return -1;

  /* Currently no variables are being sent. */
  size = 0;

  /* Get current time in long fp format. */
  ntp_get_sys_time (&now);

  /* Set opcode. */
  opcode = NTP_CONTROL_OP_READSTAT;

  /* First get the list of associations. */
  ntp_send_query (ntp_sock, opcode, 0 /* associd */, 
                  0, 0);

  ntp_recv_packet (ntp_sock, opcode, 0 /* associd */,
                   (char **)&dataq, &sizep, 
                   &status);

  /* For next control packet. */
  sequence++;

  if (sizep == 0)
    {
      /* No association IDs returned. */
      close(ntp_sock);
      return -1;
    }
 
  if (sizep & 0x3)
    {
      /* Server returned odd octets. */
      close(ntp_sock);
      return -1;
    }

  num = 0;
  while (sizep > 0)
    {
      assoc[num].association_id = ntohs(*dataq);
      dataq++;
      assoc[num].status = ntohs(*dataq);
      dataq++;

      if (++num >= MAX_NTP_ASSOCIATIONS)
        break;
      sizep -= sizeof(u_int16_t) + sizeof(u_int16_t);
    }

  /* Allocate the array for the peer status. */
  peerstats->num = num;
  peerstats->stats = XCALLOC (MTYPE_TMP, sizeof (struct ntp_peer_stats) * num);
 
  /* For all peers get the status. */
  for (i = 0; i < num; i++)
    {
      stats = &peerstats->stats[i];

      /* Set opcode. */
      opcode = NTP_CONTROL_OP_READVAR;

      /* Currently no variables are being sent. */
      size = 0;
     
      /* Get associations. */
      ntp_send_query (ntp_sock, opcode, assoc[i].association_id, 
                      data, size);
     
      ntp_recv_packet (ntp_sock, opcode, assoc[i].association_id,
                       (char **)&datap, &sizep, 
                       &status);

      /* Update status. */
      stats->status = status;

      /* Scan all variables and populate. */
      while (ntp_get_variable (&sizep, &datap, &name, &value))
        {
          operand =  ntp_find_variable (name);
          if (operand == 0)
            continue;

          switch (operand)
            {
            case NTP_VAR_VERSION:
              /* Ignore. */
              break;
            case NTP_VAR_PROCESSOR:
              /* Ignore. */
              break;
            case NTP_VAR_SYSTEM:
              /* Ignore. */
              break;
            case NTP_VAR_LEAP:
              stats->leap = (u_char)atoi(value);
              break;
            case NTP_VAR_STRATUM:
              stats->stratum = (u_char)atoi(value);
              break;
            case NTP_VAR_PRECISION:
              stats->precision = atoi(value);
              break;
            case NTP_VAR_ROOTDELAY:
              atolongfp (value, &stats->rootdelay);
              break;
            case NTP_VAR_ROOTDISPERSION:
              atolongfp (value, &stats->rootdispersion);
              break;
            case NTP_VAR_REFID:
              stats->refid = pal_strdup (MTYPE_TMP, value);
              break;
            case NTP_VAR_REFTIME:
              hextolongfp (value, &stats->reftime);
              break;
            case NTP_VAR_POLL:
              /* Ignore. */
              break;
            case NTP_VAR_CLOCK:
              /* Ignore. */
              break;
            case NTP_VAR_STATE:
              /* Ignore. */
              break;
            case NTP_VAR_OFFSET:
              millisectolongfp (value, &stats->offset);
              break;
            case NTP_VAR_FREQUENCY:
              /* Ignore. */
              break;
            case NTP_VAR_JITTER:
              millisectolongfp (value, &stats->jitter);
              break;
            case NTP_VAR_STABILITY:
              /* Ignore. */ 
              break;
            case NTP_VAR_CONFIG:
              /* Ignore. */
              break;
            case NTP_VAR_AUTHENABLE:
              /* Ignore. */
              break;
            case NTP_VAR_AUTHENTIC:
              /* Ignore. */
              break;
            case NTP_VAR_SRCADR:
              stats->srcadr = pal_strdup (MTYPE_TMP, value);
              break;
            case NTP_VAR_SRCPORT:
              stats->srcport = atoi (value);
              break;
            case NTP_VAR_DSTADR:
              stats->dstadr = pal_strdup (MTYPE_TMP, value);
              break;
            case NTP_VAR_DSTPORT:
              stats->dstport = atoi (value);
              break;
            case NTP_VAR_HMODE:
              stats->hmode = atoi (value);
              break;
            case NTP_VAR_PPOLL:
              stats->ppoll = atoi (value);
              break;
            case NTP_VAR_HPOLL:
              stats->hpoll = atoi (value);
              break;
            case NTP_VAR_ORG:
              hextolongfp (value, &stats->org);
              break;
            case NTP_VAR_REC:
              hextolongfp (value, &stats->rec);
              break;
            case NTP_VAR_XMT:
              hextolongfp (value, &stats->xmt);
              break;
            case NTP_VAR_REACH:
              decodeuint (value, &stats->reach);
              break;
            case NTP_VAR_VALID:
              /* Ignore. */
              break;
            case NTP_VAR_TIMER:
              /* Ignore. */
              break;
            case NTP_VAR_DELAY:
              /* Ignore. */
              millisectolongfp (value, &stats->delay);
              break;
            case NTP_VAR_DISPERSION:
              millisectolongfp (value, &stats->dispersion);
              break;
            case NTP_VAR_KEYID:
              stats->keyid = atoi (value);
              break;
            case NTP_VAR_FILTDELAY:
              ret = ntp_decode_longfp_arr (value, &narr, &stats->filtdelay[0]);
              if ((ret < 0) || (narr == 0))
                memset (&stats->filtdelay[0], 0, sizeof (struct long_fp) * 8);
              break;
            case NTP_VAR_FILTOFFSET:
              ret = ntp_decode_longfp_arr (value, &narr, &stats->filtoffset[0]);
              if ((ret < 0) || (narr == 0))
                memset (&stats->filtoffset[0], 0, sizeof (struct long_fp) * 8);
              break;
            case NTP_VAR_PMODE:
              stats->pmode = atoi (value);
              break;
            case NTP_VAR_RECEIVED:
              /* Ignore. */
              break;
            case NTP_VAR_SENT:
              /* Ignore. */
              break;
            case NTP_VAR_FILTDISP:
              ret = ntp_decode_longfp_arr (value, &narr, &stats->filtdisp[0]);
              if ((ret < 0) || (narr == 0))
                memset (&stats->filtdisp[0], 0, sizeof (struct long_fp) * 8);
              break;
            case NTP_VAR_FLASH:
              stats->flash = atoi (value);
              break;
            case NTP_VAR_TTL:
              /* Ignore. */
              break;
            case NTP_VAR_TTLMAX:
              /* Ignore. */
              break;
            default:
              /* Ignore. */
              break;
            }
        }

    }

  /* Get the last packet arrival time. */
  if (stats)
    stats->when = ntp_get_last_pkt (&now, &stats->rec, &stats->reftime);

  /* Close control socket. */
  close (ntp_sock);

  return 0;
}

int
pal_ntp_refresh_keys (struct lib_globals *lib_node, struct list *ntp_key_list)
{
  FILE *fp;
  struct listnode *node;
  struct ntp_key *k;
  int ret;
  char *key_code;

  IMI_ASSERT (ntp_key_list != NULL);

  /* Open file /etc/ntp.keys. */
  fp = fopen (NTP_KEYS_FILE, "w");
  if (fp == NULL)
    return IMI_API_FILE_OPEN_ERROR;

  /* Seek to start of file. */
  ret = fseek (fp, 0, 0);
  if (ret != RESULT_OK)
    {
      fclose (fp);
      return IMI_API_FILE_SEEK_ERROR;
    }

  /* Write top line. */
  ret = fprintf (fp, "%s", NTP_KEYS_TOP_FILE_TEXT);

  LIST_LOOP (ntp_key_list, k, node)
    {
      /* Only MD5 supported from CLI. */
      if (k->type == NTP_KEY_MD5)
        key_code = "M";
      else
        continue; 

      fprintf (fp, "%u\t\t%s\t\t%s\n", k->key_num, key_code, k->key);
    }

  /* Close filehandle. */
  fclose (fp);

  return 0;
}

static int 
ntp_filter_write (struct lib_globals *lib_node, FILE *fp,
                  struct pal_in4_addr addr,
                  struct pal_in4_addr addr_mask,
                  u_int32_t flags)
{
  char buf[512];
  char *ignore_str = "";
  char *nomodify_str = "";
  char *notrap_str = "";
  char *noquery_str = "";
  char *noserve_str = "";

  /* Inverse mask. */
  addr_mask.s_addr = ~addr_mask.s_addr;

  if (CHECK_NTP_RESTRICT_FLAG(flags, NTP_FLAG_IGNORE))
    ignore_str = NTP_FLAG_IGNORE_STR;

  if (CHECK_NTP_RESTRICT_FLAG(flags, NTP_FLAG_NOMODIFY))
    nomodify_str = NTP_FLAG_NOMODIFY_STR;

  if (CHECK_NTP_RESTRICT_FLAG(flags, NTP_FLAG_NOTRAP))
    notrap_str = NTP_FLAG_NOTRAP_STR;

  if (CHECK_NTP_RESTRICT_FLAG(flags, NTP_FLAG_NOQUERY))
    noquery_str = NTP_FLAG_NOQUERY_STR;

  if (CHECK_NTP_RESTRICT_FLAG(flags, NTP_FLAG_NOSERVE))
    noserve_str = NTP_FLAG_NOSERVE_STR;

  zsnprintf (buf, sizeof (buf), "restrict %r mask %r %s %s %s %s %s \n",
             &addr, &addr_mask, ignore_str, nomodify_str, notrap_str,
             noquery_str, noserve_str);

  fprintf (fp, "%s", buf);

  return 0;
}

static int
ntp_write_acl (struct lib_globals *lib_node, FILE *fp, struct access_list *acl,
               u_int32_t permit_flags, u_int32_t deny_flags)
{
  struct filter_list *node;
  struct filter_common *cfilter;

  pal_assert (acl != NULL);

  for (node = acl->head; node; node = node->next)
    {
      if (node->common != FILTER_COMMON)
        continue;
      
      cfilter = &node->u.cfilter;
      
      if (node->type == FILTER_PERMIT)
        ntp_filter_write (lib_node, fp, cfilter->addr, cfilter->addr_mask,
                          permit_flags);
      else if (node->type == FILTER_DENY)
        ntp_filter_write (lib_node, fp, cfilter->addr, cfilter->addr_mask,
                          deny_flags);
      else
        continue;
    }

  return 0;
}

static int 
ntp_restrictions_write (struct lib_globals *lib_node, FILE *fp,
                        struct ntp_master *ntpm)
{
  struct access_list *acl;
  struct ntp_access_group *access;
  char buf[10];
  struct apn_vr *vr = apn_vr_get_privileged (lib_node);
   
  access = &ntpm->access;

  /* Process full access, access group. */
  if (access->peer)
    {
      zsnprintf (buf, 10, "%d", access->peer);
 
      /* Lookup ACL. */
      acl = access_list_lookup (vr, AFI_IP, buf);
      if (acl != NULL)
        ntp_write_acl (lib_node, fp, acl, 
                     NTP_FLAG_PERMIT_PEER,   /* Permit flags. */
                     NTP_FLAG_DENY_PEER);    /* Deny flags. */
    }
  
  /* Process query-only. */
  if (access->query_only)
    {
      zsnprintf (buf, 10, "%d", access->query_only);
 
      /* Lookup ACL. */
      acl = access_list_lookup (vr, AFI_IP, buf);
      if (acl != NULL)
        ntp_write_acl (lib_node, fp, acl, 
                     NTP_FLAG_PERMIT_QUERY_ONLY, /* Permit flags. */
                     NTP_FLAG_DENY_QUERY_ONLY);  /* Deny flags. */
    }

  /* Process serve. */
  if (access->serve)
    {
      zsnprintf (buf, 10, "%d", access->serve);
 
      /* Lookup ACL. */
      acl = access_list_lookup (vr, AFI_IP, buf);
      if (acl != NULL)
        ntp_write_acl (lib_node, fp, acl, 
                     NTP_FLAG_PERMIT_SERVE,    /* Permit flags. */      
                     NTP_FLAG_DENY_SERVE);     /* Deny flags. */
    }

  /* Process serve-only. */
  if (access->serve_only)
    {
      zsnprintf (buf, 10, "%d", access->serve_only);
 
      /* Lookup ACL. */
      acl = access_list_lookup (vr, AFI_IP, buf);
      if (acl != NULL)
        ntp_write_acl (lib_node, fp, acl, 
                     NTP_FLAG_PERMIT_SERVE_ONLY,  /* Permit flags. */
                     NTP_FLAG_DENY_SERVE_ONLY);   /* Deny flags. */
    }

  return 0;
}

int
pal_ntp_refresh_config (struct lib_globals *lib_node, struct ntp_master *ntpm)
{
  FILE *fp;
  struct listnode *node;
  struct ntp_neighbor *nbr;
  ntp_auth_key_num *key_num;
  char buf[512], buf1[512];
  int ret;
  char *nbr_str;

  IMI_ASSERT(ntpm != NULL);

  /* Open file /etc/ntp.keys. */
  fp = fopen (NTP_CONF_FILE, "w");
  if (fp == NULL)
    return IMI_API_FILE_OPEN_ERROR;

  /* Seek to start of file. */
  ret = fseek (fp, 0, 0);
  if (ret != RESULT_OK)
    {
      fclose (fp);
      return IMI_API_FILE_SEEK_ERROR;
    }

  /* Write top line. */
  ret = fprintf (fp, NTP_CONF_TOP_FILE_TEXT);

  /* Master clock set? */
  if (ntpm->master_clock_flag == NTP_MASTER_CLOCK)
    {
      fprintf (fp, "server %s  #Local clock\n", NTP_MASTER_LOCAL_CLOCK);

      if (ntpm->stratum == NTP_STRATUM_ZERO)
        fprintf (fp, "fudge %s  #Not disciplined\n", NTP_MASTER_LOCAL_CLOCK);
      else
        fprintf (fp, "fudge %s %d #Not disciplined\n", NTP_MASTER_LOCAL_CLOCK,
                 ntpm->stratum);
    }

  /* Loop through the list of neighbors. */
  LIST_LOOP (ntpm->ntp_neighbor_list, nbr, node)
    {
      if (nbr->type == NTP_MODE_SERVER)
        nbr_str = "server";
      else if (nbr->type == NTP_MODE_PEER)
        nbr_str = "peer";
      else
        continue;

      /* Copy the server name. */
      zsnprintf (buf, 512, "%s %s ", nbr_str, nbr->name);

      if (nbr->key_num > 0)
        {
          zsnprintf (buf1, 512, "%s key %ld ", buf, nbr->key_num);
          memcpy (buf, buf1, strlen(buf1) + 1);
        }

      if (nbr->preference == NTP_PREFERENCE_YES)
        {
          zsnprintf (buf1, 512, "%s prefer %s", buf, " ");
          memcpy (buf, buf1, strlen(buf1) + 1);
        }

      if(nbr->version > NTP_VERSION_IGNORE)
        {
          if (nbr->version == NTP_VERSION_1)
            zsnprintf (buf1, 512, "%s version 1 ", buf);
          else if (nbr->version == NTP_VERSION_2)
            zsnprintf (buf1, 512, "%s version 2 ", buf);
          else if (nbr->version == NTP_VERSION_3)
            zsnprintf (buf1, 512, "%s version 3 ", buf);
          else if (nbr->version == NTP_VERSION_4)
            zsnprintf (buf1, 512, "%s version 4 ", buf);

          memcpy (buf, buf1, strlen(buf1) + 1);
        }
     
      zsnprintf (buf1, 512, "%s\n", buf);
     
      /* Write nbr config. */
      ret = fprintf (fp, "%s", buf1);
    }

  /* Write authentication information. */
  if (ntpm->authentication == NTP_AUTHENTICATE)
    {
      zsnprintf (buf, 512, "authenticate yes\n");
      ret = fprintf (fp, "%s", buf);
    }

  /* Write broadcast delay information. */
  if (ntpm->broadcastdelay > 0)
    {
      float delay;

      delay = (float)ntpm->broadcastdelay / (float)1000;

      ret = fprintf (fp, NTP_CONF_BDELAY_TEXT);
      zsnprintf (buf, 512, "broadcastdelay  %f\n", delay);
      ret = fprintf (fp, "%s", buf);
    }

  /* Write drift file information. */
  ret = fprintf (fp, NTP_CONF_DRIFT_TEXT);

  /* Write authentication keys information. */
  ret = fprintf (fp, NTP_CONF_KEY_TEXT);
   
  /* Write trusted key information. */
  LIST_LOOP (ntpm->ntp_trusted_key_list, key_num, node)
    {
      zsnprintf (buf, 512, "trustedkey %ld \n", *key_num);
      ret = fprintf (fp, "%s", buf);
    }

  /* Write access-lists. */
  ntp_restrictions_write (lib_node, fp, ntpm);

  /* Close filehandle. */
  fclose (fp);

  /* Restart NTP. */
  ntp_restart();
 
  return 0;
}

int
pal_ntp_init ()
{
  /* Truncate NTP configuration. */
  truncate (NTP_CONF_FILE, 0);

  /* Start NTP. */
  ntp_start();  

  return 0;
}

int
pal_ntp_deinit()
{
  /* Stop NTP. */
  ntp_stop();
  
  return 0;
}

#endif /* HAVE_NTP. */
