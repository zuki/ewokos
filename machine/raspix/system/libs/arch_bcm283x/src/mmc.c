#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <arch/bcm283x/mmc.h>
#include <ewoksys/proc.h>

#define usleep  proc_usleep

typedef uint32_t lbaint_t;
struct mmc _mmc;
struct mmc_config _config;
void* _sdio;

extern void* bcm283x_sdhost_init(void);
extern int bcm2835_set_ios(void *host, int clock, int bus_width);
extern int bcm2835_send_cmd(void *host, struct mmc_cmd *cmd, struct mmc_data *data);

int mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data){
	(void)mmc;
	//klog("cmd: %d %x %x\n", cmd->cmdidx, cmd->cmdarg, cmd->resp_type);
    int ret = bcm2835_send_cmd(_sdio, cmd, data); 
	//klog("ret:%d resp:%x %x %x %x\n", ret, cmd->response[0], cmd->response[1], cmd->response[2], cmd->response[3]);
    return ret;
}

static int mmc_send_cmd_retry(struct mmc *mmc, struct mmc_cmd *cmd,
			      struct mmc_data *data, uint32_t retries)
{
	int ret;

	do {
		ret = mmc_send_cmd(mmc, cmd, data);
	} while (ret && retries--);

	return ret;
}

int mmc_send_status(struct mmc *mmc, unsigned int *status)
{
	struct mmc_cmd cmd;
	int ret;

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	if (!mmc_host_is_spi(mmc))
		cmd.cmdarg = mmc->rca << 16;

	ret = mmc_send_cmd_retry(mmc, &cmd, NULL, 4);
	if (!ret)
		*status = cmd.response[0];

	return ret;
}

int mmc_poll_for_busy(struct mmc *mmc, int timeout_ms)
{
	unsigned int status;
	int err;

	while (1) {
		err = mmc_send_status(mmc, &status);
		if (err)
			return err;

		if ((status & MMC_STATUS_RDY_FOR_DATA) &&
		    (status & MMC_STATUS_CURR_STATE) !=
		     MMC_STATE_PRG)
			break;

		if (status & MMC_STATUS_MASK) {
			return -ECOMM;
		}

		if (timeout_ms-- <= 0)
			break;

		proc_usleep(1000);
	}

	if (timeout_ms <= 0) {
		return -ETIMEDOUT;
	}

	return 0;
}

int mmc_send_stop_transmission(struct mmc *mmc, int write)
{
	struct mmc_cmd cmd;

	cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
	cmd.cmdarg = 0;
	/*
	 * JEDEC Standard No. 84-B51 Page 126
	 * CMD12 STOP_TRANSMISSION R1/R1b[3]
	 * NOTE 3 R1 for read cases and R1b for write cases.
	 *
	 * Physical Layer Simplified Specification Version 9.00
	 * 7.3.1.3 Detailed Command Description
	 * CMD12 R1b
	 */
	cmd.resp_type = (IS_SD(mmc) || write) ? MMC_RSP_R1b : MMC_RSP_R1;

	return mmc_send_cmd(mmc, &cmd, NULL);
}

static int mmc_go_idle(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	int err;

	proc_usleep(1000);

	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	proc_usleep(2000);

	return 0;
}

static int mmc_send_if_cond(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = SD_CMD_SEND_IF_COND;
	/* We set the bit if the host supports voltages between 2.7 and 3.6 V */
	cmd.cmdarg = ((mmc->cfg->voltages & 0xff8000) != 0) << 8 | 0xaa;
	cmd.resp_type = MMC_RSP_R7;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	if ((cmd.response[0] & 0xff) != 0xaa)
		return -EOPNOTSUPP;
	else
		mmc->version = SD_VERSION_2;

	return 0;
}

static int mmc_send_op_cond_iter(struct mmc *mmc, int use_arg)
{
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = MMC_CMD_SEND_OP_COND;
	cmd.resp_type = MMC_RSP_R3;
	cmd.cmdarg = 0;
	if (use_arg)
		cmd.cmdarg = OCR_HCS |
			(mmc->cfg->voltages &
			(mmc->ocr & OCR_VOLTAGE_MASK)) |
			(mmc->ocr & OCR_ACCESS_MODE);

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		return err;
	mmc->ocr = cmd.response[0];
	return 0;
}


static int sd_send_op_cond(struct mmc *mmc, bool uhs_en)
{
	int timeout = 1000;
	int err;
	struct mmc_cmd cmd;

	while (1) {
		cmd.cmdidx = MMC_CMD_APP_CMD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;

		/*
		 * Most cards do not answer if some reserved bits
		 * in the ocr are set. However, Some controller
		 * can set bit 7 (reserved for low voltages), but
		 * how to manage low voltages SD card is not yet
		 * specified.
		 */
		cmd.cmdarg = mmc_host_is_spi(mmc) ? 0 :
			(mmc->cfg->voltages & 0xff8000);

		if (mmc->version == SD_VERSION_2)
			cmd.cmdarg |= OCR_HCS;

		if (uhs_en)
			cmd.cmdarg |= OCR_S18R;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		if (cmd.response[0] & OCR_BUSY)
			break;

		if (timeout-- <= 0)
			return -EOPNOTSUPP;

		proc_usleep(1000);
	}

	if (mmc->version != SD_VERSION_2)
		mmc->version = SD_VERSION_1_0;


	if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
		cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;
	}

	mmc->ocr = cmd.response[0];
	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 0;

	return 0;
}

void mmc_set_initial_state(struct mmc *mmc)
{
    bcm2835_set_ios(_sdio, mmc->clock, mmc->bus_width);
}

static int mmc_send_op_cond(struct mmc *mmc)
{
	int err, i;
	uint32_t timeout = 1000000;
	uint32_t retry_count = 0;

	/* Some cards seem to need this */
	mmc_go_idle(mmc);

	/* Asking to the card its capabilities */
	for (i = 0; ; i++) {
		err = mmc_send_op_cond_iter(mmc, i != 0);
		if (err)
			return err;

		/* exit if not busy (flag seems to be inverted) */
		if (mmc->ocr & OCR_BUSY)
			break;

		if (retry_count > timeout)
			return -ETIMEDOUT;
		sleep(0);
	}
	mmc->op_cond_pending = 1;
	return 0;
}


int mmc_get_op_cond(struct mmc *mmc, bool quiet)
{
	bool uhs_en = true;
	int err;
	(void)quiet;

	if (mmc->has_init)
		return 0;

retry:
	mmc_set_initial_state(mmc);

	/* Reset the Card */
	err = mmc_go_idle(mmc);

	if (err)
		return err;

	/* Test for SD version 2 */
	err = mmc_send_if_cond(mmc);

	/* Now try to get the SD card's operating condition */
	err = sd_send_op_cond(mmc, uhs_en);
	if (err && uhs_en) {
		uhs_en = false;
		goto retry;
	}

	/* If the command timed out, we check for an MMC card */
	if (err == -ETIMEDOUT) {
		err = mmc_send_op_cond(mmc);
		if (err) {
			klog("Card did not respond to voltage select! : %d\n", err);
			return -EOPNOTSUPP;
		}
	}

	return err;
}

static int mmc_complete_op_cond(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	uint32_t timeout = 1000;
	uint32_t retry_count = 0;
	int err;

	mmc->op_cond_pending = 0;
	if (!(mmc->ocr & OCR_BUSY)) {
		/* Some cards seem to need this */
		mmc_go_idle(mmc);

		while (1) {
			err = mmc_send_op_cond_iter(mmc, 1);
			if (err)
				return err;
			if (mmc->ocr & OCR_BUSY)
				break;
			if (retry_count > timeout)
				return -EOPNOTSUPP;
			sleep(0);
		}
	}

	if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
		cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		mmc->ocr = cmd.response[0];
	}

	mmc->version = MMC_VERSION_UNKNOWN;

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 1;

	return 0;
}

static const int fbase[] = {
        10000,
        100000,
        1000000,
        10000000,
    };

static const uint8_t multipliers[] = {
	0,	/* reserved */
	10,
	12,
	13,
	15,
	20,
	25,
	30,
	35,
	40,
	45,
	50,
	55,
	60,
	70,
	80,
};
int mmc_set_blocklen(struct mmc *mmc, int len)
{
	struct mmc_cmd cmd;

	cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = len;

	return mmc_send_cmd(mmc, &cmd, NULL);
}

static int sd_select_bus_width(struct mmc *mmc, int w)
{
	int err;
	struct mmc_cmd cmd;

	if ((w != 4) && (w != 1))
		return -EINVAL;

	cmd.cmdidx = MMC_CMD_APP_CMD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mmc->rca << 16;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		return err;

	cmd.cmdidx = SD_CMD_APP_SET_BUS_WIDTH;
	cmd.resp_type = MMC_RSP_R1;
	if (w == 4)
		cmd.cmdarg = 2;
	else if (w == 1)
		cmd.cmdarg = 0;
	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		return err;

	return 0;
}

static int mmc_startup(struct mmc *mmc)
{
	int err, i;
	uint32_t mult, freq;
	uint64_t cmult, csize;
	struct mmc_cmd cmd;

	/* Put the Card in Identify Mode */
	cmd.cmdidx = mmc_host_is_spi(mmc) ? MMC_CMD_SEND_CID :
		MMC_CMD_ALL_SEND_CID; /* cmd not supported in spi */
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		return err;

	memcpy(mmc->cid, cmd.response, 16);

	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relatvie Address.
	 * This also puts the cards into Standby State
	 */
	cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
	cmd.cmdarg = mmc->rca << 16;
	cmd.resp_type = MMC_RSP_R6;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	if (IS_SD(mmc))
		mmc->rca = (cmd.response[0] >> 16) & 0xffff;


	/* Get the Card-Specific Data */
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = mmc->rca << 16;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	mmc->csd[0] = cmd.response[0];
	mmc->csd[1] = cmd.response[1];
	mmc->csd[2] = cmd.response[2];
	mmc->csd[3] = cmd.response[3];

	if (mmc->version == MMC_VERSION_UNKNOWN) {
		int version = (cmd.response[0] >> 26) & 0xf;

		switch (version) {
		case 0:
			mmc->version = MMC_VERSION_1_2;
			break;
		case 1:
			mmc->version = MMC_VERSION_1_4;
			break;
		case 2:
			mmc->version = MMC_VERSION_2_2;
			break;
		case 3:
			mmc->version = MMC_VERSION_3;
			break;
		case 4:
			mmc->version = MMC_VERSION_4;
			break;
		default:
			mmc->version = MMC_VERSION_1_2;
			break;
		}
	}

	/* divide frequency by 10, since the mults are 10x bigger */
	freq = fbase[(cmd.response[0] & 0x7)];
	mult = multipliers[((cmd.response[0] >> 3) & 0xf)];

	mmc->legacy_speed = freq * mult;
	//mmc_select_mode(mmc, MMC_LEGACY);

	mmc->dsr_imp = ((cmd.response[1] >> 12) & 0x1);
	mmc->read_bl_len = 1 << ((cmd.response[1] >> 16) & 0xf);
	if (IS_SD(mmc))
		mmc->write_bl_len = mmc->read_bl_len;
	else
		mmc->write_bl_len = 1 << ((cmd.response[3] >> 22) & 0xf);

	if (mmc->high_capacity) {
		csize = (mmc->csd[1] & 0x3f) << 16
			| (mmc->csd[2] & 0xffff0000) >> 16;
		cmult = 8;
	} else {
		csize = (mmc->csd[1] & 0x3ff) << 2
			| (mmc->csd[2] & 0xc0000000) >> 30;
		cmult = (mmc->csd[2] & 0x00038000) >> 15;
	}

	mmc->capacity_user = (csize + 1) << (cmult + 2);
	mmc->capacity_user *= mmc->read_bl_len;
	mmc->capacity_boot = 0;
	mmc->capacity_rpmb = 0;
	for (i = 0; i < 4; i++)
		mmc->capacity_gp[i] = 0;

	if (mmc->read_bl_len > MMC_MAX_BLOCK_LEN)
		mmc->read_bl_len = MMC_MAX_BLOCK_LEN;

	if (mmc->write_bl_len > MMC_MAX_BLOCK_LEN)
		mmc->write_bl_len = MMC_MAX_BLOCK_LEN;

	if ((mmc->dsr_imp) && (0xffffffff != mmc->dsr)) {
		cmd.cmdidx = MMC_CMD_SET_DSR;
		cmd.cmdarg = (mmc->dsr & 0xffff) << 16;
		cmd.resp_type = MMC_RSP_NONE;
		if (mmc_send_cmd(mmc, &cmd, NULL))
			klog("MMC: SET_DSR failed\n");
	}

	/* Select the card, and put it into Transfer Mode */
	if (!mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
		cmd.cmdidx = MMC_CMD_SELECT_CARD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = mmc->rca << 16;
		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;
	}

	mmc_set_blocklen(mmc, mmc->read_bl_len);
	sd_select_bus_width(mmc, mmc->bus_width);
	/*
	 * For SD, its erase group is always one sector
	 */
	mmc->erase_grp_size = 1;
	mmc->part_config = MMCPART_NOAVAILABLE;
	mmc->has_init = 1;
	return 0;
}


int mmc_init(void){
	if(_mmc.has_init)
		return 0;

    memset(&_mmc, 0, sizeof(struct mmc));
    memset(&_config, 0, sizeof(struct mmc_config));

    _mmc.cfg = &_config; 
    _mmc.bus_width = 1;
    _mmc.clock = 400000;
    _mmc.has_init = false;
    _mmc.cfg->host_caps = MMC_MODE_4BIT | MMC_MODE_HS | MMC_MODE_HS_52MHz;
    _mmc.cfg->voltages = MMC_VDD_32_33 | MMC_VDD_33_34;

    _sdio = bcm283x_sdhost_init();

   	mmc_get_op_cond(&_mmc, false);
    //mmc_complete_op_cond(&_mmc);
	_mmc.bus_width = 4;
    _mmc.clock = 25000000;
	bcm2835_set_ios(_sdio,_mmc.clock, _mmc.bus_width);
    return mmc_startup(&_mmc);
}

int mmc_read_blocks(void *dst, lbaint_t start,
			   lbaint_t blkcnt)
{
	struct mmc_cmd cmd;
	struct mmc_data data;

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

    if(_mmc.high_capacity)
    	cmd.cmdarg = start;
    else
	    cmd.cmdarg = start * _mmc.read_bl_len;
	cmd.resp_type = MMC_RSP_R1;

	data.dest = dst;
	data.blocks = blkcnt;
	data.blocksize = _mmc.read_bl_len;
	data.flags = MMC_DATA_READ;

	if (mmc_send_cmd(&_mmc, &cmd, &data))
		return 0;

	if (blkcnt > 1) {
		if (mmc_send_stop_transmission(&_mmc, false)) {
			return 0;
		}
	}

	return blkcnt;
}

uint32_t mmc_erase_blocks(struct mmc *mmc, uint32_t start, uint32_t blkcnt)
{
	struct mmc_cmd cmd;
	uint32_t end;
	int err, start_cmd, end_cmd;

	if (mmc->high_capacity) {
		end = start + blkcnt - 1;
	} else {
		end = (start + blkcnt - 1) * mmc->write_bl_len;
		start *= mmc->write_bl_len;
	}

	if (IS_SD(mmc)) {
		start_cmd = SD_CMD_ERASE_WR_BLK_START;
		end_cmd = SD_CMD_ERASE_WR_BLK_END;
	} else {
		start_cmd = MMC_CMD_ERASE_GROUP_START;
		end_cmd = MMC_CMD_ERASE_GROUP_END;
	}

	cmd.cmdidx = start_cmd;
	cmd.cmdarg = start;
	cmd.resp_type = MMC_RSP_R1;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		goto err_out;

	cmd.cmdidx = end_cmd;
	cmd.cmdarg = end;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		goto err_out;

	cmd.cmdidx = MMC_CMD_ERASE;
	cmd.cmdarg = MMC_TRIM_ARG;
	cmd.resp_type = MMC_RSP_R1b;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		goto err_out;

	return 0;

err_out:
	klog("mmc erase failed\n");
	return err;
}

uint32_t mmc_write_blocks(uint32_t start,
		uint32_t blkcnt, const void *src)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int timeout_ms = 1000;

	if (blkcnt == 0)
		return 0;
	else if (blkcnt == 1)
		cmd.cmdidx = MMC_CMD_WRITE_SINGLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;

	if (_mmc.high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * _mmc.write_bl_len;

	cmd.resp_type = MMC_RSP_R1;

	data.src = src;
	data.blocks = blkcnt;
	data.blocksize = _mmc.write_bl_len;
	data.flags = MMC_DATA_WRITE;

	if (mmc_send_cmd(&_mmc, &cmd, &data)) {
		klog("mmc write failed\n");
		return 0;
	}

	/* SPI multiblock writes terminate using a special
	 * token, not a STOP_TRANSMISSION request.
	 */
	if (!mmc_host_is_spi(mmc) && blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		if (mmc_send_cmd(&_mmc, &cmd, NULL)) {
			klog("mmc fail to send stop cmd\n");
			return 0;
		}
	}

	/* Waiting for the ready status */
	if (mmc_poll_for_busy(&_mmc, timeout_ms))
		return 0;

	return blkcnt;
}
