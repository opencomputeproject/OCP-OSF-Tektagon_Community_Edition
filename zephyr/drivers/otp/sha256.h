/*
 * Copyright (C) 2021 ASPEED Technology Inc.
 */

#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>

#define SHA256_BLOCK_SIZE 32            /* SHA256 outputs a 32 byte digest */

struct SHA256_CTX {
	unsigned char data[64];
	unsigned int datalen;
	unsigned long long bitlen;
	unsigned int state[8];
};

void sha256_init(struct SHA256_CTX *ctx);
void sha256_update(struct SHA256_CTX *ctx, const unsigned char data[], size_t len);
void sha256_final(struct SHA256_CTX *ctx, unsigned char hash[]);

#endif /* SHA256_H */
