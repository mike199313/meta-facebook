#ifndef __PAL_SBMC_H__
#define __PAL_SBMC_H__

int cmd_set_smbc_restore_power_policy(uint8_t policy, uint8_t t_bmc_addr);
int cmd_smbc_chassis_ctrl(uint8_t cmd, uint8_t t_bmc_addr);
#endif

