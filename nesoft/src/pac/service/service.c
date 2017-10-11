/******************************************************************************
 *
 *  pac/service/service.c
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Ku Ying <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  service(业务处理)，创建于2012.2.11
 *
 */

#include "pal.h"
#include "nsmd.h"
#include "service.h"

static struct service_master s_service;

void service_master_init()
{
    pal_mem_set(&s_service, 0, sizeof(struct service_master));
    s_service.vpn = list_new();
    s_service.lsp = list_new();
	s_service.lsp_group = list_new();
	s_service.lsp_oam = list_new();
    return;
}

struct service_vpn *service_new_vpn(UINT32 index)
{
    struct service_vpn *srv;
    srv = XMALLOC (MTYPE_NSM_SI, sizeof (struct service_vpn));
    if (NULL != srv)
    {
        pal_mem_set (srv, 0, sizeof (struct service_vpn));

        srv->index = index;
        srv->type = VPN_BUT;
        srv->enable_muticast = FALSE;
        srv->enable_mac_learn = FALSE;
        srv->vport_list = list_new();
        srv->mutigrp_list = list_new();

        listnode_add(s_service.vpn, srv);
        s_service.last_change_time = pal_time_current(NULL);

        zlog_info (NSM_ZG, "add service_vpn %d successfully.", srv->index);
    }
    return srv;
}

struct service_lsp *service_new_lsp(UINT32 id)
{
    struct service_lsp *srv;
    srv = XMALLOC (MTYPE_NSM_SI, sizeof (struct service_lsp));
    if (NULL != srv)
    {
        pal_mem_set (srv, 0, sizeof (struct service_lsp));

        srv->id = id;

        listnode_add(s_service.lsp, srv);
        s_service.last_change_time = pal_time_current(NULL);

        zlog_info (NSM_ZG, "add service_lsp %d successfully.", srv->id);
    }
    return srv;
}

struct service_lsp_group *service_new_lsp_group(UINT32 id)
{
    struct service_lsp_group *srv;
    srv = XMALLOC (MTYPE_NSM_SI, sizeof (struct service_lsp_group));
    if (NULL != srv)
    {
        pal_mem_set (srv, 0, sizeof (struct service_lsp_group));

        srv->grp_id = id;

        listnode_add(s_service.lsp_group, srv);
        s_service.last_change_time = pal_time_current(NULL);

        zlog_info (NSM_ZG, "add service_lsp_group %d successfully.", srv->grp_id);
    }
    return srv;
}

struct service_lsp_oam *service_new_lsp_oam(UINT32 id)
{
    struct service_lsp_oam *srv;
    srv = XMALLOC (MTYPE_NSM_SI, sizeof (struct service_lsp_oam));
    if (NULL != srv)
    {
        pal_mem_set (srv, 0, sizeof (struct service_lsp_oam));

        srv->oam_id = id;

        listnode_add(s_service.lsp_oam, srv);
        s_service.last_change_time = pal_time_current(NULL);

        zlog_info (NSM_ZG, "add service_lsp_group %d successfully.", srv->oam_id);
    }
    return srv;
}


void service_delete_vpn(struct service_vpn *srv)
{
    pal_assert(NULL != srv);

    list_free(srv->vport_list);
    list_free(srv->mutigrp_list);

    listnode_delete(s_service.vpn, srv);
    s_service.last_change_time = pal_time_current(NULL);

    XFREE (MTYPE_NSM_SI, srv);

    zlog_info (NSM_ZG, "delete service_vpn %d successfully.", srv->index);
    return;
}

void service_delete_lsp(struct service_lsp *srv)
{
    pal_assert(NULL != srv);

    listnode_delete(s_service.lsp, srv);
    s_service.last_change_time = pal_time_current(NULL);

    XFREE (MTYPE_NSM_SI, srv);

    zlog_info (NSM_ZG, "delete service_lsp %d successfully.", srv->id);
    return;
}

void service_delete_lspgrp(struct service_lsp_group *srv)
{
    pal_assert(NULL != srv);

    listnode_delete(s_service.lsp_group, srv);
    s_service.last_change_time = pal_time_current(NULL);

    XFREE (MTYPE_NSM_SI, srv);

    zlog_info (NSM_ZG, "delete service_lsp_group %d successfully.", srv->grp_id);
    return;
}

void service_delete_lspoam(struct service_lsp_oam *srv)
{
    pal_assert(NULL != srv);

    listnode_delete(s_service.lsp_oam, srv);
    s_service.last_change_time = pal_time_current(NULL);

    XFREE (MTYPE_NSM_SI, srv);

    zlog_info (NSM_ZG, "delete service_lsp_oam %d successfully.", srv->oam_id);
    return;
}


struct service_vpn *service_find_vpn_by_index(UINT32 index)
{
	struct service_vpn *srv;
	struct listnode *node;

	LIST_LOOP(s_service.vpn, srv, node)
	{
        if (srv->index == index)
        {
            return srv;
        }
	}

	return NULL;
}

struct vpn_vport *vpn_find_vport_by_index(struct service_vpn *srv, enum em_vport_type type, UINT32 index)
{
	struct vpn_vport *data;
	struct listnode *node;

    pal_assert(srv);

	LIST_LOOP(srv->vport_list, data, node)
	{
        if ((data->type == type) && (data->id == index))
        {
            return data;
        }
	}

	return NULL;
}

struct muticast_grp *vpn_find_mutigrp_by_index(struct service_vpn *srv, UINT32 index)
{
	struct muticast_grp *data;
	struct listnode *node;

    pal_assert(srv);

	LIST_LOOP(srv->mutigrp_list, data, node)
	{
        if (data->index == index)
        {
            return data;
        }
	}

	return NULL;
}

struct mutigrp_vport *vpnmutigrp_find_vport_by_index(struct muticast_grp *grp, enum em_vport_type type, UINT32 index)
{
	struct mutigrp_vport *data;
	struct listnode *node;

    pal_assert(grp);

	LIST_LOOP(grp->vport_list, data, node)
	{
        if ((data->type == type) && (data->id == index))
        {
            return data;
        }
	}

	return NULL;
}

struct vport_vmac *vpnvport_find_vmac(struct vpn_vport *vport, CHAR mac[])
{
	struct vport_vmac *data;
	struct listnode *node;

    pal_assert(vport);

	LIST_LOOP(vport->vmac_list, data, node)
	{
        if (0 == pal_mem_cmp(data->mac, mac, ETHER_ADDR_LEN))
        {
            return data;
        }
	}

	return NULL;
}

BOOL is_mutigrpvport_in_vpn(struct service_vpn *srv, enum em_vport_type type, UINT32 index)
{
	struct muticast_grp *data;
	struct listnode *node;

    pal_assert(srv);

	LIST_LOOP(srv->mutigrp_list, data, node)
	{
        if (NULL != vpnmutigrp_find_vport_by_index(data, type, index))
        {
            return TRUE;
        }
	}

	return FALSE;
}

BOOL is_vportvmac_in_vpn(struct service_vpn *srv, CHAR mac[])
{
	struct vpn_vport *data;
	struct listnode *node;

    pal_assert(srv);

	LIST_LOOP(srv->vport_list, data, node)
	{
        if (NULL != vpnvport_find_vmac(data, mac))
        {
            return TRUE;
        }
	}

	return FALSE;
}


struct service_lsp *service_find_lsp_by_index(UINT32 index)
{
	struct service_lsp *srv;
	struct listnode *node;

	LIST_LOOP(s_service.lsp, srv, node)
	{
        if (srv->id == index)
        {
            return srv;
        }
	}

	return NULL;
}

struct service_lsp_group *service_find_lspgrp_by_index(UINT32 index)
{
	struct service_lsp_group *srv;
	struct listnode *node;

	LIST_LOOP(s_service.lsp_group, srv, node)
	{
        if (srv->grp_id == index)
        {
            return srv;
        }
	}

	return NULL;
}

struct service_lsp_oam *service_find_lspoam_by_index(UINT32 index)
{
	struct service_lsp_oam *srv;
	struct listnode *node;

	LIST_LOOP(s_service.lsp_oam, srv, node)
	{
        if (srv->oam_id == index)
        {
            return srv;
        }
	}

	return NULL;
}



void service_foreach_vpn(vpn_foreach_func func, void *para)
{
	struct listnode *node;
	struct listnode *next;

    if (NULL != func)
    {
        for (node = s_service.vpn->head; node; node = next)
        {
            next = node->next;

            if (NULL != node->data)
                (*func)(node->data, para);
        }
    }

	return;
}

void vpn_foreach_vport(struct service_vpn* srv, vpndata_foreach_func func, void *para)
{
	struct listnode *node;
	struct listnode *next;

    if (NULL != func)
    {
        for (node = srv->vport_list->head; node; node = next)
        {
            next = node->next;

            if (NULL != node->data)
                (*func)(node->data, para);
        }
    }

	return;
}

void vpn_foreach_mutigrp(struct service_vpn* srv, vpnmutigrp_foreach_func func, void *para)
{
	struct listnode *node;
	struct listnode *next;

    if (NULL != func)
    {
        for (node = srv->mutigrp_list->head; node; node = next)
        {
            next = node->next;

            if (NULL != node->data)
                (*func)(node->data, para);
        }
    }

	return;
}

void vpnmutigrp_foreach_vport(struct muticast_grp* grp, vpnmutigrpdata_foreach_func func, void *para1, void *para2)
{
	struct listnode *node;
	struct listnode *next;

    if (NULL != func)
    {
        for (node = grp->vport_list->head; node; node = next)
        {
            next = node->next;

            if (NULL != node->data)
                (*func)(node->data, para1, para2);
        }
    }

	return;
}

void vpnvport_foreach_vmac(struct vpn_vport* vport, vpnvportdata_foreach_func func, void *para1, void *para2)
{
	struct listnode *node;
	struct listnode *next;

    if (NULL != func)
    {
        for (node = vport->vmac_list->head; node; node = next)
        {
            next = node->next;

            if (NULL != node->data)
                (*func)(node->data, para1, para2);
        }
    }

	return;
}


void service_foreach_lsp(lsp_foreach_func func, void *para)
{
	struct listnode *node;
	struct listnode *next;

    if (NULL != func)
    {
        for (node = s_service.lsp->head; node; node = next)
        {
            next = node->next;

            if (NULL != node->data)
                (*func)(node->data, para);
        }
    }

	return;
}

void service_foreach_lsp_grp(lsp_grp_foreach_func func, void *para)
{
	struct listnode *node;
	struct listnode *next;

    if (NULL != func)
    {
        for (node = s_service.lsp_group->head; node; node = next)
        {
            next = node->next;

            if (NULL != node->data)
                (*func)(node->data, para);
        }
    }

	return;
}

void service_foreach_lsp_oam(lsp_oam_foreach_func func, void *para)
{
	struct listnode *node;
	struct listnode *next;

    if (NULL != func)
    {
        for (node = s_service.lsp_oam->head; node; node = next)
        {
            next = node->next;

            if (NULL != node->data)
                (*func)(node->data, para);
        }
    }

	return;
}



/*
 * 从cli模式中获取当前需要配置的service
 */
struct service_vpn *service_get_current_vpn(struct cli *cli)
{
	struct service_vpn *srv;

    if ((NULL == cli) || (NULL == cli->index))
    {
        return NULL;
    }

    srv = (struct service_vpn *)cli->index;
    if (srv != service_find_vpn_by_index(srv->index))
    {
        return NULL;
    }

    return srv;
}
struct service_lsp *service_get_current_lsp(struct cli *cli)
{
	struct service_lsp *srv;

    if ((NULL == cli) || (NULL == cli->index))
    {
        return NULL;
    }

    srv = (struct service_lsp *)cli->index;
    if (srv != service_find_lsp_by_index(srv->id))
    {
        return NULL;
    }

    return srv;
}
struct service_lsp_group *service_get_current_lsp_grp(struct cli *cli)
{
	struct service_lsp_group *srv;

    if ((NULL == cli) || (NULL == cli->index))
    {
        return NULL;
    }

    srv = (struct service_lsp_group *)cli->index;
    if (srv != service_find_lspgrp_by_index(srv->grp_id))
    {
        return NULL;
    }

    return srv;
}

struct service_lsp_oam *service_get_current_lsp_oam(struct cli *cli)
{
	struct service_lsp_oam *srv;

    if ((NULL == cli) || (NULL == cli->index))
    {
        return NULL;
    }

    srv = (struct service_lsp_oam *)cli->index;
    if (srv != service_find_lspoam_by_index(srv->oam_id))
    {
        return NULL;
    }

    return srv;
}

