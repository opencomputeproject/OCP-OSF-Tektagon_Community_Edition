#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypto/ecdsa_structs.h>
#include <crypto/ecdsa.h>
#include <zephyr.h>

int aspeed_ecdsa_verify_middlelayer(uint8_t *public_key_x, uint8_t *public_key_y, const uint8_t *digest, uint8_t *signature_r, uint8_t *signature_s);