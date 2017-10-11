/* Copyright (C) 2009  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_BFD

#include "lib.h"
#include "cli.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_neighbor.h"

void
ospf_bfd_show_nbr_detail (struct cli *cli, struct ospf_neighbor *nbr)
{
  if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_BFD))
    cli_out (cli, "    Bidirectional Forwarding Detection is enabled\n");
}

void
ospf_bfd_show_interface (struct cli *cli, struct ospf_interface *oi)
{
  if (! OSPF_IF_PARAM_CHECK (oi->params_default, BFD)
      && ! OSPF_IF_PARAM_CHECK (oi->params_default, BFD_DISABLE)
      && ! CHECK_FLAG (oi->top->config, OSPF_CONFIG_BFD))
    return;

  if (OSPF_IF_PARAM_CHECK (oi->params_default, BFD_DISABLE))
    cli_out (cli, "  Bidirectional Forwarding Detection is disabled\n");
  else if (CHECK_FLAG (oi->flags, OSPF_IF_BFD))
    cli_out (cli, "  Bidirectional Forwarding Detection is enabled\n");
  else
    cli_out (cli, "  Bidirectional Forwarding Detection is configured\n");
}

void
ospf_bfd_show_instance (struct cli *cli, struct ospf *top)
{
  if (! CHECK_FLAG (top->config, OSPF_CONFIG_BFD))
    return;

  cli_out (cli, " Bidirectional Forwarding Detection is configured\n");
}

#endif /* HAVE_BFD */
