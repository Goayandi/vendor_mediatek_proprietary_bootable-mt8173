
#include "mmc_rpmb.h"
#include "msdc.h"
#include "sec.h"

static const char * const rpmb_err_msg[] = {
    "",
    "General failure",
    "Authentication failure",
    "Counter failure",
    "Address failure",
    "Write failure",
    "Read failure",
    "Authentication key not yet programmed",

};

u16 cpu_to_be16p(u16 *p)
{
    return (((*p << 8)&0xFF00) | (*p >> 8));
}

u32 cpu_to_be32p(u32 *p)
{
    return (((*p & 0xFF) << 24) | ((*p & 0xFF00) << 8) | ((*p & 0xFF0000) >> 8) | (*p & 0xFF000000) >> 24 );
}

static void reverse_endian(void *data, u32 sz)
{
    u32 i;
    char tmp;
    char *swp=(char *)data;
    for(i = 0 ; i< (sz/2); ++i) {
        tmp = swp[i];
        swp[i]=swp[sz -1 -i];
        swp[sz -1 -i] = tmp;
    }
}

static int mmc_rpmb_pre_frame(struct mmc_rpmb_req *rpmb_req)
{
    struct mmc_rpmb_cfg *rpmb_cfg = rpmb_req->rpmb_cfg;
    u8 *data_frame = rpmb_req->data_frame;
    u16 blks = rpmb_cfg->blk_cnt;
    u16 addr;
    u16 type;
    u8 *nonce = rpmb_cfg->nonce;
    u32 wc, maclen;

    memset(data_frame, 0, 512 * blks);

    type = cpu_to_be16p(&rpmb_cfg->type);

    if (rpmb_cfg->type == RPMB_PROGRAM_KEY) {
        memcpy(data_frame + RPMB_TYPE_BEG, &type, 2);
        memcpy(data_frame + RPMB_MAC_BEG, rpmb_cfg->mac, RPMB_SZ_MAC);
    }
    else if (rpmb_cfg->type == RPMB_GET_WRITE_COUNTER ||
             rpmb_cfg->type == RPMB_READ_DATA) {

        /*
         * One package prepared
         * This request needs Nonce and type
         * If is data read, then also need addr
         */
        memcpy(data_frame + RPMB_TYPE_BEG, &type, 2);

        if (type == RPMB_READ_DATA) {
            addr = cpu_to_be16p(&rpmb_cfg->addr);
            memcpy(data_frame + RPMB_ADDR_BEG, &addr, 2);
        }

        /* convert Nonce code */
        memcpy(data_frame + RPMB_NONCE_BEG, nonce, RPMB_SZ_NONCE);
    }
    else if (rpmb_cfg->type == RPMB_WRITE_DATA) {
        memcpy(data_frame + RPMB_TYPE_BEG, &type, 2);
        addr = cpu_to_be16p(&rpmb_cfg->addr);
        memcpy(data_frame + RPMB_ADDR_BEG, &addr, 2);
        wc = cpu_to_be32p(rpmb_cfg->wc);
        memcpy(data_frame + RPMB_WCOUNTER_BEG, &wc, 4);
        blks = cpu_to_be16p(&rpmb_cfg->blk_cnt);
        memcpy(data_frame + RPMB_BLKS_BEG, &blks, 2);

        memcpy(data_frame + RPMB_DATA_BEG, rpmb_cfg->data, RPMB_SZ_DATA);
        reverse_endian(data_frame + RPMB_DATA_BEG, RPMB_SZ_DATA);

        maclen = RPMB_SZ_MAC;
        seclib_rpmb_mac_init(data_frame+RPMB_DATA_BEG, 512-RPMB_DATA_BEG);
        seclib_rpmb_mac_done(data_frame+RPMB_MAC_BEG, &maclen);
    }

    return 0;
}

static int mmc_rpmb_post_frame(struct mmc_rpmb_req *rpmb_req)
{
    struct mmc_rpmb_cfg *rpmb_cfg = rpmb_req->rpmb_cfg;
    u8 *data_frame = rpmb_req->data_frame;
    u16 result;
    u32 maclen;
    u8 mac[RPMB_SZ_MAC];

    memcpy(&result, data_frame + RPMB_RES_BEG, 2);
    rpmb_cfg->result = cpu_to_be16p(&result);

    if (rpmb_cfg->type == RPMB_GET_WRITE_COUNTER ||
        rpmb_cfg->type == RPMB_WRITE_DATA) {

        *(rpmb_cfg->wc) = cpu_to_be32p(&data_frame[RPMB_WCOUNTER_BEG]);

/*        printf("%s, rpmb_cfg->wc = %x\n", __func__, *(rpmb_cfg->wc)); */
    }

    if (rpmb_cfg->type == RPMB_GET_WRITE_COUNTER ||
        rpmb_cfg->type == RPMB_READ_DATA) {

        /* nonce check */
        if(0!= memcmp(rpmb_cfg->nonce, data_frame + RPMB_NONCE_BEG, RPMB_SZ_NONCE))
        {
            printf("%s, nonce mismatched!\n", __func__);
            return -1;
        }

        /* mac check */
        maclen = RPMB_SZ_MAC;
        seclib_rpmb_mac_init(data_frame+RPMB_DATA_BEG, 512-RPMB_DATA_BEG);
        seclib_rpmb_mac_done(mac, &maclen);

        if(0!= memcmp(mac, data_frame + RPMB_MAC_BEG, RPMB_SZ_MAC))
        {
            printf("%s, mac mismatched!\n", __func__);
            return -2;
        }
    }

    if (rpmb_cfg->mac) {
        /*
         * To do. compute if mac is legal or not. Current we don't do this since we just perform get wc to check if we need set key.
         */

    }

    return 0;
}


static int mmc_rpmb_send_command(struct mmc_card *card, u8 *data_frame, u16 blks, u16 type, u8 req_type)
{
    /*struct mmc_command cmd;*/
    struct mmc_host *host = card->host;
    int err;

	printf("%s -> req_type=0x%x, type=0x%x, blks=0x%x\n",
			__func__, req_type, type, blks);
    /*
     * CMD23

    cmd.opcode  = MMC_CMD_SET_BLOCK_COUNT;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = blks;
    if ((req_type == RPMB_REQ) && type == RPMB_WRITE_DATA || type == RPMB_PROGRAM_KEY)
        cmd.arg |= 1 << 31;

    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    err = mmc_cmd(host, &cmd);
    if (err) {
        printf("%s: CMD23 failed. (%d)\n", __func__, err);
    }
    */

    /*
     * Auto CMD23 and CMD25 or CMD18
     */
    if ((req_type == RPMB_REQ && type == RPMB_WRITE_DATA) || type == RPMB_PROGRAM_KEY)
        msdc_set_reliable_write(host, 1);
	else
        msdc_set_reliable_write(host, 0);

    msdc_set_autocmd(host, MSDC_AUTOCMD23, 1);

    if (req_type == RPMB_REQ) {

        //err = mmc_dev_bwrite(card, 0, 1, data_frame);
        err = mmc_block_write(0, 0, blks, data_frame);
        //err = mmc_rpmb_write(host, );
    }
    else {
        //err = mmc_dev_bread(card, 0, 1, data_frame);
        err = mmc_block_read(0, 0, blks, data_frame);
    }

    msdc_set_autocmd(host, MSDC_AUTOCMD23, 0);

    if (err)
        printf("%s: CMD%s failed. (%d)\n", __func__, ((req_type==RPMB_REQ) ? "25":"18"), err);

    return err;
}

static int mmc_rpmb_start_req(struct mmc_card *card, struct mmc_rpmb_req *rpmb_req)
{
    int err = 0;
    u16 blks = rpmb_req->rpmb_cfg->blk_cnt;
    u16 type = rpmb_req->rpmb_cfg->type;
    u8 *data_frame = rpmb_req->data_frame;

   /*
    * STEP 1: send request to RPMB partition
    */
    if (type == RPMB_WRITE_DATA)
        err = mmc_rpmb_send_command(card, data_frame, blks, type, RPMB_REQ);
    else
        err = mmc_rpmb_send_command(card, data_frame, 1, type, RPMB_REQ);

    if (err) {
        printf("%s step 1, request failed (%d)\n", __func__, err);
        goto out;
    }

   /*
    * STEP 2: check write result
    * Only for WRITE_DATA or Program key
    */
    memset(data_frame, 0, 512 * blks);

    if (type == RPMB_WRITE_DATA || type == RPMB_PROGRAM_KEY) {
        data_frame[RPMB_TYPE_BEG + 1] = RPMB_RESULT_READ;
        err = mmc_rpmb_send_command(card, data_frame, 1, RPMB_RESULT_READ, RPMB_REQ);
        if (err) {
            printf("%s step 2, request result failed (%d)\n", __func__, err);
            goto out;
        }
    }

    /*
     * STEP 3: get response from RPMB partition
     */
    data_frame[RPMB_TYPE_BEG] = 0;
    data_frame[RPMB_TYPE_BEG + 1] = type;

    if (type == RPMB_READ_DATA)
        err = mmc_rpmb_send_command(card, data_frame, blks, type, RPMB_RESP);
    else
        err = mmc_rpmb_send_command(card, data_frame, 1, type, RPMB_RESP);

    if (err) {
        printf("%s step 3, response failed (%d)\n", __func__, err);
    }

out:
    return err;
}

int mmc_rpmb_check_result(u16 result)
{
    if (result) {
        printf("%s %s %s\n", __func__, rpmb_err_msg[result & 0x7],
            (result & 0x80) ? "Write counter has expired" : "");
    }

    return result;
}

int mmc_rpmb_get_wc(u32 *wc)
{
    struct mmc_host *host = mmc_get_host(0);
    struct mmc_card *card = mmc_get_card(0);
    struct mmc_rpmb_cfg rpmb_cfg;
    struct mmc_rpmb_req rpmb_req;
    u8 *ext_csd = &card->raw_ext_csd[0];
    u8 rpmb_frame[512];
    u8 nonce[16];
    u8 val;
    int ret = 0, i;

    memset(&rpmb_cfg, 0, sizeof(struct mmc_rpmb_cfg));
    seclib_get_random_data(nonce, 16);

    rpmb_cfg.type = RPMB_GET_WRITE_COUNTER;
    rpmb_cfg.result = 0;
    rpmb_cfg.blk_cnt = 1;
    rpmb_cfg.addr = 0;
    rpmb_cfg.wc = wc;
    rpmb_cfg.nonce = nonce;
    rpmb_cfg.data = NULL;
    rpmb_cfg.mac = NULL;

    /*
     * 1. Switch to RPMB partition.
     */
    val = (ext_csd[EXT_CSD_PART_CFG] & ~0x7) | (EXT_CSD_PART_CFG_RPMB_PART & 0x7);
    ret = mmc_set_part_config(card, val);
    if (ret) {
        printf("%s, mmc_set_part_config failed!! (%x)\n", __func__, ret);
        return ret;
    }

    printf("%s, mmc_set_part_config done!!\n", __func__);

    rpmb_req.rpmb_cfg = &rpmb_cfg;
    rpmb_req.data_frame = rpmb_frame;

    /*
     * 2. Prepare program key data frame.
     */
    ret = mmc_rpmb_pre_frame(&rpmb_req);
    if (ret) {
        printf("%s, mmc_rpmb_req_handle failed!! (%x)\n", __func__, ret);
        return ret;
    }

    printf("%s, mmc_rpmb_pre_frame done!!\n", __func__);

    /*
     * 3. CMD 23 and followed CMD25/18 procedure.
     */
    ret = mmc_rpmb_start_req(card, &rpmb_req);
    if (ret) {
        printf("%s, mmc_rpmb_part_ops failed!! (%x)\n", __func__, ret);
        return ret;
    }

    printf("%s, mmc_rpmb_start_req done!!\n", __func__);

    ret = mmc_rpmb_post_frame(&rpmb_req);

    printf("%s, rpmb_req.result=%x\n", __func__, rpmb_cfg.result);
    if (ret) {
        if (rpmb_cfg.result != 0)
            return rpmb_cfg.result;
        /* -1 for nonce mismatch, -2 for mac mismatch */
        return ret;
    }

    //ret = be16_to_cpu(rpmb_req.result);

    /*
     * 4. Check result.
     */
    ret = mmc_rpmb_check_result(rpmb_cfg.result);

    return ret;
}

int mmc_rpmb_set_key(u8 *key)
{
    struct mmc_host *host = mmc_get_host(0);
    struct mmc_card *card = mmc_get_card(0);
    struct mmc_rpmb_cfg rpmb_cfg;
    struct mmc_rpmb_req rpmb_req;
    u8 *ext_csd = &card->raw_ext_csd[0];
    u16 result = 0;
    u8 rpmb_frame[512];
    u8 val;
    int ret, i;
    u32 wc;

    ret = mmc_rpmb_get_wc(&wc);
    if (!ret) {
        printf("mmc rpmb key is already programmed!!\n");
        return ret;
    }

    memset(&rpmb_cfg, 0, sizeof(struct mmc_rpmb_cfg));

    rpmb_cfg.type = RPMB_PROGRAM_KEY;
    rpmb_cfg.result = 0;
    rpmb_cfg.blk_cnt = 1;
    rpmb_cfg.addr = 0;
    rpmb_cfg.wc = NULL;
    rpmb_cfg.nonce = NULL;
    rpmb_cfg.data = NULL;
    rpmb_cfg.mac = key;

    /*
     * 1. Switch to RPMB partition.
     */
    val = (ext_csd[EXT_CSD_PART_CFG] & ~0x7) | (EXT_CSD_PART_CFG_RPMB_PART & 0x7);
    ret = mmc_set_part_config(card, val);
    if (ret) {
        printf("%s, mmc_set_part_config failed!! (%x)\n", __func__, ret);
        return ret;
    }

    rpmb_req.rpmb_cfg = &rpmb_cfg;
    rpmb_req.data_frame = rpmb_frame;

    /*
     * 2. Prepare program key data frame.
     */
    ret = mmc_rpmb_pre_frame(&rpmb_req);
    if (ret) {
        printf("%s, mmc_rpmb_req_handle failed!! (%x)\n", __func__, ret);
        return ret;
    }

    /*
     * 3. CMD 23 and followed CMD25/18 procedure.
     */
    ret = mmc_rpmb_start_req(card, &rpmb_req);
    if (ret) {
        printf("%s, mmc_rpmb_part_ops failed!! (%x)\n", __func__, ret);
        return ret;
    }

    ret = mmc_rpmb_post_frame(&rpmb_req);

    printf("%s, rpmb_req.result=%x\n", __func__, rpmb_cfg.result);

    //ret = be16_to_cpu(rpmb_req.result);

    /*
     * 4. Check result.
     */
    ret = mmc_rpmb_check_result(rpmb_cfg.result);
    if (ret == 0)
        printf("RPMB key is successfully programmed!!\n");



    return ret;

}

u32 mmc_rpmb_get_size()
{
    struct mmc_card *card = mmc_get_card(0);
    u8 *ext_csd = &card->raw_ext_csd[0];

    return ext_csd[EXT_CSD_RPMB_SIZE_MULT] * 128 * 1024;
}

int mmc_rpmb_block0_read(u8 data[256])
{
    struct mmc_host *host = mmc_get_host(0);
    struct mmc_card *card = mmc_get_card(0);
    struct mmc_rpmb_cfg rpmb_cfg;
    struct mmc_rpmb_req rpmb_req;
    u8 *ext_csd = &card->raw_ext_csd[0];
    u8 rpmb_frame[512];
    u8 nonce[16];
    u8 val;
    int ret = 0, i;

    memset(&rpmb_cfg, 0, sizeof(struct mmc_rpmb_cfg));
    seclib_get_random_data(nonce, 16);

    rpmb_cfg.type = RPMB_READ_DATA;
    rpmb_cfg.result = 0;
    rpmb_cfg.blk_cnt = 1;
    rpmb_cfg.addr = 0;
    rpmb_cfg.wc = NULL;
    rpmb_cfg.nonce = nonce;
    rpmb_cfg.data = NULL;
    rpmb_cfg.mac = NULL;


    /*
     * 1. Switch to RPMB partition.
     */
    val = (ext_csd[EXT_CSD_PART_CFG] & ~0x7) | (EXT_CSD_PART_CFG_RPMB_PART & 0x7);
    ret = mmc_set_part_config(card, val);
    if (ret) {
        printf("%s, mmc_set_part_config failed!! (%x)\n", __func__, ret);
        return ret;
    }

/*    printf("%s, mmc_set_part_config done!!\n", __func__); */

    rpmb_req.rpmb_cfg = &rpmb_cfg;
    rpmb_req.data_frame = rpmb_frame;

    /*
     * 2. Prepare program key data frame.
     */
    ret = mmc_rpmb_pre_frame(&rpmb_req);
    if (ret) {
        printf("%s, mmc_rpmb_req_handle failed!! (%x)\n", __func__, ret);
        return ret;
    }

    /*
     * 3. CMD 23 and followed CMD25/18 procedure.
     */
    ret = mmc_rpmb_start_req(card, &rpmb_req);
    if (ret) {
        printf("%s, mmc_rpmb_part_ops failed!! (%x)\n", __func__, ret);
        return ret;
    }

    ret = mmc_rpmb_post_frame(&rpmb_req);
    if (ret) {
        /* -1 for nonce mismatch, -2 for mac mismatch */
        return ret;
    }

/*    printf("%s, rpmb_req.result=%x\n", __func__, rpmb_cfg.result); */

    //ret = be16_to_cpu(rpmb_req.result);

    /*
     * 4. Check result.
     */
    ret = mmc_rpmb_check_result(rpmb_cfg.result);
    if(ret == 0)
    {
        /* copy the data for returning */
        memcpy(data, rpmb_frame+RPMB_DATA_BEG, RPMB_SZ_DATA);
        reverse_endian(data, RPMB_SZ_DATA);
    }

    return ret;
}

int mmc_rpmb_block0_write(u8 data[256], u32 wc)
{
    struct mmc_host *host = mmc_get_host(0);
    struct mmc_card *card = mmc_get_card(0);
    struct mmc_rpmb_cfg rpmb_cfg;
    struct mmc_rpmb_req rpmb_req;
    u8 *ext_csd = &card->raw_ext_csd[0];
    u8 rpmb_frame[512];
    u8 nonce[16];
    u8 val;
    int ret = 0, i;

    memset(&rpmb_cfg, 0, sizeof(struct mmc_rpmb_cfg));
    seclib_get_random_data(nonce, 16);

    rpmb_cfg.type = RPMB_WRITE_DATA;
    rpmb_cfg.result = 0;
    rpmb_cfg.blk_cnt = 1;
    rpmb_cfg.addr = 0;
    rpmb_cfg.wc = &wc;
    rpmb_cfg.nonce = nonce;
    rpmb_cfg.data = data;
    rpmb_cfg.mac = NULL;


    /*
     * 1. Switch to RPMB partition.
     */
    val = (ext_csd[EXT_CSD_PART_CFG] & ~0x7) | (EXT_CSD_PART_CFG_RPMB_PART & 0x7);
    ret = mmc_set_part_config(card, val);
    if (ret) {
        printf("%s, mmc_set_part_config failed!! (%x)\n", __func__, ret);
        return ret;
    }

/*    printf("%s, mmc_set_part_config done!!\n", __func__); */

    rpmb_req.rpmb_cfg = &rpmb_cfg;
    rpmb_req.data_frame = rpmb_frame;

    /*
     * 2. Prepare program key data frame.
     */
    ret = mmc_rpmb_pre_frame(&rpmb_req);
    if (ret) {
        printf("%s, mmc_rpmb_req_handle failed!! (%x)\n", __func__, ret);
        return ret;
    }

    /*
     * 3. CMD 23 and followed CMD25/18 procedure.
     */
    ret = mmc_rpmb_start_req(card, &rpmb_req);
    if (ret) {
        printf("%s, mmc_rpmb_part_ops failed!! (%x)\n", __func__, ret);
        return ret;
    }

    ret = mmc_rpmb_post_frame(&rpmb_req);

/*    printf("%s, rpmb_req.result=%x\n", __func__, rpmb_cfg.result); */

    //ret = be16_to_cpu(rpmb_req.result);

    /*
     * 4. Check result.
     */
    ret = mmc_rpmb_check_result(rpmb_cfg.result);

    return ret;
}

