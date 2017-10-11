/* File : smi.i */
%module smi
%{

#include "smi_client.h"

struct smiclient_globals *azg;
int
smiclient_create1 (int debug, char *filename);

int
smi_bridge_add (struct smiclient_globals *azg,
                char *name, enum smi_bridge_type type,
                enum smi_bridge_topo_type topo_type);
%}

#include "smi_client.h"

struct smiclient_globals *azg;

int
smi_bridge_add (struct smiclient_globals *azg,
                char *name, enum smi_bridge_type type,
                enum smi_bridge_topo_type topo_type);
int
smiclient_create1 (int debug, char *filename);
