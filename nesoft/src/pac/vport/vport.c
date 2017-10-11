/******************************************************************************
 *
 *  pac/vport/vport.c
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Ku Ying <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  2012.2.11 创建
 *  2012.4.27 整合ac pw tun为vport
 *
 */

#include "pal.h"
#include "nsmd.h"
#include "vport.h"


static struct vport_master s_vport;

UINT32 vport_clac_if_index(enum em_intf_type type, UINT16 id)
{
    if (!IS_INTF_VALID(type, id))
    {
        return UINT32_MAX;
    }

    if (INTF_FE == type)
    {
        return (id + 1);
    }

    if (INTF_GE == type)
    {
        return (id + 25);
    }

    return UINT32_MAX;
}

void vport_master_init()
{
    pal_mem_set(&s_vport, 0, sizeof(struct vport_master));
    s_vport.ac = list_new();
    s_vport.pw = list_new();
    s_vport.tun = list_new();
    s_vport.port = list_new();

    return;
}

struct vport_ac *vport_new_ac(UINT32 index)
{
	struct vport_ac *ac;

    if (!IS_VPORT_INDEX_VALID(index))
    {
        return NULL;
    }

    ac = XMALLOC(MTYPE_NSM_AC, sizeof (struct vport_ac));
    pal_mem_set(ac, 0, sizeof (struct vport_ac));

    ac->index = index;
    ac->car.is_enable = FALSE;

	/* wmx@150125 在新建ac时使priority命令的is_enable为FALSE */
	ac->priority_enable = FALSE;

    listnode_add(s_vport.ac, ac);
    s_vport.last_change_time = pal_time_current(NULL);

    zlog_info(NSM_ZG, "add ac %d successfully.", index);

    return ac;
}

struct vport_pw *vport_new_pw(UINT32 index)
{
    struct vport_pw *pw;

    if (!IS_VPORT_INDEX_VALID(index))
    {
        return NULL;
    }

    pw = XMALLOC (MTYPE_NSM_PW, sizeof (struct vport_pw));
    pal_mem_set (pw, 0, sizeof (struct vport_pw));

    pw->index = index;
    pw->car.is_enable = FALSE;

    listnode_add(s_vport.pw, pw);
    s_vport.last_change_time = pal_time_current(NULL);

    zlog_info(NSM_ZG, "add pw %d successfully.", index);
    return pw;
}

struct vport_tun *vport_new_tun(UINT32 index)
{
    struct vport_tun *tun;

    if (!IS_VPORT_INDEX_VALID(index))
    {
        return NULL;
    }

    tun = XMALLOC (MTYPE_NSM_TUN, sizeof (struct vport_tun));
    pal_mem_set (tun, 0, sizeof (struct vport_tun));

    tun->index = index;
    tun->car.is_enable = FALSE;

    listnode_add (s_vport.tun, tun);
    s_vport.last_change_time = pal_time_current(NULL);

    zlog_info (NSM_ZG, "add tunnel %d successfully.", index);
    return tun;
}

struct vport_port *vport_new_port(UINT32 index)
{
	struct vport_port*port;

    if (!IS_PORT_INDEX_VALID(index))
    {
        return NULL;
    }

    port = XMALLOC(MTYPE_NSM_IF, sizeof (struct vport_port));
    pal_mem_set(port, 0, sizeof (struct vport_port));

    port->index = index;
    port->rate_limit.is_enable = FALSE;
	port->shapping.is_enable = FALSE;
	
	/* wmx@150125 在新建port时使rgress-car、wred、weight三条命令的is_enable为FALSE */
    port->queue.car[0].is_enable = FALSE;
	port->queue.car[1].is_enable = FALSE;
	port->queue.car[2].is_enable = FALSE;
	port->queue.car[3].is_enable = FALSE;
	port->queue.car[4].is_enable = FALSE;
    port->queue.car[5].is_enable = FALSE;
    port->queue.car[6].is_enable = FALSE;
    port->queue.car[7].is_enable = FALSE;
	port->queue.weight.is_enable = FALSE;
	port->queue.wred[0].is_enable = FALSE;
	port->queue.wred[1].is_enable = FALSE;
	port->queue.wred[2].is_enable = FALSE;
	port->queue.wred[3].is_enable = FALSE;
	port->queue.wred[4].is_enable = FALSE;
	port->queue.wred[5].is_enable = FALSE;
	port->queue.wred[6].is_enable = FALSE;
	port->queue.wred[7].is_enable = FALSE;
	
	
    listnode_add(s_vport.port, port);
    s_vport.last_change_time = pal_time_current(NULL);

    zlog_info(NSM_ZG, "add port %d successfully.", index);

    return port;
}


void vport_delete_ac(struct vport_ac *ac)
{
    listnode_delete(s_vport.ac, ac);
    s_vport.last_change_time = pal_time_current(NULL);

	zlog_info (NSM_ZG, "delete ac %d successfully.", ac->index);
	XFREE (MTYPE_NSM_AC, ac);

    return;
}

void vport_delete_pw(struct vport_pw *pw)
{
    listnode_delete(s_vport.pw, pw);
    s_vport.last_change_time = pal_time_current(NULL);

    zlog_info (NSM_ZG, "delete pw %d successfully.", pw->index);
    XFREE (MTYPE_NSM_PW, pw);

    return;
}

void vport_delete_tun(struct vport_tun *tun)
{
    listnode_delete (s_vport.tun, tun);
    s_vport.last_change_time = pal_time_current(NULL);

    zlog_info (NSM_ZG, "delete tunnel %d successfully.", tun->index);
    XFREE (MTYPE_NSM_TUN, tun);

    return;
}

void vport_delete_port(struct vport_port*port)
{
    listnode_delete(s_vport.port, port);
    s_vport.last_change_time = pal_time_current(NULL);

	zlog_info (NSM_ZG, "delete port %d successfully.", port->index);
	XFREE (MTYPE_NSM_IF, port);

    return;
}

struct vport_ac *vport_find_ac_by_index(int index)
{
	struct vport_ac *ac;
	struct listnode *node;

    if (IS_VPORT_INDEX_VALID(index))
    {
    	LIST_LOOP(s_vport.ac, ac, node)
    	{
            if (ac->index == index)
            {
                return ac;
            }
    	}
    }

	return NULL;
}

struct vport_pw *vport_find_pw_by_index(int index)
{
	struct vport_pw *pw;
	struct listnode *node;

    if (IS_VPORT_INDEX_VALID(index))
    {
    	LIST_LOOP(s_vport.pw, pw, node)
    	{
            if (pw->index == index)
            {
                return pw;
            }
    	}
    }

	return NULL;
}

struct vport_tun *vport_find_tun_by_index(int index)
{
	struct vport_tun *tun;
	struct listnode *node;

    if (IS_VPORT_INDEX_VALID(index))
    {
    	LIST_LOOP (s_vport.tun, tun, node)
    	{
            if (tun->index == index)
            {
                return tun;
            }
    	}
    }

	return NULL;
}

struct vport_port *vport_find_port_by_index(int index)
{
	struct vport_port *port;
	struct listnode *node;

    if (IS_PORT_INDEX_VALID(index))
    {
    	LIST_LOOP(s_vport.port, port, node)
    	{
            if (port->index == index)
            {
                return port;
            }
    	}
    }

	return NULL;
}

void vport_foreach_ac(ac_foreach_func func, void *para)
{
	struct listnode *node;
	struct listnode *next;

    if (NULL != func)
    {
        for (node = s_vport.ac->head; node; node = next)
        {
            next = node->next;

            if (NULL != node->data)
                (*func)(node->data, para);
        }
    }

	return;
}

void vport_foreach_pw(pw_foreach_func func, void *para)
{
	struct listnode *node;
	struct listnode *next;

    if (NULL != func)
    {
        for (node = s_vport.pw->head; node; node = next)
        {
            next = node->next;

            if (NULL != node->data)
                (*func)(node->data, para);
        }
    }

	return;
}

void vport_foreach_tun(tun_foreach_func func, void *para)
{
	struct listnode *node;
	struct listnode *next;

    if (NULL != func)
    {
        for (node = s_vport.tun->head; node; node = next)
        {
            next = node->next;

            if (NULL != node->data)
                (*func)(node->data, para);
        }
    }

	return;
}

void vport_foreach_port(port_foreach_func func, void *para)
{
	struct listnode *node;
	struct listnode *next;

    if (NULL != func)
    {
        for (node = s_vport.port->head; node; node = next)
        {
            next = node->next;

            if (NULL != node->data)
                (*func)(node->data, para);
        }
    }

	return;
}

/*
 * 从cli模式中获取当前需要配置vport
 */
struct vport_ac *vport_get_current_ac(struct cli *cli)
{
	struct vport_ac *ac;

    if ((NULL == cli) || (NULL == cli->index))
    {
        return NULL;
    }

    ac = (struct vport_ac *)cli->index;

    if (ac != vport_find_ac_by_index(ac->index))
    {
        return NULL;
    }

    return ac;
}

struct vport_pw *vport_get_current_pw(struct cli *cli)
{
	struct vport_pw *pw;

    if ((NULL == cli) || (NULL == cli->index))
    {
       cli_out(cli, "%% cli = NULL.\n");
        return NULL;
    }

    pw = (struct vport_pw *)cli->index;

    if (pw != vport_find_pw_by_index(pw->index))
    {
        cli_out(cli, "%% pw != vport_find_pw_by_index(pw->index).\n");
        return NULL;
    }

    return pw;
}

struct vport_tun *vport_get_current_tun(struct cli *cli)
{
	struct vport_tun *tun;

    if ((NULL == cli) || (NULL == cli->index))
    {
        return NULL;
    }

    tun = (struct vport_tun *)cli->index;

    if (tun != vport_find_tun_by_index(tun->index))
    {
        return NULL;
    }

    return tun;
}

struct vport_port*vport_get_current_port(struct cli *cli)
{
	struct vport_port *port;

    if ((NULL == cli) || (NULL == cli->index))
    {
        return NULL;
    }

    port = (struct vport_port*)cli->index;

    if (port != vport_find_port_by_index(port->index))
    {
        return NULL;
    }

    return port;
}


