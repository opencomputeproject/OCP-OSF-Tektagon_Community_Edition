#ifdef CONFIG_ECDSA_ASPEED
	#define ECDSA_DRV_NAME DT_LABEL(DT_INST(0, aspeed_ecdsa))
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypto/ecdsa_structs.h>
#include <crypto/ecdsa.h>
#include <zephyr.h>

int aspeed_ecdsa_verify_middlelayer(uint8_t *public_key_x, uint8_t *public_key_y,
				    const uint8_t *digest, uint8_t *signature_r,
				    uint8_t *signature_s)
{
	int status = 0;
	const struct device *dev = device_get_binding(ECDSA_DRV_NAME);
	struct ecdsa_ctx ini;
	struct ecdsa_pkt pkt;
	struct ecdsa_key ek;

	ek.curve_id = ECC_CURVE_NIST_P256;
	ek.qx = public_key_x;
	ek.qy = public_key_y;
	pkt.m = digest;
	pkt.r = signature_r;
	pkt.s = signature_s;
	pkt.m_len = 32;
	pkt.r_len = 32;
	pkt.s_len = 32;
	status = ecdsa_begin_session(dev, &ini, &ek);
	if (status) {
		printk("ecdsa_begin_session fail: %d\r\n", status);
	}
	status = ecdsa_verify(&ini, &pkt);
	if (status) {
		printk("Fail\r\n");
	} else {
		printk("Success\r\n");
	}
	ecdsa_free_session(dev, &ini);

	return status;
}
