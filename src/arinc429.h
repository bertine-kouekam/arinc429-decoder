#ifndef ARINC429_H
#define ARINC429_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    A429_SSM_FAILURE_WARNING  = 0,
    A429_SSM_NO_COMPUTED_DATA = 1,
    A429_SSM_FUNCTIONAL_TEST  = 2,
    A429_SSM_NORMAL_OPERATION = 3
} a429_ssm_t;

typedef enum {
    A429_OK = 0,
    A429_ERR_PARITY,
    A429_ERR_LABEL,
    A429_ERR_PARAM,
    A429_ERR_RANGE
} a429_status_t;

typedef struct {
    uint8_t label;
    uint8_t sig_bits;
    float   range;
} a429_bnr_param_t;

bool a429_parity_ok(uint32_t word);

uint8_t a429_label(uint32_t word);

uint8_t a429_sdi(uint32_t word);

a429_ssm_t a429_ssm(uint32_t word);

a429_status_t a429_bnr_decode(uint32_t word, const a429_bnr_param_t *param, float *value);

a429_status_t a429_bnr_encode(const a429_bnr_param_t *param, float value, uint8_t sdi,
                              a429_ssm_t ssm, uint32_t *word);

#endif
