#pragma once
#include "nnue.h"
#include <fstream>
#include <iostream>
#include <immintrin.h>

NNUEParams nnue;

void load_nnue(const char* filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open NNUE file: " << filepath << std::endl;
        exit(1);
    }
    file.read(reinterpret_cast<char*>(&nnue), sizeof(NNUEParams));
}

inline int make_feature(Side perspective, Side piece_color, Piece piece_type, int sq) {
    int p_idx = (piece_type - 1) + (piece_color == perspective ? 0 : 6);
    int mapped_sq = (perspective == White) ? sq : (sq ^ 56);
    return p_idx * 64 + mapped_sq;
}

void Accumulator::init() {
    for (int i = 0; i < NNUE_HIDDEN_SIZE; i += 16) {
        __m256i bias = _mm256_load_si256((const __m256i*) & nnue.feature_bias[i]);
        _mm256_store_si256((__m256i*) & vals[0][i], bias);
        _mm256_store_si256((__m256i*) & vals[1][i], bias);
    }
}


void Accumulator::copy_from(const Accumulator& other) {
    std::memcpy(this, &other, sizeof(Accumulator));
}


void Accumulator::update_piece(Piece type, Side color, int sq, bool is_add) {
    int f_w = make_feature(White, color, type, sq);
    int f_b = make_feature(Black, color, type, sq);

    const int16_t* w_w = nnue.feature_weights[f_w];
    const int16_t* w_b = nnue.feature_weights[f_b];

    if (is_add) {
        for (int i = 0; i < NNUE_HIDDEN_SIZE; i += 16) {
            __m256i v_w = _mm256_load_si256((__m256i*) & vals[0][i]);
            __m256i weight_w = _mm256_load_si256((const __m256i*) & w_w[i]);
            _mm256_store_si256((__m256i*) & vals[0][i], _mm256_add_epi16(v_w, weight_w));

            __m256i v_b = _mm256_load_si256((__m256i*) & vals[1][i]);
            __m256i weight_b = _mm256_load_si256((const __m256i*) & w_b[i]);
            _mm256_store_si256((__m256i*) & vals[1][i], _mm256_add_epi16(v_b, weight_b));
        }
    }
    else {
        for (int i = 0; i < NNUE_HIDDEN_SIZE; i += 16) {
            __m256i v_w = _mm256_load_si256((__m256i*) & vals[0][i]);
            __m256i weight_w = _mm256_load_si256((const __m256i*) & w_w[i]);
            _mm256_store_si256((__m256i*) & vals[0][i], _mm256_sub_epi16(v_w, weight_w));

            __m256i v_b = _mm256_load_si256((__m256i*) & vals[1][i]);
            __m256i weight_b = _mm256_load_si256((const __m256i*) & w_b[i]);
            _mm256_store_si256((__m256i*) & vals[1][i], _mm256_sub_epi16(v_b, weight_b));
        }
    }
}


int32_t evaluate_nnue(const Accumulator& acc, Side stm, int piece_count) {
    const int16_t* us = (stm == White) ? acc.vals[0] : acc.vals[1];
    const int16_t* them = (stm == White) ? acc.vals[1] : acc.vals[0];

    // Determine bucket using piece count matching the Rust formula: (count - 2) / (32 / 8)
    int bucket = (piece_count - 2) / 4;
    if (bucket < 0) bucket = 0;  // Safety guard
    if (bucket > 7) bucket = 7;  // Safety guard

    __m256i zero = _mm256_setzero_si256();
    __m256i max_val = _mm256_set1_epi16(NNUE_QA);
    __m256i sum0 = _mm256_setzero_si256();

    // Single pass to handle both 'us' and 'them' segments in output_weights
    const int16_t* inputs[2] = { us, them };
    for (int p = 0; p < 2; p++) {
        const int16_t* in_ptr = inputs[p];

        // Select the weights corresponding to the specific output bucket
        const int16_t* w_ptr = &nnue.output_weights[bucket][p * NNUE_HIDDEN_SIZE];

        for (int i = 0; i < NNUE_HIDDEN_SIZE; i += 16) {
            __m256i in = _mm256_load_si256((const __m256i*) & in_ptr[i]);
            __m256i w = _mm256_load_si256((const __m256i*) & w_ptr[i]);

            // Clamp input to[0, QA]
            in = _mm256_max_epi16(in, zero);
            in = _mm256_min_epi16(in, max_val);

            // Unpack 16-bit to 32-bit.
            __m256i in_lo = _mm256_unpacklo_epi16(in, zero);
            __m256i in_hi = _mm256_unpackhi_epi16(in, zero);

            // Sign-extend weights to 32-bit elegantly via unpack & arithmetic shift
            __m256i w_lo = _mm256_srai_epi32(_mm256_unpacklo_epi16(w, w), 16);
            __m256i w_hi = _mm256_srai_epi32(_mm256_unpackhi_epi16(w, w), 16);

            // SCReLU (square the clamped values)
            __m256i sq_lo = _mm256_mullo_epi32(in_lo, in_lo);
            __m256i sq_hi = _mm256_mullo_epi32(in_hi, in_hi);

            // Multiply and accumulate
            sum0 = _mm256_add_epi32(sum0, _mm256_mullo_epi32(sq_lo, w_lo));
            sum0 = _mm256_add_epi32(sum0, _mm256_mullo_epi32(sq_hi, w_hi));
        }
    }

    // Horizontal sum of the 8 lanes in the 256-bit accumulator
    __m256i hsum1 = _mm256_hadd_epi32(sum0, sum0);
    __m256i hsum2 = _mm256_hadd_epi32(hsum1, hsum1);
    int32_t output = _mm256_extract_epi32(hsum2, 0) + _mm256_extract_epi32(hsum2, 4);

    // Reproduce the bullet-lib scaling formulas
    output /= NNUE_QA;

    // Add the specific output bias for this bucket
    output += nnue.output_bias[bucket];

    output *= NNUE_SCALE;
    output /= (NNUE_QA * NNUE_QB);

    return output;
}