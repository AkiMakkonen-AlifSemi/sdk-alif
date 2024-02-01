/* Copyright (C) 2024 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/mhuv2_ipm.h>

#define ITERATIONS 10
const struct device *mhu1_r;
const struct device *mhu1_s;

static bool msg_sent;
static bool msg_received;
uint8_t recv_count;

static void recv_cb(const struct device *mhuv2_ipmdev, uint32_t *user_data)
{
	printk("MSG received is 0x%x\n", *user_data);
	if (recv_count > 0)
		++recv_count;
	else
		msg_received = true;
}

static void send_cb(const struct device *mhuv2_ipmdev, uint32_t *user_data)
{
	printk("data sent\n");
	msg_sent = true;
}

static void send(void)
{
	uint32_t set_mhu = 0xFACEFACE;

	msg_received = false;
	recv_count = 0;
	msg_sent = false;
	mhuv2_ipm_send(mhu1_s, 0, &set_mhu);
	while (!msg_sent)
		;
	msg_sent = false;
	set_mhu = 0xFADEFADE;
	mhuv2_ipm_send(mhu1_s, 1, &set_mhu);
	while (!msg_sent)
		;
	msg_sent = false;
}

int main(void)
{
	uint32_t recv_data;
	int i = 0;

	msg_received = false;
	msg_sent = false;
	printk("RTSS-HP RTSS-HE MHU 1 example on %s\n", CONFIG_BOARD);
	mhu1_r = DEVICE_DT_GET(DT_ALIAS(rtsshemhu1r));
	mhu1_s = DEVICE_DT_GET(DT_ALIAS(rtsshemhu1s));
	if (!device_is_ready(mhu1_r) || !device_is_ready(mhu1_s)) {
		printk("MHU devices not ready\n");
		return -1;
	}
	mhuv2_ipm_rb(mhu1_r, recv_cb, &recv_data);
	mhuv2_ipm_rb(mhu1_s, send_cb, NULL);
	/* wait until receiver gets ready */
	k_sleep(K_MSEC(500));
	while (i < ITERATIONS) {
		/* Let RTSS_HP initiate data transfer */
		send();
		while (!msg_received)
			;
		++i;
	}
	return 0;
}
