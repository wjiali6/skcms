/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifdef _MSC_VER
    #define _CRT_SECURE_NO_WARNINGS
    #define SKCMS_NORETURN __declspec(noreturn)
#else
    #include <stdnoreturn.h>
    #define SKCMS_NORETURN noreturn
#endif

#include "skcms.h"
#include <stdio.h>
#include <stdlib.h>

SKCMS_NORETURN
static void fatal(const char* msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(1);
}

static void signature_to_string(uint32_t sig, char* str) {
    str[0] = (char)(sig >> 24);
    str[1] = (char)(sig >> 16);
    str[2] = (char)(sig >>  8);
    str[3] = (char)(sig >>  0);
    str[4] = 0;
}

static void dump_sig_field(const char* name, uint32_t val) {
    char valStr[5];
    signature_to_string(val, valStr);
    printf("%20s : 0x%08X : '%s'\n", name, val, valStr);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: %s <ICC filename>\n", argv[0]);
        return 1;
    }

    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        fatal("Unable to open input file");
    }

    fseek(fp, 0L, SEEK_END);
    long slen = ftell(fp);
    if (slen <= 0) {
        fatal("ftell failed");
    }
    size_t len = (size_t)slen;
    rewind(fp);

    void* buf = malloc(len);
    size_t bytesRead = fread(buf, 1, len, fp);
    fclose(fp);
    if (bytesRead != len) {
        fatal("Unable to read file");
    }

    skcms_ICCProfile profile;
    if (!skcms_ICCProfile_parse(&profile, buf, bytesRead)) {
        fatal("Unable to parse ICC profile");
    }

    printf("%20s : 0x%08X : %u\n", "Size", profile.size, profile.size);
    dump_sig_field("CMM type", profile.cmm_type);
    printf("%20s : 0x%08X : %u.%u.%u\n", "Version", profile.version,
           profile.version >> 24, (profile.version >> 20) & 0xF, (profile.version >> 16) & 0xF);
    dump_sig_field("Profile class", profile.profile_class);
    dump_sig_field("Data color space", profile.data_color_space);
    dump_sig_field("PCS", profile.pcs);
    printf("%20s :            : %u-%02u-%02u %02u:%02u:%02u\n", "Creation date/time",
           profile.creation_date_time.year, profile.creation_date_time.month,
           profile.creation_date_time.day, profile.creation_date_time.hour,
           profile.creation_date_time.minute, profile.creation_date_time.second);
    dump_sig_field("Signature", profile.signature);
    dump_sig_field("Platform", profile.platform);
    printf("%20s : 0x%08X\n", "Flags", profile.flags);
    dump_sig_field("Device manufacturer", profile.device_manufacturer);
    dump_sig_field("Device model", profile.device_model);
    printf("%20s : 0x%08X\n", "Device attributes", (uint32_t)profile.device_attributes);
    printf("%20s : 0x%08X\n", "", (uint32_t)(profile.device_attributes >> 32));
    printf("%20s : 0x%08X : %u\n", "Rendering intent", profile.rendering_intent,
           profile.rendering_intent);
    printf("%20s :            : %f\n", "Illuminant X", (double)profile.illuminant_X);
    printf("%20s :            : %f\n", "Illuminant Y", (double)profile.illuminant_Y);
    printf("%20s :            : %f\n", "Illuminant Z", (double)profile.illuminant_Z);
    dump_sig_field("Creator", profile.creator);
    printf("%20s : 0x%08X : %u\n", "Tag count", profile.tag_count, profile.tag_count);

    printf("\n");

    printf(" Tag    : Type   : Size   : Data\n");
    printf(" ------ : ------ : ------ : --------\n");
    for (uint32_t i = 0; i < profile.tag_count; ++i) {
        skcms_ICCTag tag;
        skcms_ICCProfile_getTagByIndex(&profile, i, &tag);
        char tagSig[5];
        char typeSig[5];
        signature_to_string(tag.signature, tagSig);
        signature_to_string(tag.type, typeSig);
        printf(" '%s' : '%s' : %6u : %p\n", tagSig, typeSig, tag.size, (const void*)tag.buf);
    }

    printf("\n");

    skcms_TransferFunction tf;
    float max_error;
    if (skcms_ICCProfile_getTransferFunction(&profile, &tf)) {
        printf("TRC : %f, %f, %f, %f, %f, %f, %f\n",
               (double)tf.g, (double)tf.a, (double)tf.b, (double)tf.c,
               (double)tf.d, (double)tf.e, (double)tf.f);
    } else if (skcms_ICCProfile_approximateTransferFunction(&profile, &tf, &max_error)) {
        printf("~TRC: %f, %f, %f, %f, %f, %f, %f  (Max error: %f)\n",
               (double)tf.g, (double)tf.a, (double)tf.b, (double)tf.c,
               (double)tf.d, (double)tf.e, (double)tf.f, (double)max_error);

    }

    skcms_Matrix3x3 toXYZ;
    if (skcms_ICCProfile_toXYZD50(&profile, &toXYZ)) {
        printf("XYZ : | %.7f %.7f %.7f |\n"
               "      | %.7f %.7f %.7f |\n"
               "      | %.7f %.7f %.7f |\n",
               (double)toXYZ.vals[0][0], (double)toXYZ.vals[0][1], (double)toXYZ.vals[0][2],
               (double)toXYZ.vals[1][0], (double)toXYZ.vals[1][1], (double)toXYZ.vals[1][2],
               (double)toXYZ.vals[2][0], (double)toXYZ.vals[2][1], (double)toXYZ.vals[2][2]);
    }
    return 0;
}
