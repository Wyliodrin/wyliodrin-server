// #include <stdint.h>
// #include <stdlib.h>
// #include <string.h>
// #include "base64.h"

#include "base64.h"

// static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
//                                 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
//                                 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
//                                 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
//                                 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
//                                 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
//                                 'w', 'x', 'y', 'z', '0', '1', '2', '3',
//                                 '4', '5', '6', '7', '8', '9', '+', '/'};
// static char *decoding_table = NULL;
// static int mod_table[] = {0, 2, 1};


// char *wxmpp_base64_encode(const unsigned char *data,
//                     size_t input_length,
//                     size_t *output_length) {

//     *output_length = 4 * ((input_length + 2) / 3)+1;

//     char *encoded_data = malloc(*output_length);
//     memset (encoded_data, 0, sizeof (*output_length));
//     if (encoded_data == NULL) return NULL;

//     int i;
//     int j;
//     for (i = 0, j = 0; i < input_length;) {

//         uint32_t octet_a = i < input_length ? data[i++] : 0;
//         uint32_t octet_b = i < input_length ? data[i++] : 0;
//         uint32_t octet_c = i < input_length ? data[i++] : 0;

//         uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

//         encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
//         encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
//         encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
//         encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
//     }

//     for (i = 0; i < mod_table[input_length % 3]; i++)
//         encoded_data[*output_length - 2 - i] = '=';

//     encoded_data[*output_length] = '\0';

//     return encoded_data;
// }


// char *wxmpp_base64_decode(const char *data,
//                              size_t input_length,
//                              size_t *output_length) {

//     if (decoding_table == NULL) build_decoding_table();

//     if (input_length % 4 != 0) return NULL;

//     *output_length = input_length / 4 * 3;
//     if (data[input_length - 1] == '=') (*output_length)--;
//     if (data[input_length - 2] == '=') (*output_length)--;

//     unsigned char *decoded_data = malloc(*output_length);
//     if (decoded_data == NULL) return NULL;

//     int i;
//     int j;
//     for (i = 0, j = 0; i < input_length;) {

//         uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
//         uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
//         uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
//         uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

//         uint32_t triple = (sextet_a << 3 * 6)
//         + (sextet_b << 2 * 6)
//         + (sextet_c << 1 * 6)
//         + (sextet_d << 0 * 6);

//         if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
//         if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
//         if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
//     }

//     return decoded_data;
// }

// char *x16_encode (const unsigned char *buffer, int n)
// {
//     char *encoded = malloc (n*2+1);
//     encoded[2*n] = '\0';
//     int i=0;
//     for (i=0; i<n; i++)
//     {
//         char e = buffer[i];
//         encoded [2*i] = SYMBOLS[e/16];
//         encoded [2*i+1] = SYMBOLS[e%16];
//     }
//     return encoded;
// }

// int x16_value (char s)
// {
//     if (s>='0' && s<='9') return s-'0';
//     else if (s>='a' && s<='f') return 10+s-'a';
//     return 0;
// }

// unsigned char *x16_decode (const char *buffer, int *n)
// {
//     *n = strlen (buffer)/2;
//     char *decoded = malloc (*n);
//     int i=0;
//     for (i=0; i<*n; i++)
//     {
//         char e = x16_value (buffer[2*i])*16 + x16_value (buffer[2*i+1]);
//         decoded[i] = e;
//     }
//     return decoded;
// }


// void build_decoding_table() {

//     decoding_table = malloc(256);

//     int i;
//     for (i = 0; i < 64; i++)
//         decoding_table[(unsigned char) encoding_table[i]] = i;
// }


// void base64_cleanup() {
//     free(decoding_table);
// }

// #include "Base64.h"

// const char b64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//         "abcdefghijklmnopqrstuvwxyz"
//         "0123456789+/";

// /* 'Private' declarations */
// inline void a3_to_a4(unsigned char * a4, unsigned char * a3);
// inline void a4_to_a3(unsigned char * a3, unsigned char * a4);
// inline unsigned char b64_lookup(char c);

// int base64_encode(char *output, char *input, int inputLen) {
//     int i = 0, j = 0;
//     int encLen = 0;
//     unsigned char a3[3];
//     unsigned char a4[4];

//     while(inputLen--) {
//         a3[i++] = *(input++);
//         if(i == 3) {
//             a3_to_a4(a4, a3);

//             for(i = 0; i < 4; i++) {
//                 output[encLen++] = b64_alphabet[a4[i]];
//             }

//             i = 0;
//         }
//     }

//     if(i) {
//         for(j = i; j < 3; j++) {
//             a3[j] = '\0';
//         }

//         a3_to_a4(a4, a3);

//         for(j = 0; j < i + 1; j++) {
//             output[encLen++] = b64_alphabet[a4[j]];
//         }

//         while((i++ < 3)) {
//             output[encLen++] = '=';
//         }
//     }
//     output[encLen] = '\0';
//     return encLen;
// }

char *wxmpp_base64_encode(unsigned char *data,
                     size_t input_length,
                     size_t *output_length)
{
    *output_length = base64_enc_len (input_length)+1;
    char *encoded = malloc (*output_length * sizeof(char));
    arduino_base64_encode (encoded, data, input_length);
    encoded[*output_length-1] = '\0';
    return encoded;
}

char *wxmpp_base64_decode(char *data,
                              size_t input_length,
                              size_t *output_length)
{
    *output_length = base64_dec_len (data, input_length);
    char *decoded = malloc (*output_length * sizeof(char));
    arduino_base64_decode (decoded, data, input_length);
    return decoded;
}

const char b64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

/* 'Private' declarations */
inline void a3_to_a4(unsigned char * a4, unsigned char * a3);
inline void a4_to_a3(unsigned char * a3, unsigned char * a4);
inline unsigned char b64_lookup(char c);

int arduino_base64_encode(char *output, char *input, int inputLen) {
    int i = 0, j = 0;
    int encLen = 0;
    unsigned char a3[3];
    unsigned char a4[4];

    while(inputLen--) {
        a3[i++] = *(input++);
        if(i == 3) {
            a3_to_a4(a4, a3);

            for(i = 0; i < 4; i++) {
                output[encLen++] = b64_alphabet[a4[i]];
            }

            i = 0;
        }
    }

    if(i) {
        for(j = i; j < 3; j++) {
            a3[j] = '\0';
        }

        a3_to_a4(a4, a3);

        for(j = 0; j < i + 1; j++) {
            output[encLen++] = b64_alphabet[a4[j]];
        }

        while((i++ < 3)) {
            output[encLen++] = '=';
        }
    }
    output[encLen] = '\0';
    return encLen;
}

int arduino_base64_decode(char * output, char * input, int inputLen) {
    int i = 0, j = 0;
    int decLen = 0;
    unsigned char a3[3];
    unsigned char a4[4];


    while (inputLen--) {
        if(*input == '=') {
            break;
        }

        a4[i++] = *(input++);
        if (i == 4) {
            for (i = 0; i <4; i++) {
                a4[i] = b64_lookup(a4[i]);
            }

            a4_to_a3(a3,a4);

            for (i = 0; i < 3; i++) {
                output[decLen++] = a3[i];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++) {
            a4[j] = '\0';
        }

        for (j = 0; j <4; j++) {
            a4[j] = b64_lookup(a4[j]);
        }

        a4_to_a3(a3,a4);

        for (j = 0; j < i - 1; j++) {
            output[decLen++] = a3[j];
        }
    }
    output[decLen] = '\0';
    return decLen;
}

int base64_enc_len(int plainLen) {
    int n = plainLen;
    return (n + 2 - ((n + 2) % 3)) / 3 * 4;
}

int base64_dec_len(char * input, int inputLen) {
    int i = 0;
    int numEq = 0;
    for(i = inputLen - 1; input[i] == '='; i--) {
        numEq++;
    }

    return ((6 * inputLen) / 8) - numEq;
}

inline void a3_to_a4(unsigned char * a4, unsigned char * a3) {
    a4[0] = (a3[0] & 0xfc) >> 2;
    a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
    a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
    a4[3] = (a3[2] & 0x3f);
}

inline void a4_to_a3(unsigned char * a3, unsigned char * a4) {
    a3[0] = (a4[0] << 2) + ((a4[1] & 0x30) >> 4);
    a3[1] = ((a4[1] & 0xf) << 4) + ((a4[2] & 0x3c) >> 2);
    a3[2] = ((a4[2] & 0x3) << 6) + a4[3];
}

inline unsigned char b64_lookup(char c) {
    if(c >='A' && c <='Z') return c - 'A';
    if(c >='a' && c <='z') return c - 71;
    if(c >='0' && c <='9') return c + 4;
    if(c == '+') return 62;
    if(c == '/') return 63;
    return -1;
}
