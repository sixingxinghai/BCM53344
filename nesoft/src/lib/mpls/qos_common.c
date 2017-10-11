/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#include "stream.h"
#include "mpls.h"
#include "qos_common.h"
#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */
#include "nsm_message.h"
/*
 * Helper function to initialize a resource structure.
 */
struct qos_resource *
qos_common_resource_new ()
{
  struct qos_resource *resource;

  /* Create resource */
  resource = XCALLOC (MTYPE_QOS_RESOURCE,sizeof (struct qos_resource));
  return resource;
}

/*
 * Helper function to deinitialize a resource structure.
 */
void
qos_common_resource_delete (struct qos_resource *resource)
{
  if (resource)
    XFREE (MTYPE_QOS_RESOURCE, resource);
}

void
qos_common_resource_init (struct qos_resource *resource)
{
  /*
   * Set all floats to negative infinity ...
   */

  /* Floats in tspec */
  resource->t_spec.peak_rate             = NEGATIVE_INFINITY;
  resource->t_spec.committed_bucket.rate = NEGATIVE_INFINITY;
  resource->t_spec.committed_bucket.size = NEGATIVE_INFINITY;
  resource->t_spec.excess_burst          = NEGATIVE_INFINITY;
  resource->t_spec.weight                = NEGATIVE_INFINITY;
  resource->t_spec.mpu                   = NEGATIVE_INFINITY;
  resource->t_spec.max_packet_size       = NEGATIVE_INFINITY;

  /* Floats in adspec */
  resource->ad_spec.slack                = NEGATIVE_INFINITY;
  resource->ad_spec.Ctot                 = NEGATIVE_INFINITY;
  resource->ad_spec.Dtot                 = NEGATIVE_INFINITY;
  resource->ad_spec.Csum                 = NEGATIVE_INFINITY;
  resource->ad_spec.Dsum                 = NEGATIVE_INFINITY;
  resource->ad_spec.mpl                  = NEGATIVE_INFINITY;
  resource->ad_spec.pbe                  = NEGATIVE_INFINITY;
}

/*
 * Helper function to populate a resource structure from a stream.
 *
 * Returns the command that was in the message.
 */
void
qos_common_resource_populate (struct stream *s,
                              struct qos_resource **resrc,
                              u_int8_t cmd)
{
  struct qos_resource *resource;
  struct qos_traffic_spec *tspec;
  struct qos_if_spec *ifspec;
  struct qos_ad_spec *adspec;
  u_int32_t tmp;

  /* Set resource pointer */
  resource = *resrc;

  /* Get client id */
  resource->resource_id = stream_getl (s);

  /* Get protocol id */
  resource->protocol_id = stream_getc (s);

  /* Get resource flags */
  resource->flags = stream_getc (s);

  /*
   * Depending on flag, put data. Data MUST be in the order
   * specified in qos_common.h.
   */

  /* Traffic spec ... */
  if (qos_resource_check_traffic_spec (resource))
    {
      /* Get pointer to tspec */
      tspec = &resource->t_spec;

      /* Put in service type */
      tspec->service_type = stream_getc (s);

      /* Put in exclusive flag */
      tspec->is_exclusive = stream_getc (s);

      /* Put in traffic spec flag */
      tspec->flags = stream_getc (s);

      /*
       * Depending on flag, put data. Data MUST be in the
       * order specified in qos_common.h.
       */

      /* Peak rate ... */
      if (qos_tspec_check_peak_rate (tspec))
        {
          tmp = stream_getl (s);
          pal_mem_cpy (&tspec->peak_rate, &tmp, 4);
        }

      /* Committed bucket ... */
      if (qos_tspec_check_committed_bucket (tspec))
        {
          /* Flag */
          tspec->committed_bucket.flags = stream_getc (s);
          
          if (qos_bucket_check_rate (&tspec->committed_bucket))
            {
              tmp = stream_getl (s);
              pal_mem_cpy (&tspec->committed_bucket.rate, &tmp, 4);
            }

          if (qos_bucket_check_size (&tspec->committed_bucket))
            {
              tmp = stream_getl (s);
              pal_mem_cpy (&tspec->committed_bucket.size, &tmp, 4);
            }
        }

      /* Excess burst ... */
      if (qos_tspec_check_excess_burst (tspec))
        {
          tmp = stream_getl (s);
          pal_mem_cpy (&tspec->excess_burst, &tmp, 4);
        }

      /* Weight of flow ... */
      if (qos_tspec_check_weight (tspec))
        {
          tmp = stream_getl (s);
          pal_mem_cpy (&tspec->weight, &tmp, 4);
        }

      /* Min policed unit ... */
      if (qos_tspec_check_min_policed_unit (tspec))
        {
          tspec->mpu = stream_getl (s);
        }

      /* Max packet size ... */
      if (qos_tspec_check_max_packet_size (tspec))
        {
          tspec->max_packet_size = stream_getl (s);
        }
    }
  
  /* Interface spec ... */
  if (qos_resource_check_if_spec (resource))
    {
      /* Get pointer to interface spec */
      ifspec = &resource->if_spec;

      /* Put in interface spec flag */
      ifspec->flags = stream_getc (s);
      
      /* Incoming if index */
      ifspec->in_ifindex = stream_getw (s);
      
      /* Outgoing if index */
      ifspec->out_ifindex = stream_getw (s);

      /* Prefixes for previous and next hop beings used ... */
      if (qos_ifspec_check_prefix (ifspec))
        {
          /* Prevous hop data ... */
          {
            /* Put in family */
            ifspec->prev_hop.family = stream_getc (s);

            /* Put in address based on family */
            if (ifspec->prev_hop.family == AF_INET)
              ifspec->prev_hop.u.prefix4.s_addr = stream_get_ipv4 (s);
#ifdef HAVE_IPV6
            else if (ifspec->prev_hop.family == AF_INET6)
              stream_get (&ifspec->prev_hop.u.prefix6, s, 16);
#endif /* #ifdef HAVE_IPV6 */
          }

          /* Next hop data ... */
          {
            /* Put in family */
            ifspec->next_hop.family = stream_getc (s);
            
            /* Put in address based on family */
            if (ifspec->next_hop.family == AF_INET)
              ifspec->next_hop.u.prefix4.s_addr = stream_get_ipv4 (s);
#ifdef HAVE_IPV6
            else if (ifspec->next_hop.family == AF_INET6)
              stream_get (&ifspec->next_hop.u.prefix6, s, 16);
#endif /* #ifdef HAVE_IPV6 */
          }
        }
    }
  
  /* Advertisement spec ... */
  if (qos_resource_check_ad_spec (resource))
    {
      /* Get adspec pointer */
      adspec = &resource->ad_spec;

      /* Put in adspec flag */
      adspec->flags = stream_getc (s);

      /* Slack term */
      if (qos_adspec_check_slack (adspec))
        {
          tmp = stream_getl (s);
          pal_mem_cpy (&adspec->slack, &tmp, 4);
        }

      /* Ctot */
      if (qos_adspec_check_ctot (adspec))
        {
          tmp = stream_getl (s);
          pal_mem_cpy (&adspec->Ctot, &tmp, 4);
        }

      /* Dtot */
      if (qos_adspec_check_dtot (adspec))
        {
          tmp = stream_getl (s);
          pal_mem_cpy (&adspec->Dtot, &tmp, 4);
        }

      /* Csum */
      if (qos_adspec_check_csum (adspec))
        {
          tmp = stream_getl (s);
          pal_mem_cpy (&adspec->Csum, &tmp, 4);
        }

      /* Dsum */
      if (qos_adspec_check_dsum (adspec))
        {
          tmp = stream_getl (s);
          pal_mem_cpy (&adspec->Dsum, &tmp, 4);
        }

      /* Minimum path latency */
      if (qos_adspec_check_mpl (adspec))
        {
          tmp = stream_getl (s);
          pal_mem_cpy (&adspec->mpl, &tmp, 4);
        }

      /* Path be estimate */
      if (qos_adspec_check_pbe (adspec))
        {
          tmp = stream_getl (s);
          pal_mem_cpy (&adspec->pbe, &tmp, 4);
        }
    }

  /* Setup priority ... */
  if (qos_resource_check_setup_priority (resource))
    {
      resource->setup_priority = stream_getw (s);
    }

  /* Hold priority ... */
  if (qos_resource_check_hold_priority (resource))
    {
      resource->hold_priority = stream_getw (s);
    }
}

/*
 * Helper function to fill a stream with qos_resource data.
 */
void
qos_common_stream_populate (struct qos_resource *resource,
                            u_int8_t command,
                            struct stream **s)
{
  struct qos_traffic_spec *tspec;
  struct qos_if_spec *ifspec;
  struct qos_ad_spec *adspec;
  u_int32_t tmp;

  /* Set client id */
  stream_putl (*s, resource->resource_id);

  /* Set protocol id */
  stream_putc (*s, resource->protocol_id);

  /* Set flag as-is */
  stream_putc (*s, resource->flags);

  /*
   * Depending on flag, put data. Data MUST be in the order
   * specified in qos_common.h.
   */

  /* Traffic spec ... */
  if (qos_resource_check_traffic_spec (resource))
    {
      /* Get pointer to tspec */
      tspec = &resource->t_spec;

      /* Put in service type */
      stream_putc (*s, tspec->service_type);

      /* Put in exclusive flag */
      stream_putc (*s, tspec->is_exclusive);

      /* Put in traffic spec flag */
      stream_putc (*s, tspec->flags);

      /*
       * Depending on flag, put data. Data MUST be in the
       * order specified in qos_common.h.
       */

      /* Peak rate ... */
      if (qos_tspec_check_peak_rate (tspec))
        {
          pal_mem_cpy (&tmp, &tspec->peak_rate, 4);
          stream_putl (*s, tmp);
        }

      /* Committed bucket ... */
      if (qos_tspec_check_committed_bucket (tspec))
        {
          /* Flag */
          stream_putc (*s, tspec->committed_bucket.flags);

          if (qos_bucket_check_rate (&tspec->committed_bucket))
            {
              pal_mem_cpy (&tmp, &tspec->committed_bucket.rate, 4);
              stream_putl (*s, tmp);
            }

          if (qos_bucket_check_size (&tspec->committed_bucket))
            {
              pal_mem_cpy (&tmp, &tspec->committed_bucket.size, 4);
              stream_putl (*s, tmp);
            }
        }

      /* Excess burst ... */
      if (qos_tspec_check_excess_burst (tspec))
        {
          pal_mem_cpy (&tmp, &tspec->excess_burst, 4);
          stream_putl (*s, tmp);
        }

      /* Weight of flow ... */
      if (qos_tspec_check_weight (tspec))
        {
          pal_mem_cpy (&tmp, &tspec->weight, 4);
          stream_putl (*s, tmp);
        }

      /* Min policed unit ... */
      if (qos_tspec_check_min_policed_unit (tspec))
        {
          stream_putl (*s, tspec->mpu);
        }

      /* Max packet size ... */
      if (qos_tspec_check_max_packet_size (tspec))
        {
          stream_putl (*s, tspec->max_packet_size);
        }
    }

  /* Interface spec ... */
  if (qos_resource_check_if_spec (resource))
    {
      /* Get pointer to interface spec */
      ifspec = &resource->if_spec;

      /* Put in interface spec flag */
      stream_putc (*s, ifspec->flags);
      
      /* Incoming if index */
      stream_putw (*s, ifspec->in_ifindex);
      
      /* Outgoing if index */
      stream_putw (*s, ifspec->out_ifindex);

      /* Prefixes for previous and next hop beings used ... */
      if (qos_ifspec_check_prefix(ifspec))
        {
          /* Prevous hop data ... */
          {
            /* Put in family */
            stream_putc (*s, ifspec->prev_hop.family);
            
            /* Put in address based on family */
            if (ifspec->prev_hop.family == AF_INET)
              stream_put_in_addr (*s, &ifspec->prev_hop.u.prefix4);
#ifdef HAVE_IPV6
            else if (ifspec->prev_hop.family == AF_INET6)
              stream_put_in6_addr (*s, &ifspec->prev_hop.u.prefix6);
#endif /* #ifdef HAVE_IPV6 */
          }

          /* Next hop data ... */
          {
            /* Put in family */
            stream_putc (*s, ifspec->next_hop.family);
            
            /* Put in address based on family */
            if (ifspec->next_hop.family == AF_INET)
              stream_put_in_addr (*s, &ifspec->next_hop.u.prefix4);
#ifdef HAVE_IPV6
            else if (ifspec->next_hop.family == AF_INET6)
              stream_put_in6_addr (*s, &ifspec->next_hop.u.prefix6);
#endif /* #ifdef HAVE_IPV6 */
          }
        }
    }

  /* Advertisement spec ... */
  if (qos_resource_check_ad_spec (resource))
    {
      /* Get adspec pointer */
      adspec = &resource->ad_spec;

      /* Put in adspec flag */
      stream_putc (*s, adspec->flags);

      /* Slack term */
      if (qos_adspec_check_slack (adspec))
        {
          pal_mem_cpy (&tmp, &adspec->slack, 4);
          stream_putl (*s ,tmp);
        }

      /* Ctot */
      if (qos_adspec_check_ctot (adspec))
        {
          pal_mem_cpy (&tmp, &adspec->Ctot, 4);
          stream_putl (*s ,tmp);
        }

      /* Dtot */
      if (qos_adspec_check_dtot (adspec))
        {
          pal_mem_cpy (&tmp, &adspec->Dtot, 4);
          stream_putl (*s ,tmp);
        }

      /* Csum */
      if (qos_adspec_check_csum (adspec))
        {
          pal_mem_cpy (&tmp, &adspec->Csum, 4);
          stream_putl (*s ,tmp);
        }

      /* Dsum */
      if (qos_adspec_check_dsum (adspec))
        {
          pal_mem_cpy (&tmp, &adspec->Dsum, 4);
          stream_putl (*s ,tmp);
        }

      /* Minimum path latency */
      if (qos_adspec_check_mpl (adspec))
        {
          pal_mem_cpy (&tmp, &adspec->mpl, 4);
          stream_putl (*s, tmp);
        }

      /* Path bw estimate */
      if (qos_adspec_check_pbe (adspec))
        {
          pal_mem_cpy (&tmp, &adspec->pbe, 4);
          stream_putl (*s, tmp);
        }
    }

  /* Setup priority ... */
  if (qos_resource_check_setup_priority (resource))
    {
      stream_putw (*s, resource->setup_priority);
    }

  /* Hold priority ... */
  if (qos_resource_check_hold_priority (resource))
    {
      stream_putw (*s, resource->hold_priority);
    }
}

/* Traffic type strings */
static const char *traffic_to_str [] =
{
  "Undefined",
  "Controlled Load",
  "Guaranteed"
};

/* Traffic to str routine */
const char *
qos_common_traffic_type_str (u_int8_t traffic)
{
  if (traffic <= 2)
    return traffic_to_str [traffic];
  else
    return "N/A";
}

#if 0 /* FIXME : can't use printf without a hard console */
/* Dump resource data */
void
qos_common_resource_dump (struct qos_resource *resource)
{
  struct qos_traffic_spec *tspec;
  struct qos_if_spec *ifspec;
  struct qos_ad_spec *adspec;

  /* Print client id */
  printf ("Resource ID : %ul\n", resource->resource_id);

  /* Print protocol id */
  printf (" Protocol : %s\n", get_prot_str (resource->protocol_id));

  /* Traffic spec ... */
  if (qos_resource_check_traffic_spec (resource))
    {
      /* Get pointer to tspec */
      tspec = &resource->t_spec;

      /* Put in service type */
      printf (" Service type : %s\n",
              qos_common_traffic_type_str (tspec->service_type));

      /* Put in exclusive flag */
      printf (" Is exclusive : %s\n",
              ((tspec->is_exclusive == QOS_EXCLUSIVE) ? "Yes" : "No"));

      /* Peak rate ... */
      if (qos_tspec_check_peak_rate (tspec))
        printf (" Peak rate : %.2f\n", tspec->peak_rate);

      /* Committed bucket ... */
      if (qos_tspec_check_committed_bucket (tspec))
        {
          if (qos_bucket_check_rate (&tspec->committed_bucket))
            printf (" Committed bucket rate : %.2f\n",
                    tspec->committed_bucket.rate); 

          if (qos_bucket_check_size (&tspec->committed_bucket))
            printf (" Committed bucket size : %.2f\n",
                    tspec->committed_bucket.size); 
        }

      /* Excess burst ... */
      if (qos_tspec_check_excess_burst (tspec))
        printf (" Excess burst : %.2f\n", tspec->excess_burst);

      /* Weight of flow ... */
      if (qos_tspec_check_weight (tspec))
        printf (" Weight : %.2f\n", tspec->weight);

      /* Min policed unit ... */
      if (qos_tspec_check_min_policed_unit (tspec))
        printf (" Minimum policed unit : %d\n", tspec->mpu);

      /* Max packet size ... */
      if (qos_tspec_check_max_packet_size (tspec))
        printf (" Maximum packet size : %d\n", tspec->max_packet_size);
    }

  /* Interface spec ... */
  if (qos_resource_check_if_spec (resource))
    {
      /* Get pointer to interface spec */
      ifspec = &resource->if_spec;

      /* Incoming if index */
      printf (" Incoming interface index : %ul\n", ifspec->in_ifindex);
      
      /* Outgoing if index */
      printf (" Outgoing interface index : %ul\n", ifspec->out_ifindex);

      /* Prefixes for previous and next hop beings used ... */
      if (qos_ifspec_check_prefix(ifspec))
        {
          /* Prevous hop data ... */
          {
            if (ifspec->prev_hop.family == AF_INET)
              printf (" Previous hop : %s\n",
                      inet_ntoa (ifspec->prev_hop.u.prefix4));
#ifdef HAVE_IPV6
            else if (ifspec->prev_hop.family == AF_INET6)
              {
                char buf [INET_NTOP_BUFSIZ];
                
                printf (" Previous hop : %s\n",
                        ((pal_inet_ntop (AF_INET6, &ifspec->prev_hop.u.prefix6,
                                         buf, INET_NTOP_BUFSIZ)) ? buf : "N/A"));
              }
#endif /* #ifdef HAVE_IPV6 */
          }

          /* Next hop data ... */
          {
            if (ifspec->next_hop.family == AF_INET)
              printf (" Next hop : %s\n",
                      inet_ntoa (ifspec->next_hop.u.prefix4));
#ifdef HAVE_IPV6
            else if (ifspec->next_hop.family == AF_INET6)
              {
                char buf [INET_NTOP_BUFSIZ];
                
                printf (" Next hop : %s\n",
                        ((pal_inet_ntop (AF_INET6, &ifspec->next_hop.u.prefix6,
                                         buf, INET_NTOP_BUFSIZ)) ? buf : "N/A"));
              }
#endif /* #ifdef HAVE_IPV6 */
          }
        }
    }

  /* Advertisement spec ... */
  if (qos_resource_check_ad_spec (resource))
    {
      /* Get adspec pointer */
      adspec = &resource->ad_spec;

      /* Slack term */
      if (qos_adspec_check_slack (adspec))
        printf (" Slack term : %.2f\n", adspec->slack);

      /* Ctot */
      if (qos_adspec_check_ctot (adspec))
        printf (" Ctot : %.2f\n", adspec->Ctot);

      /* Dtot */
      if (qos_adspec_check_dtot (adspec))
        printf (" Dtot : %.2f\n", adspec->Dtot);

      /* Csum */
      if (qos_adspec_check_csum (adspec))
        printf (" Csum : %.2f\n", adspec->Csum);

      /* Dsum */
      if (qos_adspec_check_dsum (adspec))
        printf (" Dsum : %.2f\n", adspec->Dsum);

      /* Minimum path latency */
      if (qos_adspec_check_mpl (adspec))
        printf (" Minimum path latency : %.2f\n", adspec->mpl);

      /* Path bw estimate */
      if (qos_adspec_check_pbe (adspec))
        printf (" Path bandwidth estimate : %.2f\n", adspec->pbe);
    }

  /* Setup priority ... */
  if (qos_resource_check_setup_priority (resource))
    printf (" Setup priority : %d\n", (s_int32_t) resource->setup_priority);

  /* Hold priority ... */
  if (qos_resource_check_hold_priority (resource))
    printf (" Hold priority : %d\n", (s_int32_t) resource->hold_priority);
}
#endif /* 0 */
