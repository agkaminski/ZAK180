/* ZAK180 Firmaware
 * Floppy disk block device driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

/* Just a higher lever wrappers for direct
 * controller access. */

#include "floppy.h"
#include "floppy_cmd.h"

static uint8_t floppy_lba2cyl(uint16_t lba)
{
	return lba / (2 * 18);
}

static uint8_t floppy_lba2head(uint16_t lba)
{
	return (lba % (2 * 18)) / 18;
}

static uint8_t floppy_lba2sector(uint16_t lba)
{
	return ((lba % (2 * 18)) % 18) + 1;
}

void floppy_access(uint8_t enable)
{
	floppy_cmd_enable(enable);
}
#include <stdio.h>
int floppy_read_sector(uint16_t lba, void *buff)
{
	uint8_t c = floppy_lba2cyl(lba);
	uint8_t h = floppy_lba2head(lba);
	uint8_t r = floppy_lba2sector(lba);
	struct floppy_cmd_result res;

	for (uint8_t hard = 3; hard != 0; --hard) {
		for (uint8_t retry = 3; retry != 0; --retry) {
			int ret = floppy_cmd_read_data(c, h, r, buff, &res);

			/* Check the result, we expect abnormal termination
			* and EOT, anything else is a fail. */
			if ((ret != 0) || ((res.st0 & 0xF0) != 0x40) || (res.st1 != 0x80)) {
				continue;
			}

			return 0;
		}

		/* Hard fail, it media ejected? */
		int eject = floppy_cmd_eject_status();
		if (eject != FLOPPY_CMD_NO_CHANGE) {
			/* It was ejected, no point in reading from the new disk */
			return eject;
		}

		/* Let's try resetting the controller and recalibrating the head */
		uint8_t rstretry;
		for (rstretry = 3; rstretry != 0; --rstretry) {
			if (floppy_cmd_reset(1) == 0) {
				break;
			}
		}

		if (!rstretry) {
			/* Reset failed, but no eject. Something very bad has
			 * happened. */
			break;
		}
	}

	return FLOPPY_IOERR;
}

int floppy_write_sector(uint16_t lba, const void *buff)
{
	uint8_t c = floppy_lba2cyl(lba);
	uint8_t h = floppy_lba2head(lba);
	uint8_t r = floppy_lba2sector(lba);
	struct floppy_cmd_result res;

	for (uint8_t hard = 3; hard != 0; --hard) {
		for (uint8_t retry = 3; retry != 0; --retry) {
			int ret = floppy_cmd_write_data(c, h, r, buff, &res);

			/* Check the result, we expect abnormal termination
			* and EOT, anything else is a fail. */
			if ((ret != 0) || ((res.st0 & 0xF) != 0x40) || (res.st1 != 0x80)) {
				continue;
			}

			return 0;
		}

		/* Hard fail, it media ejected? */
		int eject = floppy_cmd_eject_status();
		if (eject != FLOPPY_CMD_NO_CHANGE) {
			/* It was ejected, no point in reading from the new disk */
			return eject;
		}

		/* Let's try resetting the controller and recalibrating the head */
		uint8_t rstretry;
		for (rstretry = 3; rstretry != 0; --rstretry) {
			if (floppy_cmd_reset(1) == 0) {
				break;
			}
		}

		if (!rstretry) {
			/* Reset failed, but no eject. Something very bad has
			 * happened. */
			break;
		}
	}

	return FLOPPY_IOERR;
}

int floppy_init(void)
{
	for (uint8_t retry = 3; retry != 0; --retry) {
		if (floppy_cmd_reset(0) == 0) {
			return 0;
		}
	}

	return -1;
}
