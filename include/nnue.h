#pragma once
#include <cstdint>
#include <cstring>
#include "utils.h"
#include "types.h"
constexpr int NNUE_HIDDEN_SIZE = 1536;
constexpr int NNUE_QA = 255;
constexpr int NNUE_QB = 64;
constexpr int NNUE_SCALE = 400;
constexpr int NNUE_OUTPUT_BUCKETS = 8;
struct alignas(64) NNUEParams {
    int16_t feature_weights[768][NNUE_HIDDEN_SIZE];
    int16_t feature_bias[NNUE_HIDDEN_SIZE];
    int16_t output_weights[NNUE_OUTPUT_BUCKETS][2 * NNUE_HIDDEN_SIZE];
    int16_t output_bias[NNUE_OUTPUT_BUCKETS];
};

extern NNUEParams nnue;


class Accumulator {
public:
    // [0] = White perspective, [1] = Black perspective
    alignas(64) int16_t vals[2][NNUE_HIDDEN_SIZE];

    void init();
    void copy_from(const Accumulator& other);
    void update_piece(Piece type, Side color, int sq, bool is_add);
};

void load_nnue(const char* filepath);
int32_t evaluate_nnue(const Accumulator& acc, Side stm, int piece_count);