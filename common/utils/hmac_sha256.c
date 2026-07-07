#include "hmac_sha256.h"

#include <string.h>

#define SHA256_BLOCK_SIZE 64U
#define SHA256_DIGEST_SIZE 32U

typedef struct sha256_ctx {
    uint32_t state[8];
    uint64_t bit_len;
    uint8_t data[SHA256_BLOCK_SIZE];
    uint32_t data_len;
} sha256_ctx_t;

static const uint32_t sha256_k[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
    0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
    0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
    0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
    0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
    0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
    0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
    0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
    0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
    0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
    0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
    0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
    0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

static uint32_t rotr32(uint32_t value, uint32_t bits)
{
    return (value >> bits) | (value << (32U - bits));
}

static void sha256_transform(sha256_ctx_t *ctx, const uint8_t data[64])
{
    uint32_t m[64];
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t f;
    uint32_t g;
    uint32_t h;
    uint32_t i;

    for (i = 0U; i < 16U; ++i) {
        m[i] = ((uint32_t)data[i * 4U] << 24) |
               ((uint32_t)data[i * 4U + 1U] << 16) |
               ((uint32_t)data[i * 4U + 2U] << 8) |
               ((uint32_t)data[i * 4U + 3U]);
    }
    for (i = 16U; i < 64U; ++i) {
        uint32_t s0 = rotr32(m[i - 15U], 7U) ^ rotr32(m[i - 15U], 18U) ^ (m[i - 15U] >> 3);
        uint32_t s1 = rotr32(m[i - 2U], 17U) ^ rotr32(m[i - 2U], 19U) ^ (m[i - 2U] >> 10);
        m[i] = m[i - 16U] + s0 + m[i - 7U] + s1;
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0U; i < 64U; ++i) {
        uint32_t s1 = rotr32(e, 6U) ^ rotr32(e, 11U) ^ rotr32(e, 25U);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + s1 + ch + sha256_k[i] + m[i];
        uint32_t s0 = rotr32(a, 2U) ^ rotr32(a, 13U) ^ rotr32(a, 22U);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = s0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void sha256_init(sha256_ctx_t *ctx)
{
    ctx->data_len = 0U;
    ctx->bit_len = 0U;
    ctx->state[0] = 0x6a09e667UL;
    ctx->state[1] = 0xbb67ae85UL;
    ctx->state[2] = 0x3c6ef372UL;
    ctx->state[3] = 0xa54ff53aUL;
    ctx->state[4] = 0x510e527fUL;
    ctx->state[5] = 0x9b05688cUL;
    ctx->state[6] = 0x1f83d9abUL;
    ctx->state[7] = 0x5be0cd19UL;
}

static void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, size_t len)
{
    size_t i;

    for (i = 0U; i < len; ++i) {
        ctx->data[ctx->data_len++] = data[i];
        if (ctx->data_len == SHA256_BLOCK_SIZE) {
            sha256_transform(ctx, ctx->data);
            ctx->bit_len += 512U;
            ctx->data_len = 0U;
        }
    }
}

static void sha256_final(sha256_ctx_t *ctx, uint8_t hash[32])
{
    uint32_t i = ctx->data_len;
    uint32_t j;

    ctx->data[i++] = 0x80U;
    if (i > 56U) {
        while (i < 64U) {
            ctx->data[i++] = 0U;
        }
        sha256_transform(ctx, ctx->data);
        i = 0U;
    }
    while (i < 56U) {
        ctx->data[i++] = 0U;
    }

    ctx->bit_len += (uint64_t)ctx->data_len * 8ULL;
    ctx->data[63] = (uint8_t)(ctx->bit_len);
    ctx->data[62] = (uint8_t)(ctx->bit_len >> 8);
    ctx->data[61] = (uint8_t)(ctx->bit_len >> 16);
    ctx->data[60] = (uint8_t)(ctx->bit_len >> 24);
    ctx->data[59] = (uint8_t)(ctx->bit_len >> 32);
    ctx->data[58] = (uint8_t)(ctx->bit_len >> 40);
    ctx->data[57] = (uint8_t)(ctx->bit_len >> 48);
    ctx->data[56] = (uint8_t)(ctx->bit_len >> 56);
    sha256_transform(ctx, ctx->data);

    for (j = 0U; j < 4U; ++j) {
        hash[j] = (uint8_t)(ctx->state[0] >> (24U - j * 8U));
        hash[j + 4U] = (uint8_t)(ctx->state[1] >> (24U - j * 8U));
        hash[j + 8U] = (uint8_t)(ctx->state[2] >> (24U - j * 8U));
        hash[j + 12U] = (uint8_t)(ctx->state[3] >> (24U - j * 8U));
        hash[j + 16U] = (uint8_t)(ctx->state[4] >> (24U - j * 8U));
        hash[j + 20U] = (uint8_t)(ctx->state[5] >> (24U - j * 8U));
        hash[j + 24U] = (uint8_t)(ctx->state[6] >> (24U - j * 8U));
        hash[j + 28U] = (uint8_t)(ctx->state[7] >> (24U - j * 8U));
    }
}

static void sha256_digest(const uint8_t *data, size_t len, uint8_t hash[32])
{
    sha256_ctx_t ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
}

int hmac_sha256_hex(const uint8_t *key,
                    size_t key_len,
                    const uint8_t *message,
                    size_t message_len,
                    char out_hex[65])
{
    uint8_t key_block[SHA256_BLOCK_SIZE];
    uint8_t inner_pad[SHA256_BLOCK_SIZE];
    uint8_t outer_pad[SHA256_BLOCK_SIZE];
    uint8_t inner_hash[SHA256_DIGEST_SIZE];
    uint8_t digest[SHA256_DIGEST_SIZE];
    sha256_ctx_t ctx;
    size_t i;
    static const char hex[] = "0123456789abcdef";

    if ((key == 0) || (message == 0) || (out_hex == 0)) {
        return -1;
    }

    memset(key_block, 0, sizeof(key_block));
    if (key_len > SHA256_BLOCK_SIZE) {
        sha256_digest(key, key_len, key_block);
    } else {
        memcpy(key_block, key, key_len);
    }

    for (i = 0U; i < SHA256_BLOCK_SIZE; ++i) {
        inner_pad[i] = (uint8_t)(key_block[i] ^ 0x36U);
        outer_pad[i] = (uint8_t)(key_block[i] ^ 0x5cU);
    }

    sha256_init(&ctx);
    sha256_update(&ctx, inner_pad, sizeof(inner_pad));
    sha256_update(&ctx, message, message_len);
    sha256_final(&ctx, inner_hash);

    sha256_init(&ctx);
    sha256_update(&ctx, outer_pad, sizeof(outer_pad));
    sha256_update(&ctx, inner_hash, sizeof(inner_hash));
    sha256_final(&ctx, digest);

    for (i = 0U; i < SHA256_DIGEST_SIZE; ++i) {
        out_hex[i * 2U] = hex[(digest[i] >> 4) & 0x0FU];
        out_hex[i * 2U + 1U] = hex[digest[i] & 0x0FU];
    }
    out_hex[64] = '\0';
    return 0;
}
