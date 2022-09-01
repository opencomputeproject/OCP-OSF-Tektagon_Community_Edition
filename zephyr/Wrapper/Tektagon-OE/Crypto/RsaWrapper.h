#include <crypto/rsa.h>


struct rsa_engine_wrapper{
    struct rsa_engine base;
};

int rsa_wrapper_init (struct rsa_engine_wrapper *engine);
int rsa_wrapper_sig_verify(struct rsa_engine *engine, const struct rsa_public_key *key,const uint8_t *signature, size_t sig_length, const uint8_t *match, size_t match_length);
