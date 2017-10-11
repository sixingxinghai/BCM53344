/* Copyright (C) 2001 - 2004  All Rights Reserved. */

#ifndef _PACOS_L2_SNMP_H
#define _PACOS_L2_SNMP_H

#include "pal.h"
#include "lib.h"
#include "l2lib.h"

struct vlan_table {
  int vlanid;
};

#define L2_SNMP_PROTO_VAL_LEN               2
#define L2_SNMP_FRAME_TYPE_MIN              0
#define L2_SNMP_FRAME_TYPE_MAX              5
#define APN_CLASS_FRAME_TYPE_ETHER          1
#define APN_CLASS_FRAME_TYPE_SNAPOTHER      4
#define APN_CLASS_FRAME_TYPE_LLCOTHER       5
#define L2_SNMP_FALSE                      -1

void oid_copy_bytes2oid (oid oid[], void *addr, s_int32_t len);
int oid2bytes (oid oid[], s_int32_t len, void *addr);
int l2_snmp_index_get (struct variable *v, oid * name, size_t * length,
                    u_int32_t * index, int exact);
int l2_snmp_port_vlan_index_get (struct variable *v, oid * name, size_t * length,
                    u_int32_t * port, u_int32_t * vlan, int exact);
void l2_snmp_index_set (struct variable *v, oid * name, size_t * length,
                    u_int32_t index);
void l2_snmp_port_vlan_index_set (struct variable *v, oid * name, size_t * length,
                    u_int32_t port, u_int32_t vlan);
int l2_snmp_mac_addr_index_get (struct variable *v, oid *name, size_t *length, 
                    struct mac_addr *addr, int exact);
void l2_snmp_mac_addr_index_set (struct variable *v, oid *name, size_t *length,
                    struct mac_addr *addr);
int l2_snmp_mac_addr_int_index_get (struct variable *v, oid *name, size_t *length, 
                    struct mac_addr *addr, int *idx, int exact);
void l2_snmp_mac_addr_int_index_set (struct variable *v, oid *name, size_t *length,
                    struct mac_addr *addr, int idx );
int
l2_snmp_integer_index_get (struct variable *v, oid *name, size_t *length,
                             u_int32_t *index1, u_int32_t *index2, int exact);

void
l2_snmp_integer_index_set (struct variable *v, oid * name, size_t * length,
                    u_int32_t index1, u_int32_t index2);

int
l2_snmp_integer_index3_get (struct variable *v, oid *name, size_t *length,
                            u_int32_t *index1, u_int32_t *index2,
                            u_int32_t *index3, int exact);
void
l2_snmp_integer_index3_set (struct variable *v, oid * name, size_t * length,
                            u_int32_t index1, u_int32_t index2,
                            u_int32_t index3);
int
l2_snmp_integer_index4_get (struct variable *v, oid *name, size_t *length,
                            u_int32_t *index1, u_int32_t *index2,
                            u_int32_t *index3, u_int32_t *index4, int exact);
void
l2_snmp_integer_index4_set (struct variable *v, oid * name, size_t * length,
                            u_int32_t index1, u_int32_t index2,
                            u_int32_t index3, u_int32_t index4);
/* API for setting five INTEGER indices. */
void
l2_snmp_integer_index5_set (struct variable *v, oid * name, size_t * length,
                            u_int32_t index1, u_int32_t index2,
                            u_int32_t index3, u_int32_t index4,
                            u_int32_t index5);


/* Utility function to get FIVE integer indices.  */
int
l2_snmp_integer_index5_get (struct variable *v, oid *name, size_t *length,
                            u_int32_t *index1, u_int32_t *index2,
                            u_int32_t *index3, u_int32_t *index4,
                            u_int32_t *index5, int exact);


void
l2_snmp_int_mac_addr_int_index_set (struct variable *v, oid *name, size_t *length,
                   int idx1, struct mac_addr *addr ,int idx2);

int
l2_snmp_int_mac_addr_int_index_get (struct variable *v, oid *name, size_t *length,
                    int *idx1,struct mac_addr *addr,int *idx2, int exact);
void
l2_snmp_frame_type_proto_val_index_set(struct variable *v, oid *name, 
                                       size_t *length, char *value,
                                       int frame_type);
int
l2_snmp_frame_type_proto_val_index_get(struct variable *v, oid *name, 
                                       size_t *length, char 
                                       *proto_value, int *frame_type,
                                       int exact);

int 
l2_snmp_vlan_descend_cmp(const void *e1, const void *e2);
int 
l2_snmp_vlan_ascend_cmp(const void *e1, const void *e2);
int 
l2_snmp_port_descend_cmp(const void *e1, const void *e2);
int 
l2_snmp_port_ascend_cmp(const void *e1, const void *e2);
float64_t
time_ticks (pal_time_t time1, pal_time_t time0);

/* bit string utilities. SNMP bit strings start at the MSBit and move to the 
 * right for increasing bit numbers. There must enough bytes in the string to
 * contain each multiple of 8 bits.
 */

#define BITSTRINGSIZE(n) (((n)+7)/8)

char *bitstring_init(char *bstring, char value, int bstring_length);
int bitstring_testbit(char *bstring, int number);
void bitstring_setbit(char *bstring, int number);
void bitstring_clearbit(char *bstring, int number);
int bitstring_count(char *bstring, int bstring_length);
void *
l2_bsearch_next(const void *key, const void *base0, size_t nelements,
             size_t element_size, int (*compar)(const void *, const void *));
int
l2_snmp_int_mac_addr_index_get (struct variable *v, oid *name, size_t *length,
                    int *idx,struct mac_addr *addr, int exact);
void
l2_snmp_int_mac_addr_index_set (struct variable *v, oid *name, size_t *length,
                   int idx, struct mac_addr *addr );


#endif /* _PACOS_L2_SNMP_H */



