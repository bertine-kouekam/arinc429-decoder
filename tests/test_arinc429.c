#include "arinc429.h"

#include <math.h>
#include <stdio.h>

static int tests_run = 0;
static int tests_failed = 0;

static void check(int ok, const char *expr, int line)
{
    tests_run++;
    if (!ok) {
        tests_failed++;
        printf("ECHEC ligne %d : %s\n", line, expr);
    }
}

#define CHECK(cond) check((cond), #cond, __LINE__)

static const a429_bnr_param_t ALTITUDE = { 0203, 17u, 131072.0f };
static const a429_bnr_param_t AIRSPEED = { 0206, 14u, 1024.0f };

static int close_to(float a, float b)
{
    float diff = a - b;
    if (diff < 0.0f) {
        diff = -diff;
    }
    return diff <= 0.001f;
}

static a429_status_t encode_alt(float value, uint32_t *word)
{
    return a429_bnr_encode(&ALTITUDE, value, 0u, A429_SSM_NORMAL_OPERATION, word);
}

static void test_parity(void)
{
    CHECK(!a429_parity_ok(0x00000000u));
    CHECK(a429_parity_ok(0x80000000u));
    CHECK(a429_parity_ok(0x6445C0C1u));
    CHECK(!a429_parity_ok(0x6445C0C0u));
}

static void test_label(void)
{
    CHECK(a429_label(0x000000C1u) == 0203);
    CHECK(a429_label(0x00000061u) == 0206);
    CHECK(a429_label(0x00000050u) == 0012);
    CHECK(a429_label(0xFFFFFF00u) == 0000);
}

static void test_sdi_ssm(void)
{
    CHECK(a429_sdi(0x00000100u) == 1u);
    CHECK(a429_sdi(0x00000300u) == 3u);
    CHECK(a429_sdi(0xFFFFFCFFu) == 0u);
    CHECK(a429_ssm(0x00000000u) == A429_SSM_FAILURE_WARNING);
    CHECK(a429_ssm(0x20000000u) == A429_SSM_NO_COMPUTED_DATA);
    CHECK(a429_ssm(0x40000000u) == A429_SSM_FUNCTIONAL_TEST);
    CHECK(a429_ssm(0x60000000u) == A429_SSM_NORMAL_OPERATION);
}

static void test_encode_altitude(void)
{
    uint32_t word = 0u;

    CHECK(encode_alt(35000.0f, &word) == A429_OK);
    CHECK(word == 0x6445C0C1u);
    CHECK(encode_alt(1.0f, &word) == A429_OK);
    CHECK(word == 0xE00008C1u);
}

static void test_encode_negative(void)
{
    uint32_t word = 0u;

    CHECK(encode_alt(-1000.0f, &word) == A429_OK);
    CHECK(word == 0x7FE0C0C1u);
}

static void test_encode_airspeed(void)
{
    uint32_t word = 0u;

    CHECK(a429_bnr_encode(&AIRSPEED, 250.5f, 1u, A429_SSM_NORMAL_OPERATION, &word) == A429_OK);
    CHECK(word == 0x63EA0161u);
}

static void test_decode(void)
{
    float value = 0.0f;

    CHECK(a429_bnr_decode(0x6445C0C1u, &ALTITUDE, &value) == A429_OK);
    CHECK(close_to(value, 35000.0f));
    CHECK(a429_bnr_decode(0x7FE0C0C1u, &ALTITUDE, &value) == A429_OK);
    CHECK(close_to(value, -1000.0f));
    CHECK(a429_bnr_decode(0x63EA0161u, &AIRSPEED, &value) == A429_OK);
    CHECK(close_to(value, 250.5f));
    CHECK(a429_sdi(0x63EA0161u) == 1u);
}

static void test_decode_ignores_padding(void)
{
    float value = 0.0f;

    CHECK(a429_bnr_decode(0xE3EA0561u, &AIRSPEED, &value) == A429_OK);
    CHECK(close_to(value, 250.5f));
}

static void test_decode_errors(void)
{
    float value = 123.0f;

    CHECK(a429_bnr_decode(0x6445C0C1u ^ 0x00004000u, &ALTITUDE, &value) == A429_ERR_PARITY);
    CHECK(a429_bnr_decode(0x6445C0C1u, &AIRSPEED, &value) == A429_ERR_LABEL);
    CHECK(close_to(value, 123.0f));
}

static void test_encode_range(void)
{
    uint32_t word = 0u;
    float value = 0.0f;

    CHECK(encode_alt(131071.0f, &word) == A429_OK);
    CHECK(a429_bnr_decode(word, &ALTITUDE, &value) == A429_OK);
    CHECK(close_to(value, 131071.0f));
    CHECK(encode_alt(-131072.0f, &word) == A429_OK);
    CHECK(a429_bnr_decode(word, &ALTITUDE, &value) == A429_OK);
    CHECK(close_to(value, -131072.0f));

    word = 0xAAAAAAAAu;
    CHECK(encode_alt(131072.0f, &word) == A429_ERR_RANGE);
    CHECK(encode_alt(-131073.0f, &word) == A429_ERR_RANGE);
    CHECK(encode_alt(1.0e30f, &word) == A429_ERR_RANGE);
    CHECK(encode_alt(INFINITY, &word) == A429_ERR_RANGE);
    CHECK(encode_alt(NAN, &word) == A429_ERR_RANGE);
    CHECK(word == 0xAAAAAAAAu);
}

static void test_invalid_arguments(void)
{
    const a429_bnr_param_t zero_bits = { 0203, 0u, 131072.0f };
    const a429_bnr_param_t too_many_bits = { 0203, 19u, 131072.0f };
    const a429_bnr_param_t zero_range = { 0203, 17u, 0.0f };
    uint32_t word = 0u;
    float value = 0.0f;

    CHECK(a429_bnr_decode(0x6445C0C1u, NULL, &value) == A429_ERR_PARAM);
    CHECK(a429_bnr_decode(0x6445C0C1u, &ALTITUDE, NULL) == A429_ERR_PARAM);
    CHECK(a429_bnr_decode(0x6445C0C1u, &zero_bits, &value) == A429_ERR_PARAM);
    CHECK(a429_bnr_decode(0x6445C0C1u, &too_many_bits, &value) == A429_ERR_PARAM);
    CHECK(a429_bnr_decode(0x6445C0C1u, &zero_range, &value) == A429_ERR_PARAM);
    CHECK(encode_alt(0.0f, NULL) == A429_ERR_PARAM);
    CHECK(a429_bnr_encode(NULL, 0.0f, 0u, A429_SSM_NORMAL_OPERATION, &word) == A429_ERR_PARAM);
    CHECK(a429_bnr_encode(&ALTITUDE, 0.0f, 4u, A429_SSM_NORMAL_OPERATION, &word) == A429_ERR_PARAM);
}

static void test_round_trip(void)
{
    uint32_t word = 0u;
    float value = 0.0f;
    int i;

    for (i = -4; i <= 90; i++) {
        float altitude = 500.0f * (float)i;

        CHECK(a429_bnr_encode(&ALTITUDE, altitude, 2u, A429_SSM_NORMAL_OPERATION, &word) == A429_OK);
        CHECK(a429_parity_ok(word));
        CHECK(a429_label(word) == 0203);
        CHECK(a429_sdi(word) == 2u);
        CHECK(a429_ssm(word) == A429_SSM_NORMAL_OPERATION);
        CHECK(a429_bnr_decode(word, &ALTITUDE, &value) == A429_OK);
        CHECK(close_to(value, altitude));
    }
}

int main(void)
{
    test_parity();
    test_label();
    test_sdi_ssm();
    test_encode_altitude();
    test_encode_negative();
    test_encode_airspeed();
    test_decode();
    test_decode_ignores_padding();
    test_decode_errors();
    test_encode_range();
    test_invalid_arguments();
    test_round_trip();

    printf("%d verifications, %d echec(s)\n", tests_run, tests_failed);
    return (tests_failed == 0) ? 0 : 1;
}
