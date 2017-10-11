/* Copyright (C) 2002-2003   All Rights Reserved. */

#include <pal.h>

#include "lib.h"
#include "table.h"
#include "memory.h"
#include "modbmap.h"
#include "thread.h"
#include "version.h"
#include "snprintf.h"
#include "keychain.h"
#include "show.h"
#ifdef MEMMGR
#include "memmgr.h"
#endif /* MEMMGR */
#include "distribute.h"
#include "filter.h"
#ifdef HAVE_MPLS
#include "label_pool.h"
#endif /* HAVE_MPLS */
#include "log.h"
#ifdef HAVE_IMI
#include "imi_client.h"
#endif /* HAVE_IMI */
#ifdef HAVE_SNMP
#include "snmp.h"
#endif /* HAVE_SNMP */
#ifdef HAVE_LICENSE_MGR
#include "licmgr.h"
#endif /* HAVE_LICENSE_MGR */
#ifdef HAVE_BFD
#include "bfd_client_api.h"
#endif /* HAVE_BFD */
#ifdef HAVE_HA
#include "lib_cal.h"
#endif /* HAVE_HA */
#ifdef HAVE_PBR
#include "pbr_filter.h"
#endif /* HAVE_PBR */

#define MTYPE_APN_VR            MTYPE_TMP /* XXX-VR */
#define MTYPE_APN_VRF           MTYPE_TMP

#define APN_MOTD_SIZE           100
#define APN_COPYRIGHT_SIZE      80


/* Default Message Of The Day. */
char *
apn_motd_new (struct lib_globals *zg)
{
  char buf[APN_COPYRIGHT_SIZE];
  char *motd;

  motd = XCALLOC (MTYPE_CONFIG_MOTD, APN_MOTD_SIZE);
  if (motd == NULL)
    return NULL;

  /* Set the Message of The Day.  */
  zsnprintf (motd, APN_MOTD_SIZE, "\r\nPacOS%s",
             pacos_copyright (buf, APN_COPYRIGHT_SIZE));

  return motd;
}

void
apn_motd_free (struct lib_globals *zg)
{
  XFREE (MTYPE_CONFIG_MOTD, zg->motd);
}

char *
apn_cwd_new (struct lib_globals *zg)
{
  char buf[MAXPATHLEN];
  char *cwd;

  if (pal_getcwd (buf, MAXPATHLEN) != (int)NULL)
    cwd = XSTRDUP (MTYPE_CONFIG, buf);
  else
    {
      cwd = XCALLOC (MTYPE_CONFIG, 2);
      if (cwd == NULL)
        return NULL;

      /* Set the separator.  */
      cwd[0] = PAL_FILE_SEPARATOR;
    }
  return cwd;
}

void
apn_cwd_free (struct lib_globals *zg)
{
  XFREE (MTYPE_CONFIG, zg->cwd);
}


struct apn_vr *
apn_vr_new (void)
{
  struct apn_vr *vr;

  vr = XCALLOC (MTYPE_APN_VR, sizeof (struct apn_vr));

  return vr;
}

void
apn_vr_free (struct apn_vr *vr)
{
  if (vr->name)
    XFREE (MTYPE_TMP, vr->name);

  if (vr->entLogical)
    {
      if (vr->entLogical->entLogicalCommunity)
        XFREE (MTYPE_COMMUNITY_STR, vr->entLogical->entLogicalCommunity);
      XFREE (MTYPE_TMP, vr->entLogical);
    }
  if (vr->mappedPhyEntList)
    list_delete (vr->mappedPhyEntList);

  THREAD_TIMER_OFF (vr->t_if_stat_threshold);

  XFREE (MTYPE_APN_VR, vr);
}

void
apn_vr_config_file_remove (struct apn_vr *vr)
{
 char *path;
 char buf[MAXPATHLEN];

 if (vr->host == NULL)
   return;

 if (vr->name)
   {
     zsnprintf (buf, sizeof buf, "%s%c%s",
                vr->zg->cwd, PAL_FILE_SEPARATOR, vr->name);
     path = buf;
   }
 else
   {
     /* No VR Directory Path is found */
     return;
   }

  /* Delete the VR Config File */
  if (vr->host->config_file != NULL)
    pal_unlink (vr->host->config_file);

  /* Delete the VR Config Directory */
  if (path != NULL)
    pal_rmdir (path);
}

void
apn_vr_delete (struct apn_vr *vr)
{
  struct apn_vr *pvr = apn_vr_get_privileged (vr->zg);
  struct route_node *rn;
  struct interface *ifp;
  struct apn_vrf *vrf, *vrf_next;

  /* Cleanup the interface.  */
  for (rn = route_top (vr->ifm.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      {
        /* Bind the interfaces back to PVR if this VR is not the PVR.  */
        if (vr == pvr)
          if_vr_unbind (&vr->ifm, ifp->ifindex);
        else
          if_vr_bind (&pvr->ifm, ifp->ifindex);
      }

  for (vrf = vr->vrf_list; vrf; vrf = vrf_next)
    {
      vrf_next = vrf->next;
      apn_vrf_delete (vrf);
    }

  vector_unset (vr->zg->vr_vec, vr->id);

  THREAD_OFF (vr->t_config);

  /* Finish the route-map.  */
  route_map_finish (vr);

  /* Finish the prefix-list.  */
  prefix_list_finish (vr);

  /* Finish the access-list.  */
  access_list_finish (vr);

  /* Finish the key chain.  */
  keychain_finish (vr);

  /* Remove the VR Directory, if present */
  if (CHECK_FLAG(vr->flags, LIB_FLAG_DELETE_VR_CONFIG_FILE))
    apn_vr_config_file_remove (vr);

  /* Free the host.  */
  host_free (vr->host);

  /* Finish VR interface master.  */
  if_vr_master_finish (&vr->ifm, vr);

  /* Finish the VRF vector.  */
  vector_free (vr->vrf_vec);

#ifdef HAVE_HA
  lib_cal_delete_vr (vr->zg, vr);
#endif /* HAVE_HA */

  /* Free VR instance.  */
  apn_vr_free (vr);
}

void
apn_vr_delete_all (struct lib_globals *zg)
{
  struct apn_vr *vr;
  int i;

  for (i = 0; i < vector_max (zg->vr_vec); i++)
    if ((vr = vector_slot (zg->vr_vec, i)))
      apn_vr_delete (vr);
}

struct apn_vr *
apn_vr_get (struct lib_globals *zg)
{
  struct apn_vr *vr;

  vr = apn_vr_new ();
  vr->zg = zg;

#ifdef HAVE_HA
  lib_cal_create_vr (zg, vr);
#endif /* HAVE_HA */

  /* Create default VRF.  */
  vr->vrf_vec = vector_init (1);
  apn_vrf_get_by_name (vr, NULL);

  /* Initialize Host. */
  vr->host = host_new (vr);

  /* Initialize VR interface master. */
  if_vr_master_init (&vr->ifm, vr);

  /* Initialize prefix lists. */
  vr->prefix_master_ipv4.seqnum = 1;
#ifdef HAVE_IPV6
  vr->prefix_master_ipv6.seqnum = 1;
#endif /* def HAVE_IPV6 */
  vr->prefix_master_orf.seqnum = 1;

  /* Initialize Key Chain. */
  keychain_init (vr);

  /* Initialize Route Map. */
  route_map_init (vr);

  /* Initialize SNMP Community */
  snmp_community_init (vr);

  /* Initialize access list master with AFI */
  vr->access_master_ipv4.afi = AFI_IP;
  vr->access_master_ipv4.max_count = ACCESS_LIST_ENTRY_MAX;
#ifdef HAVE_IPV6
  vr->access_master_ipv6.afi = AFI_IP6;
#endif /* def HAVE_IPV6 */

  return vr;
}

struct apn_vr *
apn_vr_get_by_id (struct lib_globals *zg, u_int32_t id)
{
  struct apn_vr *vr;

  if (vector_max (zg->vr_vec) > id)
    {
      vr = vector_slot (zg->vr_vec, id);
      if (vr != NULL)
        return (struct apn_vr *)vr;
    }

  vr = apn_vr_get (zg);
  vr->id = vector_set_index (zg->vr_vec, id, vr);

  return vr;
}

struct apn_vr *
apn_vr_get_privileged (struct lib_globals *zg)
{
  return (struct apn_vr *)vector_slot (zg->vr_vec, 0);
}

struct apn_vr *
apn_vr_lookup_by_id (struct lib_globals *zg, u_int32_t id)
{
  if (vector_max (zg->vr_vec) > id)
    return vector_slot (zg->vr_vec, id);

  return NULL;
}

struct apn_vr *
apn_vr_lookup_by_name (struct lib_globals *zg, char *name)
{
  struct apn_vr *vr;
  int i;

  for (i = 0; i < vector_max (zg->vr_vec); i++)
    if ((vr = vector_slot (zg->vr_vec, i)))
      {
        if (name == NULL)
          {
            if (vr->name == NULL)
              return vr;
          }
        else
          {
            if (vr->name != NULL)
              if (pal_strcmp (vr->name, name) == 0)
                return vr;
          }
      }

  return NULL;
}

ZRESULT
apn_vrs_walk_and_exec(struct lib_globals *zg,
                      APN_VRS_WALK_CB func,
                      intptr_t user_ref)
{
  struct apn_vr *vr;
  int i;
  ZRESULT res;

  if (zg==NULL || func==NULL)
    return ZRES_ERR;

  for (i = 0; i < vector_max (zg->vr_vec); i++)
    if ((vr = vector_slot (zg->vr_vec, i)) != NULL)
      if (func)
        if ((res = func(vr, user_ref)) != ZRES_OK)
          return res;

  return ZRES_OK;
}

void
apn_vr_logical_entity_create (struct apn_vr *vr)
{
  char community [255];

  pal_snprintf (community, sizeof (community), "public%d", vr->id);

  vr->entLogical = XMALLOC (MTYPE_TMP, sizeof (struct entLogicalEntry));
  if (! vr->entLogical)
    return;

  vr->entLogical->entLogicalIndex = vr->id;

  vr->entLogical->entLogicalDescr = "Virtual Router";
  vr->entLogical->entLogicalType = "MIB II";

  vr->entLogical->entLogicalCommunity = XMALLOC (MTYPE_COMMUNITY_STR,
                                                 sizeof (community));

  pal_strcpy (vr->entLogical->entLogicalCommunity, community);

  vr->entLogical->entLogicalTAddress = "161";
  vr->entLogical->entLogicalTDomain = "snmpUDPDomain";

  vr->entLogical->entLogicalContextEngineId = "";
  vr->entLogical->entLogicalContextName = "";
}

struct apn_vr *
apn_vr_get_by_name (struct lib_globals *zg, char *name)
{
  struct apn_vr *vr;
  u_int32_t id;

  vr = apn_vr_lookup_by_name (zg, name);
  if (vr != NULL)
    return vr;

  id = vector_empty_slot (zg->vr_vec);
  vr = apn_vr_get_by_id (zg, id);
  if (name)
    vr->name = XSTRDUP (MTYPE_TMP, name);

  apn_vr_logical_entity_create (vr);
  vr->mappedPhyEntList = list_new ();

  return vr;
}

struct apn_vr *
apn_vr_update_by_name (struct lib_globals *zg, char *name, u_int32_t vr_id)
{
  struct apn_vr *vr = NULL, *vr_old = NULL;

  /* For default VR the name is NULL */
  vr = apn_vr_lookup_by_name (zg, name);
  if (vr == NULL)
    return NULL;

  if (vr->id == vr_id)
    return vr;

  /* Delete the duplicated VR.  */
  vr_old = apn_vr_lookup_by_id (zg, vr_id);
  if (vr_old != NULL)
    {
      /* Connection close callback.  */
      if (zg->vr_callback[VR_CALLBACK_CLOSE])
        (*zg->vr_callback[VR_CALLBACK_CLOSE]) (vr_old);

      /* Protocol callback. */
      if (zg->vr_callback[VR_CALLBACK_DELETE])
        (*zg->vr_callback[VR_CALLBACK_DELETE]) (vr_old);

      apn_vr_delete (vr_old);
    }

  /* Update VR vector.  */
  vector_unset (zg->vr_vec, vr->id);
  vr->id = vector_set_index (zg->vr_vec, vr_id, vr);

  return vr;
}

void
apn_vr_init (struct lib_globals *zg)
{
  /* Initialize VR vector.  */
  zg->vr_vec = vector_init (1);

  /* Initialize FIB to VRF map vector.  */
  zg->fib2vrf = vector_init (1);
}

void
apn_vr_finish (struct lib_globals *zg)
{
  /* Delete all the VR.  */
  apn_vr_delete_all (zg);

  /* Free the FIB to VRF map vector. */
  vector_free (zg->fib2vrf);

  /* Free the vector.  */
  vector_free (zg->vr_vec);
}


struct apn_vrf *
apn_vrf_new (void)
{
  struct apn_vrf *vrf;

  vrf = XCALLOC (MTYPE_APN_VRF, sizeof (struct apn_vrf));

  return vrf;
}

void
apn_vrf_free (struct apn_vrf *vrf)
{
  if (vrf->name)
    XFREE (MTYPE_VRF_NAME, vrf->name);

  XFREE (MTYPE_APN_VRF, vrf);
}

void
apn_vrf_add_to_list (struct apn_vr *vr, struct apn_vrf *vrf)
{
  vrf->prev = vrf->next = NULL;
  if (vr->vrf_list)
    vr->vrf_list->prev = vrf;
  vrf->next = vr->vrf_list;
  vr->vrf_list = vrf;

  return;
}

void
apn_vrf_delete_from_list (struct apn_vr *vr, struct apn_vrf *vrf)
{
  if (vrf->next)
    vrf->next->prev = vrf->prev;
  if (vrf->prev)
    vrf->prev->next = vrf->next;
  else
    vr->vrf_list = vrf->next;

  vrf->next = vrf->prev = NULL;

  return;
}

struct apn_vrf *
apn_vrf_get_by_name (struct apn_vr *vr, char *name)
{
  struct apn_vrf *vrf;

  vrf = apn_vrf_lookup_by_name (vr, name);
  if (vrf == NULL)
    {
      vrf = apn_vrf_new ();
      vrf->vr = vr;
      if (name != NULL)
        vrf->name = XSTRDUP (MTYPE_VRF_NAME, name);

      /* Reset VRF and FIB IDs.  */
      vrf->id = VRF_ID_DISABLE;
      vrf->fib_id = FIB_ID_DISABLE;

      /* Enlist Into VRF-List */
      apn_vrf_add_to_list (vr, vrf);

      /* Initialize VRF interface master. */
      if_vrf_master_init (&vrf->ifv, vrf);

#ifdef HAVE_HA
      lib_cal_create_vrf (vr->zg, vrf);
#endif /* HAVE_HA */
    }

  return vrf;
}

void
apn_vrf_delete (struct apn_vrf *vrf)
{
  /* Unset from VRF2FIB vector.  */
  vector_unset (vrf->vr->zg->fib2vrf, vrf->fib_id);

  /* Unset from VRF vector.  */
  vector_unset (vrf->vr->vrf_vec, vrf->id);

  /* Delete from the list.  */
  apn_vrf_delete_from_list (vrf->vr, vrf);

  /* Finish VRF interface master.  */
  if_vrf_master_finish (&vrf->ifv, vrf);

#ifdef HAVE_HA
  lib_cal_delete_vrf (vrf->vr->zg, vrf);
#endif /* HAVE_HA */

  /* Free VRF.  */
  apn_vrf_free (vrf);
}

struct apn_vrf *
apn_vrf_lookup_by_name (struct apn_vr *vr, char *name)
{
  struct apn_vrf *vrf;

  if (vr == NULL)
    return NULL;

  for (vrf = vr->vrf_list; vrf; vrf = vrf->next)
    {
      if (vrf->name == NULL && name == NULL)
        return vrf;

      if (vrf->name != NULL && name != NULL)
        if (pal_strncmp (vrf->name, name, MAX_VRF_NAMELEN) == 0)
          return vrf;
    }

  return NULL;
}

struct apn_vrf *
apn_vrf_lookup_by_id (struct apn_vr *vr, u_int32_t id)
{
  if (vector_max (vr->vrf_vec) > id)
    return (struct apn_vrf *)vector_slot (vr->vrf_vec, id);

  return NULL;
}

struct apn_vrf *
apn_vrf_lookup_default (struct apn_vr *vr)
{
  return apn_vrf_lookup_by_id (vr, 0);
}

void
apn_vr_add_callback (struct lib_globals *zg, enum vr_callback_type type,
                     int (*func) (struct apn_vr *))
{
  if (type < 0 || type >= VR_CALLBACK_MAX)
    return;

  zg->vr_callback[type] = func;
}

void
apn_vrf_add_callback (struct lib_globals *zg, enum vrf_callback_type type,
                     int (*func) (struct apn_vrf *))
{
  if (type < 0 || type >= VRF_CALLBACK_MAX)
    return;

  zg->vrf_callback[type] = func;
}


struct lib_globals *
lib_clean (struct lib_globals *zg)
{
  if (!zg)
    return NULL;

#ifdef HAVE_LICENSE_MGR
  /* Finish license manager.  */
  lic_mgr_finish (zg->handle);
#endif /* HAVE_LICENSE_MGR */

#ifdef HAVE_BFD
  bfd_client_api_client_delete (zg);
#endif /* HAVE_BFD */

  /* Stop the host configuration.  */
  HOST_CONFIG_STOP (zg);

  /* Finish the show server.  */
  if (zg->ss != NULL)
    show_server_finish (zg);

  /* Finish the interface master.  */
  if (zg->ifg.zg != NULL)
    if_master_finish (&zg->ifg);

  /* Finish the VR.  */
  if (zg->vr_vec != NULL)
    apn_vr_finish (zg);

  /* Close the logger. */
  if (zg->log)
    closezlog (zg, zg->log);

  if (zg->log_default)
    closezlog (zg, zg->log_default);

  if (zg->motd != NULL)
    apn_motd_free (zg);

  if (zg->cwd != NULL)
    apn_cwd_free (zg);

  if (zg->pal_socket)
    pal_sock_stop (zg);

  if (zg->pal_stdlib)
    pal_stdlib_stop (zg);

  if (zg->pal_string)
    pal_strstop (zg);

  if (zg->pal_time)
    pal_time_stop (zg);

  pal_log_stop (zg);

  /* Free the thread master.  */
  if (zg->master != NULL)
    thread_master_finish (zg->master);

  /* Unset the lib globals in memory manager.  */
  memory_unset_lg ((void *)zg);

  XFREE (MTYPE_ZGLOB, zg);

  /* Finish the memory module.  */
  memory_finish ();

  return NULL;
}

struct lib_globals *
lib_create (char *progname)
{
  struct lib_globals *zg;
  int ret;

  /* Initialize Protocol Module Bitmaps. */
  modbmap_init_all ();

  zg = XCALLOC (MTYPE_ZGLOB, sizeof (struct lib_globals));
  if (zg == NULL)
    return NULL;

  /* Mark lib as not in shutdown */
  UNSET_LIB_IN_SHUTDOWN (zg);

  pal_strncpy(zg->progname, progname, LIB_MAX_PROG_NAME);

  /* PAL log.  */
  ret = pal_log_start (zg);
  if (ret != 0)
    return lib_clean (zg);

  /* PAL socket.  */
  zg->pal_socket = pal_sock_start (zg);
  if (!zg->pal_socket)
    return lib_clean (zg);

  /* PAL stdlib.  */
  zg->pal_stdlib = pal_stdlib_start (zg);
  if (!zg->pal_stdlib)
    return lib_clean (zg);

  /* PAL string.  */
  zg->pal_string = pal_strstart (zg);
  if (!zg->pal_string)
    return lib_clean (zg);

  /* PAL time.  */
  zg->pal_time = pal_time_start (zg);
  if (!zg->pal_time)
    return lib_clean (zg);

  /* Set the default MOTD.  */
  zg->motd = apn_motd_new (zg);
  if (zg->motd == NULL)
    return lib_clean (zg);

  /* Get the current working directory.  */
  zg->cwd = apn_cwd_new (zg);
  if (zg->cwd == NULL)
    return lib_clean (zg);

  zg->ctree = cli_tree_new ();
  if (zg->ctree == NULL)
    return lib_clean (zg);

  zg->pend_read_thread = NULL;

  zg->master = thread_master_create ();
  if (zg->master == NULL)
    return lib_clean (zg);

  /* Initialize default log. */
/*  zg->log_default = openzlog (zg, 0, zg->protocol, LOGDEST_DEFAULT); */

  /* Fault recording may use zlog to report fault occurance. */
  fmInitFaultRecording(zg);

  /* Set the lib globals in memory manager */
  memory_set_lg ((void *)zg);

  /* Initialize the VR.  */
  apn_vr_init (zg);

  /* Initialize Interface Master. */
  if_master_init (&zg->ifg, zg);
  meg_master_init(&zg->megg,  zg);
  //&zg->sig.pstack.base = XMALLOC (MTYPE_NSM_STACK, sizeof (STACK_INIT_SIZE*sizeof(int)));

  //malloc(20*sizeof(int));

  return zg;
}

result_t
lib_start (struct lib_globals *zg)
{
  struct apn_vr *vr;

#ifdef HAVE_SNMP
  snmp_make_tree (zg);

#ifdef HAVE_AGENTX
  agentx_initialize (&zg->snmp);
#endif /* HAVE_AGENTX */
#endif /* def HAVE_SNMP */

  /* Initialize PVR here. */
  vr = apn_vr_get_by_id (zg, 0);

  LIB_GLOB_SET_VR_CONTEXT(zg, vr);

  return RESULT_OK;
}

/* Shut down the library and prepare for disuse.  */
result_t
lib_stop (struct lib_globals *zg)
{
  if (zg)
    {
      /* Mark lib in shutdown */
      SET_LIB_IN_SHUTDOWN (zg);

      /* Clean-up the global structures.  */
      if ((lib_clean (zg)) == NULL)
        zg = NULL;
    }

  return (RESULT_OK);
}

/* modname str */
char *
modname_strl (int index)
{
  switch (index)
    {
    case APN_PROTO_UNSPEC:
      return "Unspec";
    case APN_PROTO_NSM:
      return "NSM";
    case APN_PROTO_RIP:
      return "RIP";
    case APN_PROTO_RIPNG:
      return "RIPng";
    case APN_PROTO_OSPF:
      return "OSPF";
    case APN_PROTO_OSPF6:
      return "OSPFv3";
    case APN_PROTO_ISIS:
      return "IS-IS";
    case APN_PROTO_BGP:
      return "BGP";
    case APN_PROTO_LDP:
      return "LDP";
    case APN_PROTO_RSVP:
      return "RSVP";
    case APN_PROTO_PIMDM:
      return "PIM-DM";
    case APN_PROTO_PIMSM:
      return "PIM-SM";
    case APN_PROTO_PIMSM6:
      return "PIM-SMv6";
    case APN_PROTO_PIMPKTGEN:
      return "PIMPKTGEN";
    case APN_PROTO_DVMRP:
      return "DVMRP";
    case APN_PROTO_8021X:
      return "802.1X";
    case APN_PROTO_ONM:
      return "ONMD";
    case APN_PROTO_LACP:
      return "LACP";
    case APN_PROTO_STP:
      return "STP";
    case APN_PROTO_RSTP:
      return "RSTP";
    case APN_PROTO_MSTP:
      return "MSTP";
    case APN_PROTO_IMI:
      return "IMI";
    case APN_PROTO_IMISH:
      return "IMISH";
    case APN_PROTO_VTYSH:
      return "VTYSH";
    case APN_PROTO_OAM:
      return "OAM";
    case APN_PROTO_ELMI:
      return "ELMI";
    case APN_PROTO_LMP:
      return "lmpd";
    case APN_PROTO_VLOG:
      return "VLOG";
    case APN_PROTO_HSL:
      return "HSL";
    case APN_PROTO_MAX:
      return "Unknown";
    }
  return "Unknown";
}

/* modname str */
char *
modname_strs (int index)
{
  switch (index)
    {
    case APN_PROTO_UNSPEC:
      return "unspec";
    case APN_PROTO_NSM:
      return "nsm";
    case APN_PROTO_RIP:
      return "ripd";
    case APN_PROTO_RIPNG:
      return "ripngd";
    case APN_PROTO_OSPF:
      return "ospfd";
    case APN_PROTO_OSPF6:
      return "ospf6d";
    case APN_PROTO_ISIS:
      return "isisd";
    case APN_PROTO_BGP:
      return "bgpd";
    case APN_PROTO_LDP:
      return "ldpd";
    case APN_PROTO_RSVP:
      return "rsvpd";
    case APN_PROTO_PIMDM:
      return "pdmd";
    case APN_PROTO_PIMSM:
      return "pimd";
    case APN_PROTO_PIMSM6:
      return "pim6d";
    case APN_PROTO_PIMPKTGEN:
      return "pimpktgend";
    case APN_PROTO_DVMRP:
      return "dvmrpd";
    case APN_PROTO_8021X:
      return "authd";
    case APN_PROTO_ONM:
      return "onmd";
    case APN_PROTO_LACP:
      return "lacpd";
    case APN_PROTO_STP:
      return "stpd";
    case APN_PROTO_RSTP:
      return "rstpd";
    case APN_PROTO_MSTP:
      return "mstpd";
    case APN_PROTO_IMI:
      return "imi";
    case APN_PROTO_IMISH:
      return "imish";
    case APN_PROTO_VTYSH:
      return "PacOS";
    case APN_PROTO_NPFIM:
      return "npifmgr";
    case APN_PROTO_RMON:
      return "rmon";
    case APN_PROTO_OAM:
      return "oam";
    case APN_PROTO_LMP:
      return "lmpd";
    case APN_PROTO_VLOG:
      return "vlogd";
    case APN_PROTO_ELMI:
      return "elmid";
    case APN_PROTO_HSL:
      return "HSL";
    case APN_PROTO_MAX:
      return "unknown";
    }
  return "unknown";
}

/* Protocol ID to route type. */
int
protoid2routetype (int proto_id)
{
  switch (proto_id)
    {
    case APN_PROTO_RIP:
      return APN_ROUTE_RIP;
    case APN_PROTO_RIPNG:
      return APN_ROUTE_RIPNG;
    case APN_PROTO_OSPF:
      return APN_ROUTE_OSPF;
    case APN_PROTO_OSPF6:
      return APN_ROUTE_OSPF6;
    case APN_PROTO_ISIS:
      return APN_ROUTE_ISIS;
    case APN_PROTO_BGP:
      return APN_ROUTE_BGP;
    default:
      return -1;
    }

  return -1;
}


#ifdef HAVE_VLOGD
ZRESULT
lib_reg_vlog_cbs(struct lib_globals *zg,
                 VLOG_SET_LOG_FILE_CB   vlog_file_set_cb,
                 VLOG_UNSET_LOG_FILE_CB vlog_file_unset_cb,
                 VLOG_GET_LOG_FILE_CB   vlog_file_get_cb)
{
  if (! zg)
    return ZRES_ERR;

  zg->vlog_file_set_cb   = vlog_file_set_cb;
  zg->vlog_file_unset_cb = vlog_file_unset_cb;
  zg->vlog_file_get_cb   = vlog_file_get_cb;
  return ZRES_OK;
}

#endif
