#include "types.h"
#include "sdio.h"
#include "log.h"

#define MMC_BLOCK_SIZE(fn)  (((fn)==2)?(512):(64))
#define sdio_func_id(x)  (x)
/* Split an arbitrarily sized data transfer into several
 * IO_RW_EXTENDED commands. */
static int sdio_io_rw_ext_helper(int fn, int write,
	unsigned addr, int incr_addr, u8 *buf, unsigned size)
{
	unsigned remainder = size;
	unsigned max_blocks;
	int ret;

	/* Do the bulk of the transfer using block mode (if supported). */
	if (size > MMC_BLOCK_SIZE(fn)) {
		/* Blocks per command is limited by host count, host transfer
		 * size and the maximum for IO_RW_EXTENDED of 511 blocks. */
		max_blocks = 511;

		while (remainder >= MMC_BLOCK_SIZE(fn)) {
			unsigned blocks;

			blocks = remainder / MMC_BLOCK_SIZE(fn);
			if (blocks > max_blocks)
				blocks = max_blocks;
			size = blocks * MMC_BLOCK_SIZE(fn);

			ret = mmc_io_rw_extended(write,
				fn, addr, incr_addr, buf,
				blocks, MMC_BLOCK_SIZE(fn));
			if (ret)
				return ret;

			remainder -= size;
			buf += size;
			if (incr_addr)
				addr += size;
		}
	}

	/* Write the remainder using byte mode. */
	while (remainder > 0) {
		size = min(remainder, MMC_BLOCK_SIZE(fn));
		/* Indicate byte mode by setting "blocks" = 0 */
		ret = mmc_io_rw_extended(write, fn, addr,
			 incr_addr, buf, 0, size);
		if (ret)
			return ret;

		remainder -= size;
		buf += size;
		if (incr_addr)
			addr += size;
	}
	return 0;
}

int sdio_memcpy_fromio(int func, void *dst,
	unsigned int addr, int count)
{
	return sdio_io_rw_ext_helper(func, 0, addr, 1, dst, count);
}

int sdio_memcpy_toio(int func, unsigned int addr,
	void *src, int count)
{
	return sdio_io_rw_ext_helper(func, 1, addr, 1, src, count);
}

u8 sdio_readb(int func, unsigned int addr, int *err_ret)
{
	int ret;
	u8 val;

	ret = mmc_io_rw_direct(0, func, addr, 0, &val);
	if (err_ret)
		*err_ret = ret;
	if (ret)
		return 0xFF;

	return val;
}

void sdio_writeb(int func, u8 b, unsigned int addr, int *err_ret)
{
	int ret;

	ret = mmc_io_rw_direct(1, func, addr, b, NULL);
	if (err_ret)
		*err_ret = ret;
}
int sdio_readsb(int func, void *dst, unsigned int addr,
	int count)
{
	return sdio_io_rw_ext_helper(func, 0, addr, 0, dst, count);
}

int sdio_writesb(int func, unsigned int addr, void *src,
	int count)
{
	return sdio_io_rw_ext_helper(func, 1, addr, 0, src, count);
}

u16 sdio_readw(int func, unsigned int addr, int *err_ret)
{
	int ret;
    uint16_t val;
	ret = sdio_memcpy_fromio(func, &val, addr, 2);
	if (err_ret)
		*err_ret = ret;
	if (ret)
		return 0xFFFF;

	return val;
}

void sdio_writew(int func, u16 b, unsigned int addr, int *err_ret)
{
	int ret;

	ret = sdio_memcpy_toio(func, addr, &b, 2);
	if (err_ret)
		*err_ret = ret;
}

u32 sdio_readl(int func, unsigned int addr, int *err_ret)
{
	int ret;
    uint32_t val;
	ret = sdio_memcpy_fromio(func,&val, addr, 4);
	if (err_ret)
		*err_ret = ret;
	if (ret)
		return 0xFFFFFFFF;

	return val;
}

void sdio_writel(int func, u32 b, unsigned int addr, int *err_ret)
{
	int ret;

	ret = sdio_memcpy_toio(func, addr, &b, 4);
	if (err_ret)
		*err_ret = ret;
}

int sdio_reset(struct mmc_host *host)
{
	int ret;
	u8 abort;

	/* SDIO Simplified Specification V2.0, 4.4 Reset for SDIO */

	ret = mmc_io_rw_direct_host(0, 0, SDIO_CCCR_ABORT, 0, &abort);
	if (ret)
		abort = 0x08;
	else
		abort |= 0x08;

	return mmc_io_rw_direct_host(1, 0, SDIO_CCCR_ABORT, abort, NULL);
}


int sdio_set_block_size(int func, unsigned blksz)
{
    int ret;

    ret = mmc_io_rw_direct(1, 0,
        SDIO_FBR_BASE(func) + SDIO_FBR_BLKSIZE,
        blksz & 0xff, NULL);
    if (ret)
        return ret;
    ret = mmc_io_rw_direct(1, 0,
        SDIO_FBR_BASE(func) + SDIO_FBR_BLKSIZE + 1,
        (blksz >> 8) & 0xff, NULL);
    if (ret)
        return ret;
    return 0;
}

int sdio_enable_func(int func)
{
    int ret;
    unsigned char reg;
    unsigned long timeout;

    if (!func)
        return -EINVAL;

    brcm_klog("SDIO: Enabling device %d...\n", sdio_func_id(func));

    ret = mmc_io_rw_direct(0, 0, SDIO_CCCR_IOEx, 0, &reg);
    if (ret)
        goto err;

    reg |= 1 << func;

    ret = mmc_io_rw_direct(1, 0, SDIO_CCCR_IOEx, reg, NULL);
    if (ret)
        goto err;

    timeout = get_timer(0) + 3000;

    while (1) {
        ret = mmc_io_rw_direct(0, 0, SDIO_CCCR_IORx, 0, &reg);
        if (ret)
            goto err;
        if (reg & (1 << func))
            break;
        ret = -ETIME;
        if (get_timer(timeout) > 0)
            goto err;
    }

    return 0;

err:
    brcm_klog("SDIO: Failed to enable device %s\n", sdio_func_id(func));
    return ret;
}

int sdio_disable_func(int func)
{
	int ret;
	unsigned char reg;

	if (!func)
		return -EINVAL;

	brcm_klog("SDIO: Disabling device %d ...\n", func);

	ret = mmc_io_rw_direct(0, 0, SDIO_CCCR_IOEx, 0, &reg);
	if (ret)
		goto err;

	reg &= ~(1 << func);

	ret = mmc_io_rw_direct(1, 0, SDIO_CCCR_IOEx, reg, NULL);
	if (ret)
		goto err;

	return 0;

err:
	brcm_klog("SDIO: Failed to disable device %d\n", func);
	return ret;
}

int sdio_claim_irq(int func)
{
    int ret;
    unsigned char reg;

    if (!func)
        return -EINVAL;

    brcm_klog("SDIO: Enabling IRQ for %d...\n", sdio_func_id(func));

    ret = mmc_io_rw_direct(0, 0, SDIO_CCCR_IENx, 0, &reg);
    if (ret)
        return ret;

    reg |= 1 << func;

    reg |= 1; /* Master interrupt enable */

    ret = mmc_io_rw_direct(1, 0, SDIO_CCCR_IENx, reg, NULL);
    if (ret)
        return ret;

	sdhci_enable_irq(1);

    return ret;
}