/* Copyright (C) 2001 - 2004  All Rights Reserved. */


#include "pal.h"
#include "lib.h"

#ifdef HAVE_SNMP

#include "l2_snmp.h"

/* an array of bit masks in SNMP bit order (*MSBit first) */
static char bitstring_mask[] = { 1<<7, 1<<6, 1<<5, 1<<4, 1<<3, 1<<2, 1<<1, 1<<0 };

/* copies a byte array into an oid, converting to unsigned int values. */

void
oid_copy_bytes2oid (oid oid[], void *addr, s_int32_t len)
{
  s_int32_t i;
  u_char *pnt = (u_char *) addr;
  
  for (i = 0; i < len; i++)
    oid[i] = *pnt++;
}

/* copy oid values to an unsigned byte array. 
 */

int
oid2bytes (oid oid[], s_int32_t len, void *addr)
{

  s_int32_t i;
  u_char *pnt = (u_char *) addr;
  
  for (i = 0; i < len; i++)
  {
    *pnt++ = (oid[i] > UINT8_MAX ? UINT8_MAX : oid[i] );
  }
  return 0;
}


/*
 * bsearch_next
 *
 * performs a binary search and returns the next element in the table. 
 * returns NULL if no such element exists.
 */

void *
l2_bsearch_next(const void *key, const void *base0, size_t nelements,
             size_t element_size, int (*compar)(const void *, const void *))
{
  const char *base = base0;
  size_t lim;
  int cmp = 0;
  const void *p = NULL, *pnext;
  int i=0;

  for (lim = nelements; lim != 0; lim >>= 1)
  {
    p = base + (lim >> 1) * element_size;
    cmp = (*compar)(key, p);

    if (cmp == 0)
      break;

    if (cmp > 0)
    {
      lim--;
      base = (const char *)p + element_size;
    }
  }

  if ( lim == 0 && cmp < 0 )
      return (void *)p;

  pnext = (const char *)p + element_size;

  if ( (const char *)pnext >= (const char *)base0 +
                            (nelements * element_size) )
    return NULL;

  /* This has been specifically added for the Multicast Mac-Address.
   * Their can be more then 1 instance of Multicast Mac, we have
   * to find the next best match.
   */
  while (!(*compar)(key, pnext))
    {
      pnext = (const char *)p + i++ * element_size;

      if ( (const char *)pnext >= (const char *)base0 +
                                  (nelements * element_size) )
        return NULL;
    }

  if ( (const char *)pnext < (const char *)base0 + (nelements * element_size) )
    return (void *)pnext;
  else
    return NULL;
}

/* Utility function to get INTEGER index.  */
int
l2_snmp_index_get (struct variable *v, oid * name, size_t * length,
                    u_int32_t * index, int exact)
{

  int result, len;

  *index = 0;

  result = oid_compare (name, v->namelen, v->name, v->namelen);

  if (exact)
    {
      /* Check the length. */
      if (result != 0 || *length - v->namelen != 1)
        return -1;

      *index = name[v->namelen];
      return 0;
    }
  else                          /* GETNEXT request */
    {
      /* return -1 if greater than our our table entry */
      if (result > 0)
        return -1;
      else if (result == 0)
        {
          /* use index value if present, otherwise 0. */
          len = *length - v->namelen;
          if (len >= 1)
            *index = name[v->namelen];
          return 0;
        }
      else
        {
          /* set the user's oid to be ours */
          pal_mem_cpy (name, v->name, ((int) v->namelen) * sizeof (oid));
          return 0;
        }
    }
}

/* Utility function to set the object name and INTEGER index. */
void
l2_snmp_index_set (struct variable *v, oid * name, size_t * length,
                    u_int32_t index)
{
  name[v->namelen] = index;
  *length = v->namelen + 1;
}

/* Utility function to get a MAC address index.  */
int
l2_snmp_mac_addr_index_get (struct variable *v, oid *name, size_t *length, 
                    struct mac_addr *addr, int exact)
{
  int len;
  
  if (exact)
    {
      /* Check the length. */
      if (*length - v->namelen != sizeof (struct mac_addr))
        return -1;

      if ( oid2bytes (name + v->namelen, sizeof (struct mac_addr), addr) != 0 )
        return -1;
      
      return 0;
    }
  else
    {
      pal_mem_set( addr, 0, sizeof(struct mac_addr) );

      len = *length - v->namelen;
      if (len > sizeof (struct mac_addr)) len = sizeof (struct mac_addr);

      if ( oid2bytes (name + v->namelen, len, addr) != 0 )
        return -1;
      
      return 0;
    }
  return -1;
}

/* Utility function to set a mac address index.  */
void
l2_snmp_mac_addr_index_set (struct variable *v, oid *name, size_t *length,
                    struct mac_addr *addr)
{
  oid_copy (name, v->name, v->namelen * sizeof(oid) );
  
  oid_copy_bytes2oid (name + v->namelen, addr, sizeof (struct mac_addr));
  *length = v->namelen + sizeof (struct mac_addr);
}

/* Utility function to get a MAC address + integer index.  */
int
l2_snmp_port_vlan_index_get (struct variable *v, oid *name, size_t *length, 
                             u_int32_t *port, u_int32_t *vlan, int exact)
{
  int result, len;

  *port = 0;
  *vlan = 0;
  
  result = oid_compare (name, v->namelen, v->name, v->namelen);
  
  if (exact)
    {
      /* Check the length. */
      if (result != 0 || *length - v->namelen != 2)
        return -1;
      
      *port = name[v->namelen];
      *vlan = name[v->namelen+1];
      return 0;
    }
  else {
    /* return -1 if greater than our our table entry */
      if (result > 0)
        return -1;
      else if (result == 0)
        {
          /* use index value if present, otherwise 0. */
          len = *length - v->namelen;
          if (len >= 2) {
            *port = name[v->namelen];
            *vlan = name[v->namelen+1];
          }
          return 0;
        }
      else
        {
          /* set the user's oid to be ours */
          pal_mem_cpy (name, v->name, ((int) v->namelen) * sizeof (oid));
          return 0;
        }
    }
}

/* Utility function to set the object name and INTEGER index. */
void
l2_snmp_port_vlan_index_set (struct variable *v, oid * name, size_t * length,
                    u_int32_t port, u_int32_t vlan)
{
  name[v->namelen] = port;
  name[v->namelen+1] = vlan;
  *length = v->namelen + 2;
}


/* Utility function to get a MAC address + integer index.  */
int
l2_snmp_mac_addr_int_index_get (struct variable *v, oid *name, size_t *length, 
                    struct mac_addr *addr, int *idx, int exact)
{
  s_int32_t maclen, len;
  u_int8_t val = 0;
  
  if (exact)
    {
      /* Check the length. */
      if (*length - v->namelen != sizeof (struct mac_addr) + 1)
        return -1;

      if ( oid2bytes (name + v->namelen, sizeof (struct mac_addr), addr) != 0 )
        return -1;
      
      *idx = name[v->namelen + sizeof (struct mac_addr)];
      return 0;
    }
  else
    {
      pal_mem_set( addr, 0, sizeof(struct mac_addr) );
      *idx = 0;

      maclen = len = *length - v->namelen;
      
      val = sizeof (struct mac_addr);
      if (maclen > val)
        maclen = sizeof (struct mac_addr);

      if ( oid2bytes (name + v->namelen, maclen, addr) != 0 )
        return -1;
      
      len = len - maclen;
      if ( len > 0 )
        *idx = name[v->namelen + sizeof (struct mac_addr)];
      
      return 0;
    }

  return -1;
}

/* Utility function to set a mac address + integer index.  */
void
l2_snmp_mac_addr_int_index_set (struct variable *v, oid *name, size_t *length,
                    struct mac_addr *addr, int idx )
{
  oid_copy (name, v->name, v->namelen * sizeof(oid) );
  
  oid_copy_bytes2oid (name + v->namelen, addr, sizeof (struct mac_addr));
  *length = v->namelen + sizeof (struct mac_addr);

  name[v->namelen + sizeof (struct mac_addr)] = idx;
  *length += 1;

}
/* Utility function to get TWO integer indices.  */
int
l2_snmp_integer_index_get (struct variable *v, oid *name, size_t *length,
                             u_int32_t *index1, u_int32_t *index2, int exact)
{
  int result, len;

  *index1 = 0;
  *index2 = 0;

  result = oid_compare (name, v->namelen, v->name, v->namelen);

   if (exact)
    {     
            /* Check the length. */
      if (result != 0 || *length - v->namelen != 2)
        return -1; 
      *index1 = name[v->namelen];
      *index2 = name[v->namelen+1];
      return 0;
    }
  else {
    /* return -1 if greater than our our table entry */
      if (result > 0)
        return -1;
      else if (result == 0)
        {
          /* use index value if present, otherwise 0. */
          len = *length - v->namelen;
          if (len >= 2) {
            *index1 = name[v->namelen];
            *index2 = name[v->namelen+1];
          }
          return 0;
        }
      else
        {
          /* set the user's oid to be ours */
          pal_mem_cpy (name, v->name, ((int) v->namelen) * sizeof (oid));
          return 0;
        }
    }
}

/* Utility function to set the two INTEGER indices. */
void
l2_snmp_integer_index_set (struct variable *v, oid * name, size_t * length,
                    u_int32_t index1, u_int32_t index2)
{
  name[v->namelen] = index1;
  name[v->namelen+1] = index2;
  *length = v->namelen + 2;
}

/* Utility function to get FIVE integer indices.  */
int
l2_snmp_integer_index5_get (struct variable *v, oid *name, size_t *length,
                            u_int32_t *index1, u_int32_t *index2,
                            u_int32_t *index3, u_int32_t *index4,
                            u_int32_t *index5, int exact)
{
  int result, len;

  *index1 = 0;
  *index2 = 0;
  *index3 = 0;
  *index4 = 0;
  *index5 = 0;

  result = oid_compare (name, v->namelen, v->name, v->namelen);

  if (exact)
    {
      /* Check the length. */
      if (result != 0 || *length - v->namelen != 5)
        return -1;

      *index1 = name[v->namelen];
      *index2 = name[v->namelen+1];
      *index3 = name[v->namelen+2];
      *index4 = name[v->namelen+3];
      *index5 = name[v->namelen+4];

      return 0;
    }
  else
    {
      /* return -1 if greater than our our table entry */
      if (result > 0)
        return -1;
      else if (result == 0)
        {
          /* use index value if present, otherwise 0. */
          len = *length - v->namelen;
          if (len >= 5) {
              *index1 = name[v->namelen];
              *index2 = name[v->namelen+1];
              *index3 = name[v->namelen+2];
              *index4 = name[v->namelen+3];
              *index5 = name[v->namelen+4];
          }
          return 0;
        }
      else
        {
          /* set the user's oid to be ours */
          pal_mem_cpy (name, v->name, ((int) v->namelen) * sizeof (oid));
          return 0;
        }
    }
}

	/* Utility function to get FOUR integer indices.  */
int
l2_snmp_integer_index4_get (struct variable *v, oid *name, size_t *length,
                              u_int32_t *index1, u_int32_t *index2, u_int32_t *index3,
                              u_int32_t *index4, int exact)
{
  int result, len;

  *index1 = 0;
  *index2 = 0;
  *index3 = 0;
  *index4 = 0;
  result = oid_compare (name, v->namelen, v->name, v->namelen);

  if (exact)
    {
      /* Check the length. */
      if (result != 0 || *length - v->namelen != 4)
        return -1;
      *index1 = name[v->namelen];
      *index2 = name[v->namelen+1];
      *index3 = name[v->namelen+2];
      *index4 = name[v->namelen+3];
      return 0;
    }
  else
    {
      /* return -1 if greater than our our table entry */
      if (result > 0)
        return -1;
      else if (result == 0)
        {
          /* use index value if present, otherwise 0. */
          len = *length - v->namelen;
          if (len >= 4) {
              *index1 = name[v->namelen];
              *index2 = name[v->namelen+1];
              *index3 = name[v->namelen+2];
              *index4 = name[v->namelen+3];

          }
          return 0;
        }
      else
        {
          /* set the user's oid to be ours */
          pal_mem_cpy (name, v->name, ((int) v->namelen) * sizeof (oid));
          return 0;
        }
    }
}

/* Utility function to get THREE integer indices.  */
int
l2_snmp_integer_index3_get (struct variable *v, oid *name, size_t *length,
    u_int32_t *index1, u_int32_t *index2,
    u_int32_t *index3, int exact)
{
  int result, len;

  *index1 = 0;
  *index2 = 0;
  *index3 = 0;

  result = oid_compare (name, v->namelen, v->name, v->namelen);

  if (exact)
    {
      /* Check the length. */
      if (result != 0 || *length - v->namelen != 3)
        return -1;

      *index1 = name[v->namelen];
      *index2 = name[v->namelen+1];
      *index3 = name[v->namelen+2];
      return 0;
    }
  else
    {
      /* return -1 if greater than our our table entry */
      if (result > 0)
        return -1;
      else if (result == 0)
        {
          /* use index value if present, otherwise 0. */
          len = *length - v->namelen;
          if (len >= 3)
            {
              *index1 = name[v->namelen];
              *index2 = name[v->namelen+1];
              *index3 = name[v->namelen+2];
            }
          return 0;
        }
      else
        {
          /* set the user's oid to be ours */
          pal_mem_cpy (name, v->name, ((int) v->namelen) * sizeof (oid));
          return 0;
        }
    }
}

/* API for setting three INTEGER indices. */
void
l2_snmp_integer_index3_set (struct variable *v, oid * name, size_t * length,
    u_int32_t index1, u_int32_t index2, u_int32_t index3)
{
  name[v->namelen] = index1;
  name[v->namelen+1] = index2;
  name[v->namelen+2] = index3;
  *length = v->namelen + 3;
}

/* API for setting four INTEGER indices. */
void
l2_snmp_integer_index4_set (struct variable *v, oid * name, size_t * length,
    u_int32_t index1, u_int32_t index2,
    u_int32_t index3, u_int32_t index4)
{
  name[v->namelen] = index1;
  name[v->namelen+1] = index2;
  name[v->namelen+2] = index3;
  name[v->namelen+3] = index4;
  *length = v->namelen + 4;
}

/* API for setting five INTEGER indices. */
void
l2_snmp_integer_index5_set (struct variable *v, oid * name, size_t * length,
                            u_int32_t index1, u_int32_t index2,
                            u_int32_t index3, u_int32_t index4,
                            u_int32_t index5)

{
  name[v->namelen] = index1;
  name[v->namelen+1] = index2;
  name[v->namelen+2] = index3;
  name[v->namelen+3] = index4;
  name[v->namelen+4] = index5;
  *length = v->namelen + 5;

}

/* Utility function to get a Integer index + MAC address */
int
l2_snmp_int_mac_addr_index_get (struct variable *v, oid *name, size_t *length,
                    int *idx,struct mac_addr *addr, int exact)
{
  s_int32_t maclen, len;
  u_int8_t val = 0;
  if (exact)
    {
      /* Check the length. */
      if (*length - v->namelen != sizeof (struct mac_addr) + 1)
        return -1;
      *idx = name[v->namelen];
       
      if ( oid2bytes (name + v->namelen + 1, sizeof (struct mac_addr), addr) != 0 )
        return -1;
      return 0;
    }
  else
    {
      pal_mem_set( addr, 0, sizeof(struct mac_addr) );
      *idx = 0;

      len = *length - v->namelen;
      if ( len > 0)   
        *idx = name[v->namelen];
 
      maclen = *length - (v->namelen + 1);

      val = sizeof (struct mac_addr);

      if (maclen > val)
        maclen = sizeof (struct mac_addr);

      if ( oid2bytes (name + v->namelen + 1, maclen, addr) != 0 )
        return -1;

      return 0;
    }

  return -1;
}

/* Utility function to set a Integer Index + MAC address.  */
void
l2_snmp_int_mac_addr_index_set (struct variable *v, oid *name, size_t *length,
                   int idx, struct mac_addr *addr )
{
  oid_copy (name, v->name, v->namelen * sizeof(oid) );

  name[v->namelen] = idx;
  *length = v->namelen + 1;

  oid_copy_bytes2oid (name + v->namelen + 1, addr, sizeof (struct mac_addr));
  *length += sizeof (struct mac_addr);

}

/* Utility function to get a Integer index + MAC address + Integer Index*/

int
l2_snmp_int_mac_addr_int_index_get (struct variable *v, oid *name, 
                                    size_t *length,int *idx1,
                                    struct mac_addr *addr,int *idx2, int exact)
{
  s_int32_t maclen, len;
  u_int8_t val = 0;

  if (exact)
    {
      /* To Check the length of the OID.Size will be the size of 
       * Mac Address and the two integer indices we pass
       */
      if (*length - v->namelen != sizeof (struct mac_addr) + 2)
        return -1;
      *idx1 = name[v->namelen];

      if ( oid2bytes (name + v->namelen + 1, sizeof (struct mac_addr), addr) != 0 )
        return -1;
     *idx2 = name[v->namelen + sizeof (struct mac_addr) + 1 ];
      return 0;
   }
  else
    {
      pal_mem_set( addr, 0, sizeof(struct mac_addr) );
      *idx1 = 0;

      len = *length - v->namelen;
      if ( len > 0)
        *idx1 = name[v->namelen];

      maclen = *length - (v->namelen + 1);

      val = sizeof (struct mac_addr);

      if (maclen > val)
        maclen = sizeof (struct mac_addr);

      if ( oid2bytes (name + v->namelen + 1, maclen, addr) != 0 )
        return -1;

      len = len - maclen;
      if ( len > 0 )
        /*Adding the Length of MAC address and first integer index*/
        *idx2 = name[v->namelen + sizeof (struct mac_addr) + 1 ];

      return 0;
    }
 return -1;
}

/* Utility function to set a Integer Index + MAC address + Integer Index.  */
void
l2_snmp_int_mac_addr_int_index_set (struct variable *v, oid *name, size_t *length,
                   int idx1, struct mac_addr *addr ,int idx2)
{
  oid_copy (name, v->name, v->namelen * sizeof(oid) );

  name[v->namelen] = idx1;
  *length = v->namelen + 1;

  /* copying the size of MAC address and the first integer index*/
  oid_copy_bytes2oid (name + v->namelen + 1, addr, sizeof (struct mac_addr));
  *length += sizeof (struct mac_addr);

  name[v->namelen + 1 + sizeof (struct mac_addr)] = idx2;
  *length += 1;

}
char *
bitstring_init( char *bstring, char fillvalue, int bitstring_length )
{
  pal_mem_set( bstring, fillvalue, bitstring_length );

  return( bstring );
}

int
bitstring_testbit(char *bstring, int number)
{
        bstring += number / 8;
        return ((*bstring & bitstring_mask[(number % 8)]));
}

void
bitstring_setbit(char *bstring, int number)
{
        bstring += number / 8;
        *bstring |= bitstring_mask[(number % 8)];
}

void
bitstring_clearbit(char *bstring, int number)
{
        bstring += number / 8;
        *bstring &= ~bitstring_mask[(number % 8)];
}

int
bitstring_count(char *bstring, int bstring_length)
{
  int i;
  char c;
  int n = 0;
 
  for (i=0; i<bstring_length; i++)
  {
    c = *bstring++;
    if (c)
      do
        n++;
      while((c = c & (c-1)));
  }
        
  return( n );
}

int
l2_snmp_frame_type_proto_val_index_get (struct variable *v, oid *name,
                                        size_t *length, char *proto_value,
                                        int *frame_type, int exact)
{
  int len = L2_SNMP_FALSE;

  if (exact)
    {
      *frame_type = name[v->namelen];
      if (*frame_type < L2_SNMP_FRAME_TYPE_MIN ||
          *frame_type > L2_SNMP_FRAME_TYPE_MAX)
        return L2_SNMP_FALSE;                     

      if (oid2bytes (name + (v->namelen + 1), L2_SNMP_PROTO_VAL_LEN,
                     proto_value) != 0)
        return L2_SNMP_FALSE;
    }
  else
    {
      len = *length - v->namelen;
      pal_mem_set(proto_value, 0, L2_SNMP_PROTO_VAL_LEN);
      *frame_type = 0;

      if (len < 1)
        return 0;

      *frame_type = name[v->namelen];

      if (*frame_type > L2_SNMP_FRAME_TYPE_MAX)            
        return L2_SNMP_FALSE;

      if (oid2bytes (name + (v->namelen + 1), L2_SNMP_PROTO_VAL_LEN, 
                     proto_value) != 0 )
        return L2_SNMP_FALSE;
    }

  return 0;
}


void
l2_snmp_frame_type_proto_val_index_set (struct variable *v, oid *name, 
                                        size_t *length, char *value,
                                        int frame_type)
{

  oid_copy (name, v->name, v->namelen * sizeof(oid));

  name[v->namelen] = frame_type;
  *length = v->namelen + 1;

  /* copying the size of protcol_value and the first integer index*/
  oid_copy_bytes2oid (name + v->namelen + 1, value, L2_SNMP_PROTO_VAL_LEN);	
  *length += L2_SNMP_PROTO_VAL_LEN;
}


/* comparison function for sorting ports in the L2 port vlan table. Index
 * is the PORT.  DESCENDING ORDER 
 */
int 
l2_snmp_port_descend_cmp(const void *e1, const void *e2)
{
  int *vp1;
  int *vp2;

  if (e1 == e2)
    return 0;

  vp1 = *(int **)e1;
  vp2 = *(int **)e2;

  if (vp1 == 0)
    return 1;
  if (vp2 == 0)
    return -1;

  if ( *vp1 < *vp2 )
      return 1;
  else 
    if ( *vp1 > *vp2 )
      return -1;
    else
      return 0;
}

/* comparison function for sorting ports in the L2 port vlan table. Index
 * is the PORT.  ASCENDING ORDER 
 */
int 
l2_snmp_port_ascend_cmp(const void *e1, const void *e2)
{
  int *vp1;  
  int *vp2;

  if (e1 == e2)
    return 0;

  vp1 = *(int **)e1;
  vp2 = *(int **)e2;

  if (vp1 == 0)
    return 1;
  if (vp2 == 0)
    return -1;

  if ( *vp1 < *vp2 )
      return -1;
  else 
    if ( *vp1 > *vp2 )
      return 1;
    else
      return 0;
}

/* comparison function for sorting VLANs in the L2 port vlan table. Index
 * is the PORT.  DESCENDING ORDER 
 */
int 
l2_snmp_vlan_descend_cmp(const void *e1, const void *e2)
{
  struct vlan_table *vp1 = *(struct vlan_table **)e1;
  struct vlan_table *vp2 = *(struct vlan_table **)e2;

  /* if pointer is the same data is the same */
  if (vp1 == vp2)
    return 0;
  if (vp1 == 0)
    return 1;
  if (vp2 == 0)
    return -1;

  if ( vp1->vlanid < vp2->vlanid )
      return 1;
  else 
    if ( vp1->vlanid > vp2->vlanid )
      return -1;
    else
      return 0;
}

/* comparison function for sorting VLANs in the L2 port vlan table. Index
 * is the PORT.  ASCENDING ORDER 
 */
int 
l2_snmp_vlan_ascend_cmp(const void *e1, const void *e2)
{
  struct vlan_table *vp1 = *(struct vlan_table **)e1;
  struct vlan_table *vp2 = *(struct vlan_table **)e2;
  
  /* if pointer is the same data is the same */
  if (vp1 == vp2)
    return 0;
  if (vp1 == 0)
    return 1;
  if (vp2 == 0)
    return -1;

  if ( vp1->vlanid < vp2->vlanid )
      return -1;
  else 
    if ( vp1->vlanid > vp2->vlanid )
      return 1;
    else
      return 0;
}

float64_t
time_ticks (pal_time_t time1, pal_time_t time0)
{
        pal_time_t      delta;
        pal_time_t      hibit;

        if (sizeof (pal_time_t) < sizeof(float64_t))
                return (float64_t) time1 - (double) time0;
        if (time1 < time0)
                return -time_ticks (time0, time1);
        /*
        ** As much as possible, avoid loss of precision
        ** by computing the difference before converting to float64_t.
        */
        delta = time1 - time0;
        if (delta >= 0)
                return delta;
        /*
        ** Repair delta overflow.
        */
        hibit = (~ (pal_time_t) 0) << ((sizeof (pal_time_t)*8) - 1);
        return (delta - 2 * (float64_t) hibit)/100;
}

#endif /* HAVE_SNMP */

