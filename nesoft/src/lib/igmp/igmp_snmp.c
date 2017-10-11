/* Copyright (C) 2003  All Rights Reserved. */

#include <igmp_incl.h>
#ifdef HAVE_SNMP


/* Register IGMP MIB.*/
s_int32_t
igmp_snmp_init (struct igmp_instance *igi)
{
  struct lib_globals *lg = IGMP_LG (igi);
  s_int32_t ret;

  /* IGMP MIB instances. */
  oid igmp_oid [] = { IGMPMIB };

  struct variable igmp_var[] =
  {
    {IGMPROUTERIFINDEX,                ASN_INTEGER,   NOACCESS,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 1},    lg},

    {IGMPROUTERIFQTYPE,                ASN_INTEGER,   NOACCESS,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 2},    lg},

    {IGMPROUTERIFQUERIER,              ASN_IPADDRESS, RONLY,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 3},    lg},

    {IGMPROUTERIFQINTERVAL,            ASN_INTEGER,   RWRITE,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 4},    lg},

    {IGMPROUTERIFSTATUS,               ASN_INTEGER,   RWRITE,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 5},    lg},

    {IGMPROUTERIFVERSION,              ASN_INTEGER,   RWRITE,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 6},    lg},

    {IGMPROUTERIFQMAXRESTIME,          ASN_INTEGER,   RWRITE,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 7},    lg},

    {IGMPROUTERIFQUPTIME,              ASN_TIMETICKS, RONLY,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 8},    lg},

    {IGMPROUTERIFQEXPIRYTIME,          ASN_TIMETICKS, RONLY,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 9},    lg},

    {IGMPROUTERIFQWRONGVERQUERY,       ASN_INTEGER,   RONLY,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 10},   lg},

    {IGMPROUTERIFJOINS,                ASN_INTEGER,   RONLY,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 11},   lg},

    {IGMPROUTERIFPROXYIFINDEX,         ASN_INTEGER,   RWRITE,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 12},   lg},

    {IGMPROUTERIFGROUPS,               ASN_INTEGER,   RONLY,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 13},   lg},

    {IGMPROUTERIFROBUSTNESS,           ASN_INTEGER,   RWRITE,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 14},   lg},

    {IGMPROUTERIFLASTMEMQINTVL,        ASN_INTEGER,   RWRITE,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 15},   lg},

    {IGMPROUTERIFLASTMEMQCOUNT,        ASN_INTEGER,   RONLY,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 16},   lg},

    {IGMPROUTERIFSTARTUPQCOUNT,        ASN_INTEGER,   RWRITE,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 17},   lg},

    {IGMPROUTERIFSTARTUPQINTVL,        ASN_INTEGER,   RWRITE,
     igmp_snmp_rtr_if_tab,             4, {1, 4, 1, 18},   lg},

    {IGMPROUTERCACHEADDRESSTYPE,       ASN_INTEGER,   NOACCESS,
     igmp_snmp_rtr_cache_tab,          4, {1, 6, 1, 1},    lg},

    {IGMPROUTERCACHEADDRESS,           ASN_IPADDRESS, NOACCESS,
     igmp_snmp_rtr_cache_tab,          4, {1, 6, 1, 2},    lg},

    {IGMPROUTERCACHEIFINDEX,           ASN_INTEGER,   NOACCESS,
     igmp_snmp_rtr_cache_tab,          4, {1, 6, 1, 3},    lg},

    {IGMPROUTERCACHELASTREPORTER,      ASN_IPADDRESS, RONLY,
     igmp_snmp_rtr_cache_tab,          4, {1, 6, 1, 4},    lg},

    {IGMPROUTERCACHEUPTIME,            ASN_TIMETICKS, RONLY,
     igmp_snmp_rtr_cache_tab,          4, {1, 6, 1, 5},    lg},

    {IGMPROUTERCACHEEXPIRYTIME,        ASN_TIMETICKS, RONLY,
     igmp_snmp_rtr_cache_tab,          4, {1, 6, 1, 6},    lg},

    {IGMPROUTERCACHEEXCLMODEEXPTIMER,  ASN_TIMETICKS, RONLY,
     igmp_snmp_rtr_cache_tab,          4, {1, 6, 1, 7},    lg},

    {IGMPROUTERCACHEVER1HOSTTIMER,     ASN_TIMETICKS, RONLY,
     igmp_snmp_rtr_cache_tab,          4, {1, 6, 1, 8},    lg},

    {IGMPROUTERCACHEVER2HOSTTIMER,     ASN_TIMETICKS, RONLY,
     igmp_snmp_rtr_cache_tab,          4, {1, 6, 1, 9},    lg},

    {IGMPROUTERCACHESRCFILTERMODE,     ASN_INTEGER,   RONLY,
     igmp_snmp_rtr_cache_tab,          4, {1, 6, 1, 10},   lg},

    {IGMPINVERSEROUTERCACHEIFINDEX,    ASN_INTEGER,   NOACCESS,
     igmp_snmp_inv_rtr_cache_tab,      4, {1, 8, 1, 1},    lg},

    {IGMPINVERSEROUTERCACHEADDRTYPE,   ASN_INTEGER,   NOACCESS,
     igmp_snmp_inv_rtr_cache_tab,      4, {1, 8, 1, 2},    lg},

    {IGMPINVERSEROUTERCACHEADDR,       ASN_IPADDRESS, RONLY,
     igmp_snmp_inv_rtr_cache_tab,      4, {1, 8, 1, 3},    lg},

    {IGMPROUTERSRCLISTADDRTYPE,        ASN_INTEGER,   NOACCESS,
     igmp_snmp_rtr_src_list_tab,       4, {1, 10, 1, 1},   lg},

    {IGMPROUTERSRCLISTADDR,            ASN_IPADDRESS, NOACCESS,
     igmp_snmp_rtr_src_list_tab,       4, {1, 10, 1, 2},   lg},

    {IGMPROUTERSRCLISTIFINDEX,         ASN_INTEGER,   NOACCESS,
     igmp_snmp_rtr_src_list_tab,       4, {1, 10, 1, 3},   lg},

    {IGMPROUTERSRCLISTHOSTADDRESS,     ASN_IPADDRESS, RONLY,
     igmp_snmp_rtr_src_list_tab,       4, {1, 10, 1, 4},   lg},

    {IGMPROUTERSRCLISTEXPIRE,          ASN_TIMETICKS, RONLY,
     igmp_snmp_rtr_src_list_tab,       4, {1, 10, 1, 5},   lg}
  };

  IGMP_FN_ENTER (Igmp_snmp init);

  ret = IGMP_ERR_NONE;
  igi->igi_mib_vars = XCALLOC (MTYPE_IGMP_SNMP_VAR, sizeof (igmp_var));
  pal_mem_cpy (igi->igi_mib_vars, igmp_var, sizeof (igmp_var));

  REGISTER_MIB2 (IGMP_LG (igi), "mibII/igmp", igi->igi_mib_vars, variable,
                 igmp_oid, (sizeof (igmp_var)) / (sizeof (struct variable)));

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_snmp_deinit (struct igmp_instance *igi)
{
#ifdef HAVE_AGENTX
  oid igmpd_oid [] = { IGMPDOID };
#endif /* HAVE_AGENTX */
  s_int32_t ret;

  IGMP_FN_ENTER (Igmp_snmp deinit);

  ret = IGMP_ERR_NONE;

#ifdef HAVE_AGENTX
  UNREGISTER_MIB2 (IGMP_LG (igi), igmpd_oid,
                   sizeof (igmpd_oid) / sizeof (oid));
#endif /* HAVE_AGENTX */

  XFREE (MTYPE_IGMP_SNMP_VAR, igi->igi_mib_vars);

  IGMP_FN_EXIT (ret);
}

/*  Utility function to get igmp_snmp_rtr_if_tab index.  */
u_int32_t
igmp_snmp_rtr_if_index_get (struct variable *v, oid *name,
                            pal_size_t *length,
                            struct igmp_snmp_rtr_if_index *index,
                            bool_t exact)
{
  u_int32_t oidlen_tmp;
  s_int32_t ret;
  oid *poid;

  IGMP_FN_ENTER (IgmpRtrIfaceIdxGet);

  ret = IGMP_ERR_NONE;

  oidlen_tmp = *length - v->namelen;
  poid = name + v->namelen;

  if (! index)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (exact && oidlen_tmp < IGMP_SNMP_OIDIDX_LEN_RTRIFTABENT)
    {
       ret = IGMP_ERR_API_GET;
       goto EXIT;
    }

  index->len = oidlen_tmp;

  if (oidlen_tmp)
    {
      index->ifindex = poid [0];
      oidlen_tmp -= 1;
    }

  if (oidlen_tmp)
    {
      index->qtype = poid [1];
      oidlen_tmp -= 1;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/*  Utility function to get igmp_snmp_rtr_cache_tab Index.  */
u_int32_t
igmp_snmp_rtr_cache_index_get (struct variable *v, oid *name,
                               pal_size_t *length,
                               struct igmp_snmp_rtr_cache_index *index,
                               bool_t exact)
{
  u_int32_t oidlen_tmp;
  s_int32_t ret;
  s_int32_t i;
  oid *poid;

  IGMP_FN_ENTER (IgmpRtrCacheIdxGet);

  ret = IGMP_ERR_NONE;
  oidlen_tmp = *length - v->namelen;
  poid = name + v->namelen;
  i = 0;

  if (! index)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (exact && oidlen_tmp < IGMP_SNMP_OIDIDX_LEN_RTRCACHETABENT)
    {
       ret = IGMP_ERR_API_GET;
       goto EXIT;
    }

  index->len = oidlen_tmp;

  if (oidlen_tmp)
    {
      index->qtype = poid [i++];
      oidlen_tmp -= 1;
    }
  else
    goto EXIT;
 
  if (oidlen_tmp >= IN_ADDR_SIZE)
    {
      oid2in_addr (poid + i, IN_ADDR_SIZE, &index->cache_addr);
      oidlen_tmp -= IN_ADDR_SIZE;
    }
  else if (oidlen_tmp > 0)
    {
      oid2in_addr (poid + i, oidlen_tmp, &index->cache_addr);
      goto EXIT;
    }
  else
    goto EXIT;

  if (oidlen_tmp)
    {
      index->ifindex = poid [index->len-1];
      oidlen_tmp -= 1;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/*  Utility function to get igmp_snmp_inv_rtr_cache_tab index.  */
u_int32_t
igmp_snmp_rtr_inv_cache_index_get (struct variable *v, oid *name,
                                   pal_size_t *length,
                                   struct igmp_snmp_inv_rtr_index *index,
                                   bool_t exact)
{
  u_int32_t oidlen_tmp;
  s_int32_t ret;
  oid *poid;

  IGMP_FN_ENTER (IgmpRtrInvCacheIdxGet);

  ret = IGMP_ERR_NONE;

  oidlen_tmp = *length - v->namelen;
  poid = name + v->namelen;

  if (! index)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (exact && oidlen_tmp < IGMP_SNMP_OIDIDX_LEN_INVRTRCACHETABENT)
    {
       ret = IGMP_ERR_API_GET;
       goto EXIT;
    }

  index->len = oidlen_tmp;
  index->qtype = 1;

  if ( oidlen_tmp)
    {
      index->ifindex = poid [0];
      oidlen_tmp -= 1;
    }
  else
    goto EXIT;

  if (oidlen_tmp)
    {
      index->qtype = poid [1];
      oidlen_tmp -= 1;
    }
  else
    goto EXIT;

  if (oidlen_tmp >= IN_ADDR_SIZE)
    oid2in_addr (poid + 2, IN_ADDR_SIZE, &index->cache_addr);
  else if (oidlen_tmp > 0)
    oid2in_addr (poid + 2, oidlen_tmp, &index->cache_addr);

EXIT:

  IGMP_FN_EXIT (ret);
}

/*  Utility function to get igmp_snmp_rtr_src_list_tab index.  */
u_int32_t
igmp_snmp_rtr_srclist_index_get (struct variable *v, oid *name,
                                 pal_size_t *length,
                                 struct igmp_snmp_rtr_src_list_index *index,
                                 bool_t exact)
{
  u_int32_t oidlen_tmp;
  s_int32_t ret;
  s_int32_t i;
  oid *poid;

  IGMP_FN_ENTER (IgmpRtrSrclistIdxGet);

  ret = IGMP_ERR_NONE;
  oidlen_tmp = *length - v->namelen;
  poid = name + v->namelen;
  i = 0;

  if (! index)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (exact && oidlen_tmp < IGMP_SNMP_OIDIDX_LEN_RTRSRCLSTTABENT)
    {
       ret = IGMP_ERR_API_GET;
       goto EXIT;
    }

  index->len = oidlen_tmp;

  if (oidlen_tmp)
    {
      index->qtype = poid [i++];
      oidlen_tmp -= 1;
    }
  else
    goto EXIT;

  if (oidlen_tmp >= IN_ADDR_SIZE)
    {
      oid2in_addr (poid + i, IN_ADDR_SIZE, &index->group_addr);
      i += IN_ADDR_SIZE;
      oidlen_tmp -= IN_ADDR_SIZE;
    }
  else if (oidlen_tmp > 0)
    {
      oid2in_addr (poid + i, oidlen_tmp, &index->group_addr);
      goto EXIT; 
    }
  else
    goto EXIT;
 
  if (oidlen_tmp)
    {
      index->ifindex = poid [i++];
      oidlen_tmp -= 1;
    }
  else
   goto EXIT;

  if (oidlen_tmp >= IN_ADDR_SIZE)
    oid2in_addr (poid + i, IN_ADDR_SIZE, &index->host_addr);
  else if (oidlen_tmp > 0)
    oid2in_addr (poid + i, oidlen_tmp, &index->host_addr);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Utility function to set igmp_snmp_rtr_if_tab index.  */
s_int32_t
igmp_snmp_rtr_if_index_set (struct variable *v, oid *name,
                            pal_size_t *length,
                            struct interface *ifp)
{
  u_int32_t len;
  s_int32_t ret;
  oid *poid;

  IGMP_FN_ENTER (IgmpRtrIfaceIdxSet);

  ret = IGMP_ERR_NONE;

  poid = name + v->namelen;
  len = IGMP_SNMP_OIDIDX_LEN_RTRIFTABENT;

  if (ifp)
    {
      poid [0] = ifp->ifindex;
      poid [1] = IGMP_SNMP_INETADDRTYPE_IPV4;
    }
  else
    {
      poid [0] = 0;
      poid [1] = 0;
    }

  *length = len + v->namelen;

  IGMP_FN_EXIT (ret);
}

/* Utility function to set igmp_snmp_rtr_cache_tab index.  */
s_int32_t
igmp_snmp_rtr_cache_index_set (struct variable *v, oid *name,
                               pal_size_t *length,
                               struct igmp_snmp_rtr_cache_index *index,
                               struct interface *ifp)
{
  s_int32_t ret;
  s_int32_t i;
  oid *poid;

  IGMP_FN_ENTER (IgmpRtrCacheIdxSet);

  ret = IGMP_ERR_NONE;
  poid = name + v->namelen;
  i = 0;

  if (index)
    {
      index->len = IGMP_SNMP_OIDIDX_LEN_RTRCACHETABENT;

      poid [i++] = index->qtype;
      oid_copy_addr (poid + i, &index->cache_addr, IN_ADDR_SIZE);
      poid [index->len-1] = index->ifindex;

      *length = index->len + v->namelen;
    }
  else
    {
      for (i = 0; i < IGMP_SNMP_OIDIDX_LEN_RTRCACHETABENT; i ++)
        poid [i] = 0;
    }

  IGMP_FN_EXIT (ret);
}

/* Utility function to set igmp_snmp_inv_rtr_cache_tab index.  */
s_int32_t
igmp_snmp_rtr_inv_cache_index_set (struct variable *v, oid *name,
                                   pal_size_t *length,
                                   struct igmp_snmp_inv_rtr_index *index,
                                   struct interface *ifp)
{
  s_int32_t ret;
  s_int32_t i;
  oid *poid;

  IGMP_FN_ENTER (IgmpRtrInvCacheIdxSet);

  poid = name + v->namelen;
  ret = IGMP_ERR_NONE;
  i = 0;

  if (index)
    {
      index->len = IGMP_SNMP_OIDIDX_LEN_INVRTRCACHETABENT;

      poid [0] = index->ifindex;
      poid [1] = index->qtype;
      oid_copy_addr (poid + 2, &index->cache_addr, IN_ADDR_SIZE);

      *length = index->len + v->namelen;
    }

  else
    {
      for (i = 0; i < IGMP_SNMP_OIDIDX_LEN_INVRTRCACHETABENT; i ++)
        poid [i] = 0;
    }

  IGMP_FN_EXIT (ret);
}

/* Utility function to set igmp_snmp_rtr_src_list_tab index.  */
s_int32_t
igmp_snmp_rtr_srclist_index_set (struct variable *v, oid *name,
                                 pal_size_t *length,
                                 struct igmp_snmp_rtr_src_list_index *index,
                                 struct interface *ifp)
{
  u_int32_t ifindex;
  u_int8_t *c_pnt;
  s_int32_t ret;
  s_int32_t i;
  oid *poid;

  IGMP_FN_ENTER (IgmpRtrSrclistIdxSet);

  poid = name + v->namelen;
  ret = IGMP_ERR_NONE;
  ifindex = 0;

  index->len = IGMP_SNMP_OIDIDX_LEN_RTRSRCLSTTABENT;
  index->qtype = IGMP_SNMP_INETADDRTYPE_IPV4;

  if (ifp)
    ifindex = ifp->ifindex;

  c_pnt = (u_int8_t *) &(index->qtype);

  for (i=0; i < index->len; i++)
    poid [i] = *c_pnt++;

  oid_copy_addr (poid + 1, &index->group_addr, IN_ADDR_SIZE);
  poid [IN_ADDR_SIZE + 1] = ifindex;

  oid_copy_addr (poid + 2 + IN_ADDR_SIZE, &index->host_addr, IN_ADDR_SIZE);

  *length = index->len + v->namelen;

  IGMP_FN_EXIT (ret);
}

/* Write method for igmp_snmp_rtr_if_tab.  */
s_int32_t
write_igmp_snmp_rtr_if_tab (s_int32_t action, u_char *var_val,
                            u_char var_val_type, pal_size_t var_val_len,
                            u_char *statP, oid *name, pal_size_t length,
                            struct variable *v, u_int32_t vr_id)
{
  struct igmp_snmp_rtr_if_index index;
  struct igmp_instance *igi;
  struct interface *pxy_ifp;
  struct pal_in4_addr addr;
  struct lib_globals *lg;
  struct interface *ifp;
  struct apn_vrf *ivrf;
  u_int8_t  *pxy_name;
  struct apn_vr *vr;
  s_int32_t ret_val;
  u_int32_t intval = 0;
  s_int32_t ret;

  IGMP_FN_ENTER (WriteIgmpRtrIfaceTable);

  ret_val = SNMP_ERR_NOERROR;
  pxy_name = NULL;
  pxy_ifp = NULL;
  lg = v->lg;
  ivrf = NULL;
  igi = NULL;
  ifp = NULL;
  vr = NULL;

  pal_mem_set (&index, 0, sizeof (struct igmp_snmp_rtr_if_index));

  if (var_val_type == ASN_INTEGER)
    pal_mem_cpy (&intval, var_val, 4);

  else if (var_val_type == ASN_IPADDRESS)
    {
      if (var_val_len != IN_ADDR_SIZE)
        {
          ret_val = SNMP_ERR_WRONGLENGTH;
          goto EXIT;
        }

      pal_mem_cpy (&addr.s_addr, var_val, 4);
    }
  else if (var_val_type != ASN_OCTET_STR)
    {
      ret_val = SNMP_ERR_WRONGTYPE;
      goto EXIT;
    }

  ret = igmp_snmp_rtr_if_index_get (v, name, &length, &index, PAL_TRUE);

  if (ret < IGMP_ERR_NONE)
    {
      ret_val = SNMP_ERR_NOSUCHNAME;
      goto EXIT;
    }

  if (index.qtype == IGMP_SNMP_INETADDRTYPE_IPV6)
    {
      ret_val = SNMP_ERR_GENERR;
      goto EXIT;
    }

 /* Get the Default VR */
  vr = apn_vr_lookup_by_id (lg, vr_id);
  if (! vr)
    {
      ret_val = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the Default VRF */
  ivrf = apn_vrf_lookup_default (vr);

  if (! ivrf)
    {
      ret_val = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the Default IGI */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret_val = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ifp = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                             index.ifindex);

  if (! ifp)
    {
      ret_val = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  switch (v->magic)
    {
      case IGMPROUTERIFQINTERVAL:
        if (intval < IGMP_QUERY_INTERVAL_MIN
            || intval > IGMP_QUERY_INTERVAL_MAX)
          {
            ret_val = SNMP_ERR_BADVALUE;
            goto EXIT;
          }

        ret = igmp_if_query_interval_set (igi, ifp, intval);
        if (ret < IGMP_ERR_NONE)
          {
            ret_val = SNMP_ERR_COMMITFAILED;
            goto EXIT;
          }

        break;
 
      case IGMPROUTERIFSTATUS:
        if (intval < IGMP_SNMP_ROW_STATUS_ACTIVE
            || intval > IGMP_SNMP_ROW_STATUS_DESTROY)
          {
            ret_val = SNMP_ERR_BADVALUE;
            goto EXIT;
          }

        ret = igmp_if_status_set (igi, ifp, intval);
        if (ret < IGMP_ERR_NONE)
          {
            ret_val = SNMP_ERR_COMMITFAILED;
            goto EXIT;
          }

        break;

      case IGMPROUTERIFVERSION:
        if (intval < IGMP_VERSION_MIN
            || intval > IGMP_VERSION_MAX)
          {
            ret_val = SNMP_ERR_BADVALUE;
            goto EXIT;
          }

        ret = igmp_if_version_set (igi, ifp, intval);
        if (ret < IGMP_ERR_NONE)
          {
            ret_val = SNMP_ERR_COMMITFAILED;
            goto EXIT;
          }

        break;

      case IGMPROUTERIFQMAXRESTIME:
        intval /= ONE_SEC_TENTHS_OF_SECOND;
        if (intval < IGMP_QUERY_RESPONSE_INTERVAL_MIN
            || intval > IGMP_QUERY_RESPONSE_INTERVAL_MAX)
          {
            ret_val = SNMP_ERR_BADVALUE;
            goto EXIT;
          }

        ret = igmp_if_query_response_interval_set (igi, ifp, intval);
        if (ret < IGMP_ERR_NONE)
          {
            ret_val = SNMP_ERR_COMMITFAILED;
            goto EXIT;
          }

        break;

      case IGMPROUTERIFPROXYIFINDEX:
        if ( intval > 0)
          {
            pxy_ifp = ifv_lookup_by_index 
                      (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf), intval);

            if (pxy_ifp)
              pxy_name = pxy_ifp->name;

            ret = igmp_if_mroute_pxy_set (igi, ifp, pxy_name);
          }
        else if ( intval == 0)
          {
            ret = igmp_if_mroute_pxy_unset (igi, ifp); 
          }
 
        if (ret < IGMP_ERR_NONE)
          {
            ret_val = SNMP_ERR_COMMITFAILED;
            goto EXIT;
          }

        break;

      case IGMPROUTERIFROBUSTNESS:
        if (intval < IGMP_ROBUSTNESS_VAR_MIN
            || intval > IGMP_ROBUSTNESS_VAR_MAX)
          {
            ret_val = SNMP_ERR_BADVALUE;
            goto EXIT;
          }

        ret = igmp_if_robustness_var_set (igi, ifp, intval);
        if (ret < IGMP_ERR_NONE)
          {
            ret_val = SNMP_ERR_COMMITFAILED;
            goto EXIT;
          }

        break;

      case IGMPROUTERIFLASTMEMQINTVL:
        intval *= DECISEC_TO_MILLISEC;
        if (intval < IGMP_LAST_MEMBER_QUERY_INTERVAL_MIN
            || intval > IGMP_LAST_MEMBER_QUERY_INTERVAL_MAX)
          {
            ret_val = SNMP_ERR_BADVALUE;
            goto EXIT;
          }

        ret = igmp_if_lmqi_set (igi, ifp, intval);
        if (ret < IGMP_ERR_NONE)
          {
            ret_val = SNMP_ERR_COMMITFAILED;
            goto EXIT;
          }

        break;

       case IGMPROUTERIFSTARTUPQCOUNT:
        if (intval < IGMP_STARTUP_QUERY_COUNT_MIN 
            || intval > IGMP_STARTUP_QUERY_COUNT_MAX)
          {
            ret_val = SNMP_ERR_BADVALUE;
            goto EXIT;
          }

        ret = igmp_if_startup_query_count_set (igi, ifp, intval);
        if (ret < IGMP_ERR_NONE)
          {
            ret_val = SNMP_ERR_COMMITFAILED;
            goto EXIT;
          }

        break;

       case IGMPROUTERIFSTARTUPQINTVL:
        if (intval < IGMP_QUERY_INTERVAL_MIN
            || intval > IGMP_QUERY_INTERVAL_MAX)
          {
            ret_val = SNMP_ERR_BADVALUE;
            goto EXIT;
          }

        ret = igmp_if_startup_query_interval_set (igi, ifp, intval);
        if (ret < IGMP_ERR_NONE)
          {
            ret_val = SNMP_ERR_COMMITFAILED;
            goto EXIT;
          }

        break;

      default:
        break;
    }

EXIT:

  IGMP_FN_EXIT (ret_val);
}

u_int8_t *
igmp_snmp_rtr_if_tab (struct variable *v, oid *name, pal_size_t *length,
                      bool_t exact, pal_size_t *var_len,
                      WriteMethod **write_method, u_int32_t vr_id)
{
  struct igmp_snmp_rtr_if_index index;
  struct igmp_instance *igi;
  struct lib_globals *lg;
  struct interface *ifp;
  struct apn_vrf *ivrf;
  u_int8_t *ret_var;
  s_int32_t ret_val;
  struct apn_vr *vr;
  s_int32_t ret;

  IGMP_FN_ENTER (IgmpSnmpRtrIfTable);

  ret_val = SNMP_ERR_NOERROR;
  *write_method = NULL;
  ret_var = NULL;
  ivrf = NULL;
  igi = NULL;
  lg = v->lg;
  vr = NULL;

  pal_mem_set (&index, 0, sizeof (struct igmp_snmp_rtr_if_index));

  ret = igmp_snmp_rtr_if_index_get (v, name, length, &index, exact);

  if (ret < IGMP_ERR_NONE)
    {
      ret_var = NULL;
      goto EXIT;
    }

  if (index.qtype == IGMP_SNMP_INETADDRTYPE_IPV6 && exact)
    {
      ret_var = NULL;
      goto EXIT;
    }

   /* Get the Default VR */
  vr = apn_vr_lookup_by_id (lg, vr_id);
  if (! vr)
    {
      ret_var = NULL;
      goto EXIT;
    }

  /* Get the Default VRF */
  ivrf = apn_vrf_lookup_default (vr);

  if (! ivrf)
    {
      ret_var = NULL;
      goto EXIT;
    }

  /* Get the Default IGI */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret_var = NULL;
      goto EXIT;
    }

  ifp = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                             index.ifindex);

  /* Return the current value of the variable */
  switch (v->magic)
    {
      case IGMPROUTERIFINDEX:
      /* 1 -- igmpRouterIfIndex */
        /* Not accessible */
      case IGMPROUTERIFQTYPE:
       /*  2 -- igmpRouterIfQType */
        /* Not accessible */
        break;

      case IGMPROUTERIFQUERIER:
        /* 3 -- igmpRouterIfQuerier */
        IGMP_SNMP_GET (exact, querier, igi, &ifp,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFQINTERVAL:
      /* 4 -- igmpRouterIfQueryInterval */
        *write_method = write_igmp_snmp_rtr_if_tab;
        IGMP_SNMP_GET (exact, query_interval, igi, &ifp,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFSTATUS:
        /* 5 -- igmpRouterIfStatus */
        *write_method = write_igmp_snmp_rtr_if_tab;
        IGMP_SNMP_GET (exact, status, igi, &ifp,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFVERSION:
        /*  6 -- igmpRouterIfVersion */
        *write_method = write_igmp_snmp_rtr_if_tab;
        IGMP_SNMP_GET (exact, version, igi, &ifp,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFQMAXRESTIME:
        /*  7 -- igmpRouterIfQueryMaxResTime */
        *write_method = write_igmp_snmp_rtr_if_tab;
        IGMP_SNMP_GET (exact, query_response_interval, igi, &ifp,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFQUPTIME:
        /*   8 -- igmpRouterIfQueryUpTime */
        IGMP_SNMP_GET (exact, querier_uptime, igi, &ifp,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFQEXPIRYTIME:
      /* 9 -- igmpRouterIfQueryExpiryTime */
        IGMP_SNMP_GET (exact, querier_expiry_time, igi, &ifp,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFQWRONGVERQUERY:
        /* 10 -- igmpRouterIfWrongVersionQueries */
        IGMP_SNMP_GET (exact, wrong_version_queries, igi, &ifp,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFJOINS:
        /* 11 -- igmpRouterIfJoins */
        IGMP_SNMP_GET (exact, joins, igi, &ifp,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFPROXYIFINDEX:
        /* 12 -- igmpRouterIfProxyIfIndex */
        *write_method = write_igmp_snmp_rtr_if_tab;
        IGMP_SNMP_GET (exact, mroute_pxy, igi, &ifp,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFGROUPS:
        /* 13 -- igmpRouterIfGroups */
        IGMP_SNMP_GET (exact, groups, igi, &ifp,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFROBUSTNESS:
        /* 14 -- igmpRouterIfRobustness */
        *write_method = write_igmp_snmp_rtr_if_tab;
        IGMP_SNMP_GET (exact, robustness_var, igi, &ifp,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFLASTMEMQINTVL:
        /* 15 -- igmpRouterIfLastMemberQueryInterval */
        *write_method = write_igmp_snmp_rtr_if_tab;
        IGMP_SNMP_GET (exact, lmqi, igi, &ifp, &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFLASTMEMQCOUNT:
        /* 16 -- igmpRouterIfLastMemberQueryCount */
        IGMP_SNMP_GET (exact, lmqc, igi, &ifp, &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFSTARTUPQCOUNT:
        /* 17 -- igmpRouterIfStartupQueryCount */
        *write_method = write_igmp_snmp_rtr_if_tab;
        IGMP_SNMP_GET (exact, sqc, igi, &ifp, &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      case IGMPROUTERIFSTARTUPQINTVL:
        /* 18 -- igmpRouterIfStartupQueryInterval */
        *write_method = write_igmp_snmp_rtr_if_tab;
        IGMP_SNMP_GET (exact, sqi, igi, &ifp, &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_if_index_set (v, name, length, ifp);
        break;

      default:
        break;
    }

EXIT:

  IGMP_FN_EXIT (ret_var);
}

u_int8_t *
igmp_snmp_rtr_cache_tab (struct variable *v, oid *name, pal_size_t *length,
                         bool_t exact, pal_size_t *var_len,
                         WriteMethod **write_method, u_int32_t vr_id)
{
  struct igmp_snmp_rtr_cache_index index;
  struct igmp_instance *igi;
  struct lib_globals *lg;
  struct interface *ifp;
  struct apn_vrf *ivrf;
  s_int32_t ret_val;
  u_int8_t *ret_var;
  struct apn_vr *vr;
  s_int32_t ret;

  IGMP_FN_ENTER (IgmpSnmpRtrCacheTab);

  ret_val = SNMP_ERR_NOERROR;
  *write_method = NULL;
  ret_var = NULL;
  ivrf = NULL;
  lg = v->lg;
  igi = NULL;
  vr = NULL;
  
  pal_mem_set (&index, 0, sizeof (struct igmp_snmp_rtr_cache_index)) ;
   
  ret = igmp_snmp_rtr_cache_index_get (v, name, length, &index, exact);

  if (ret < IGMP_ERR_NONE)
    {
      ret_var = NULL;
      goto EXIT;
    }

  if (index.qtype == IGMP_SNMP_INETADDRTYPE_IPV6 && exact)
    {
      ret_var = NULL;
      goto EXIT;
    }

  /* Get the Default VR */
  vr = apn_vr_lookup_by_id (lg, vr_id);
  if (! vr)
    {
      ret_var = NULL;
      goto EXIT;
    }

  /* Get the Default VRF */
  ivrf = apn_vrf_lookup_default (vr);

  if (! ivrf)
    {
      ret_var = NULL;
      goto EXIT;
    }

  /* Get the Default IGI */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret_var = NULL;
      goto EXIT;
    }

  ifp = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                             index.ifindex);

  /* Return the current value of the variable */
  switch (v->magic)
    {
      case IGMPROUTERCACHELASTREPORTER:
        /* 4 -- igmpRouterCacheLastReporter */
        IGMP_SNMP_GET (exact, cache_last_reporter, igi, &ifp,
                       &index, &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_cache_index_set (v, name, length, &index, ifp);
        break;

      case IGMPROUTERCACHEUPTIME:
        /* 5 -- igmpRouterCacheUptime */
        IGMP_SNMP_GET (exact, cache_uptime, igi, &ifp,
                       &index, &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_cache_index_set (v, name, length, &index, ifp);

        break;

      case IGMPROUTERCACHEEXPIRYTIME:
        /* 6 -- igmpRouterCacheExpirytime */
        IGMP_SNMP_GET (exact, cache_expiry_time, igi, &ifp,
                       &index, &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_cache_index_set (v, name, length, &index, ifp);
        break;

      case IGMPROUTERCACHEEXCLMODEEXPTIMER:
        /* 7 -- igmpRouterCacheExcludeModeExpirytimer */
        IGMP_SNMP_GET (exact, cache_exclmode_exp_timer, igi, &ifp,
                       &index, &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_cache_index_set (v, name, length, &index, ifp);
        break;

      case IGMPROUTERCACHEVER1HOSTTIMER:
        /* 8 -- igmpRouterCacheVer1HostTimer */
        IGMP_SNMP_GET (exact, cache_ver1_host_timer, igi, &ifp,
                       &index, &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_cache_index_set (v, name, length, &index, ifp);
        break;

      case IGMPROUTERCACHEVER2HOSTTIMER:
        /* 9 -- igmpRouterCacheVer2HostTimer */
        IGMP_SNMP_GET (exact, cache_ver2_host_timer, igi, &ifp,
                       &index, &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_cache_index_set (v, name, length, &index, ifp);
        break;

      case IGMPROUTERCACHESRCFILTERMODE:
        /* 10 -- igmpRouterCacheSrcFilterMode */
        IGMP_SNMP_GET (exact, cache_src_filter_mode, igi, &ifp,
                       &index, &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_cache_index_set (v, name, length, &index, ifp);
        break;

      default:
        break;
   }

EXIT:

  IGMP_FN_EXIT (ret_var);
}

u_int8_t *
igmp_snmp_inv_rtr_cache_tab (struct variable *v, oid *name, pal_size_t *length,
                             bool_t exact, pal_size_t *var_len,
                             WriteMethod **write_method, u_int32_t vr_id)
{
  struct igmp_snmp_inv_rtr_index index;
  struct igmp_instance *igi;
  struct lib_globals *lg;
  struct interface *ifp;
  struct apn_vrf *ivrf;
  s_int32_t ret_val;
  struct apn_vr *vr;
  u_int8_t *ret_var;
  s_int32_t ret;

  IGMP_FN_ENTER (IgmpSnmpInvRtrCacheTab);

  ret_val = SNMP_ERR_NOERROR;
  *write_method = NULL;
  ret_var = NULL;
  ivrf = NULL;
  igi = NULL;
  lg = v->lg;
  vr = NULL;

  pal_mem_set (&index, 0, sizeof (struct igmp_snmp_inv_rtr_index));

  ret = igmp_snmp_rtr_inv_cache_index_get (v, name, length, &index, exact);

  if (ret < IGMP_ERR_NONE)
    {
      ret_var = NULL;
      goto EXIT;
    }

  if (index.qtype == IGMP_SNMP_INETADDRTYPE_IPV6 && exact)
    {
      ret_var = NULL;
      goto EXIT;
    }

  /* Get the Default VR */
  vr = apn_vr_lookup_by_id (lg, vr_id);
  if (! vr)
    {
      ret_var = NULL;
      goto EXIT;
    }

  /* Get the Default VRF */
  ivrf = apn_vrf_lookup_default (vr);

  if (! ivrf)
    {
      ret_var = NULL;
      goto EXIT;
    }

  /* Get the Default IGI */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret_var = NULL;
      goto EXIT;
    }

  ifp = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                             index.ifindex);

  /* Return the current value of the variable */
  switch (v->magic)
    {
      case IGMPINVERSEROUTERCACHEIFINDEX:
      /* 1 -- igmpInverseRouterCacheIfIndex */
      /* Not accessible */
      case IGMPINVERSEROUTERCACHEADDRTYPE:
      /* 2 -- igmpInverseRouterCacheAddrType */
      /* Not accessible */
        break;

      case IGMPINVERSEROUTERCACHEADDR:
      /* 3 -- igmpInverseRouterCacheAddr */
        IGMP_SNMP_GET (exact, inv_cache_address, igi, &ifp,
                       &index, &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_inv_cache_index_set (v, name, length, &index, ifp);

        break;

      default:
        break;
    }

EXIT:

  IGMP_FN_EXIT (ret_var);
}

u_int8_t *
igmp_snmp_rtr_src_list_tab (struct variable *v, oid *name, pal_size_t *length,
                            bool_t exact, pal_size_t *var_len,
                            WriteMethod **write_method, u_int32_t vr_id)
{
  struct igmp_snmp_rtr_src_list_index index;
  struct igmp_instance *igi;
  struct lib_globals *lg;
  struct interface *ifp;
  struct apn_vrf *ivrf;
  s_int32_t ret_val;
  struct apn_vr *vr;
  u_int8_t *ret_var;
  s_int32_t ret;

  IGMP_FN_ENTER (IgmpSnmpRtrSrcListTab);

  ret_val = SNMP_ERR_NOERROR;
  *write_method = NULL;
  ret_var = NULL;
  ivrf = NULL;
  igi = NULL;
  lg = v->lg;
  vr = NULL;

  pal_mem_set (&index, 0, sizeof (struct igmp_snmp_rtr_src_list_index));

  ret = igmp_snmp_rtr_srclist_index_get (v, name, length, &index, exact);

  if (ret < IGMP_ERR_NONE)
    {
      ret_var = NULL;
      goto EXIT;
    }

  if (index.qtype == IGMP_SNMP_INETADDRTYPE_IPV6 && exact)
    {
      ret_var = NULL;
      goto EXIT;
    }

  /* Get the Default VR */
  vr = apn_vr_lookup_by_id (lg, vr_id);
  if (! vr)
    {
      ret_var = NULL;
      goto EXIT;
    }

  /* Get the Default VRF */
  ivrf = apn_vrf_lookup_default (vr);

  if (! ivrf)
    {
      ret_var = NULL;
      goto EXIT;
    }

  /* Get the Default IGI */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret_var = NULL;
      goto EXIT;
    }

  ifp = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                             index.ifindex);

  /* Return the current value of the variable */
  switch (v->magic)
    {
      case IGMPROUTERSRCLISTADDRTYPE:
        /* 1 -- igmpRouterSrcListAddrType */
        /* Not accessible */
      case IGMPROUTERSRCLISTADDR:
        /* 2 -- igmpRouterSrcListAddr */
        /* Not accessible */
      case IGMPROUTERSRCLISTIFINDEX:
        /* 3 -- igmpRouterSrcListIfIndex */
        /* Not accessible */
        break;

      case IGMPROUTERSRCLISTHOSTADDRESS:
        /* 4 -- igmpRouterSrcListHostAddr */
        IGMP_SNMP_GET (exact, srclist_host_address, igi, &ifp,
                       &index.group_addr, &index.host_addr,&index,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_srclist_index_set (v, name, length, &index, ifp);

        break;

      case IGMPROUTERSRCLISTEXPIRE:
        /* 5 -- igmpRouterSrcListExpire */
        IGMP_SNMP_GET (exact, srclist_expiry_time, igi, &ifp,
                       &index.group_addr, &index.host_addr,&index,
                       &ret_var, var_len);
        if (! exact)
          igmp_snmp_rtr_srclist_index_set (v, name, length, &index, ifp);

        break;

      default:
        break;
    }

EXIT:

  IGMP_FN_EXIT (ret_var);
}

#endif /* HAVE_SNMP */
