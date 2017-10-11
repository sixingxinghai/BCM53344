/* Copyright (C) 2003. All Rights Reserved. */
#include "pal.h"
#include "lib.h"
#include "hash.h"
#include "cli.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */
#include "nsmd.h"
#include "nsm/nsm_hw_reg_api.h"
#include "nsm_interface.h"
#include "nsm_router.h"
#include "rib/rib.h"
#include "nsm/nsm_api.h"

#define CLI_GET_HEXA_DECIMAL(NAME,V,STR)                                       \
{                                                                              \
  char *endptr = NULL;                                                         \
  (V) = pal_strtou32 ((STR), &endptr, 16);                                     \
  if (*endptr != '\0' || ((V) == ULONG_MAX && errno == ERANGE))                \
    {                                                                          \
      cli_out (cli, "%% Invalid %s value\n", NAME);                            \
      return CLI_ERROR;                                                        \
    }                                                                          \
}                                                                              \

int
nsm_hw_reg_cli_return (struct cli *cli, int ret)
{
  char *str;

  switch (ret)
    {
      case NSM_API_GET_SUCCESS:
        return CLI_SUCCESS;
      case NSM_API_ERR_HW_REG_GET_FAILED:
        str = "HW Register Get operation is Invalid";
        break;
      default:
        return CLI_SUCCESS;
    }
  cli_out (cli, "%% %s\n", str);

  return CLI_ERROR;
}

CLI (hw_register_get,
     hw_register_get_cmd,
     "hardware register get ADDR ",
      "hardware"
     "register",
     "get the value from the Register",
     "Register address in 0xhhhh format")
{
  int ret = 0;

#ifdef HAVE_USER_HSL
  u_int32_t reg_addr;

  struct hal_reg_addr *reg;
  
  reg = XCALLOC (MTYPE_HSL_SERVER_ENTRY, sizeof (struct hal_reg_addr));

  CLI_GET_HEXA_DECIMAL ("Register Address", reg_addr, argv[0]);

  ret = nsm_hw_reg_get (reg_addr, reg);

  if (ret != HAL_SUCCESS)
    ret = NSM_API_ERR_HW_REG_GET_FAILED;
  else
    {
      cli_out (cli, "value:0x%x\n", reg->value);
      free (reg);
      return CLI_ERROR;
    }

#endif /* HAVE_USER_HSL */

  return nsm_hw_reg_cli_return (cli, ret);
}

CLI (hw_register_set,
     hw_register_set_cmd,
     "hardware register set ADDR VALUE",
      "hardware"
     "register",
     "set the value to the Register",
     "Register address in 0xhhhh format",
     "Value in hex format")
{
#ifdef HAVE_USER_HSL
  u_int32_t value, ret;
  u_int32_t reg_addr;

  /*Converting string input to an integer */
  CLI_GET_HEXA_DECIMAL ("Register Address", reg_addr, argv[0]);
  CLI_GET_HEXA_DECIMAL ("value", value, argv[1]);
 
  if ((ret = nsm_hw_reg_set (reg_addr, value)) != HAL_SUCCESS)
    return ret;

#endif /* HAVE_USER_HSL */

  return CLI_SUCCESS;
}

/* NSM registers cli init */
void
nsm_hw_reg_init (struct cli_tree *ctree)
{

  cli_install (ctree, EXEC_MODE, &hw_register_get_cmd);
  cli_install (ctree, EXEC_MODE, &hw_register_set_cmd);
}
