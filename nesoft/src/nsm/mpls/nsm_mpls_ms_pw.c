/* Copyright (C) 2009-2010  All Rights Reserved. */

/**@file nsm_mpls_ms_pw.c
   @brief This files is to define all the functions related to support
          the MS-PW feature for VCs.
*/

#include "pal.h"
#include "lib.h"

#ifdef HAVE_MPLS_VC
#ifdef HAVE_MS_PW
#include "log.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#include "mpls_common.h"

#include "nsmd.h"

#include "nsm_debug.h"
#include "nsm_mpls.h"
#include "nsm_mpls_vc.h"
#include "nsm_mpls_rib.h"
#include "nsm_interface.h"
#include "mpls_common.h"
#include "mpls.h"
#include "nsmd.h"
#include "nsm_server.h"
#include "nsm_mpls_ms_pw.h"

/**@brief Function to display the MS PW data
    @param *bucket - Pointer to the hash bucket
    @param *cli    - Pointer to the CLI context
    @return        - void
*/    
void
nsm_ms_pw_show_iterator (struct hash_backet *bucket, struct cli *cli)
{
  struct nsm_mpls_ms_pw *ms_pw = NULL;

  if (!bucket)
    {
      cli_out (cli, "Got a NULL Hash table\n");
      return;
    }

  ms_pw = (struct nsm_mpls_ms_pw *) bucket->data;
  if (ms_pw)
    {
      if (ms_pw->user_config)
        {
          cli_out (cli, "mpls ms-pw-stitch %s %s %s mtu %d %s", 
              ms_pw->ms_pw_name, 
              ms_pw->vc1->name, ms_pw->vc2->name,
              ms_pw->mtu, ms_pw->vlan_id ? "vlan" : "ethernet");
          if (ms_pw->vlan_id)
            cli_out (cli, " %d\n", ms_pw->vlan_id);
          else
            cli_out (cli, "\n");
        }
      else
      cli_out (cli, "mpls ms-pw-stitch %s %s %s\n", ms_pw->ms_pw_name, 
               ms_pw->vc1->name, ms_pw->vc2->name);

      if (ms_pw->ms_pw_spe_descr_set)
        cli_out (cli, "mpls ms-pw %s %s\n", ms_pw->ms_pw_name, 
            ms_pw->ms_pw_spe_descr);
    }
  return;
}

/**@brief Function to sync the MSPW data from NSM to NSM client when
          the client has started after the MSPW has been configured.
    @param *bucket - Pointer to the MSPW hash table bucket data.
    @param *nm     - Pointer to the NSM master.
    @return        - void.
*/
void
nsm_ms_pw_sync (struct hash_backet *bucket, struct nsm_master *nm)
{
  struct nsm_mpls_ms_pw *ms_pw = NULL;
  int ret = NSM_SUCCESS;

  if (!bucket)
    return;

  ms_pw = (struct nsm_mpls_ms_pw *) bucket->data;
  if (ms_pw)
    ret = nsm_mpls_ms_pw_stitch_send (nm, ms_pw, NSM_TRUE);

  return;
}

/**@brief Function to create a key into the MSPW hash table with the 
          name as the key.
    @param  *name - pointer to the name of the MSPW instance.
    @return       - the Key to the MSPW hash table.
*/    
u_int32_t
nsm_ms_pw_hash_key_make (char *name)
{
  int i, len;
  u_int32_t key;

  if (name)
    {      
      key = 0;      
      len = pal_strlen (name);      
      for (i = 1; i <= len; i++)        
        key += (name[i] * i);

      return key;
    }

  return NSM_SUCCESS;                                      
}     

/**@brief Support function for handling the MSPW hash table.
          This is the comparision function for the MSPW hash table.
    @param *ms_pw   - pointer to the ms_pw structure.
    @param *name    - pointer to the name of the ms_pw instance name.
    @return boolean - TRUE or FALSE.
*/
bool_t
nsm_ms_pw_hash_cmp (struct nsm_mpls_ms_pw *ms_pw, char *name)
{
  if (ms_pw && ms_pw->ms_pw_name && name && 
      pal_strcmp (ms_pw->ms_pw_name, name) == 0)
    return NSM_TRUE;

  return NSM_FALSE;
}

/**@brief Function to initialise the NSM MSPW hash table.
    @param *nm - pointer to the nsm master global.
    @return    - NSM_SUCCESS/NSM_FAILURE.
*/
int
nsm_ms_pw_hash_init (struct nsm_master *nm)
{
  if (!NSM_MPLS)
    return NSM_FAILURE;

  NSM_MS_PW_HASH = hash_create(nsm_ms_pw_hash_key_make,
                               nsm_ms_pw_hash_cmp);
  if (! NSM_MS_PW_HASH)
    return NSM_FAILURE;

  return NSM_SUCCESS;
}

/**@brief Function to allocate function for NSM MSPW hash table.
    @param *name  - pointer to the name of the MSPW instance.
    @return *void - pointer to the MSPW structure that has been allocated.
*/
void *
nsm_ms_pw_hash_alloc (char *name)
{
  struct nsm_mpls_ms_pw *ms_pw = NULL;

  ms_pw = XCALLOC (MTYPE_TMP, sizeof(struct nsm_mpls_ms_pw)); 
  if (!ms_pw)
    return NULL;

  return ms_pw;
}

/**@brief Function to remove an entry from NSM MSPW Hash table.
    @param *nm    - pointer to the NSM master global
    @param *ms_pw - pointer to the MSPW structure that needs to be removed
                    from the NSM MSPW global hash table.
    @return void.
*/
void
nsm_ms_pw_remove_from_hash (struct nsm_master *nm,
                            struct nsm_mpls_ms_pw *ms_pw)
{     
  if ((!ms_pw) || (!NSM_MPLS) || (!NSM_MS_PW_HASH))
    return;

  hash_release (NSM_MS_PW_HASH, ms_pw->ms_pw_name);
} 

/**@brief Function to look up MS-PW hash table with MSPW name as key
    @param *nm    - pointer to the NSM master global
    @param *name  - pointer to the name of the MSPW instance.
    return *ms_pw - pointer to the MS_PW struct, if it exists OR NULL;
*/
struct nsm_mpls_ms_pw *
nsm_ms_pw_lookup_by_name (struct nsm_master* nm, char *name)
{
  if ((!NSM_MPLS) || (!name) || (!NSM_MS_PW_HASH))
    return NULL;

  return hash_lookup (NSM_MS_PW_HASH, name);
}

/**@brief Function to get a MSPW instance. When none found, create a new one.
    @param *nm     - pointer to the NSM master global
    @param *name   - pointer to the name of the MSPW instance.
    @param *status - pointer to the status of the get operation.
    return *ms_pw  - pointer to the MS_PW struct, if it exists OR NULL;
*/
struct nsm_mpls_ms_pw *
nsm_ms_pw_get_by_name (struct nsm_master* nm,
                       char *name, 
                       int *status)
{
  struct nsm_mpls_ms_pw *ms_pw = NULL;

  if ((!NSM_MPLS) || (!name) || (!NSM_MS_PW_HASH))
    return NULL;

  *status = NSM_FAILURE;

  /* GET from hash. */
  ms_pw = hash_get (NSM_MS_PW_HASH, name, nsm_ms_pw_hash_alloc);
  if (! ms_pw)
    {
      zlog_warn (NSM_ZG, "GET for mspw with name %s failed", name);
      return NULL;
    }

  /* Set MSPW name */
  ms_pw->ms_pw_name = XSTRDUP (MTYPE_TMP, name);
  if (! ms_pw->ms_pw_name)
    {
      pal_strncpy (ms_pw->ms_pw_name, name, pal_strlen (name));
      nsm_ms_pw_remove_from_hash (nm, ms_pw);
      XFREE (MTYPE_TMP, ms_pw);

      return NULL;
    }

  zlog_info (NSM_ZG, "Got new MSPW with name %s", ms_pw->ms_pw_name);

  *status = NSM_SUCCESS;
  return ms_pw;
}

/**@brief Function to prepare the vc details in the MSPW message to ldp
    @param *mpls_vc_msg - pointer to nsm_msg_mpls_vc
    @param *vc          - pointer to nsm_mpls_circuit
    @param is_add       - flag indicating add, delete of vc.
    return              - NSM_SUCCESS/NSM_FAILURE.
*/
int
nsm_mpls_ms_pw_prepare_vc_msg (struct nsm_msg_mpls_vc *msg,
                               struct nsm_mpls_circuit *vc, 
                               bool_t is_add, int ms_pw_role)
{
  if ((!msg) || (!vc))
    return NSM_FAILURE;

  /* Set id. */
  msg->vc_id = vc->id;

  if (!is_add)
    return NSM_SUCCESS;
  
  /* Set end-point address. */
  NSM_SET_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_NEIGHBOR);
  if (vc->address.family == AF_INET)
    {
      msg->afi = AFI_IP;
      msg->nbr_addr_ipv4 = vc->address.u.prefix4;
    }

  /* Set control word. */
  NSM_SET_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_CONTROL_WORD);
  msg->c_word = vc->cw;

  /* Send the PW Status */
  NSM_SET_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_PW_STATUS);
  msg->pw_status = vc->pw_status;

  /* Send the MS PW role */
  NSM_SET_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_MS_PW_ROLE);
  msg->ms_pw_role = ms_pw_role;

  /* Send group id. */
  NSM_SET_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_GROUP_ID);
  if (vc->group)
    msg->grp_id = vc->group->id;

  if (vc->ms_pw->user_config)
    {
      /* Send vc type. */
      NSM_SET_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_VC_TYPE);
      if (vc->ms_pw->vlan_id > 0)
        msg->vc_type = VC_TYPE_ETH_VLAN;
      else
        msg->vc_type = VC_TYPE_ETHERNET;

      /* Send vlan-id. */
      NSM_SET_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_VLAN_ID);
      msg->vlan_id = vc->ms_pw->vlan_id;

      /* Send ifmtu */
      if (vc->ms_pw->mtu > 0)
        {
          NSM_SET_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_IF_MTU);
          msg->ifmtu = vc->ms_pw->mtu;
        }
      else
        return NSM_FAILURE;
    }
  return NSM_SUCCESS;
}

/**@brief Function to clean up the vc member.
    @param *nm - pointer to the nsm master global.
    @param *vc - poiner to the vc to be deleted.
    return     - NSM_FAILURE/NSM_SUCCESS.
*/
int
nsm_mpls_pw_stitch_cleanup_vc (struct nsm_master *nm,
                               struct nsm_mpls_circuit *vc)
{
  struct nsm_mpls_circuit_container *vc_container = NULL;

  if ((!nm) || (!vc))
    return NSM_FAILURE;
        
  /* If stitched vc's are not created, delete the vc container */
  if (FLAG_ISSET (vc->flags, NSM_MPLS_VC_FLAG_CONFIGURED))
    {
      vc->ms_pw = NULL;
      vc->vc_other = NULL;
    }
  else
    {
      vc_container = nsm_mpls_vc_lookup_by_name (nm, vc->name);

      XFREE (MTYPE_TMP, vc->name);
      XFREE (MTYPE_NSM_VIRTUAL_CIRCUIT, vc);

      if (vc_container)
        {
          nsm_mpls_vc_remove_from_hash (nm, vc_container->name);
          XFREE (MTYPE_NSM_VC_CONTAINER, vc_container);
        }
    }
  return NSM_SUCCESS;
}

/**@brief Function to clean up the vc member of ms_pw stitching instance.
    @param *nm - pointer to the nsm master global.
    @param *vc - poiner to the vc to be deleted.
    return     - NSM_FAILURE/NSM_SUCCESS.
*/
int 
nsm_mpls_pw_stitch_delete_vc (struct nsm_master *nm, 
                              struct nsm_mpls_circuit *vc)
{
  int ret = NSM_SUCCESS;

  if ((!nm) || (!vc) || (!vc->vc_other))
    return NSM_FAILURE;

  /* Clean up the switching vcfib and vc_info of the stitched vcs */
  ret = nsm_mpls_ms_pw_deactivate (nm, vc);
  if (ret < 0)
    {
      zlog_err (nzg, "%s-%d: Failed to delete MSPW and vc %d\n",
                __FUNCTION__, __LINE__, vc->id);
      return ret;
    }

  /* Remove MS-PW name from the hash table */
  nsm_ms_pw_remove_from_hash (nm, vc->ms_pw);

  ret = nsm_mpls_pw_stitch_cleanup_vc (nm, vc->vc_other);
  if (ret < 0)
    {
      zlog_err (nzg, "%s-%d: Failed to nsm_mpls_pw_stitch_cleanup_vc",
                __FUNCTION__, __LINE__, vc->vc_other->id);
      return ret;
    }

  ret = nsm_mpls_pw_stitch_cleanup_vc (nm, vc);
  if (ret < 0)
    {
      zlog_err (nzg, "%s-%d: Failed to nsm_mpls_pw_stitch_cleanup_vc",
                __FUNCTION__, __LINE__, vc->id);
      return ret;
    }

  return NSM_SUCCESS;
}

/**@brief Helper Function to clean up the vc_info for MSPW VCs.
    @param *nm - pointer to the nsm master global.
    @param *vc - pointer to the vc to be cleaned.
    return     - NSM_FAILURE/NSM_SUCCESS.
*/
int
nsm_mpls_ms_pw_vc_info_cleanup (struct nsm_master *nm,
                                struct nsm_mpls_circuit *vc)
{
  struct nsm_mpls_if *mif = NULL;

  if (!vc || !nm)
    return NSM_FAILURE;

  /* Clean up the vc_info, since it will be learnt
   * again from the LDP, on a fib create.
   */
  if ((vc->vc_info) && (vc->vc_info->mif))
    {
      mif = vc->vc_info->mif;

      if (mif)
        {
          if (LISTCOUNT (mif->vc_info_list) == 1)
            {
              if (vc->vc_info->vlan_id != 0)
                UNSET_FLAG (vc->vc_info->mif->ifp->bind,
                    NSM_IF_BIND_MPLS_VC_VLAN);
              else
                UNSET_FLAG (mif->ifp->bind, NSM_IF_BIND_MPLS_VC);
            }

          nsm_mpls_vc_info_del (mif, mif->vc_info_list, vc->vc_info);
          nsm_mpls_vc_info_free (vc->vc_info);
        }
    }
  return NSM_SUCCESS;
}

/**@brief Helper Function to clean up the MSPW related information from 
          vc member.
    @param *nm - pointer to the nsm master global.
    @param *vc - poiner to the vc to be deleted.
    return     - NSM_FAILURE/NSM_SUCCESS.
*/
int 
nsm_mpls_ms_pw_cleanup (struct nsm_master *nm,
                        struct nsm_mpls_circuit *vc,
                        s_int32_t del_vc_fib)
{
  struct nsm_mpls_if *mif = NULL;

  if ((!nm) || (!vc))
    return NSM_FAILURE;

  /* If the vc is not physically configured return a success */
  if (!FLAG_ISSET (vc->flags, NSM_MPLS_VC_FLAG_CONFIGURED))
    return NSM_SUCCESS;

  /* Clean up the vc fib */
  if (vc->vc_fib)
    nsm_mpls_vc_fib_cleanup (nm, vc, del_vc_fib);

  if ((vc->vc_info) && (vc->vc_info->mif))
    mif = vc->vc_info->mif;

  if (!mif)
    {
      if (IS_NSM_DEBUG_EVENT)
        zlog_err (NSM_ZG, "%s -%d: mif NULL for vcid %d\n",
                  __FUNCTION__, __LINE__, vc->id);
      return NSM_FAILURE;
    }

  if (vc->state >= NSM_MPLS_L2_CIRCUIT_ACTIVE)
    {
      nsm_mpls_if_withdraw_vc_data (mif, vc, NSM_FALSE);
      vc->state = NSM_MPLS_L2_CIRCUIT_DOWN;
    }

  if (LISTCOUNT (mif->vc_info_list) == 1)
    {
      if (vc->vc_info->vlan_id != 0)
        UNSET_FLAG (vc->vc_info->mif->ifp->bind, NSM_IF_BIND_MPLS_VC_VLAN);
      else
        UNSET_FLAG (mif->ifp->bind, NSM_IF_BIND_MPLS_VC);
    }

  /* Delete vc_info from vc_info_list. */
  if (NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "%s-%d: For vc %d, to delete vcInfo: %x\n",
                __FUNCTION__, __LINE__, vc->id, vc->vc_info);
  nsm_mpls_vc_info_del (mif, mif->vc_info_list, vc->vc_info);
  nsm_mpls_vc_info_free (vc->vc_info);

  return NSM_SUCCESS;
}

/**@brief Function to clean up the MSPW related information from vc member
    @param *nm - pointer to the nsm master global.
    @param *vc - poiner to the vc to be deleted.
    return     - NSM_FAILURE/NSM_SUCCESS.
*/
int
nsm_mpls_ms_pw_deactivate (struct nsm_master *nm,
                           struct nsm_mpls_circuit *vc)
{
  int ret = NSM_SUCCESS;

  if ((!nm) || (!vc))
    return NSM_FAILURE;

  /* Clean up the switching peer vc's fib */
  /* If the vc other is manual, the FIB should not be deleted since the
   * user provided data will be lost.
   */
  if (vc->vc_other->fec_type_vc == PW_OWNER_MANUAL)
    ret = nsm_mpls_ms_pw_cleanup (nm, vc->vc_other, NSM_FALSE);
  else
    ret = nsm_mpls_ms_pw_cleanup (nm, vc->vc_other, NSM_TRUE);
  if (ret < 0)
    {
       zlog_info (NSM_ZG, "%s - %d: Cannot clean vc%d\n", 
                  __FUNCTION__, __LINE__, vc->vc_other->id);
    }
  
  /* Clean up the vc's fib */
  ret = nsm_mpls_ms_pw_cleanup (nm, vc, NSM_TRUE);
  if (ret < 0)
    {
       zlog_info (NSM_ZG, "%s - %d: Cannot clean vc%d\n", 
                  __FUNCTION__, __LINE__, vc->id);
    }

  /* Stitching is equivalent to binding. Set state of VC */
  vc->state = NSM_MPLS_L2_CIRCUIT_DOWN;
  vc->vc_other->state = NSM_MPLS_L2_CIRCUIT_DOWN;

  return NSM_SUCCESS;
}

/**@brief Function to send the MSPW add/del/update message to nsm clients
          that have subscribed for the MPLS VC service.
    @param *nm  - pointer to the nsm master global.
    @param *msg - pointer to nsm_msg_mpls_ms_pw_msg.
    return      - NSM_FAILURE/NSM_SUCCESS.
*/
int
nsm_mpls_ms_pw_send_msg (struct nsm_master *nm, 
                         struct nsm_msg_mpls_ms_pw_msg *msg)
{
  int i = 0, len = 0;
  struct nsm_server_entry *nse = NULL;
  struct nsm_server_client *nsc = NULL;
  vrf_id_t vrf_id = 0;
  int ret = NSM_SUCCESS; 

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_MPLS_VC))
      {
        /* Set encode pointer and size.  */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode the MSPW message */
        len = nsm_encode_mpls_ms_pw_msg (&nse->send.pnt, 
                                         &nse->send.size,
                                         msg);
        if (len < 0)
          {
            zlog_err (NSM_ZG, "%s - %d: Encode Error\n", 
                __FUNCTION__, __LINE__);
            return len;
          }

        ret = nsm_server_send_message (nse, nm->vr->id, vrf_id,
              NSM_MSG_MPLS_MS_PW, 0, len);
      }
  return ret;
}

/**@brief Function to create the vc info for the VC in MSPW.
    @param *nm - pointer to NSM master global.
    @param *vc - pointer to the vc structure.
    return     - NSM_SUCCESS/NSM_FAILURE.
*/
int
nsm_mpls_ms_pw_vc_info_create (struct nsm_master *nm,
                               struct nsm_mpls_circuit *vc)
{
  struct nsm_mpls_vc_info *vc_info = NULL;
  struct nsm_mpls_if *mif = NULL;
  struct interface* nw_if = NULL;

  NSM_MPLS_GET_INDEX_PTR(PAL_FALSE, vc->vc_fib->ac_if_ix, nw_if);

  if (!nw_if)
    zlog_err (NSM_ZG, "%s-%d: Could not get nwintf of vc %d\n",
        __FUNCTION__, __LINE__, vc->id);

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (nw_if);
  if (! mif)
    return NSM_ERR_NOT_FOUND;

  vc_info = nsm_mpls_vc_info_create (vc->name, mif,
                                     vc->vlan_id ?
                                      VC_TYPE_ETH_VLAN :
                                      VC_TYPE_ETHERNET,
                                     vc->vlan_id,
                                     NSM_MPLS_VC_PRIMARY,
                                     vc->vlan_id ?
                                     NSM_IF_BIND_MPLS_VC_VLAN :
                                     NSM_IF_BIND_MPLS_VC);

  if (! vc_info)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  if (vc->vlan_id == 0)
    SET_FLAG (nw_if->bind, NSM_IF_BIND_MPLS_VC);
  else
    SET_FLAG (nw_if->bind, NSM_IF_BIND_MPLS_VC_VLAN);

  vc->vc_info = vc_info;
  vc_info->u.vc = vc;

  if (NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "%s-%d: For vc %d, created vcInfo: %x\n",
               __FUNCTION__, __LINE__, vc->id, vc_info);
  return NSM_SUCCESS;
}

/**@brief Function to prepare the MSPW message to be sent.
    @param *nm    - pointer to the NSM master global.
    @param *ms_pw - pointer to the mspw structure.
    @param is_add - to indicate an add/delete.
    @return       - NSM_SUCCESS/NSM_FAILURE.
*/
int
nsm_mpls_ms_pw_stitch_send (struct nsm_master *nm,
                            struct nsm_mpls_ms_pw *ms_pw,
                            bool_t is_add)
{
  struct nsm_msg_mpls_ms_pw_msg msg;
  int ret = NSM_SUCCESS;
  int ms_pw_role = NSM_MSPW_ROLE_ACTIVE;

  if (!nm || !ms_pw)
    return NSM_FAILURE;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mpls_ms_pw_msg));

  if (is_add)
    {
      /* Continue ONLY if both vcs are physically created */
      if ((!CHECK_FLAG (ms_pw->vc1->flags, NSM_MPLS_VC_FLAG_CONFIGURED)) ||
          (!CHECK_FLAG (ms_pw->vc2->flags, NSM_MPLS_VC_FLAG_CONFIGURED)))
        return ret;

      /* Stitching is equivalent to binding. Set state of VC */
      ms_pw->vc1->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;
      ms_pw->vc2->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;

      /* Update the VC role as passive if both segments are signaled */
      if ((ms_pw->vc1->fec_type_vc != PW_OWNER_MANUAL) &&
          (ms_pw->vc2->fec_type_vc != PW_OWNER_MANUAL))
        ms_pw_role = NSM_MSPW_ROLE_PASSIVE;

      /* check vc1 and vc2 for static, send msg to LDP if not static
       * Message content varies, based on vc1, vc2 or both being signalled
       */
      msg.ms_pw_action = NSM_MPLS_MS_PW_ADD;
      pal_strncpy (msg.ms_pw_name, ms_pw->ms_pw_name, NSM_MS_PW_NAME_LEN);

      /* Do not send to LDP if both the VCs are manually created . */
      if ((ms_pw->vc1->fec_type_vc == PW_OWNER_MANUAL) &&
          (ms_pw->vc2->fec_type_vc == PW_OWNER_MANUAL))
        return ret;

      if (ms_pw->vc1->fec_type_vc != PW_OWNER_MANUAL)
        {
          NSM_SET_CTYPE (msg.cindex, NSM_MPLS_MS_PW_VC1_INFO);
          ret = nsm_mpls_ms_pw_prepare_vc_msg (&msg.vc1_info, ms_pw->vc1,
                                               is_add, ms_pw_role);
          if (ret < 0)
            {
              zlog_err (NSM_ZG, "%s-%d: Error in vc-%d prepare msg\n",
                  __FUNCTION__, __LINE__, ms_pw->vc1->id);
              return NSM_FAILURE;
            }

          /* check if the other vc is manual 
           * - If yes, check if the fib is installed */
          if (ms_pw->vc2->fec_type_vc == PW_OWNER_MANUAL) 
            {
              if (!CHECK_FLAG (ms_pw->ms_pw_flag, 
                               NSM_MPLS_MSPW_MANUAL_VC_FIB_READY))
                return NSM_SUCCESS;
            }
        }

      if (ms_pw->vc2->fec_type_vc != PW_OWNER_MANUAL)
        {
          NSM_SET_CTYPE (msg.cindex, NSM_MPLS_MS_PW_VC2_INFO);
          ret = nsm_mpls_ms_pw_prepare_vc_msg (&msg.vc2_info, ms_pw->vc2,
                                               is_add, ms_pw_role);
          if (ret < 0)
            {
              zlog_err (NSM_ZG, "%s-%d: Error in vc-%d prepare msg\n",
                  __FUNCTION__, __LINE__, ms_pw->vc2->id);
              return NSM_FAILURE;
            }

          /* check if the other vc is manual */
          if (ms_pw->vc1->fec_type_vc == PW_OWNER_MANUAL)
            {
              if (!CHECK_FLAG (ms_pw->ms_pw_flag, 
                               NSM_MPLS_MSPW_MANUAL_VC_FIB_READY))
                return NSM_SUCCESS;
            }
        }
    }
  else /* Delete */
    {
      if ((ms_pw->vc1->fec_type_vc == PW_OWNER_MANUAL) &&
          (ms_pw->vc2->fec_type_vc == PW_OWNER_MANUAL))
        {
          return ret;
        }

      /* check vc1 and vc2 for static, send msg to LDP if not static
       * Message content varies, based on vc1, vc2 or both being signalled
       */
      msg.ms_pw_action = NSM_MPLS_MS_PW_DEL;
      pal_strncpy (msg.ms_pw_name, ms_pw->ms_pw_name, NSM_MS_PW_NAME_LEN);

      if (ms_pw->vc1->fec_type_vc != PW_OWNER_MANUAL)
        {
          NSM_SET_CTYPE (msg.cindex, NSM_MPLS_MS_PW_VC1_ID);
          msg.vcid1 = ms_pw->vc1->id;
        }

      if (ms_pw->vc2->fec_type_vc != PW_OWNER_MANUAL)
        {
          NSM_SET_CTYPE (msg.cindex, NSM_MPLS_MS_PW_VC2_ID);
          msg.vcid2 = ms_pw->vc2->id;
        }
    }

  ret = nsm_mpls_ms_pw_send_msg (nm, &msg);
  if (ret < 0)
    zlog_err (nzg, "%s-%d: Failed to send mspw msg\n",
              __FUNCTION__, __LINE__);

  return ret;
}

/**@brief Function to stitch two vcs as a part of MSPW.
    @param *nm         - pointer to NSM master
    @param *ms_pw_name - pointer to the name of the ms_pw instance
    @param *vc1_name   - name of vc1
    @param *vc2_name   - name of vc2
    @param is_add      - addition/deletion flag
    @param vlan_id     - vlan id
    @param mtu         - mtu size
    @return            - NSM_SUCCESS/NSM_FAILURE
*/
int
nsm_mpls_ms_pw_stitch_vc_instance (struct nsm_master *nm, char* ms_pw_name,
                                   char *vc_container1_name, 
                                   char* vc_container2_name,
                                   bool_t is_add, u_int32_t vlan_id,
                                   u_int32_t mtu)
{
  struct nsm_mpls_circuit_container *vc_container1 = NULL; 
  struct nsm_mpls_circuit_container *vc_container2 = NULL;
  struct nsm_mpls_ms_pw *ms_pw_new = NULL, *mspw_exist = NULL;
  int ret = NSM_SUCCESS;
  struct nsm_mpls_vc_info *vc_info = NULL;
  struct nsm_mpls_if *mif_tmp = NULL;
  struct listnode *ln = NULL, *node = NULL;

  if (pal_strcmp (vc_container1_name, vc_container2_name) == 0)
    return NSM_ERR_SAME_VC_NAME;

  if (is_add)
    {
      /* check if vc names are bound to any interfaces */
      LIST_LOOP (NSM_MPLS->iflist, mif_tmp, ln)
        {
          vc_info = NULL;
          LIST_LOOP (mif_tmp->vc_info_list, vc_info, node)
            {
              if (vc_info->vc_name &&
                  ((pal_strcmp (vc_info->vc_name, vc_container1_name) == 0) ||
                   (pal_strcmp (vc_info->vc_name, vc_container2_name) == 0)))
                return NSM_ERR_VC_ALREADY_BOUND;
            }
        }
    }

  /* check if the element exists in the global ms-pw table */
  mspw_exist = nsm_ms_pw_lookup_by_name (nm, ms_pw_name);

  if (is_add)
    {
      if (mspw_exist)
        return NSM_ERR_MS_PW_EXISTS;

      /* Create an entry into the MPLS VC hash table */
      vc_container1 = nsm_mpls_vc_get_by_name (nm, vc_container1_name);
      if (!vc_container1)
        return NSM_ERR_MEM_ALLOC_FAILURE;

      vc_container2 = nsm_mpls_vc_get_by_name (nm, vc_container2_name);
      if (!vc_container2)
        {
          /* clean up vc_container1 */
          nsm_mpls_vc_remove_from_hash (nm, vc_container1_name);
          XFREE (MTYPE_NSM_VC_CONTAINER, vc_container1);

          return NSM_ERR_MEM_ALLOC_FAILURE;
        }

      /* Create a dummy vc entry  for the just created vc hash entry */
      if (!vc_container1->vc)
        vc_container1->vc = nsm_mpls_l2_circuit_create (nm, 
                                                        vc_container1_name, 
                                                        0, NSM_FALSE, &ret);

      if (!vc_container2->vc)
        vc_container2->vc = nsm_mpls_l2_circuit_create (nm, 
                                                        vc_container2_name, 
                                                        0, NSM_FALSE, &ret);

      if (!vc_container1->vc || !vc_container2->vc)
        return NSM_ERR_MEM_ALLOC_FAILURE;

      if (vc_container1->vc->ms_pw || vc_container2->vc->ms_pw)
        {
          return NSM_ERR_MS_PW_ALREADY_STITCHED;
        }

      /* create a new element in the global ms-pw table */
      ms_pw_new = nsm_ms_pw_get_by_name (nm, ms_pw_name, &ret);

      /* Fill MS PW */
      ms_pw_new->vc1 = vc_container1->vc;
      ms_pw_new->vc2 = vc_container2->vc;

      /* Update each vc with the switching peer */
      vc_container1->vc->vc_other = vc_container2->vc;
      vc_container1->vc->ms_pw = ms_pw_new;

      vc_container2->vc->vc_other = vc_container1->vc;
      vc_container2->vc->ms_pw = ms_pw_new;

      /* Update the user provided data ONLY if one segment is
       * MANUAL and the other is signaled
       */
      if (((vc_container1->vc->fec_type_vc == PW_OWNER_MANUAL) &&
           (vc_container2->vc->fec_type_vc != PW_OWNER_MANUAL)) ||
          ((vc_container1->vc->fec_type_vc != PW_OWNER_MANUAL) &&
           (vc_container2->vc->fec_type_vc == PW_OWNER_MANUAL)))
        {
          if (!mtu)
            zlog_err (nzg, "%s-%d: Invalid config: mtu cannot be zero\n", 
                      __FUNCTION__, __LINE__);
          ms_pw_new->mtu = mtu; 
          ms_pw_new->vlan_id = vlan_id;
          ms_pw_new->user_config = NSM_TRUE;
        }

      if ((!CHECK_FLAG (vc_container1->vc->flags, 
                        NSM_MPLS_VC_FLAG_CONFIGURED)) ||
          (!CHECK_FLAG (vc_container2->vc->flags, 
                        NSM_MPLS_VC_FLAG_CONFIGURED)))
        {
          return NSM_SUCCESS;
        }

      ret = nsm_mpls_ms_pw_stitch_send (nm, ms_pw_new, is_add);
      if (ret < 0)
        zlog_err (nzg, "%s-%d: Failed to send add vc %d\n",
                  __FUNCTION__, __LINE__, vc_container1->vc->id);
    }
  else
    {
      if (!mspw_exist)
        return NSM_ERR_MS_PW_NOT_FOUND;

      vc_container1 = nsm_mpls_vc_lookup_by_name (nm, vc_container1_name);
      vc_container2 = nsm_mpls_vc_lookup_by_name (nm, vc_container2_name);

      if (!vc_container1 || !vc_container2)
        return NSM_ERR_VC_NAME_NOT_FOUND;

      /* Send to LDP only if both the VCs are actually created. */
      if ((CHECK_FLAG (vc_container1->vc->flags, 
                       NSM_MPLS_VC_FLAG_CONFIGURED)) &&
          (CHECK_FLAG (vc_container2->vc->flags, 
                       NSM_MPLS_VC_FLAG_CONFIGURED)))
        {
          ret = nsm_mpls_ms_pw_stitch_send (nm, mspw_exist, is_add);
          if (ret < 0)
            zlog_err (nzg, "%s-%d: Failed to delete vc %d\n",
                __FUNCTION__, __LINE__, vc_container1->vc->id);
        }

      ret = nsm_mpls_pw_stitch_delete_vc (nm, vc_container1->vc);
      if (ret < 0)
        zlog_err (nzg, "%s-%d: Failed to delete vc %d\n",
                  __FUNCTION__, __LINE__, vc_container1->vc->id);
    }
  return ret;
}

/**@brief API to set the SPE description name for an MSPW instance.
    @param *nm          - pointer to the NSM master global.
    @param *ms_pw_name  - pointer to the MSPW name.
    @param *ms_pw_descr - pointer to MSPW SPE descr string.
    @result             - CLI_ERROR/CLI_SUCCESS
*/
int
nsm_mpls_ms_pw_set_spe_descr (struct nsm_master *nm, char *ms_pw_name,
                              char *ms_pw_descr, bool_t is_add)
{
    struct nsm_mpls_ms_pw *mspw_exist = NULL;
    struct nsm_msg_mpls_ms_pw_msg msg;
    int ret = NSM_SUCCESS; 

  if ((!ms_pw_name) || (!ms_pw_descr))
    return CLI_ERROR;

  /* check if the element exists in the global ms-pw table */
  mspw_exist = nsm_ms_pw_lookup_by_name (nm, ms_pw_name);
  if (!mspw_exist)
   return NSM_ERR_MS_PW_NOT_FOUND;

  if (is_add)
    {
      pal_mem_set (mspw_exist->ms_pw_spe_descr, 0, 
                   NSM_MPLS_MS_PW_DESCR_LEN + 1);
      pal_strncpy (mspw_exist->ms_pw_spe_descr, 
                   ms_pw_descr, 
                   NSM_MPLS_MS_PW_DESCR_LEN);
      mspw_exist->ms_pw_spe_descr_set = NSM_TRUE;

      /* Do not send to LDP if either of the VCs is not created. */
      if ((!CHECK_FLAG (mspw_exist->vc1->flags, NSM_MPLS_VC_FLAG_CONFIGURED)) ||
          (!CHECK_FLAG (mspw_exist->vc2->flags, NSM_MPLS_VC_FLAG_CONFIGURED)))
        return ret;

      /* Send to LDP if any of the segments are signaled */
      if ((mspw_exist->vc1->fec_type_vc != PW_OWNER_MANUAL) ||
          (mspw_exist->vc2->fec_type_vc != PW_OWNER_MANUAL))
        {
          pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mpls_ms_pw_msg));
          msg.ms_pw_action = NSM_MPLS_MS_PW_ADD_SPE_DESCR;
          pal_strncpy (msg.ms_pw_name, ms_pw_name, NSM_MS_PW_NAME_LEN);
          pal_strncpy (msg.ms_pw_spe_descr, mspw_exist->ms_pw_spe_descr, 
                       NSM_MPLS_MS_PW_DESCR_LEN);
          NSM_SET_CTYPE (msg.cindex, NSM_MPLS_MS_PW_SPE_DESCR);
        }
    }
  else
    {
      if (!mspw_exist->ms_pw_spe_descr_set)
        return NSM_ERR_MS_PW_SPE_DESCR_NOT_SET;

      mspw_exist->ms_pw_spe_descr_set = NSM_FALSE;
      pal_mem_set (&mspw_exist->ms_pw_spe_descr_set, 0, NSM_MS_PW_DESCR_LEN);

      /* Send to LDP if any of the segments are signaled */
      if ((mspw_exist->vc1->fec_type_vc != PW_OWNER_MANUAL) ||
          (mspw_exist->vc2->fec_type_vc != PW_OWNER_MANUAL))
        {
          pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mpls_ms_pw_msg));
          pal_strncpy (msg.ms_pw_name, ms_pw_name, NSM_MS_PW_NAME_LEN);
          msg.ms_pw_action = NSM_MPLS_MS_PW_DEL_SPE_DESCR;
        }
      else
        {
          return ret;
        }
    }

  ret = nsm_mpls_ms_pw_send_msg (nm, &msg);
  
  return ret;
}

/**@brief API for the MSPW show detailed command.
    @param *nm         - pointer to NSM master global.
    @param *ms_pw_name - pointer to the MSPW name.
    @param *vrep       - pointer to the vrep table.
    @return            - void
*/
void
nsm_mpls_ms_pw_show_det (struct nsm_master *nm, char *ms_pw_name, 
                         struct vrep_table *vrep)
{
  struct nsm_mpls_ms_pw *mspw_exist = NULL;

  /* check if the element exists in the global ms-pw table */
  mspw_exist = nsm_ms_pw_lookup_by_name (nm, ms_pw_name);
  if (!mspw_exist)
   return;

  vrep_add (vrep, 0, 0, "VC1: \t%s \tVC2: \t%s ", 
            mspw_exist->vc1->name, mspw_exist->vc2->name);
  vrep_add (vrep, 1, 0, "id: \t%d \tid: \t%d ", 
      mspw_exist->vc1->id, mspw_exist->vc2->id);
  
  vrep_add_next_row (vrep, NULL,
                     "Endpoint: \t%r \tEndpoint: \t%r ",
                     &mspw_exist->vc1->address.u.prefix4,
                     &mspw_exist->vc2->address.u.prefix4);
                     
  vrep_add_next_row (vrep, NULL,
                     "Control Word: \t%d \tControl Word: \t%d ",
                     mspw_exist->vc1->cw, 
                     mspw_exist->vc2->cw); 
  vrep_add_next_row (vrep, NULL,
                     "VC Type: \t%s \tVC Type: \t%s ",
                     (mspw_exist->vc1->vc_info)?
                      mpls_vc_type_to_str (mspw_exist->vc1->vc_info->vc_type):
                      "N/A",
                     (mspw_exist->vc2->vc_info)?
                      mpls_vc_type_to_str (mspw_exist->vc2->vc_info->vc_type):
                      "N/A");
  vrep_add_next_row (vrep, NULL,
                     "Owner: \t%s \tOwner: \t%s ",
                     (mspw_exist->vc1->fec_type_vc == PW_OWNER_MANUAL) ?
                     "Manual":"Signaled",
                     (mspw_exist->vc2->fec_type_vc == PW_OWNER_MANUAL) ?
                     "Manual":"Signaled");
  vrep_add_next_row (vrep, NULL,
                     "Role: \t%s \tRole: \t%s ", 
                     mspw_exist->vc1->ms_pw_role ? "Passive": "Active",
                     mspw_exist->vc2->ms_pw_role ? "Passive": "Active");
  return;
}

/**@brief API for the MSPW summary show command.
    @param *bucket - pointer to the hash bucket of the MSPW hash table.
    @param *vrep   - pointer to vrep.
    @return        - void.
*/
void
nsm_mpls_ms_pw_show (struct hash_backet *bucket, struct vrep_table *vrep)
{
  struct nsm_mpls_ms_pw *ms_pw = NULL;

  if (!bucket)
    {
      return;
    }
  ms_pw = (struct nsm_mpls_ms_pw *) bucket->data;

  if (ms_pw)
    {
      vrep_add_next_row (vrep, NULL,
                         "%s \t %s \t %d \t %s \t %d",
                         ms_pw->ms_pw_name,
                         ms_pw->vc1->name, ms_pw->vc1->id,
                         ms_pw->vc2->name, ms_pw->vc2->id);
    }
  return;
}

/**@brief Function to display the MSPW VC details.
    @param *vc  - pointer to the vc of an MSPW.
    @param *cli - pointer to the cli structure.
    @return     - void.
*/
void
nsm_mpls_ms_pw_show_vc_det (struct nsm_mpls_circuit *vc,
                            struct cli *cli)
{
  struct gmpls_gen_label tmp_lbl;

  if (!cli || !vc)
    return;

  cli_out (cli, "%s\t", vc->name);

  if (vc->vlan_id)
    cli_out (cli, " %d\t\t", vc->vlan_id);
  else
    cli_out (cli, " %s\t\t", "N/A");

  if (vc->vc_fib)
    cli_out (cli, " %d\t",  vc->vc_fib->in_label);
  else 
    cli_out (cli, " %s\t", "-");

  if (vc->vc_fib) 
    cli_out (cli, " %s\t\t", vc->vc_fib->vc_fib_data_temp.if_in.if_name);
  else 
    cli_out (cli, " %s\t\t", "-");

  if ((vc->vc_other) && (vc->vc_other->vc_fib) &&
      (vc->vc_other->vc_fib->out_label > 0))
    cli_out (cli, " %d\t\t",  vc->vc_other->vc_fib->out_label);
  else
    cli_out (cli, " %s\t\t", "-");

  if ((vc->vc_fib) && (vc->vc_fib->install_flag == NSM_TRUE))
    cli_out (cli, " %s\t\t", "Active");
  else if (vc->state == NSM_MPLS_L2_CIRCUIT_UP &&
      ! IS_NSM_CODE_PW_FAULT (vc->pw_status) &&
      ! IS_NSM_CODE_PW_FAULT (vc->remote_pw_status))
    cli_out (cli, " %s\t\t", "Standby");
  else
    cli_out (cli, " %s\t\t", "Inactive");

  if ((vc->vc_other) && (vc->vc_other->ftn) && (FTN_XC (vc->vc_other->ftn))
      && (FTN_NHLFE (vc->vc_other->ftn)))
    {
      pal_mem_set (&tmp_lbl, 0, sizeof (struct gmpls_gen_label));
      gmpls_nhlfe_outgoing_label (FTN_NHLFE (vc->vc_other->ftn), &tmp_lbl);

      cli_out (cli, "%d", tmp_lbl.u.pkt);
    }
  else
    cli_out (cli, "%s", "N/A");

  return;
}

/**@brief Function to display the vc table of a MSPW.
    @param *nm         - pointer to NSM master global structure.
    @param *cli        - pointer to cli context.
    @param *ms_pw_name - pointer to MSPW name.
    @return            - void.
*/
void
nsm_mpls_ms_pw_show_vc_tab (struct nsm_master *nm, struct cli *cli,
                            char *ms_pw_name)
{
  struct nsm_mpls_ms_pw *mspw_exist = NULL;

  /* check if the element exists in the global ms-pw table */
  mspw_exist = nsm_ms_pw_lookup_by_name (nm, ms_pw_name);
  if (!mspw_exist)
    return;

  cli_out (cli, 
        "In VC\t vlan-id\t in-lbl\t nw-intf\t out-lbl\t status\t tunnel-lbl\n");
  
  nsm_mpls_ms_pw_show_vc_det (mspw_exist->vc1, cli);
  cli_out (cli, "\n");
  nsm_mpls_ms_pw_show_vc_det (mspw_exist->vc2, cli);

  return;
}

/**@brief A wrapper function for vrep for all MSPW commands.
    @param usr_ref - user context.
    @param *str    - pointer to a string.
*/
s_int16_t
nsm_mpls_ms_pw_vrep_show (intptr_t usr_ref, char *str)
{
  struct cli *cli = (struct cli *)usr_ref;

  if (! cli)
    return VREP_ERROR;

  cli_out (cli, "%s", str);
  return VREP_SUCCESS;
}

#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */
