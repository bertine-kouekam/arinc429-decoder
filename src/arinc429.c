#include "arinc429.h"

#include <stddef.h>

#define A429_LABEL_MASK   0x000000FFu
#define A429_SDI_SHIFT    8u
#define A429_SDI_MASK     0x3u
#define A429_DATA_SHIFT   10u
#define A429_DATA_MASK    0x7FFFFu
#define A429_DATA_SIGN    0x40000u
#define A429_SSM_SHIFT    29u
#define A429_SSM_MASK     0x3u
#define A429_PARITY_BIT   0x80000000u

#define A429_DATA_BITS    18u
#define A429_FULL_SCALE   262144.0f

static uint8_t count_ones(uint32_t word)
{
    uint8_t count = 0u;
    uint8_t i;

    for (i = 0u; i < 32u; i++) {
        if ((word & 1u) != 0u) {
            count++;
        }
        word >>= 1u;
    }
    return count;
}

static uint8_t reverse_byte(uint8_t byte)
{
    uint8_t result = 0u;
    uint8_t i;

    for (i = 0u; i < 8u; i++) {
        result = (uint8_t)(result << 1u);
        if ((byte & 1u) != 0u) {
            result = (uint8_t)(result | 1u);
        }
        byte = (uint8_t)(byte >> 1u);
    }
    return result;
}

static bool param_is_valid(const a429_bnr_param_t *param)
{
    return (param != NULL) && (param->sig_bits >= 1u) && (param->sig_bits <= A429_DATA_BITS)
           && (param->range > 0.0f);
}

bool a429_parity_ok(uint32_t word)
{
    return (count_ones(word) % 2u) == 1u;
}

uint8_t a429_label(uint32_t word)
{
    return reverse_byte((uint8_t)(word & A429_LABEL_MASK));
}

uint8_t a429_sdi(uint32_t word)
{
    return (uint8_t)((word >> A429_SDI_SHIFT) & A429_SDI_MASK);
}

a429_ssm_t a429_ssm(uint32_t word)
{
    return (a429_ssm_t)((word >> A429_SSM_SHIFT) & A429_SSM_MASK);
}

a429_status_t a429_bnr_decode(uint32_t word, const a429_bnr_param_t *param, float *value)
{
    uint32_t field;
    uint32_t padding_mask;
    int32_t  raw;

    if (!param_is_valid(param) || (value == NULL)) {
        return A429_ERR_PARAM;
    }
    if (!a429_parity_ok(word)) {
        return A429_ERR_PARITY;
    }
    if (a429_label(word) != param->label) {
        return A429_ERR_LABEL;
    }

    field = (word >> A429_DATA_SHIFT) & A429_DATA_MASK;
    padding_mask = (1u << (A429_DATA_BITS - param->sig_bits)) - 1u;
    field &= ~padding_mask;

    raw = (int32_t)field;
    if ((field & A429_DATA_SIGN) != 0u) {
        raw -= (int32_t)(A429_DATA_MASK + 1u);
    }

    *value = ((float)raw * param->range) / A429_FULL_SCALE;
    return A429_OK;
}

a429_status_t a429_bnr_encode(const a429_bnr_param_t *param, float value, uint8_t sdi,
                              a429_ssm_t ssm, uint32_t *word)
{
    float    steps_f;
    int32_t  steps;
    int32_t  max_steps;
    int32_t  raw;
    uint32_t result;

    if (!param_is_valid(param) || (word == NULL) || (sdi > A429_SDI_MASK)
        || ((uint32_t)ssm > A429_SSM_MASK)) {
        return A429_ERR_PARAM;
    }

    max_steps = (int32_t)(1u << param->sig_bits);
    steps_f = (value / param->range) * (float)max_steps;
    if (!(steps_f < (float)max_steps) || !(steps_f > (-(float)max_steps - 1.0f))) {
        return A429_ERR_RANGE;
    }
    steps = (steps_f >= 0.0f) ? (int32_t)(steps_f + 0.5f) : (int32_t)(steps_f - 0.5f);
    if ((steps > (max_steps - 1)) || (steps < -max_steps)) {
        return A429_ERR_RANGE;
    }

    raw = steps * (int32_t)(1u << (A429_DATA_BITS - param->sig_bits));

    result = (uint32_t)reverse_byte(param->label);
    result |= (uint32_t)sdi << A429_SDI_SHIFT;
    result |= ((uint32_t)raw & A429_DATA_MASK) << A429_DATA_SHIFT;
    result |= (uint32_t)ssm << A429_SSM_SHIFT;
    if (!a429_parity_ok(result)) {
        result |= A429_PARITY_BIT;
    }

    *word = result;
    return A429_OK;
}
