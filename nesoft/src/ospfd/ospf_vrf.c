/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "thread.h"
#include "prefix.h"
#include "vty.h"
#include "log.h"
#include "table.h"
#include "nsm_client.h"
#include "nsm_message.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_cli.h"
#include "ospfd/ospf_vrf.h"
#ifdef HAVE_MPLS
#include "ospfd/ospf_mpls.h"
#endif /* HAVE_MPLS */

struct ospf_vrf *
ospf_vrf_new (struct apn_vrf *iv)
{
  struct ospf_vrf *ov;

  ov = XCALLOC (MTYPE_OSPF_VRF, sizeof (struct ospf_vrf));
  if (ov == NULL)
    return NULL;

  ov->redist_table = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);
  ov->igp_lsp_table = route_table_init ();
  ov->ospf = list_new ();
  ov->iv = iv;
  iv->proto = ov;

  return ov;
}

void
ospf_vrf_free (struct ospf_vrf *ov)
{
  XFREE (MTYPE_OSPF_VRF, ov);
}

struct ospf_vrf *
ospf_vrf_get (struct ospf_master *om, char *name)
{
  struct ospf_vrf *ov;
  struct apn_vrf *iv;

  iv = apn_vrf_get_by_name (om->vr, name);
  if (iv == NULL)
    return NULL;

  if (iv->proto != NULL)
    return iv->proto;

  ov = ospf_vrf_new (iv);
  if (ov == NULL)
    return NULL;

  return ov;
}

int
ospf_vrf_add (struct apn_vrf *iv)
{
  if (iv->proto == NULL)
    ospf_vrf_new (iv);

  return 0;
}

int
ospf_vrf_delete (struct apn_vrf *iv)
{
  struct ospf_vrf *ov = iv->proto;
  struct ospf_redist_info *ri;
  struct listnode *node, *next;
  struct ls_node *rn;
  struct ospf *top;

  if (ov == NULL)
    return 0;

  /* OSPF instances. */
  for (node = LISTHEAD (ov->ospf); node; node = next)
    {
      top = GETDATA (node);
      next = node->next;

      ospf_proc_destroy (top);
    }
  LIST_DELETE (ov->ospf);

  /* Remove redistribute informations.  */
  for (rn = ls_table_top (ov->redist_table); rn; rn = ls_route_next (rn))
    if ((ri = RN_INFO (rn, RNI_DEFAULT)))
      {
        ospf_redist_info_free (ri);
        RN_INFO_UNSET (rn, RNI_DEFAULT);
      }
  TABLE_FINISH (ov->redist_table);

  if (ov->igp_lsp_table)
    {
#ifdef HAVE_MPLS
      ospf_igp_shortcut_lsp_delete_all (ov);
#endif /* HAVE_MPLS */
      route_table_finish (ov->igp_lsp_table);
      ov->igp_lsp_table = NULL;
    }

  ospf_vrf_free (ov);

  iv->proto = NULL;

  return 0;
}

int
ospf_vrf_update (struct apn_vrf *iv)
{
  struct ospf_vrf *ov = iv->proto;
  struct listnode *node;
  struct ospf *top;

  if (ov != NULL)
    LIST_LOOP (ov->ospf, top, node)
      {
        ospf_proc_down (top);
        ospf_proc_up (top);
      }

  return 0;
}

int
ospf_vrf_update_router_id (struct apn_vrf *iv)
{
  struct ospf_vrf *ov = iv->proto;
  struct listnode *node;
  struct ospf *top;

  if (ov != NULL)
    LIST_LOOP (ov->ospf, top, node)
      ospf_router_id_update (top);

  return 0;
}

struct ospf_vrf *
ospf_vrf_lookup_by_id (struct ospf_master *om, vrf_id_t id)
{
  struct apn_vrf *iv;

  iv = apn_vrf_lookup_by_id (om->vr, id);
  if (iv != NULL)
    return iv->proto;

  return NULL;
}

struct ospf_vrf *
ospf_vrf_lookup_by_name (struct ospf_master *om, char *name)
{
  struct apn_vrf *iv;

  iv = apn_vrf_lookup_by_name (om->vr, name);
  if (iv != NULL)
    return iv->proto;

  return NULL;
}


#ifdef HAVE_VRF_OSPF
/* CLI. */
int
ospf_vrf_show_vrf_brief (struct cli *cli, struct apn_vrf *iv)
{
  struct interface *ifp;
  struct route_node *rn;

  cli_out (cli, "%-16s ", iv->name ? iv->name : "(null)");

  for (rn = route_top (iv->ifv.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      cli_out (cli, "%s ", ifp->name);

  cli_out (cli, "\n");

  return 0;
}

CLI (ospf_show_ip_vrf,
     ospf_show_ip_vrf_cmd,
     "show ip vrf",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_VRF_STR)
{
  struct apn_vr *vr = cli->vr;
  struct apn_vrf *iv;
  int i;

  cli_out (cli, "Name             Interfaces\n");

  for (i = 1; i < vector_max (vr->vrf_vec); i++)
    if ((iv = vector_slot (vr->vrf_vec, i)))
      ospf_vrf_show_vrf_brief (cli, iv);

  return CLI_SUCCESS;
}

CLI (ospf_show_ip_vrf_name,
     ospf_show_ip_vrf_name_cmd,
     "show ip vrf WORD",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
{
  /* Nothing done here. */
  return CLI_SUCCESS;
}

/* "router ospf" for VRF OSPF. */
CLI (router_ospf_id_vrf,
     router_ospf_id_vrf_cmd,
     "router ospf <1-65535> WORD",
     "Enable a routing process",
     "Start OSPF configuration",
     "OSPF process ID",
     "VRF Name to associate with this instance")
{
  struct ospf_master *om = cli->vr->proto;
  int ospf_id;
  int ret;

  CLI_GET_INTEGER_RANGE ("OSPF process ID", ospf_id, argv[0], 0, 65535);

  ret = ospf_process_set_vrf (cli->vr->id, ospf_id, argv[1]);
  if (ret == CLI_SUCCESS)
    {
      cli->mode = OSPF_MODE;
      cli->index = ospf_proc_lookup (om, ospf_id);
    }

  return ospf_cli_return (cli, ret);
}

#define OSPF_CLI_DOMAIN_TYPE_CMD "(type-as|type-as4|type-back-comp)"

#define OSPF_CLI_DOMIAN_TYPE_STR              \
   "Type 0x0005",                           \
   "Type 0x0205",                           \
   "Type 0x8005"

CLI (ospf_domain_id,
     ospf_domain_id_cmd,
     "domain-id ((A.B.C.D (|secondary))|(type" OSPF_CLI_DOMAIN_TYPE_CMD "value HEX_DATA (|secondary))| NULL)",
     "domain-id for ospf process",
     "OSPF domain ID in IP address format",
     "Secondary Domain-ID",
     "OSPF domainID type in Hex format",
     OSPF_CLI_DOMIAN_TYPE_STR,
     "OSPF domain ID value in Hex format",
     "OSPF domain ID ext. community value in Hex",
     "Secondary Domain-ID",
     "NULL Domain-ID")
{
  struct ospf *top = cli->index;
  int ret = 0;
  bool_t is_primary = PAL_FALSE;

  if (!CHECK_FLAG (top->ov->flags,OSPF_VRF_ENABLED))
    {
     cli_out (cli,
         "%%Domain-Id cannot be configured on non-VRF-OSPF instance\n");
     return CLI_ERROR;
    }

  if (argv[0][0] != 't') 
    { 
      if (argv[0][0] != 'N')
        {
          if (argc == 1)
            is_primary = PAL_TRUE;

          ret = ospf_domain_id_set (cli->vr->id, top->ospf_id, NULL,
                                     argv[0], is_primary);
          if (ret != OSPF_API_SET_SUCCESS)
            return ospf_cli_return (cli,ret);

          if ( argc == 1)
            {
              cli_out (cli,
                "Use \"clear ip ospf process\" command to take effect\n");
              return CLI_ERROR;
            }
        }
      else if (!pal_strcmp (argv[0], "NULL"))
        {
          ret = ospf_domain_id_set (cli->vr->id, top->ospf_id, NULL,
                                     argv[0], is_primary);
          if (ret != OSPF_API_SET_SUCCESS)
            return ospf_cli_return (cli, ret);
        }
     
    }
  else if (argv[0][0] == 't')
    {
      if (argc == 4)
        is_primary = PAL_TRUE;

      if (!pal_strcmp (argv[1],"type-as") 
           || !pal_strcmp (argv[1],"type-as4") 
           || !pal_strcmp (argv[1], "type-back-comp"))
        {
          if (pal_strlen (argv[3]) < 12)
            {
              cli_out (cli, "%% enter 6 btye domain-id value\n");
              return CLI_ERROR;
            }

          ret = ospf_domain_id_set (cli->vr->id, top->ospf_id, argv[1],
                                      argv[3], is_primary);
          if (ret != OSPF_API_SET_SUCCESS)
            return ospf_cli_return (cli, ret);

          if ((argc == 4))
            {
              cli_out (cli, 
                   "Use \"clear ip ospf process\" command to take effect\n");
              return CLI_ERROR;
            }
        }
      else 
        {
          cli_out (cli,"%%Inavlid type value\n");
          return CLI_ERROR;
        }
    }

   return CLI_SUCCESS;
}

CLI (no_ospf_domain_id,
     no_ospf_domain_id_cmd,
     "no domain-id ((A.B.C.D (|secondary))|(type" OSPF_CLI_DOMAIN_TYPE_CMD "value HEX_DATA (|secondary))|NULL)",
     CLI_NO_STR,
     "domain-id for ospf process",
     "OSPF domain ID in IP address format",
     "Secondary Domain-ID",
     "OSPF domainID type in Hex format",
     OSPF_CLI_DOMIAN_TYPE_STR,
     "OSPF domain ID value in Hex format",
     "OSPF domain ID ext. community value in Hex",
     "Secondary Domain-ID",
     "NULL Domain-ID")
{
  struct ospf *top = cli->index;
  int ret = 0;
  bool_t is_primary = PAL_FALSE;

  if (!CHECK_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_SEC)&&
      !CHECK_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_PRIMARY))
    {
      cli_out (cli, "%% Domain-id not configured\n");
      return CLI_ERROR;
    }

  if (argv[0][0] != 't')
    {
      if (argv[0][0] != 'N')
        {
          if (argc == 1)
            is_primary = PAL_TRUE;

          ret = ospf_domain_id_unset (cli->vr->id, top->ospf_id, NULL,
                                       argv[0], is_primary);
          if (!ret && !CHECK_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_PRIMARY))
            {
              cli_out (cli,
                    "Use \"clear ip ospf process\" command to take effect\n");
              return CLI_ERROR;
            }
        }
      else if (!pal_strcmp (argv[0], "NULL"))
        {
          ret = ospf_domain_id_unset (cli->vr->id, top->ospf_id, NULL,
                                      argv[0], is_primary);
          if (!ret)
            {
              cli_out (cli,
                   "Use \"clear ip ospf process\" command to take effect\n");
              return CLI_ERROR;
            }
        }
    }
  else if (argv[0][0] == 't')
    {
      if (argc == 4)
        is_primary = PAL_TRUE;

      if (!pal_strcmp (argv[1],"type-as") 
           || !pal_strcmp (argv[1],"type-as4") 
           || !pal_strcmp (argv[1], "type-back-comp"))
        {
          if (pal_strlen (argv[3]) < 12)
            {
              cli_out (cli, "%% enter 6 btye domain-id value\n");
              return CLI_ERROR;
            }
          ret = ospf_domain_id_unset (cli->vr->id, top->ospf_id, argv[1],
                                      argv[3], is_primary);
          if (!ret && !CHECK_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_PRIMARY))
              {
                cli_out (cli,
                    "Use \"clear ip ospf process\" command to take effect\n");
                return CLI_ERROR;
              }
        }
      else
        {
          cli_out (cli, "%% Invalid type value\n");
          return CLI_ERROR;
        }
    }
  if (ret != OSPF_API_SET_SUCCESS)
     return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}
#endif /* HAVE_VRF_OSPF */

void
ospf_vrf_init (struct lib_globals *zg)
{
  /* Set callbacks. */
  apn_vrf_add_callback (zg, VRF_CALLBACK_ADD, ospf_vrf_add);
  apn_vrf_add_callback (zg, VRF_CALLBACK_DELETE, ospf_vrf_delete);
  apn_vrf_add_callback (zg, VRF_CALLBACK_UPDATE, ospf_vrf_update);
  apn_vrf_add_callback (zg, VRF_CALLBACK_ROUTER_ID, ospf_vrf_update_router_id);

#ifdef HAVE_VRF_OSPF
  /* Install CLIs. */
  cli_install_gen (zg->ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_show_ip_vrf_cmd);
  cli_install_gen (zg->ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_show_ip_vrf_name_cmd);
  cli_install_gen (zg->ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &router_ospf_id_vrf_cmd);
  cli_install_gen (zg->ctree, OSPF_MODE,  PRIVILEGE_NORMAL, 0,
                   &ospf_domain_id_cmd);
  cli_install_gen (zg->ctree, OSPF_MODE,  PRIVILEGE_NORMAL, 0,
                   &no_ospf_domain_id_cmd);
  
#endif /* HAVE_VRF_OSPF */
}
