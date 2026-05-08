#pragma once
#include "types.h"
#include "ttable.h"
#include <vector>
#include "utils.h"
#include "constant_eval.h"
#include "nnue.h"
class BitBoard {
private:
    int halfMoves;
    int fullMoves;
    bool castingRight[2][2];//casting right for black and white,for kingside and queenside


    //the 4 arr below return the bitboard that mask all the attack of 4 pice type.Note :it also including attack to same-side pieces and enemy pieces 
    uint64_t arrPawnAttacks[2][64];
    uint64_t arrKingAttacks[64];
    uint64_t arrKnightAttacks[64];
    uint64_t arrBishopAttacks[64][512]{ 0 };
    uint64_t arrRookAttacks[64][4096]{ 0 };
    uint64_t arrRectangular[64][64];
    uint64_t arrFrontPawn[2][64];
    uint64_t arrAfterPawn[2][64];
    uint64_t arrNeighorPawn[64];
    uint64_t arrOuterRing[64];
    struct Magic {
        uint64_t mask;  // to mask relevant squares of both lines (no outer squares)
        uint64_t magic; // magic 64-bit factor
        int shift;
    };

    Magic mBishopTbl[64];
    Magic mRookTbl[64];
    Direction precalcDir[64][64];
    uint64_t flipped[2][64];
    uint64_t dirMask[64][64];
    struct RandomHash {
        uint64_t pieceRand[2][7][64];
        uint64_t enPassantRand[64];
        uint64_t castlingRight[2][2];
        uint64_t sideMoving;
    } randomHash;
    int halfMoveHistory[2048];
    Accumulator accumulators[2048];
public:
    std::map<std::string, uint64_t> debugPerft;
    RepetitionTable repetitionTable;
    Piece pieceTable[64];//incrementally updated
    int moveNum;
    bool isWhiteTurn;
    int cntNode = 0;
    int cntNode2 = 0;
    enum enumSquare {
        a1, b1, c1, d1, e1, f1, g1, h1,
        a2, b2, c2, d2, e2, f2, g2, h2,
        a3, b3, c3, d3, e3, f3, g3, h3,
        a4, b4, c4, d4, e4, f4, g4, h4,
        a5, b5, c5, d5, e5, f5, g5, h5,
        a6, b6, c6, d6, e6, f6, g6, h6,
        a7, b7, c7, d7, e7, f7, g7, h7,
        a8, b8, c8, d8, e8, f8, g8, h8
    };

    int arrCenterManhattanDistance[64] = { // char is sufficient as well, also unsigned
    6, 5, 4, 3, 3, 4, 5, 6,
    5, 4, 3, 2, 2, 3, 4, 5,
    4, 3, 2, 1, 1, 2, 3, 4,
    3, 2, 1, 0, 0, 1, 2, 3,
    3, 2, 1, 0, 0, 1, 2, 3,
    4, 3, 2, 1, 1, 2, 3, 4,
    5, 4, 3, 2, 2, 3, 4, 5,
    6, 5, 4, 3, 3, 4, 5, 6
    };
    int shiftsBishop[64] = {
        // clang-format off
        6, 5, 5, 5, 5, 5, 5, 6,
        5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 7, 7, 7, 7, 5, 5,
        5, 5, 7, 9, 9, 7, 5, 5,
        5, 5, 7, 9, 9, 7, 5, 5,
        5, 5, 7, 7, 7, 7, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5,
        6, 5, 5, 5, 5, 5, 5, 6
        // clang-format on
    };

    int shiftsRook[64] = {
        // clang-format off
        12, 11, 11, 11, 11, 11, 11, 12,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        12, 11, 11, 11, 11, 11, 11, 12
        // clang-format on
    };

    uint64_t magicBishop[64] = {

              9368648609924554880ULL, 9009475591934976ULL,     4504776450605056ULL,
              1130334595844096ULL,    1725202480235520ULL,     288516396277699584ULL,
              613618303369805920ULL,  10168455467108368ULL,    9046920051966080ULL,
              36031066926022914ULL,   1152925941509587232ULL,  9301886096196101ULL,
              290536121828773904ULL,  5260205533369993472ULL,  7512287909098426400ULL,
              153141218749450240ULL,  9241386469758076456ULL,  5352528174448640064ULL,
              2310346668982272096ULL, 1154049638051909890ULL,  282645627930625ULL,
              2306405976892514304ULL, 11534281888680707074ULL, 72339630111982113ULL,
              8149474640617539202ULL, 2459884588819024896ULL,  11675583734899409218ULL,
              1196543596102144ULL,    5774635144585216ULL,     145242600416216065ULL,
              2522607328671633440ULL, 145278609400071184ULL,   5101802674455216ULL,
              650979603259904ULL,     9511646410653040801ULL,  1153493285013424640ULL,
              18016048314974752ULL,   4688397299729694976ULL,  9226754220791842050ULL,
              4611969694574863363ULL, 145532532652773378ULL,   5265289125480634376ULL,
              288239448330604544ULL,  2395019802642432ULL,     14555704381721968898ULL,
              2324459974457168384ULL, 23652833739932677ULL,    282583111844497ULL,
              4629880776036450560ULL, 5188716322066279440ULL,  146367151686549765ULL,
              1153170821083299856ULL, 2315697107408912522ULL,  2342448293961403408ULL,
              2309255902098161920ULL, 469501395595331584ULL,   4615626809856761874ULL,
              576601773662552642ULL,  621501155230386208ULL,   13835058055890469376ULL,
              3748138521932726784ULL, 9223517207018883457ULL,  9237736128969216257ULL,
              1127068154855556ULL
    };

    uint64_t magicRook[64] = {
              612498416294952992ULL,  2377936612260610304ULL,  36037730568766080ULL,
              72075188908654856ULL,   144119655536003584ULL,   5836666216720237568ULL,
              9403535813175676288ULL, 1765412295174865024ULL,  3476919663777054752ULL,
              288300746238222339ULL,  9288811671472386ULL,     146648600474026240ULL,
              3799946587537536ULL,    704237264700928ULL,      10133167915730964ULL,
              2305983769267405952ULL, 9223634270415749248ULL,  10344480540467205ULL,
              9376496898355021824ULL, 2323998695235782656ULL,  9241527722809755650ULL,
              189159985010188292ULL,  2310421375767019786ULL,  4647717014536733827ULL,
              5585659813035147264ULL, 1442911135872321664ULL,  140814801969667ULL,
              1188959108457300100ULL, 288815318485696640ULL,   758869733499076736ULL,
              234750139167147013ULL,  2305924931420225604ULL,  9403727128727390345ULL,
              9223970239903959360ULL, 309094713112139074ULL,   38290492990967808ULL,
              3461016597114651648ULL, 181289678366835712ULL,   4927518981226496513ULL,
              1155212901905072225ULL, 36099167912755202ULL,    9024792514543648ULL,
              4611826894462124048ULL, 291045264466247688ULL,   83880127713378308ULL,
              1688867174481936ULL,    563516973121544ULL,      9227888831703941123ULL,
              703691741225216ULL,     45203259517829248ULL,    693563138976596032ULL,
              4038638777286134272ULL, 865817582546978176ULL,   13835621555058516608ULL,
              11541041685463296ULL,   288511853443695360ULL,   283749161902275ULL,
              176489098445378ULL,     2306124759338845321ULL,  720584805193941061ULL,
              4977040710267061250ULL, 10097633331715778562ULL, 325666550235288577ULL,
              1100057149646ULL
    };

    uint64_t maskRook[64] = {

        0x101010101017eULL, 0x202020202027cULL, 0x404040404047aULL, 0x8080808080876ULL, 0x1010101010106eULL, 0x2020202020205eULL, 0x4040404040403eULL, 0x8080808080807eULL,
        0x1010101017e00ULL, 0x2020202027c00ULL, 0x4040404047a00ULL, 0x8080808087600ULL, 0x10101010106e00ULL, 0x20202020205e00ULL, 0x40404040403e00ULL, 0x80808080807e00ULL,
        0x10101017e0100ULL, 0x20202027c0200ULL, 0x40404047a0400ULL, 0x8080808760800ULL, 0x101010106e1000ULL, 0x202020205e2000ULL, 0x404040403e4000ULL, 0x808080807e8000ULL,
        0x101017e010100ULL, 0x202027c020200ULL, 0x404047a040400ULL, 0x8080876080800ULL, 0x1010106e101000ULL, 0x2020205e202000ULL, 0x4040403e404000ULL, 0x8080807e808000ULL,
        0x1017e01010100ULL, 0x2027c02020200ULL, 0x4047a04040400ULL, 0x8087608080800ULL, 0x10106e10101000ULL, 0x20205e20202000ULL, 0x40403e40404000ULL, 0x80807e80808000ULL,
        0x17e0101010100ULL, 0x27c0202020200ULL, 0x47a0404040400ULL, 0x8760808080800ULL, 0x106e1010101000ULL, 0x205e2020202000ULL, 0x403e4040404000ULL, 0x807e8080808000ULL,
        0x7e010101010100ULL, 0x7c020202020200ULL, 0x7a040404040400ULL, 0x76080808080800ULL, 0x6e101010101000ULL, 0x5e202020202000ULL, 0x3e404040404000ULL, 0x7e808080808000ULL,
        0x7e01010101010100ULL, 0x7c02020202020200ULL, 0x7a04040404040400ULL, 0x7608080808080800ULL, 0x6e10101010101000ULL, 0x5e20202020202000ULL, 0x3e40404040404000ULL, 0x7e80808080808000ULL

    };
    uint64_t maskBishop[64] = {
    0x40201008040200ULL, 0x402010080400ULL, 0x4020100a00ULL, 0x40221400ULL, 0x2442800ULL, 0x204085000ULL, 0x20408102000ULL, 0x2040810204000ULL,
    0x20100804020000ULL, 0x40201008040000ULL, 0x4020100a0000ULL, 0x4022140000ULL, 0x244280000ULL, 0x20408500000ULL, 0x2040810200000ULL, 0x4081020400000ULL,
    0x10080402000200ULL, 0x20100804000400ULL, 0x4020100a000a00ULL, 0x402214001400ULL, 0x24428002800ULL, 0x2040850005000ULL, 0x4081020002000ULL, 0x8102040004000ULL,
    0x8040200020400ULL, 0x10080400040800ULL, 0x20100a000a1000ULL, 0x40221400142200ULL, 0x2442800284400ULL, 0x4085000500800ULL, 0x8102000201000ULL, 0x10204000402000ULL,
    0x4020002040800ULL, 0x8040004081000ULL, 0x100a000a102000ULL, 0x22140014224000ULL, 0x44280028440200ULL, 0x8500050080400ULL, 0x10200020100800ULL, 0x20400040201000ULL,
    0x2000204081000ULL, 0x4000408102000ULL, 0xa000a10204000ULL, 0x14001422400000ULL, 0x28002844020000ULL, 0x50005008040200ULL, 0x20002010080400ULL, 0x40004020100800ULL,
    0x20408102000ULL, 0x40810204000ULL, 0xa1020400000ULL, 0x142240000000ULL, 0x284402000000ULL, 0x500804020000ULL, 0x201008040200ULL, 0x402010080400ULL,
    0x2040810204000ULL,0x4081020400000ULL, 0xa102040000000ULL, 0x14224000000000ULL, 0x28440200000000ULL, 0x50080402000000ULL, 0x20100804020000ULL, 0x40201008040200ULL
    };
    uint64_t pieceBB[2][8];
    int enPassantPos;
    BitBoard();
    BitBoard(std::string fen);

    void initConstant();
    void parseFEN(std::string fen);
    std::string toFEN();
    uint64_t move(Move& move, uint64_t zHash = 0);
    void undoMove(Move& move);
    std::vector<Move> getLegalMoves();
    void getLegalMovesAlt(MoveVector& ans);
    void getLegalCapturesAlt(MoveVector& ans);

    uint64_t getKnightAttackSquares(int knightPos);
    uint64_t getPawnAttackSquares(Side side, int pawnPos);
    uint64_t getKingAttackSquares(int pos);
    uint64_t getBishopAttackSquares(uint64_t occ, int pos);
    uint64_t getRookAttackSquares(uint64_t occ, int pos);
    uint64_t getBishopXrayAttackSquares(uint64_t occ, uint64_t blockers, int pos);
    uint64_t getRookXrayAttackSquares(uint64_t occ, uint64_t blockers, int pos);
    uint64_t getPinnedPiece(uint64_t occupied, int pos, Side side);
    uint64_t getAllAttackSquares(Side side);
    uint64_t getSingleCheckInterposingSquares(uint64_t occupied, int kingPos, Side side);
    bool isEndgame();
    bool isValidMove(Move& move);
    bool isThreat(const Move& move);
    bool SEE_GE(Move m, int threashold);
    int evaluate(int alpha, int beta);
    int evaluateHCE(int alpha, int beta);
    int posToRank(int pos) {
        return pos >> 3;
    }
    int posToFile(int pos) {
        return pos & 0b111;
    }
    std::pair<int, int> posToRowCol(int pos);
    int rowColToPos(int row, int col);
    int isSquareAttacked(uint64_t occupied, int pos, Side side);
    uint64_t getAttackersOfSq(uint64_t occupied, int pos);
    uint64_t getCurrentPositionHash();
    uint64_t getMaskOfRank(int rank) {
        return 0x00000000000000FFull << (8 * rank);
    }
    uint64_t getMaskOfFile(int file) {
        return 0x0101010101010101ull << file;
    }
    void initMagic();
    uint64_t perft(int depth, int maxDepth);
    Direction getDirFromTwoSquares(int pos1, int pos2);
    bool isKingInCheck();
    bool isPassedPawn(Side side, Side opSide, int pawnPos) {
        return !(arrFrontPawn[side][pawnPos] & (pieceBB[opSide][Piece::pawn] | pieceBB[side][Piece::pawn]));
    }
    bool isConnectedPawn(Side side, int pawnPos) {
        return arrPawnAttacks[!side][pawnPos] & pieceBB[side][Piece::pawn];
    }
    bool isRepeatedPosition() {
        return repetitionTable.isRepetition();
    }
    uint64_t nonPawnMaterial(Side side) {
        return pieceBB[side][Piece::knight] | pieceBB[side][Piece::bishop] |
            pieceBB[side][Piece::rook] | pieceBB[side][Piece::queen];
    }
    bool isNullMoveEnable() {
        uint64_t occupied = (pieceBB[Side::White][Piece::any] | pieceBB[Side::Black][Piece::any]);
        int nPieces = popcount64(occupied);
        //number of piece >6 and not king and pawn endgame
        return (nPieces > 6) && (occupied & ~(pieceBB[Side::White][Piece::pawn] | pieceBB[Side::Black][Piece::pawn] | pieceBB[Side::White][Piece::king] | pieceBB[Side::Black][Piece::king]));
    }
    // Pawn hash table
    struct PawnEntry {
        uint64_t pawnHash;
        int score[2];
        bool valid;
    };
    static constexpr int PAWN_TABLE_SIZE = 1 << 16; // 64K entries
    PawnEntry pawnHashTable[PAWN_TABLE_SIZE];

    int quickMaterialScore();
    int probePawnHash(uint64_t pawnHash, int* score);
    void storePawnHash(uint64_t pawnHash, int score[2]);
    uint64_t getPawnHash();
};
