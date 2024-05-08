#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <set>
#include <fstream>
#include <array>
#include <chrono>
#include <random>
#include "ttable.h"
#include <algorithm>
#include "constant_eval.h"
#include "constant_search.h"
#include "constant_bitboard.h"
#include "constant_hash.h"
#include <thread>
const std::string PATH = "./";

const int index64[64] = {
    0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};
inline int BitScanForward64(uint64_t x) {
    return index64[((x ^ (x - 1)) * 0x03f79d71b4cb0a89) >> 58];
}

inline int popcount64(uint64_t x) {
#if defined(_MSC_VER)
    return static_cast<int>(__popcnt64(x));

#else  // Assumed gcc or compatible compiler

    return __builtin_popcountll(x);

#endif
}

/*
//BitScanForward : Search the mask data from least significant bit (LSB) to the most significant bit (MSB) for a set bit (1)
inline int BitScanForward64(uint64_t x) {
    unsigned long index = 0;
    _BitScanForward64(&index,x);
    return index;
}
*/

const int pieceValue[7] = {
    1,
    100,
    300,
    300,
    500,
    900,
    10000
};
struct Trace
{
    int score;

    int material[7][2]{};
    int mobilities[7][2]{};
    int king_attacks[100][2]{};
    int passers[8][2]{};
    int open_files_penalty[4][2]{};
    int semi_open_files_penalty[4][2]{};
    int pawn_doubled_penalty[2]{};
    int pawn_isolated_penalty[2]{};
    int pawn_passed_protected[2]{};
    int bishop_pair[2]{};
    int king_shield[2]{};
    int tropism_factor[2]{};
};
#define TraceIncr(parameter) trace.parameter[side]++
#define TraceAdd(parameter, count) trace.parameter[side] += count
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
    uint64_t dirMask[64][64];
    struct RandomHash {
        uint64_t pieceRand[2][7][64];
        uint64_t enPassantRand[64];
        uint64_t castlingRight[2][2];
        uint64_t sideMoving;
    } randomHash;
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



    int bonusOpKingCornerEndgame[64] = {
        50, 45, 45, 45, 45, 45, 45, 50,
        40, 35, 35, 35, 35, 35, 35, 45,
        40, 35, 20, 20, 20, 20, 35, 45,
        40, 35, 20,  0,  0, 20, 35, 45,
        40, 35, 20,  0,  0, 20, 35, 45,
        40, 35, 20, 20, 20, 20, 35, 45,
        40, 35, 35, 35, 35, 35, 35, 45,
        50, 45, 45, 45, 45, 45, 45, 50
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
    uint64_t move(Move& move, uint64_t zHash = 0);
    void undoMove(Move& move);
    std::vector<Move> getLegalMoves();
    void getLegalMovesAlt(MoveVector& ans);
    std::vector<Move> getLegalCaptures();
    void getLegalCapturesAlt(MoveVector& ans);
    std::vector<Move> getPseudoLegalMoves();

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
    bool isInsufficientMaterial();
    bool isValidMove(Move& move);
    bool SEE_GE(Move m, int threashold);
    int evaluate(int alpha, int beta);
    Trace evaluate2();
    int posToRank(int pos) {
        return pos / 8;
    }
    int posToFile(int pos) {
        return pos % 8;
    }
    std::pair<int, int> posToRowCol(int pos);
    int rowColToPos(int row, int col);
    int isSquareAttacked(uint64_t occupied, int pos, Side side);
    uint64_t getAttackersOfSq(uint64_t occupied, int pos);
    bool isLegalMove(Move move, int checks, bool isAbsolutePinned, Direction pinnedDir, uint64_t opAttackSquares, uint64_t canInterposingSquares);
    uint64_t getCurrentPositionHash();
    uint64_t getMaskOfRank(int rank) {
        return 0x00000000000000FFull << (8 * rank);
    }
    uint64_t getMaskOfFile(int file) {
        return 0x0101010101010101ull << file;
    }
    void initMagic();
    int getMobility(Side side, int phaseOpening, int phaseEndgame);
    uint64_t perft(int depth, int maxDepth);
    Direction getDirFromTwoSquares(int pos1, int pos2);
    bool isKingInCheck();
    bool isPassedPawn(Side side, Side opSide, int pawnPos) {
        return !(arrFrontPawn[side][pawnPos] & (pieceBB[opSide][Piece::pawn] | pieceBB[side][Piece::pawn]));
    }
    bool isConnectedPawn(Side side, int pawnPos) {
        return arrPawnAttacks[!side][pawnPos] & pieceBB[side][Piece::pawn];
    }

    bool isPromoting() {
        Side sideToMove = isWhiteTurn ? Side::White : Side::Black;
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        return pieceBB[sideToMove][Piece::pawn] & mask7thRank;
    }
    bool isPawnNearPromoting(Side sideToMove, int pawnPos) {
        uint64_t maskRank = sideToMove == Side::White ? 0x00ffff0000000000 : 0x0000000000ffff00;
        return (1ull << pawnPos) & maskRank;
    }
    bool isEndgame() {
        uint64_t occupied = (pieceBB[Side::White][Piece::any] | pieceBB[Side::Black][Piece::any]) & ~(pieceBB[Side::White][Piece::pawn] | pieceBB[Side::Black][Piece::pawn]);
        int nPieces = popcount64(occupied);
        return (nPieces <= 4);
    }
    bool isRepeatedPosition() {
        return repetitionTable.isRepetition();
    }
    bool isKingAndPawnEndgame() {
        return !((pieceBB[Side::White][Piece::any] | pieceBB[Side::Black][Piece::any]) & ~(pieceBB[Side::White][Piece::pawn] | pieceBB[Side::Black][Piece::pawn] | pieceBB[Side::White][Piece::king] | pieceBB[Side::Black][Piece::king]));
    }
    /*
    bool giveCheck(Move m) {
        Piece movePiece = m.getMovePiece();
        Side side = m.getSideToMove();
        int endPos = m.getEndPos();
        int opKingPos = kingPos[!side];
        uint64_t occupied = pieceBB[side][Piece::any] | pieceBB[!side][Piece::any];
        switch (movePiece)
        {
        case any:
            break;
        case pawn:
            return arrPawnAttacks[side][endPos] & opKingPos;
            break;
        case knight:
            return arrKnightAttacks[endPos] & opKingPos;
            break;
        case bishop:
            return getBishopAttackSquares(occupied, endPos) & opKingPos;
            break;
        case rook:
            return getRookAttackSquares(occupied, endPos) & opKingPos;
            break;
        case queen:
            return (getRookAttackSquares(occupied, endPos) | getBishopAttackSquares(occupied, endPos)) & opKingPos;
            break;
        case king:
            break;
        default:
            break;
        }
        return false;
    }
    */
    std::vector<std::vector<char>> toPieceTableRepresentation();
};

BitBoard::BitBoard() {
    initConstant();
    initMagic();
    parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}
std::pair<int, int> BitBoard::posToRowCol(int pos) {
    return std::make_pair(pos / 8, pos % 8);
}

int BitBoard::rowColToPos(int row, int col) {
    return row * 8 + col;
}

std::vector<std::vector<char>> BitBoard::toPieceTableRepresentation() {
    std::vector<std::vector<char>> res = std::vector<std::vector<char>>(8, std::vector<char>(8, ' '));
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Side side = Side::White;
            int idx = ((8 * (7 - i)) + j);
            if (pieceTable[idx] == Piece::any)
                continue;
            else {
                side = ((1ull << idx) & pieceBB[Side::White][Piece::any]) ? Side::White : Side::Black;
            }
            switch (pieceTable[((8 * (7 - i)) + j)])
            {
            case Piece::pawn:
                res[i][j] = side == Side::Black ? 'p' : toupper('p');
                break;
            case Piece::bishop:
                res[i][j] = side == Side::Black ? 'b' : toupper('b');
                break;
            case Piece::knight:
                res[i][j] = side == Side::Black ? 'n' : toupper('n');
                break;
            case Piece::rook:
                res[i][j] = side == Side::Black ? 'r' : toupper('r');
                break;
            case Piece::queen:
                res[i][j] = side == Side::Black ? 'q' : toupper('q');
                break;
            case Piece::king:
                res[i][j] = side == Side::Black ? 'k' : toupper('k');
                break;
            case Piece::any:
                res[i][j] = ' ';
                break;
            default:
                break;
            }
        }
    }
    return res;
}

void BitBoard::initMagic() {
    for (int i = 0; i < 64; i++) {
        mBishopTbl[i].magic = magicBishop[i];
        mBishopTbl[i].mask = maskBishop[i];
        mBishopTbl[i].shift = shiftsBishop[i];

        mRookTbl[i].magic = magicRook[i];
        mRookTbl[i].mask = maskRook[i];
        mRookTbl[i].shift = shiftsRook[i];
    }
    //std::ifstream in(PATH + "data/magicRook.txt");
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 4096; j++) {
            arrRookAttacks[i][j] = 0;
        }
        for (int j = 0; j < 512; j++) {
            arrBishopAttacks[i][j] = 0;
        }
    }
    const int SIZE_ROOK = 102400;
    const int SIZE_BISHOP = 5248;

    for (int i = 0; i < SIZE_ROOK; i++) {
        uint64_t pos = arrMagicRook[i][0];
        uint64_t occupancy = arrMagicRook[i][1];
        uint64_t res = arrMagicRook[i][2];
        uint64_t key = (mRookTbl[pos].magic * occupancy) >> (64 - mRookTbl[pos].shift);
        arrRookAttacks[pos][key] = res;
    }
    for (int i = 0; i < SIZE_BISHOP; i++) {
        uint64_t pos = arrMagicBishop[i][0];
        uint64_t occupancy = arrMagicBishop[i][1];
        uint64_t res = arrMagicBishop[i][2];
        uint64_t key = (mBishopTbl[pos].magic * occupancy) >> (64 - mBishopTbl[pos].shift);
        arrBishopAttacks[pos][key] = res;
    }

}

uint64_t BitBoard::getCurrentPositionHash() {
    uint64_t zHash = 0;
    for (int i = 0; i < 64; i++) {
        Piece currPiece = pieceTable[i];
        if (currPiece == Piece::any)
            continue;
        Side currSide = ((1ull << i) & pieceBB[Side::White][Piece::any]) ? Side::White : Side::Black;
        zHash ^= randomHash.pieceRand[currSide][currPiece][i];
    }
    if (castingRight[0][0]) {
        zHash ^= randomHash.castlingRight[0][0];
    }
    if (castingRight[0][1]) {
        zHash ^= randomHash.castlingRight[0][1];
    }
    if (castingRight[1][0]) {
        zHash ^= randomHash.castlingRight[1][0];
    }
    if (castingRight[1][1]) {
        zHash ^= randomHash.castlingRight[1][1];
    }
    if (enPassantPos != 0) {
        zHash ^= randomHash.enPassantRand[enPassantPos];
    }
    if (isWhiteTurn) {
        zHash ^= randomHash.sideMoving;
    }
    return zHash;
}

uint64_t BitBoard::getKnightAttackSquares(int knightPos) {
    return arrKnightAttacks[knightPos];
}


uint64_t BitBoard::getPawnAttackSquares(Side side, int pawnPos) {
    return arrPawnAttacks[side][pawnPos];
}

uint64_t BitBoard::getKingAttackSquares(int kingPos) {
    return arrKingAttacks[kingPos];
}



BitBoard::BitBoard(std::string fen) {
    isWhiteTurn = true;
    initConstant();
    initMagic();
    parseFEN(fen);
}

void BitBoard::parseFEN(std::string fen) {
    repetitionTable = RepetitionTable();
    moveNum = 0;
    isWhiteTurn = true;
    std::map<char, Piece> mapping = {
        {'p',Piece::pawn},
        {'n',Piece::knight},
        {'b',Piece::bishop},
        {'r',Piece::rook},
        {'q',Piece::queen},
        {'k',Piece::king}
    };
    std::stringstream ss(fen);
    std::string s;
    std::vector<std::string> v;
    while (std::getline(ss, s, ' ')) {
        v.emplace_back(s);
    }
    for (int i = 0; i < 64; i++) {
        pieceTable[i] = Piece::any;
    }
    for (int i = 0; i < 8; i++) {
        pieceBB[Side::White][i] = 0;
        pieceBB[Side::Black][i] = 0;
    }
    int currPos = 56;
    //rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    for (int i = 0; i < v[0].size(); i++) {
        Side pieceSide = islower(v[0][i]) ? Side::Black : Side::White;
        if (v[0][i] == '/') {
            currPos -= 16;
            continue;
        }
        char lowercasePiece = tolower(v[0][i]);
        //std::cout << currPos << std::endl;
        switch (lowercasePiece)
        {
        case 'r':
            pieceBB[pieceSide][Piece::rook] |= (1ull << currPos);
            pieceBB[pieceSide][Piece::any] |= (1ull << currPos);
            pieceTable[currPos] = Piece::rook;
            break;
        case 'b':
            pieceBB[pieceSide][Piece::bishop] |= (1ull << currPos);
            pieceBB[pieceSide][Piece::any] |= (1ull << currPos);
            pieceTable[currPos] = Piece::bishop;
            break;
        case 'q':
            pieceBB[pieceSide][Piece::queen] |= (1ull << currPos);
            pieceBB[pieceSide][Piece::any] |= (1ull << currPos);
            pieceTable[currPos] = Piece::queen;
            break;
        case 'p':
            pieceBB[pieceSide][Piece::pawn] |= (1ull << currPos);
            pieceBB[pieceSide][Piece::any] |= (1ull << currPos);
            pieceTable[currPos] = Piece::pawn;
            break;
        case 'n':
            pieceBB[pieceSide][Piece::knight] |= (1ull << currPos);
            pieceBB[pieceSide][Piece::any] |= (1ull << currPos);
            pieceTable[currPos] = Piece::knight;
            break;
        case 'k':
            pieceBB[pieceSide][Piece::king] |= (1ull << currPos);
            pieceBB[pieceSide][Piece::any] |= (1ull << currPos);
            pieceTable[currPos] = Piece::king;
            //std::cout << currPos << std::endl;
            break;
        default:
            currPos += (v[0][i] - '1');
            break;
        }
        currPos++;
    }
    isWhiteTurn = v[1][0] == 'w';
    for (int i = 0; i < v[2].size(); i++) {
        switch (v[2][i])
        {
        case 'Q':
            castingRight[Side::White][0] = true;
            break;
        case 'K':
            castingRight[Side::White][1] = true;
            break;
        case 'q':
            castingRight[Side::Black][0] = true;
            break;
        case 'k':
            castingRight[Side::Black][1] = true;
            break;
        default:
            castingRight[Side::Black][0] = false;
            castingRight[Side::Black][1] = false;
            castingRight[Side::White][0] = false;
            castingRight[Side::White][1] = false;
            break;
        }
    }
    if (v[3][0] != '-') {
        enPassantPos = squareToIndex[v[3]];
    }
    else {
        enPassantPos = 0;
    }
    if (v.size() > 4) {
        halfMoves = stoi(v[4]);
    }
    if (v.size() > 5) {
        fullMoves = stoi(v[5]);
    }
    repetitionTable.add(getCurrentPositionHash(), false);

}
/*
BitBoard& BitBoard::operator=(const BitBoard& other){
    if (this == &other) {
        // Self-assignment, no action needed
        return *this;
    }
    moveNum=other.moveNum;
    enPassantPos=other.enPassantPos;


}
*/
void BitBoard::initConstant() {
    //init arrPawnAttcks that contains the attack squres of pawn precomputed
    for (int i = 0; i < 64; i++) {
        if (posToFile(i) == 0) {
            arrPawnAttacks[Side::White][i] = (1ull << (i + 9));
            arrPawnAttacks[Side::Black][i] = (1ull << (i - 7));
        }
        else if (posToFile(i) == 7) {
            arrPawnAttacks[Side::White][i] = (1ull << (i + 7));
            arrPawnAttacks[Side::Black][i] = (1ull << (i - 9));
        }
        else {
            arrPawnAttacks[Side::White][i] = (1ull << (i + 7)) | (1ull << (i + 9));
            arrPawnAttacks[Side::Black][i] = (1ull << (i - 7)) | (1ull << (i - 9));
        }
    }

    for (int i = 0; i < 64; i++) {
        std::pair<int, int> rc = posToRowCol(i);
        std::vector<std::pair<int, int>> atkSet = {
            std::make_pair(rc.first - 1,rc.second - 1),
            std::make_pair(rc.first - 1,rc.second),
            std::make_pair(rc.first - 1,rc.second + 1),
            std::make_pair(rc.first,rc.second - 1),
            std::make_pair(rc.first,rc.second + 1),
            std::make_pair(rc.first + 1,rc.second - 1),
            std::make_pair(rc.first + 1,rc.second),
            std::make_pair(rc.first + 1,rc.second + 1),
        };
        uint64_t atkSquares = 0;
        for (auto p : atkSet) {
            if (p.first >= 0 && p.second >= 0 && p.first < 8 && p.second < 8) {
                atkSquares |= (1ull << rowColToPos(p.first, p.second));
            }
        }
        arrKingAttacks[i] = atkSquares;
    }

    for (int i = 0; i < 64; i++) {
        uint64_t knights = 1ull << i;
        uint64_t l1 = (knights >> 1) & 0x7f7f7f7f7f7f7f7fULL;
        uint64_t l2 = (knights >> 2) & 0x3f3f3f3f3f3f3f3fULL;
        uint64_t r1 = (knights << 1) & 0xfefefefefefefefeULL;
        uint64_t r2 = (knights << 2) & 0xfcfcfcfcfcfcfcfcULL;
        uint64_t h1 = l1 | r1;
        uint64_t h2 = l2 | r2;
        uint64_t res = ((h1 << 16) | (h1 >> 16) | (h2 << 8) | (h2 >> 8));
        arrKnightAttacks[i] = res;
    }

    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {
            int row1 = i / 8;
            int col1 = i % 8;
            int row2 = j / 8;
            int col2 = j % 8;
            Direction dir = Direction::None;
            uint64_t mask = 0;
            uint64_t fullMask = 0;
            if (row1 == row2) {
                dir = Direction::Horizontal;
                int colFrom = std::min(col1, col2);
                int colTo = std::max(col1, col2);
                for (int k = colFrom; k <= colTo; k++) {
                    int pos = row1 * 8 + k;
                    mask |= (1ull << pos);
                }
                for (int k = 0; k <= 7; k++) {
                    int pos = row1 * 8 + k;
                    fullMask |= (1ull << pos);
                }

            }
            else if (col1 == col2) {
                dir = Direction::Vertical;
                int rowFrom = std::min(row1, row2);
                int rowTo = std::max(row1, row2);
                for (int k = rowFrom; k <= rowTo; k++) {
                    int pos = k * 8 + col1;
                    mask |= (1ull << pos);
                }
                for (int k = 0; k <= 7; k++) {
                    int pos = k * 8 + col1;
                    fullMask |= (1ull << pos);
                }
            }
            else if (row2 - row1 == col2 - col1) {
                dir = Direction::DiagonalMain;
                int rowFrom = std::min(row1, row2);
                int rowTo = std::max(row1, row2);
                int colFrom = std::min(col1, col2);
                int colTo = std::max(col1, col2);
                for (int k = rowFrom; k <= rowTo; k++) {
                    int pos = k * 8 + colFrom;
                    mask |= (1ull << pos);
                    colFrom++;
                }
                int r1 = row1;
                int c1 = col1;
                while (r1 >= 0 && r1 < 8 && c1 >= 0 && c1 < 8) {
                    int pos = r1 * 8 + c1;
                    fullMask |= (1ull << pos);
                    r1++;
                    c1++;
                }
                r1 = row1;
                c1 = col1;
                while (r1 >= 0 && r1 < 8 && c1 >= 0 && c1 < 8) {
                    int pos = r1 * 8 + c1;
                    fullMask |= (1ull << pos);
                    r1--;
                    c1--;
                }
            }
            else if (row2 - row1 == -(col2 - col1)) {
                dir = Direction::DiagonalCounter;
                int rowFrom = std::min(row1, row2);
                int rowTo = std::max(row1, row2);
                int colFrom = std::max(col1, col2);
                int colTo = std::min(col1, col2);
                for (int k = rowFrom; k <= rowTo; k++) {
                    int pos = k * 8 + colFrom;
                    mask |= (1ull << pos);
                    colFrom--;
                }
                int r1 = row1;
                int c1 = col1;
                while (r1 >= 0 && r1 < 8 && c1 >= 0 && c1 < 8) {
                    int pos = r1 * 8 + c1;
                    fullMask |= (1ull << pos);
                    r1++;
                    c1--;
                }
                r1 = row1;
                c1 = col1;
                while (r1 >= 0 && r1 < 8 && c1 >= 0 && c1 < 8) {
                    int pos = r1 * 8 + c1;
                    fullMask |= (1ull << pos);
                    r1--;
                    c1++;
                }
            }
            precalcDir[i][j] = dir;
            arrRectangular[i][j] = mask;
            dirMask[i][j] = fullMask;
        }
    }

    for (int i = 0; i < 64; i++) {
        int pawnFile = posToFile(i);
        uint64_t mask = 0;
        uint64_t maskNeighbor = 0;
        if (pawnFile == 0) {
            mask = getMaskOfFile(pawnFile) | getMaskOfFile(pawnFile + 1);
        }
        else if (pawnFile == 7) {
            mask = getMaskOfFile(pawnFile) | getMaskOfFile(pawnFile - 1);
        }
        else {
            mask = getMaskOfFile(pawnFile) | getMaskOfFile(pawnFile + 1) | getMaskOfFile(pawnFile - 1);
            maskNeighbor = getMaskOfFile(pawnFile + 1) | getMaskOfFile(pawnFile - 1);;
        }
        int pawnRank = posToRank(i);
        uint64_t n = ~0;
        n <<= ((pawnRank + 1) * 8);
        arrFrontPawn[Side::White][i] = n & mask;
        arrAfterPawn[Side::White][i] = n & getMaskOfFile(pawnFile);
        arrNeighorPawn[i] = maskNeighbor;
        uint64_t n2 = ~0;
        n2 >>= ((8 - pawnRank) * 8);
        arrFrontPawn[Side::Black][i] = n2 & mask;
        arrAfterPawn[Side::Black][i] = n2 & getMaskOfFile(pawnFile);
    }

    for (int i = 0; i < 64; i++) {
        int x1 = i / 8;
        int y1 = i % 8;
        uint64_t mask = 0;
        for (int j = 0; j < 8; j++) {

            for (int k = 0; k < 8; k++) {
                if (std::max(abs(x1 - j), abs(y1 - k)) == 2) {
                    mask |= 1ull << (j * 8 + k);
                }
            }
        }
        arrOuterRing[i] = mask;
    }

    int cntHash = 0;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 7; j++) {
            for (int k = 0; k < 64; k++) {
                randomHash.pieceRand[i][j][k] = precalcHash[cntHash];
                cntHash++;
            }
        }
    }

    for (int i = 0; i < 64; i++) {
        randomHash.enPassantRand[i] = precalcHash[cntHash];
        cntHash++;
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            randomHash.castlingRight[i][j] = precalcHash[cntHash];
            cntHash++;
        }
    }

    randomHash.sideMoving = precalcHash[cntHash];
}
//Return the attack on a certain square(pos) by side
int BitBoard::isSquareAttacked(uint64_t occupied, int pos, Side side) {
    uint64_t pawns = pieceBB[side][Piece::pawn];
    uint64_t knights = pieceBB[side][Piece::knight];
    uint64_t king = pieceBB[side][Piece::king];
    uint64_t bishopsQueens = pieceBB[side][queen] | pieceBB[side][Piece::bishop];
    uint64_t rooksQueens = pieceBB[side][Piece::queen] | pieceBB[side][Piece::rook];
    //std::cout << arrPawnAttacks[!side][pos]<<" "<<pawns << " "<<occupied<<" "<<pos<< std::endl;
    //std::cout << (arrPawnAttacks[!side][pos] & pawns) << std::endl;

    return !!(arrPawnAttacks[!side][pos] & pawns) + !!(arrKnightAttacks[pos] & knights) + !!(getBishopAttackSquares(occupied, pos) & bishopsQueens) + !!(getRookAttackSquares(occupied, pos) & rooksQueens);
}

uint64_t BitBoard::getAttackersOfSq(uint64_t occupied, int pos) {
    uint64_t queen = pieceBB[Side::White][Piece::queen] | pieceBB[Side::Black][Piece::queen];
    uint64_t rq = pieceBB[Side::White][Piece::rook] | pieceBB[Side::Black][Piece::rook] | queen;
    uint64_t bq = pieceBB[Side::White][Piece::bishop] | pieceBB[Side::Black][Piece::bishop] | queen;
    return (arrPawnAttacks[Side::White][pos] & pieceBB[Side::Black][Piece::pawn])
        | (arrPawnAttacks[Side::Black][pos] & pieceBB[Side::White][Piece::pawn])
        | (arrKnightAttacks[pos] & (pieceBB[Side::White][Piece::knight] | pieceBB[Side::Black][Piece::knight]))
        | (getRookAttackSquares(occupied, pos) & rq)
        | (getBishopAttackSquares(occupied, pos) & bq)
        | (arrKingAttacks[pos] & (pieceBB[Side::White][Piece::king] | pieceBB[Side::Black][Piece::king]));


}

uint64_t BitBoard::getBishopAttackSquares(uint64_t occ, int pos) {
    occ &= mBishopTbl[pos].mask;
    occ *= mBishopTbl[pos].magic;
    occ >>= 64 - mBishopTbl[pos].shift;
    return arrBishopAttacks[pos][occ];
}

uint64_t BitBoard::getRookAttackSquares(uint64_t occ, int pos) {
    //std::cout << pos << " ";
    occ &= mRookTbl[pos].mask;
    occ *= mRookTbl[pos].magic;
    occ >>= 64 - mRookTbl[pos].shift;

    return arrRookAttacks[pos][occ];
}

uint64_t BitBoard::getBishopXrayAttackSquares(uint64_t occ, uint64_t blockers, int pos) {
    uint64_t bishopAttackSquares = getBishopAttackSquares(occ, pos);
    uint64_t blockPieceMask = blockers & bishopAttackSquares;
    uint64_t occWithoutBlockPiece = occ ^ blockPieceMask;
    return bishopAttackSquares ^ getBishopAttackSquares(occWithoutBlockPiece, pos);
    /*
    Example: pos = 0
             occ=                           0
                                       1
                                   0
                               1
                           0
                       0
                   1(bishop pos)

           Output=                         0
                                       1
                                   1
                               0
                           0
                       0
                   0(bishop pos)
    */
}

uint64_t BitBoard::getRookXrayAttackSquares(uint64_t occ, uint64_t blockers, int pos) {
    uint64_t rookAttackSquares = getRookAttackSquares(occ, pos);
    uint64_t blockPieceMask = blockers & rookAttackSquares;
    uint64_t occWithoutBlockPiece = occ ^ blockPieceMask;
    return rookAttackSquares ^ getRookAttackSquares(occWithoutBlockPiece, pos);
}

Direction BitBoard::getDirFromTwoSquares(int pos1, int pos2) {
    return precalcDir[pos1][pos2];
}

uint64_t BitBoard::getAllAttackSquares(Side side) {
    uint64_t allAttackSquares = 0;
    uint64_t pawnBB = pieceBB[side][Piece::pawn];
    uint64_t occ = pieceBB[side][Piece::any] | pieceBB[!side][Piece::any];
    occ ^= pieceBB[!side][Piece::king];
    while (pawnBB) {
        int pawnPos = BitScanForward64(pawnBB);
        uint64_t pawnAttacks = getPawnAttackSquares(side, pawnPos);
        allAttackSquares |= pawnAttacks;
        pawnBB ^= (1ull << pawnPos);
    }
    uint64_t knightBB = pieceBB[side][Piece::knight];
    while (knightBB) {
        int knightPos = BitScanForward64(knightBB);
        uint64_t knightAttacks = getKnightAttackSquares(knightPos);
        allAttackSquares |= knightAttacks;
        knightBB ^= (1ull << knightPos);
    }
    uint64_t bishopQueenBB = pieceBB[side][Piece::bishop] | pieceBB[side][Piece::queen];
    while (bishopQueenBB) {
        int bishopPos = BitScanForward64(bishopQueenBB);
        uint64_t bishopAttacks = getBishopAttackSquares(occ, bishopPos);
        allAttackSquares |= bishopAttacks;
        bishopQueenBB ^= (1ull << bishopPos);
    }
    uint64_t rookQueenBB = pieceBB[side][Piece::rook] | pieceBB[side][Piece::queen];
    while (rookQueenBB) {
        int rookPos = BitScanForward64(rookQueenBB);
        //std::cout << "Rook pos: " << rookPos << std::endl;
        uint64_t rookAttacks = getRookAttackSquares(occ, rookPos);
        allAttackSquares |= rookAttacks;
        rookQueenBB ^= (1ull << rookPos);
    }
    uint64_t kingPos = BitScanForward64(pieceBB[side][Piece::king]);
    allAttackSquares |= getKingAttackSquares(kingPos);

    return allAttackSquares;
}

uint64_t BitBoard::getPinnedPiece(uint64_t occ, int kingPos, Side side) {
    //Get all pinned piece of a certain side
    uint64_t queens = pieceBB[!side][Piece::queen];
    uint64_t opRQ = queens | pieceBB[!side][Piece::rook];
    uint64_t opBQ = queens | pieceBB[!side][Piece::bishop];
    uint64_t pinned = 0;
    uint64_t pinner = getRookXrayAttackSquares(occ, pieceBB[side][Piece::any], kingPos) & opRQ;
    while (pinner) {
        int sq = BitScanForward64(pinner);
        pinned |= (arrRectangular[sq][kingPos] & pieceBB[side][Piece::any]) ^ (1ull << kingPos);
        pinner &= pinner - 1;
    }
    pinner = getBishopXrayAttackSquares(occ, pieceBB[side][Piece::any], kingPos) & opBQ;
    while (pinner) {
        int sq = BitScanForward64(pinner);
        pinned |= (arrRectangular[sq][kingPos] & pieceBB[side][Piece::any]) ^ (1ull << kingPos);
        pinner &= pinner - 1;
    }
    return pinned;
}

uint64_t BitBoard::getSingleCheckInterposingSquares(uint64_t occ, int kingPos, Side side) {
    uint64_t pawns = pieceBB[!side][Piece::pawn];
    if (arrPawnAttacks[side][kingPos] & pawns) {
        return arrPawnAttacks[side][kingPos] & pawns;
    }
    uint64_t knights = pieceBB[!side][Piece::knight];
    if (arrKnightAttacks[kingPos] & knights) {
        return arrKnightAttacks[kingPos] & knights;
    }
    uint64_t bishopsQueens = pieceBB[!side][queen] | pieceBB[!side][Piece::bishop];
    uint64_t maskCheckingPieceBishop = getBishopAttackSquares(occ, kingPos) & bishopsQueens;
    if (maskCheckingPieceBishop) {
        int checkingPieceIndex = BitScanForward64(maskCheckingPieceBishop);
        return arrRectangular[kingPos][checkingPieceIndex];
    }
    uint64_t rooksQueens = pieceBB[!side][Piece::queen] | pieceBB[!side][Piece::rook];
    uint64_t maskCheckingPieceRook = getRookAttackSquares(occ, kingPos) & rooksQueens;
    if (maskCheckingPieceRook) {
        int checkingPieceIndex = BitScanForward64(maskCheckingPieceRook);
        return arrRectangular[kingPos][checkingPieceIndex];
    }
}

uint64_t BitBoard::move(Move& move, uint64_t zHash) {
    moveNum++;
    assert(moveNum >= 0);
    Side sideToMove = move.getSideToMove();
    //std::cout << "Move make move" << std::endl;
    if (move.isNullMove()) {
        sideToMove = isWhiteTurn ? Side::White : Side::Black;
        move.setSideToMove(sideToMove);
        move.setCastleRightBefore(castingRight[sideToMove][0], castingRight[sideToMove][1]);
        move.setEnpassantPosBefore(enPassantPos);
        if (enPassantPos != 0) {
            zHash ^= randomHash.enPassantRand[enPassantPos];
        }
        enPassantPos = 0;
        isWhiteTurn = !isWhiteTurn;
        zHash ^= randomHash.sideMoving;
        repetitionTable.add(zHash, true);
        return zHash;
    }
    Piece movePiece = move.getMovePiece();
    Piece capturePiece = move.getCapturePiece();
    MoveType moveType = move.getMoveType();
    int startPos = move.getStartPos();
    int endPos = move.getEndPos();
    move.setCastleRightBefore(castingRight[sideToMove][0], castingRight[sideToMove][1]);
    move.setEnpassantPosBefore(enPassantPos);
    if (enPassantPos != 0) {
        zHash ^= randomHash.enPassantRand[enPassantPos];
    }
    enPassantPos = 0;
    if (movePiece == Piece::king) {
        if (castingRight[sideToMove][0]) {
            castingRight[sideToMove][0] = false;
            zHash ^= randomHash.castlingRight[sideToMove][0];
        }
        if (castingRight[sideToMove][1]) {
            castingRight[sideToMove][1] = false;
            zHash ^= randomHash.castlingRight[sideToMove][1];
        }
    }
    if (movePiece == Piece::rook) {
        if (sideToMove == Side::White) {
            if (startPos == 7 && castingRight[sideToMove][0]) {
                castingRight[sideToMove][0] = false;
                zHash ^= randomHash.castlingRight[sideToMove][0];
            }
            else if (startPos == 0 && castingRight[sideToMove][1]) {
                castingRight[sideToMove][1] = false;
                zHash ^= randomHash.castlingRight[sideToMove][1];
            }
        }
        else {
            if (startPos == 63 && castingRight[sideToMove][0]) {
                castingRight[sideToMove][0] = false;
                zHash ^= randomHash.castlingRight[sideToMove][0];
            }
            else if (startPos == 56 && castingRight[sideToMove][1]) {
                castingRight[sideToMove][1] = false;
                zHash ^= randomHash.castlingRight[sideToMove][1];
            }
        }
    }
    switch (moveType) {
    case MoveType::NORMAL:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][movePiece] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][movePiece][endPos];
        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = movePiece;
        if (movePiece == Piece::pawn && abs(endPos - startPos) == 16) {
            enPassantPos = (startPos + endPos) / 2;
            zHash ^= randomHash.enPassantRand[enPassantPos];
        }
        break;
    case MoveType::CAPTURE:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][movePiece] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceBB[!sideToMove][capturePiece] ^= (1ull << endPos);
        pieceBB[!sideToMove][Piece::any] ^= (1ull << endPos);

        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][movePiece][endPos] ^ randomHash.pieceRand[!sideToMove][capturePiece][endPos];

        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = movePiece;
        break;
    case MoveType::EN_PASSANT:
    {
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][movePiece] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        int pawnOffset = sideToMove == Side::White ? endPos - 8 : endPos + 8;
        pieceBB[!sideToMove][capturePiece] ^= (1ull << pawnOffset);
        pieceBB[!sideToMove][Piece::any] ^= (1ull << pawnOffset);
        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = movePiece;
        pieceTable[pawnOffset] = Piece::any;

        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][movePiece][endPos] ^ randomHash.pieceRand[!sideToMove][capturePiece][pawnOffset];
    }
    break;
    case MoveType::CASTLE_KINGSIDE:
    {
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][movePiece] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        int rookPos = sideToMove == Side::White ? 7 : 63;
        pieceBB[sideToMove][Piece::rook] ^= (1ull << rookPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << rookPos);
        pieceBB[sideToMove][Piece::rook] ^= (1ull << (rookPos - 2));
        pieceBB[sideToMove][Piece::any] ^= (1ull << (rookPos - 2));

        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = movePiece;
        pieceTable[rookPos] = Piece::any;
        pieceTable[rookPos - 2] = Piece::rook;
        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][movePiece][endPos] ^ randomHash.pieceRand[sideToMove][Piece::rook][rookPos] ^ randomHash.pieceRand[sideToMove][Piece::rook][rookPos - 2];
    }
    break;
    case MoveType::CASTLE_QUEENSIDE:
    {
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][movePiece] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        int rookPos = sideToMove == Side::White ? 0 : 56;
        pieceBB[sideToMove][Piece::rook] ^= (1ull << rookPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << rookPos);
        pieceBB[sideToMove][Piece::rook] ^= (1ull << (rookPos + 3));
        pieceBB[sideToMove][Piece::any] ^= (1ull << (rookPos + 3));

        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = movePiece;
        pieceTable[rookPos] = Piece::any;
        pieceTable[rookPos + 3] = Piece::rook;

        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][movePiece][endPos] ^ randomHash.pieceRand[sideToMove][Piece::rook][rookPos] ^ randomHash.pieceRand[sideToMove][Piece::rook][rookPos + 3];
    }
    break;
    case MoveType::PROMOTION_BISHOP:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::bishop] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = Piece::bishop;
        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][Piece::bishop][endPos];
        break;
    case MoveType::PROMOTION_BISHOP_AND_CAPTURE:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::bishop] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceBB[!sideToMove][capturePiece] ^= (1ull << endPos);
        pieceBB[!sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = Piece::bishop;

        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][Piece::bishop][endPos] ^ randomHash.pieceRand[!sideToMove][capturePiece][endPos];
        break;
    case MoveType::PROMOTION_KNIGHT:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::knight] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = Piece::knight;
        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][Piece::knight][endPos];
        break;
    case MoveType::PROMOTION_KNIGHT_AND_CAPTURE:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::knight] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceBB[!sideToMove][capturePiece] ^= (1ull << endPos);
        pieceBB[!sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = Piece::knight;
        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][Piece::knight][endPos] ^ randomHash.pieceRand[!sideToMove][capturePiece][endPos];
        break;
    case MoveType::PROMOTION_QUEEN:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::queen] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = Piece::queen;
        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][Piece::queen][endPos];
        break;
    case MoveType::PROMOTION_QUEEN_AND_CAPTURE:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::queen] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceBB[!sideToMove][capturePiece] ^= (1ull << endPos);
        pieceBB[!sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = Piece::queen;
        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][Piece::queen][endPos] ^ randomHash.pieceRand[!sideToMove][capturePiece][endPos];
        break;
    case MoveType::PROMOTION_ROOK:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::rook] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = Piece::rook;
        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][Piece::rook][endPos];
        break;
    case MoveType::PROMOTION_ROOK_AND_CAPTURE:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::rook] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceBB[!sideToMove][capturePiece] ^= (1ull << endPos);
        pieceBB[!sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[startPos] = Piece::any;
        pieceTable[endPos] = Piece::rook;
        zHash ^= randomHash.pieceRand[sideToMove][movePiece][startPos] ^ randomHash.pieceRand[sideToMove][Piece::rook][endPos] ^ randomHash.pieceRand[!sideToMove][capturePiece][endPos];
        break;
    default:
        break;

    }
    isWhiteTurn = !isWhiteTurn;
    zHash ^= randomHash.sideMoving;
    repetitionTable.add(zHash, moveType > 0 || movePiece == Piece::pawn);
    return zHash;
}

void BitBoard::undoMove(Move& move) {
    moveNum--;
    assert(moveNum >= 0);
    Side sideToMove = move.getSideToMove();
    if (move.isNullMove()) {
        castingRight[sideToMove][0] = move.getCastleRightBefore(0);
        castingRight[sideToMove][1] = move.getCastleRightBefore(1);
        enPassantPos = move.getEnpassantPosBefore();
        isWhiteTurn = !isWhiteTurn;
        repetitionTable.pop();
        return;
    }
    //std::cout << "Move make move" << std::endl;
    Piece movePiece = move.getMovePiece();
    Piece capturePiece = move.getCapturePiece();

    MoveType moveType = move.getMoveType();
    int startPos = move.getStartPos();
    int endPos = move.getEndPos();
    castingRight[sideToMove][0] = move.getCastleRightBefore(0);
    castingRight[sideToMove][1] = move.getCastleRightBefore(1);
    enPassantPos = move.getEnpassantPosBefore();
    switch (moveType) {
    case MoveType::NORMAL:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][movePiece] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[endPos] = Piece::any;
        pieceTable[startPos] = movePiece;
        break;
    case MoveType::CAPTURE:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][movePiece] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceBB[!sideToMove][capturePiece] ^= (1ull << endPos);
        pieceBB[!sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[endPos] = capturePiece;
        pieceTable[startPos] = movePiece;
        break;
    case MoveType::EN_PASSANT:
    {
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][movePiece] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        int pawnOffset = sideToMove == Side::White ? endPos - 8 : endPos + 8;
        pieceBB[!sideToMove][capturePiece] ^= (1ull << pawnOffset);
        pieceBB[!sideToMove][Piece::any] ^= (1ull << pawnOffset);
        pieceTable[endPos] = Piece::any;
        pieceTable[startPos] = movePiece;
        pieceTable[pawnOffset] = Piece::pawn;
        enPassantPos = endPos;
    }
    break;
    case MoveType::CASTLE_KINGSIDE:
    {
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][movePiece] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        int rookPos = sideToMove == Side::White ? 7 : 63;
        pieceBB[sideToMove][Piece::rook] ^= (1ull << rookPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << rookPos);
        pieceBB[sideToMove][Piece::rook] ^= (1ull << (rookPos - 2));
        pieceBB[sideToMove][Piece::any] ^= (1ull << (rookPos - 2));

        pieceTable[endPos] = Piece::any;
        pieceTable[startPos] = movePiece;
        pieceTable[rookPos - 2] = Piece::any;
        pieceTable[rookPos] = Piece::rook;
    }
    break;
    case MoveType::CASTLE_QUEENSIDE:
    {
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][movePiece] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        int rookPos = sideToMove == Side::White ? 0 : 56;
        pieceBB[sideToMove][Piece::rook] ^= (1ull << rookPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << rookPos);
        pieceBB[sideToMove][Piece::rook] ^= (1ull << (rookPos + 3));
        pieceBB[sideToMove][Piece::any] ^= (1ull << (rookPos + 3));

        pieceTable[endPos] = Piece::any;
        pieceTable[startPos] = movePiece;
        pieceTable[rookPos + 3] = Piece::any;
        pieceTable[rookPos] = Piece::rook;
    }
    break;
    case MoveType::PROMOTION_BISHOP:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::bishop] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[endPos] = Piece::any;
        pieceTable[startPos] = Piece::pawn;
        break;
    case MoveType::PROMOTION_BISHOP_AND_CAPTURE:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::bishop] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceBB[!sideToMove][capturePiece] ^= (1ull << endPos);
        pieceBB[!sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[endPos] = capturePiece;
        pieceTable[startPos] = Piece::pawn;
        break;
    case MoveType::PROMOTION_KNIGHT:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::knight] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[endPos] = Piece::any;
        pieceTable[startPos] = Piece::pawn;
        break;
    case MoveType::PROMOTION_KNIGHT_AND_CAPTURE:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::knight] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceBB[!sideToMove][capturePiece] ^= (1ull << endPos);
        pieceBB[!sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[endPos] = capturePiece;
        pieceTable[startPos] = Piece::pawn;
        break;
    case MoveType::PROMOTION_QUEEN:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::queen] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[endPos] = Piece::any;
        pieceTable[startPos] = Piece::pawn;
        break;
    case MoveType::PROMOTION_QUEEN_AND_CAPTURE:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::queen] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceBB[!sideToMove][capturePiece] ^= (1ull << endPos);
        pieceBB[!sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[endPos] = capturePiece;
        pieceTable[startPos] = Piece::pawn;
        break;
    case MoveType::PROMOTION_ROOK:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::rook] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[endPos] = Piece::any;
        pieceTable[startPos] = Piece::pawn;
        break;
    case MoveType::PROMOTION_ROOK_AND_CAPTURE:
        pieceBB[sideToMove][movePiece] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << startPos);
        pieceBB[sideToMove][Piece::rook] ^= (1ull << endPos);
        pieceBB[sideToMove][Piece::any] ^= (1ull << endPos);
        pieceBB[!sideToMove][capturePiece] ^= (1ull << endPos);
        pieceBB[!sideToMove][Piece::any] ^= (1ull << endPos);
        pieceTable[endPos] = capturePiece;
        pieceTable[startPos] = Piece::pawn;
        break;
    default:
        break;

    }
    isWhiteTurn = !isWhiteTurn;
    repetitionTable.pop();
}

std::vector<Move> BitBoard::getLegalMoves() {
    Side sideToMove = isWhiteTurn ? Side::White : Side::Black;
    Side opSide = sideToMove == Side::White ? Side::Black : Side::White;

    int kingPos = BitScanForward64(pieceBB[sideToMove][Piece::king]);
    //std::cout << pieceBB[sideToMove][Piece::king] << std::endl;
    uint64_t occupied = pieceBB[sideToMove][Piece::any] | pieceBB[!sideToMove][Piece::any];

    int checks = isSquareAttacked(occupied, kingPos, opSide);
    //std::cout << "Number of checks: " << checks << std::endl;
    uint64_t opAttackSquares = getAllAttackSquares(opSide);
    std::vector<Move> ans;
    ans.reserve(40);
    if (checks == 0) {
        uint64_t pinnedPiece = getPinnedPiece(occupied, kingPos, sideToMove);
        //pawn pseudo legal move
        uint64_t pawnBB = pieceBB[sideToMove][Piece::pawn];
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        while (pawnBB) {
            int pawnPos = BitScanForward64(pawnBB);
            bool isPinned = (1ull << pawnPos) & pinnedPiece;
            bool isPromotion = (1ull << pawnPos) & mask7thRank;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[pawnPos][kingPos];
            }
            uint64_t pawnCaptures = getPawnAttackSquares(sideToMove, pawnPos) & pieceBB[opSide][Piece::any] & maskDirectionPin;
            bool isStartingRank = sideToMove == Side::White ? (pawnPos >= 8 && pawnPos <= 15) : (pawnPos >= 48 && pawnPos <= 55);
            while (pawnCaptures) {
                int capturePos = BitScanForward64(pawnCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if(!isPinned|| (isPinned && getDirFromTwoSquares(pawnPos,capturePos)==getDirFromTwoSquares(pawnPos,kingPos)))
                if (isPromotion) {
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_QUEEN_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_KNIGHT_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_BISHOP_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_ROOK_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                }
                else {
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::pawn, capturePiece));
                }
                pawnCaptures ^= (1ull << capturePos);
            }
            //TODO: add check promotion,promotionCaptures, en pasasant
            int moveOffset = sideToMove == Side::White ? 8 : -8;
            if (!isPinned || (isPinned && getDirFromTwoSquares(pawnPos, pawnPos + moveOffset) == getDirFromTwoSquares(pawnPos, kingPos))) {
                if (!(1ull << (pawnPos + moveOffset) & occupied)) {
                    if (isPromotion) {
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_QUEEN, sideToMove, Piece::pawn));
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_KNIGHT, sideToMove, Piece::pawn));
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_BISHOP, sideToMove, Piece::pawn));
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_ROOK, sideToMove, Piece::pawn));
                    }
                    else {
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::NORMAL, sideToMove, Piece::pawn));
                    }
                    if (isStartingRank && !(1ull << (pawnPos + moveOffset * 2) & occupied))
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset * 2, MoveType::NORMAL, sideToMove, Piece::pawn));
                }

            }
            pawnBB ^= (1ull << pawnPos);
        }
        //check for en passant
        if (enPassantPos != 0) {
            uint64_t enPassantMask = arrPawnAttacks[opSide][enPassantPos] & pieceBB[sideToMove][Piece::pawn];
            while (enPassantMask) {
                int pawnPos = BitScanForward64(enPassantMask);
                Move pseudoLegal = Move(pawnPos, enPassantPos, MoveType::EN_PASSANT, sideToMove, Piece::pawn, Piece::pawn);
                move(pseudoLegal);
                if (!(getAllAttackSquares(opSide) & (1ull << kingPos))) {
                    ans.emplace_back(pseudoLegal);
                }
                undoMove(pseudoLegal);
                enPassantMask ^= (1ull << pawnPos);
            }
        }


        uint64_t knightBB = pieceBB[sideToMove][Piece::knight];
        while (knightBB) {
            int knightPos = BitScanForward64(knightBB);
            bool isPinned = (1ull << knightPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                knightBB ^= (1ull << knightPos);
                continue;
            }
            uint64_t knightAttacks = getKnightAttackSquares(knightPos) & ~pieceBB[sideToMove][Piece::any];
            uint64_t knightCaptures = knightAttacks & pieceBB[opSide][Piece::any];
            uint64_t knightNormalMoves = knightAttacks ^ knightCaptures;
            while (knightCaptures) {
                int capturePos = BitScanForward64(knightCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }

            while (knightNormalMoves) {
                int movePos = BitScanForward64(knightNormalMoves);
                //TODO: add move priority
                ans.emplace_back(Move(knightPos, movePos, MoveType::NORMAL, sideToMove, Piece::knight));
                knightNormalMoves ^= (1ull << movePos);
            }
            knightBB ^= (1ull << knightPos);
        }

        uint64_t bishopBB = pieceBB[sideToMove][Piece::bishop] | pieceBB[sideToMove][Piece::queen];
        while (bishopBB) {
            int bishopPos = BitScanForward64(bishopBB);
            Piece movePiece = pieceTable[bishopPos];
            bool isPinned = (1ull << bishopPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[bishopPos][kingPos];
            }
            uint64_t bishopAttacks = getBishopAttackSquares(occupied, bishopPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin;
            uint64_t bishopCaptures = bishopAttacks & pieceBB[opSide][Piece::any];
            uint64_t bishopNormalMoves = bishopAttacks ^ bishopCaptures;
            while (bishopCaptures) {
                int capturePos = BitScanForward64(bishopCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(bishopPos, capturePos) == getDirFromTwoSquares(bishopPos, kingPos)))
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, movePiece, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }

            while (bishopNormalMoves) {
                int movePos = BitScanForward64(bishopNormalMoves);
                bishopNormalMoves ^= (1ull << movePos);
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(bishopPos, movePos) == getDirFromTwoSquares(bishopPos, kingPos)))
                ans.emplace_back(Move(bishopPos, movePos, MoveType::NORMAL, sideToMove, movePiece));

            }
            bishopBB ^= (1ull << bishopPos);
        }
        uint64_t rookBB = pieceBB[sideToMove][Piece::rook] | pieceBB[sideToMove][Piece::queen];
        while (rookBB) {
            int rookPos = BitScanForward64(rookBB);
            Piece movePiece = pieceTable[rookPos];
            bool isPinned = (1ull << rookPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[rookPos][kingPos];
            }
            uint64_t rookAttacks = getRookAttackSquares(occupied, rookPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin;
            uint64_t rookCaptures = rookAttacks & pieceBB[opSide][Piece::any];
            uint64_t rookNormalMoves = rookAttacks ^ rookCaptures;
            while (rookCaptures) {
                int capturePos = BitScanForward64(rookCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(rookPos, capturePos) == getDirFromTwoSquares(rookPos, kingPos)))
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, movePiece, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }

            while (rookNormalMoves) {
                int movePos = BitScanForward64(rookNormalMoves);
                rookNormalMoves ^= (1ull << movePos);
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(rookPos, movePos) == getDirFromTwoSquares(rookPos, kingPos)))
                ans.emplace_back(Move(rookPos, movePos, MoveType::NORMAL, sideToMove, movePiece));

            }
            rookBB ^= (1ull << rookPos);
        }
        uint64_t kingBB = pieceBB[sideToMove][Piece::king];
        while (kingBB) {
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            uint64_t kingNormalMoves = kingAttacks ^ kingCaptures;
            while (kingCaptures) {
                int capturePos = BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = BitScanForward64(kingNormalMoves);
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, movePos, MoveType::NORMAL, sideToMove, Piece::king));
                kingNormalMoves ^= (1ull << movePos);
            }
            kingBB ^= (1ull << kingPos);
        }
        //special move 

        uint64_t maskCastleKingSide = sideToMove == Side::White ? 0x60 : (0x60ull << 56);
        uint64_t maskCastleQueenSide = sideToMove == Side::White ? 0xe : (0xeull << 56);
        uint64_t maskAttackQueenSide = sideToMove == Side::White ? 0xc : (0xcull << 56);
        uint64_t rookKingSidePos = sideToMove == Side::White ? (1ull << 7) : (1ull << 63);
        uint64_t rookQueenSidePos = sideToMove == Side::White ? (1ull << 0) : (1ull << 56);
        if (castingRight[sideToMove][0] && !(maskCastleKingSide & occupied) && !(maskCastleKingSide & opAttackSquares) && (pieceBB[sideToMove][Piece::rook] & rookKingSidePos)) {
            ans.emplace_back(Move(kingPos, sideToMove == Side::White ? 6 : 62, MoveType::CASTLE_KINGSIDE, sideToMove, Piece::king));
        }
        if (castingRight[sideToMove][1] && !(maskCastleQueenSide & occupied) && !(maskAttackQueenSide & opAttackSquares) && (pieceBB[sideToMove][Piece::rook] & rookQueenSidePos)) {
            ans.emplace_back(Move(kingPos, sideToMove == Side::White ? 2 : 58, MoveType::CASTLE_QUEENSIDE, sideToMove, Piece::king));
        }

    }
    else if (checks == 1) {
        uint64_t pinnedPiece = getPinnedPiece(occupied, kingPos, sideToMove);
        uint64_t interposingSquares = getSingleCheckInterposingSquares(occupied, kingPos, sideToMove);
        //std::cout << interposingSquares << std::endl;
        //pawn pseudo legal move
        uint64_t pawnBB = pieceBB[sideToMove][Piece::pawn];
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        while (pawnBB) {
            int pawnPos = BitScanForward64(pawnBB);
            bool isPinned = (1ull << pawnPos) & pinnedPiece;
            bool isPromotion = (1ull << pawnPos) & mask7thRank;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[pawnPos][kingPos];
            }
            uint64_t pawnCaptures = getPawnAttackSquares(sideToMove, pawnPos) & pieceBB[opSide][Piece::any] & maskDirectionPin & interposingSquares;
            bool isStartingRank = sideToMove == Side::White ? (pawnPos >= 8 && pawnPos <= 15) : (pawnPos >= 48 && pawnPos <= 55);
            while (pawnCaptures) {
                int capturePos = BitScanForward64(pawnCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if(!isPinned|| (isPinned && getDirFromTwoSquares(pawnPos,capturePos)==getDirFromTwoSquares(pawnPos,kingPos)))
                if (isPromotion) {
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_QUEEN_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_KNIGHT_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_BISHOP_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_ROOK_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                }
                else {
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::pawn, capturePiece));
                }
                pawnCaptures ^= (1ull << capturePos);
            }
            //TODO: add check promotion,promotionCaptures, en pasasant
            int moveOffset = sideToMove == Side::White ? 8 : -8;
            if (!isPinned || (isPinned && getDirFromTwoSquares(pawnPos, pawnPos + moveOffset) == getDirFromTwoSquares(pawnPos, kingPos))) {
                //if up 1 square is in interposing squares and the square does not have any piece
                if ((1ull << (pawnPos + moveOffset)) & (interposingSquares & ~occupied)) {
                    if (isPromotion) {
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_QUEEN, sideToMove, Piece::pawn));
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_KNIGHT, sideToMove, Piece::pawn));
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_BISHOP, sideToMove, Piece::pawn));
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_ROOK, sideToMove, Piece::pawn));
                    }
                    else {
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::NORMAL, sideToMove, Piece::pawn));
                    }

                }
                if (isStartingRank && ((1ull << (pawnPos + moveOffset * 2)) & (interposingSquares & ~occupied)) && !((1ull << (pawnPos + moveOffset)) & occupied))
                    ans.emplace_back(Move(pawnPos, pawnPos + moveOffset * 2, MoveType::NORMAL, sideToMove, Piece::pawn));

            }
            pawnBB ^= (1ull << pawnPos);
        }
        if (enPassantPos != 0) {
            uint64_t enPassantMask = arrPawnAttacks[opSide][enPassantPos] & pieceBB[sideToMove][Piece::pawn];
            while (enPassantMask) {
                int pawnPos = BitScanForward64(enPassantMask);
                Move pseudoLegal = Move(pawnPos, enPassantPos, MoveType::EN_PASSANT, sideToMove, Piece::pawn, Piece::pawn);
                move(pseudoLegal);
                if (!(getAllAttackSquares(opSide) & (1ull << kingPos))) {
                    ans.emplace_back(pseudoLegal);
                }
                undoMove(pseudoLegal);
                enPassantMask ^= (1ull << pawnPos);
            }
        }


        uint64_t knightBB = pieceBB[sideToMove][Piece::knight];
        while (knightBB) {
            int knightPos = BitScanForward64(knightBB);
            bool isPinned = (1ull << knightPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                knightBB ^= (1ull << knightPos);
                continue;
            }
            uint64_t knightAttacks = getKnightAttackSquares(knightPos) & ~pieceBB[sideToMove][Piece::any] & interposingSquares;
            uint64_t knightCaptures = knightAttacks & pieceBB[opSide][Piece::any];
            uint64_t knightNormalMoves = knightAttacks ^ knightCaptures;
            while (knightCaptures) {
                int capturePos = BitScanForward64(knightCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }

            while (knightNormalMoves) {
                int movePos = BitScanForward64(knightNormalMoves);
                //TODO: add move priority
                ans.emplace_back(Move(knightPos, movePos, MoveType::NORMAL, sideToMove, Piece::knight));
                knightNormalMoves ^= (1ull << movePos);
            }
            knightBB ^= (1ull << knightPos);
        }

        uint64_t bishopBB = pieceBB[sideToMove][Piece::bishop];
        while (bishopBB) {
            int bishopPos = BitScanForward64(bishopBB);
            bool isPinned = (1ull << bishopPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[bishopPos][kingPos];
            }
            uint64_t bishopAttacks = getBishopAttackSquares(occupied, bishopPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t bishopCaptures = bishopAttacks & pieceBB[opSide][Piece::any];
            uint64_t bishopNormalMoves = bishopAttacks ^ bishopCaptures;
            while (bishopCaptures) {
                int capturePos = BitScanForward64(bishopCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(bishopPos, capturePos) == getDirFromTwoSquares(bishopPos, kingPos)))
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::bishop, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }

            while (bishopNormalMoves) {
                int movePos = BitScanForward64(bishopNormalMoves);
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(bishopPos, movePos) == getDirFromTwoSquares(bishopPos, kingPos)))
                ans.emplace_back(Move(bishopPos, movePos, MoveType::NORMAL, sideToMove, Piece::bishop));
                bishopNormalMoves ^= (1ull << movePos);
            }
            bishopBB ^= (1ull << bishopPos);
        }
        uint64_t rookBB = pieceBB[sideToMove][Piece::rook];
        while (rookBB) {
            int rookPos = BitScanForward64(rookBB);
            bool isPinned = (1ull << rookPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[rookPos][kingPos];
            }
            uint64_t rookAttacks = getRookAttackSquares(occupied, rookPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t rookCaptures = rookAttacks & pieceBB[opSide][Piece::any];
            uint64_t rookNormalMoves = rookAttacks ^ rookCaptures;
            while (rookCaptures) {
                int capturePos = BitScanForward64(rookCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(rookPos, capturePos) == getDirFromTwoSquares(rookPos, kingPos)))
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::rook, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }

            while (rookNormalMoves) {
                int movePos = BitScanForward64(rookNormalMoves);
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(rookPos, movePos) == getDirFromTwoSquares(rookPos, kingPos)))
                ans.emplace_back(Move(rookPos, movePos, MoveType::NORMAL, sideToMove, Piece::rook));
                rookNormalMoves ^= (1ull << movePos);
            }
            rookBB ^= (1ull << rookPos);
        }
        uint64_t queenBB = pieceBB[sideToMove][Piece::queen];
        while (queenBB) {
            int queenPos = BitScanForward64(queenBB);
            bool isPinned = (1ull << queenPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[queenPos][kingPos];
            }
            uint64_t queenAttacks = (getBishopAttackSquares(occupied, queenPos) | getRookAttackSquares(occupied, queenPos)) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t queenCaptures = queenAttacks & pieceBB[opSide][Piece::any];
            uint64_t queenNormalMoves = queenAttacks ^ queenCaptures;
            while (queenCaptures) {
                int capturePos = BitScanForward64(queenCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(queenPos, capturePos) == getDirFromTwoSquares(queenPos, kingPos)))
                ans.emplace_back(Move(queenPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::queen, capturePiece));
                queenCaptures ^= (1ull << capturePos);
            }

            while (queenNormalMoves) {
                int movePos = BitScanForward64(queenNormalMoves);
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(queenPos, movePos) == getDirFromTwoSquares(queenPos, kingPos)))
                ans.emplace_back(Move(queenPos, movePos, MoveType::NORMAL, sideToMove, Piece::queen));
                queenNormalMoves ^= (1ull << movePos);
            }
            queenBB ^= (1ull << queenPos);
        }
        uint64_t kingBB = pieceBB[sideToMove][Piece::king];
        while (kingBB) {
            int kingPos = BitScanForward64(kingBB);
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            uint64_t kingNormalMoves = kingAttacks ^ kingCaptures;
            while (kingCaptures) {
                int capturePos = BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = BitScanForward64(kingNormalMoves);
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, movePos, MoveType::NORMAL, sideToMove, Piece::king));
                kingNormalMoves ^= (1ull << movePos);
            }
            kingBB ^= (1ull << kingPos);
        }
        //filter out absolute pinned piece


    }
    else {
        uint64_t kingBB = pieceBB[sideToMove][Piece::king];
        while (kingBB) {
            int kingPos = BitScanForward64(kingBB);
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            uint64_t kingNormalMoves = kingAttacks ^ kingCaptures;
            while (kingCaptures) {
                int capturePos = BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = BitScanForward64(kingNormalMoves);
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, movePos, MoveType::NORMAL, sideToMove, Piece::king));
                kingNormalMoves ^= (1ull << movePos);
            }
            kingBB ^= (1ull << kingPos);
        }
    }
    //For move-ordering


    return ans;

}
void BitBoard::getLegalMovesAlt(MoveVector& ans) {
    ans.reset();
    Side sideToMove = isWhiteTurn ? Side::White : Side::Black;
    Side opSide = sideToMove == Side::White ? Side::Black : Side::White;

    int kingPos = BitScanForward64(pieceBB[sideToMove][Piece::king]);
    //std::cout << pieceBB[sideToMove][Piece::king] << std::endl;
    uint64_t occupied = pieceBB[sideToMove][Piece::any] | pieceBB[!sideToMove][Piece::any];

    int checks = isSquareAttacked(occupied, kingPos, opSide);
    //std::cout << "Number of checks: " << checks << std::endl;
    uint64_t opAttackSquares = getAllAttackSquares(opSide);
    if (checks == 0) {
        uint64_t pinnedPiece = getPinnedPiece(occupied, kingPos, sideToMove);
        //pawn pseudo legal move
        uint64_t pawnBB = pieceBB[sideToMove][Piece::pawn];
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        while (pawnBB) {
            int pawnPos = BitScanForward64(pawnBB);
            bool isPinned = (1ull << pawnPos) & pinnedPiece;
            bool isPromotion = (1ull << pawnPos) & mask7thRank;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[pawnPos][kingPos];
            }
            uint64_t pawnCaptures = getPawnAttackSquares(sideToMove, pawnPos) & pieceBB[opSide][Piece::any] & maskDirectionPin;
            bool isStartingRank = sideToMove == Side::White ? (pawnPos >= 8 && pawnPos <= 15) : (pawnPos >= 48 && pawnPos <= 55);
            while (pawnCaptures) {
                int capturePos = BitScanForward64(pawnCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if(!isPinned|| (isPinned && getDirFromTwoSquares(pawnPos,capturePos)==getDirFromTwoSquares(pawnPos,kingPos)))
                if (isPromotion) {
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_QUEEN_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_KNIGHT_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_BISHOP_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_ROOK_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                }
                else {
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::pawn, capturePiece));
                }
                pawnCaptures ^= (1ull << capturePos);
            }
            //TODO: add check promotion,promotionCaptures, en pasasant
            int moveOffset = sideToMove == Side::White ? 8 : -8;
            if (!isPinned || (isPinned && getDirFromTwoSquares(pawnPos, pawnPos + moveOffset) == getDirFromTwoSquares(pawnPos, kingPos))) {
                if (!(1ull << (pawnPos + moveOffset) & occupied)) {
                    if (isPromotion) {
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_QUEEN, sideToMove, Piece::pawn));
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_KNIGHT, sideToMove, Piece::pawn));
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_BISHOP, sideToMove, Piece::pawn));
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_ROOK, sideToMove, Piece::pawn));
                    }
                    else {
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::NORMAL, sideToMove, Piece::pawn));
                    }
                    if (isStartingRank && !(1ull << (pawnPos + moveOffset * 2) & occupied))
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset * 2, MoveType::NORMAL, sideToMove, Piece::pawn));
                }

            }
            pawnBB ^= (1ull << pawnPos);
        }
        //check for en passant
        if (enPassantPos != 0) {
            uint64_t enPassantMask = arrPawnAttacks[opSide][enPassantPos] & pieceBB[sideToMove][Piece::pawn];
            while (enPassantMask) {
                int pawnPos = BitScanForward64(enPassantMask);
                Move pseudoLegal = Move(pawnPos, enPassantPos, MoveType::EN_PASSANT, sideToMove, Piece::pawn, Piece::pawn);
                move(pseudoLegal);
                if (!(getAllAttackSquares(opSide) & (1ull << kingPos))) {
                    ans.emplace_back(pseudoLegal);
                }
                undoMove(pseudoLegal);
                enPassantMask ^= (1ull << pawnPos);
            }
        }


        uint64_t knightBB = pieceBB[sideToMove][Piece::knight];
        while (knightBB) {
            int knightPos = BitScanForward64(knightBB);
            bool isPinned = (1ull << knightPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                knightBB ^= (1ull << knightPos);
                continue;
            }
            uint64_t knightAttacks = getKnightAttackSquares(knightPos) & ~pieceBB[sideToMove][Piece::any];
            uint64_t knightCaptures = knightAttacks & pieceBB[opSide][Piece::any];
            uint64_t knightNormalMoves = knightAttacks ^ knightCaptures;
            while (knightCaptures) {
                int capturePos = BitScanForward64(knightCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }

            while (knightNormalMoves) {
                int movePos = BitScanForward64(knightNormalMoves);
                //TODO: add move priority
                ans.emplace_back(Move(knightPos, movePos, MoveType::NORMAL, sideToMove, Piece::knight));
                knightNormalMoves ^= (1ull << movePos);
            }
            knightBB ^= (1ull << knightPos);
        }

        uint64_t bishopBB = pieceBB[sideToMove][Piece::bishop] | pieceBB[sideToMove][Piece::queen];
        while (bishopBB) {
            int bishopPos = BitScanForward64(bishopBB);
            Piece movePiece = pieceTable[bishopPos];
            bool isPinned = (1ull << bishopPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[bishopPos][kingPos];
            }
            uint64_t bishopAttacks = getBishopAttackSquares(occupied, bishopPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin;
            uint64_t bishopCaptures = bishopAttacks & pieceBB[opSide][Piece::any];
            uint64_t bishopNormalMoves = bishopAttacks ^ bishopCaptures;
            while (bishopCaptures) {
                int capturePos = BitScanForward64(bishopCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(bishopPos, capturePos) == getDirFromTwoSquares(bishopPos, kingPos)))
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, movePiece, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }

            while (bishopNormalMoves) {
                int movePos = BitScanForward64(bishopNormalMoves);
                bishopNormalMoves ^= (1ull << movePos);
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(bishopPos, movePos) == getDirFromTwoSquares(bishopPos, kingPos)))
                ans.emplace_back(Move(bishopPos, movePos, MoveType::NORMAL, sideToMove, movePiece));

            }
            bishopBB ^= (1ull << bishopPos);
        }
        uint64_t rookBB = pieceBB[sideToMove][Piece::rook] | pieceBB[sideToMove][Piece::queen];
        while (rookBB) {
            int rookPos = BitScanForward64(rookBB);
            Piece movePiece = pieceTable[rookPos];
            bool isPinned = (1ull << rookPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[rookPos][kingPos];
            }
            uint64_t rookAttacks = getRookAttackSquares(occupied, rookPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin;
            uint64_t rookCaptures = rookAttacks & pieceBB[opSide][Piece::any];
            uint64_t rookNormalMoves = rookAttacks ^ rookCaptures;
            while (rookCaptures) {
                int capturePos = BitScanForward64(rookCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(rookPos, capturePos) == getDirFromTwoSquares(rookPos, kingPos)))
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, movePiece, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }

            while (rookNormalMoves) {
                int movePos = BitScanForward64(rookNormalMoves);
                rookNormalMoves ^= (1ull << movePos);
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(rookPos, movePos) == getDirFromTwoSquares(rookPos, kingPos)))
                ans.emplace_back(Move(rookPos, movePos, MoveType::NORMAL, sideToMove, movePiece));

            }
            rookBB ^= (1ull << rookPos);
        }
        uint64_t kingBB = pieceBB[sideToMove][Piece::king];
        while (kingBB) {
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            uint64_t kingNormalMoves = kingAttacks ^ kingCaptures;
            while (kingCaptures) {
                int capturePos = BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = BitScanForward64(kingNormalMoves);
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, movePos, MoveType::NORMAL, sideToMove, Piece::king));
                kingNormalMoves ^= (1ull << movePos);
            }
            kingBB ^= (1ull << kingPos);
        }
        //special move 

        uint64_t maskCastleKingSide = sideToMove == Side::White ? 0x60 : (0x60ull << 56);
        uint64_t maskCastleQueenSide = sideToMove == Side::White ? 0xe : (0xeull << 56);
        uint64_t maskAttackQueenSide = sideToMove == Side::White ? 0xc : (0xcull << 56);
        uint64_t rookKingSidePos = sideToMove == Side::White ? (1ull << 7) : (1ull << 63);
        uint64_t rookQueenSidePos = sideToMove == Side::White ? (1ull << 0) : (1ull << 56);
        if (castingRight[sideToMove][0] && !(maskCastleKingSide & occupied) && !(maskCastleKingSide & opAttackSquares) && (pieceBB[sideToMove][Piece::rook] & rookKingSidePos)) {
            ans.emplace_back(Move(kingPos, sideToMove == Side::White ? 6 : 62, MoveType::CASTLE_KINGSIDE, sideToMove, Piece::king));
        }
        if (castingRight[sideToMove][1] && !(maskCastleQueenSide & occupied) && !(maskAttackQueenSide & opAttackSquares) && (pieceBB[sideToMove][Piece::rook] & rookQueenSidePos)) {
            ans.emplace_back(Move(kingPos, sideToMove == Side::White ? 2 : 58, MoveType::CASTLE_QUEENSIDE, sideToMove, Piece::king));
        }

    }
    else if (checks == 1) {
        uint64_t pinnedPiece = getPinnedPiece(occupied, kingPos, sideToMove);
        uint64_t interposingSquares = getSingleCheckInterposingSquares(occupied, kingPos, sideToMove);
        //std::cout << interposingSquares << std::endl;
        //pawn pseudo legal move
        uint64_t pawnBB = pieceBB[sideToMove][Piece::pawn];
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        while (pawnBB) {
            int pawnPos = BitScanForward64(pawnBB);
            bool isPinned = (1ull << pawnPos) & pinnedPiece;
            bool isPromotion = (1ull << pawnPos) & mask7thRank;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[pawnPos][kingPos];
            }
            uint64_t pawnCaptures = getPawnAttackSquares(sideToMove, pawnPos) & pieceBB[opSide][Piece::any] & maskDirectionPin & interposingSquares;
            bool isStartingRank = sideToMove == Side::White ? (pawnPos >= 8 && pawnPos <= 15) : (pawnPos >= 48 && pawnPos <= 55);
            while (pawnCaptures) {
                int capturePos = BitScanForward64(pawnCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if(!isPinned|| (isPinned && getDirFromTwoSquares(pawnPos,capturePos)==getDirFromTwoSquares(pawnPos,kingPos)))
                if (isPromotion) {
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_QUEEN_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_KNIGHT_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_BISHOP_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_ROOK_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                }
                else {
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::pawn, capturePiece));
                }
                pawnCaptures ^= (1ull << capturePos);
            }
            //TODO: add check promotion,promotionCaptures, en pasasant
            int moveOffset = sideToMove == Side::White ? 8 : -8;
            if (!isPinned || (isPinned && getDirFromTwoSquares(pawnPos, pawnPos + moveOffset) == getDirFromTwoSquares(pawnPos, kingPos))) {
                //if up 1 square is in interposing squares and the square does not have any piece
                if ((1ull << (pawnPos + moveOffset)) & (interposingSquares & ~occupied)) {
                    if (isPromotion) {
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_QUEEN, sideToMove, Piece::pawn));
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_KNIGHT, sideToMove, Piece::pawn));
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_BISHOP, sideToMove, Piece::pawn));
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_ROOK, sideToMove, Piece::pawn));
                    }
                    else {
                        ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::NORMAL, sideToMove, Piece::pawn));
                    }

                }
                if (isStartingRank && ((1ull << (pawnPos + moveOffset * 2)) & (interposingSquares & ~occupied)) && !((1ull << (pawnPos + moveOffset)) & occupied))
                    ans.emplace_back(Move(pawnPos, pawnPos + moveOffset * 2, MoveType::NORMAL, sideToMove, Piece::pawn));

            }
            pawnBB ^= (1ull << pawnPos);
        }
        if (enPassantPos != 0) {
            uint64_t enPassantMask = arrPawnAttacks[opSide][enPassantPos] & pieceBB[sideToMove][Piece::pawn];
            while (enPassantMask) {
                int pawnPos = BitScanForward64(enPassantMask);
                Move pseudoLegal = Move(pawnPos, enPassantPos, MoveType::EN_PASSANT, sideToMove, Piece::pawn, Piece::pawn);
                move(pseudoLegal);
                if (!(getAllAttackSquares(opSide) & (1ull << kingPos))) {
                    ans.emplace_back(pseudoLegal);
                }
                undoMove(pseudoLegal);
                enPassantMask ^= (1ull << pawnPos);
            }
        }


        uint64_t knightBB = pieceBB[sideToMove][Piece::knight];
        while (knightBB) {
            int knightPos = BitScanForward64(knightBB);
            bool isPinned = (1ull << knightPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                knightBB ^= (1ull << knightPos);
                continue;
            }
            uint64_t knightAttacks = getKnightAttackSquares(knightPos) & ~pieceBB[sideToMove][Piece::any] & interposingSquares;
            uint64_t knightCaptures = knightAttacks & pieceBB[opSide][Piece::any];
            uint64_t knightNormalMoves = knightAttacks ^ knightCaptures;
            while (knightCaptures) {
                int capturePos = BitScanForward64(knightCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }

            while (knightNormalMoves) {
                int movePos = BitScanForward64(knightNormalMoves);
                //TODO: add move priority
                ans.emplace_back(Move(knightPos, movePos, MoveType::NORMAL, sideToMove, Piece::knight));
                knightNormalMoves ^= (1ull << movePos);
            }
            knightBB ^= (1ull << knightPos);
        }

        uint64_t bishopBB = pieceBB[sideToMove][Piece::bishop];
        while (bishopBB) {
            int bishopPos = BitScanForward64(bishopBB);
            bool isPinned = (1ull << bishopPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[bishopPos][kingPos];
            }
            uint64_t bishopAttacks = getBishopAttackSquares(occupied, bishopPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t bishopCaptures = bishopAttacks & pieceBB[opSide][Piece::any];
            uint64_t bishopNormalMoves = bishopAttacks ^ bishopCaptures;
            while (bishopCaptures) {
                int capturePos = BitScanForward64(bishopCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(bishopPos, capturePos) == getDirFromTwoSquares(bishopPos, kingPos)))
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::bishop, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }

            while (bishopNormalMoves) {
                int movePos = BitScanForward64(bishopNormalMoves);
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(bishopPos, movePos) == getDirFromTwoSquares(bishopPos, kingPos)))
                ans.emplace_back(Move(bishopPos, movePos, MoveType::NORMAL, sideToMove, Piece::bishop));
                bishopNormalMoves ^= (1ull << movePos);
            }
            bishopBB ^= (1ull << bishopPos);
        }
        uint64_t rookBB = pieceBB[sideToMove][Piece::rook];
        while (rookBB) {
            int rookPos = BitScanForward64(rookBB);
            bool isPinned = (1ull << rookPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[rookPos][kingPos];
            }
            uint64_t rookAttacks = getRookAttackSquares(occupied, rookPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t rookCaptures = rookAttacks & pieceBB[opSide][Piece::any];
            uint64_t rookNormalMoves = rookAttacks ^ rookCaptures;
            while (rookCaptures) {
                int capturePos = BitScanForward64(rookCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(rookPos, capturePos) == getDirFromTwoSquares(rookPos, kingPos)))
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::rook, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }

            while (rookNormalMoves) {
                int movePos = BitScanForward64(rookNormalMoves);
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(rookPos, movePos) == getDirFromTwoSquares(rookPos, kingPos)))
                ans.emplace_back(Move(rookPos, movePos, MoveType::NORMAL, sideToMove, Piece::rook));
                rookNormalMoves ^= (1ull << movePos);
            }
            rookBB ^= (1ull << rookPos);
        }
        uint64_t queenBB = pieceBB[sideToMove][Piece::queen];
        while (queenBB) {
            int queenPos = BitScanForward64(queenBB);
            bool isPinned = (1ull << queenPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[queenPos][kingPos];
            }
            uint64_t queenAttacks = (getBishopAttackSquares(occupied, queenPos) | getRookAttackSquares(occupied, queenPos)) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t queenCaptures = queenAttacks & pieceBB[opSide][Piece::any];
            uint64_t queenNormalMoves = queenAttacks ^ queenCaptures;
            while (queenCaptures) {
                int capturePos = BitScanForward64(queenCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(queenPos, capturePos) == getDirFromTwoSquares(queenPos, kingPos)))
                ans.emplace_back(Move(queenPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::queen, capturePiece));
                queenCaptures ^= (1ull << capturePos);
            }

            while (queenNormalMoves) {
                int movePos = BitScanForward64(queenNormalMoves);
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(queenPos, movePos) == getDirFromTwoSquares(queenPos, kingPos)))
                ans.emplace_back(Move(queenPos, movePos, MoveType::NORMAL, sideToMove, Piece::queen));
                queenNormalMoves ^= (1ull << movePos);
            }
            queenBB ^= (1ull << queenPos);
        }
        uint64_t kingBB = pieceBB[sideToMove][Piece::king];
        while (kingBB) {
            int kingPos = BitScanForward64(kingBB);
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            uint64_t kingNormalMoves = kingAttacks ^ kingCaptures;
            while (kingCaptures) {
                int capturePos = BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = BitScanForward64(kingNormalMoves);
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, movePos, MoveType::NORMAL, sideToMove, Piece::king));
                kingNormalMoves ^= (1ull << movePos);
            }
            kingBB ^= (1ull << kingPos);
        }
        //filter out absolute pinned piece


    }
    else {
        uint64_t kingBB = pieceBB[sideToMove][Piece::king];
        while (kingBB) {
            int kingPos = BitScanForward64(kingBB);
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            uint64_t kingNormalMoves = kingAttacks ^ kingCaptures;
            while (kingCaptures) {
                int capturePos = BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = BitScanForward64(kingNormalMoves);
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, movePos, MoveType::NORMAL, sideToMove, Piece::king));
                kingNormalMoves ^= (1ull << movePos);
            }
            kingBB ^= (1ull << kingPos);
        }
    }
    //For move-ordering
}
std::vector<Move> BitBoard::getLegalCaptures() {
    Side sideToMove = isWhiteTurn ? Side::White : Side::Black;
    Side opSide = sideToMove == Side::White ? Side::Black : Side::White;

    int kingPos = BitScanForward64(pieceBB[sideToMove][Piece::king]);
    //std::cout << pieceBB[sideToMove][Piece::king] << std::endl;
    uint64_t occupied = pieceBB[sideToMove][Piece::any] | pieceBB[!sideToMove][Piece::any];

    int checks = isSquareAttacked(occupied, kingPos, opSide);
    //std::cout << "Number of checks: " << checks << std::endl;
    uint64_t opAttackSquares = getAllAttackSquares(opSide);
    std::vector<Move> ans;
    ans.reserve(12);
    if (checks == 0) {
        uint64_t pinnedPiece = getPinnedPiece(occupied, kingPos, sideToMove);
        //pawn pseudo legal move
        uint64_t pawnBB = pieceBB[sideToMove][Piece::pawn];
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        int moveOffset = sideToMove == Side::White ? 8 : -8;
        while (pawnBB) {
            int pawnPos = BitScanForward64(pawnBB);
            bool isPinned = (1ull << pawnPos) & pinnedPiece;
            bool isPromotion = (1ull << pawnPos) & mask7thRank;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[pawnPos][kingPos];
            }
            uint64_t pawnCaptures = getPawnAttackSquares(sideToMove, pawnPos) & pieceBB[opSide][Piece::any] & maskDirectionPin;
            bool isStartingRank = sideToMove == Side::White ? (pawnPos >= 8 && pawnPos <= 15) : (pawnPos >= 48 && pawnPos <= 55);
            while (pawnCaptures) {
                int capturePos = BitScanForward64(pawnCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if(!isPinned|| (isPinned && getDirFromTwoSquares(pawnPos,capturePos)==getDirFromTwoSquares(pawnPos,kingPos)))
                if (isPromotion) {
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_QUEEN_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_KNIGHT_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_BISHOP_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_ROOK_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                }
                else {
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::pawn, capturePiece));
                }
                pawnCaptures ^= (1ull << capturePos);
            }
            //TODO: add check promotion,promotionCaptures, en pasasant

            if (isPromotion && (!isPinned || (isPinned && getDirFromTwoSquares(pawnPos, pawnPos + moveOffset) == getDirFromTwoSquares(pawnPos, kingPos)))) {
                if (!(1ull << (pawnPos + moveOffset) & occupied)) {
                    ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_QUEEN, sideToMove, Piece::pawn));
                    ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_KNIGHT, sideToMove, Piece::pawn));
                    ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_BISHOP, sideToMove, Piece::pawn));
                    ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_ROOK, sideToMove, Piece::pawn));
                }

            }
            pawnBB ^= (1ull << pawnPos);
        }
        //check for en passant
        if (enPassantPos != 0) {
            uint64_t enPassantMask = arrPawnAttacks[opSide][enPassantPos] & pieceBB[sideToMove][Piece::pawn];
            while (enPassantMask) {
                int pawnPos = BitScanForward64(enPassantMask);
                Move pseudoLegal = Move(pawnPos, enPassantPos, MoveType::EN_PASSANT, sideToMove, Piece::pawn, Piece::pawn);
                move(pseudoLegal);
                if (!(getAllAttackSquares(opSide) & (1ull << kingPos))) {
                    ans.emplace_back(pseudoLegal);
                }
                undoMove(pseudoLegal);
                enPassantMask ^= (1ull << pawnPos);
            }
        }


        uint64_t knightBB = pieceBB[sideToMove][Piece::knight];
        while (knightBB) {
            int knightPos = BitScanForward64(knightBB);
            bool isPinned = (1ull << knightPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                knightBB ^= (1ull << knightPos);
                continue;
            }
            uint64_t knightAttacks = getKnightAttackSquares(knightPos) & ~pieceBB[sideToMove][Piece::any];
            uint64_t knightCaptures = knightAttacks & pieceBB[opSide][Piece::any];
            while (knightCaptures) {
                int capturePos = BitScanForward64(knightCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }
            knightBB ^= (1ull << knightPos);
        }

        uint64_t bishopBB = pieceBB[sideToMove][Piece::bishop];
        while (bishopBB) {
            int bishopPos = BitScanForward64(bishopBB);
            bool isPinned = (1ull << bishopPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[bishopPos][kingPos];
            }
            uint64_t bishopAttacks = getBishopAttackSquares(occupied, bishopPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin;
            uint64_t bishopCaptures = bishopAttacks & pieceBB[opSide][Piece::any];
            while (bishopCaptures) {
                int capturePos = BitScanForward64(bishopCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(bishopPos, capturePos) == getDirFromTwoSquares(bishopPos, kingPos)))
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::bishop, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }
            bishopBB ^= (1ull << bishopPos);
        }
        uint64_t rookBB = pieceBB[sideToMove][Piece::rook];
        while (rookBB) {
            int rookPos = BitScanForward64(rookBB);
            bool isPinned = (1ull << rookPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[rookPos][kingPos];
            }
            uint64_t rookAttacks = getRookAttackSquares(occupied, rookPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin;
            uint64_t rookCaptures = rookAttacks & pieceBB[opSide][Piece::any];
            while (rookCaptures) {
                int capturePos = BitScanForward64(rookCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(rookPos, capturePos) == getDirFromTwoSquares(rookPos, kingPos)))
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::rook, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }
            rookBB ^= (1ull << rookPos);
        }
        uint64_t queenBB = pieceBB[sideToMove][Piece::queen];
        while (queenBB) {
            int queenPos = BitScanForward64(queenBB);
            bool isPinned = (1ull << queenPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[queenPos][kingPos];
            }
            uint64_t queenAttacks = (getBishopAttackSquares(occupied, queenPos) | getRookAttackSquares(occupied, queenPos)) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin;
            uint64_t queenCaptures = queenAttacks & pieceBB[opSide][Piece::any];
            while (queenCaptures) {
                int capturePos = BitScanForward64(queenCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(queenPos, capturePos) == getDirFromTwoSquares(queenPos, kingPos)))
                ans.emplace_back(Move(queenPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::queen, capturePiece));
                queenCaptures ^= (1ull << capturePos);
            }
            queenBB ^= (1ull << queenPos);
        }
        uint64_t kingBB = pieceBB[sideToMove][Piece::king];
        while (kingBB) {
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            while (kingCaptures) {
                int capturePos = BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }
            kingBB ^= (1ull << kingPos);
        }

    }
    else if (checks == 1) {
        uint64_t pinnedPiece = getPinnedPiece(occupied, kingPos, sideToMove);
        uint64_t interposingSquares = getSingleCheckInterposingSquares(occupied, kingPos, sideToMove);
        //std::cout << interposingSquares << std::endl;
        //pawn pseudo legal move
        uint64_t pawnBB = pieceBB[sideToMove][Piece::pawn];
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        int moveOffset = sideToMove == Side::White ? 8 : -8;
        while (pawnBB) {
            int pawnPos = BitScanForward64(pawnBB);
            bool isPinned = (1ull << pawnPos) & pinnedPiece;
            bool isPromotion = (1ull << pawnPos) & mask7thRank;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[pawnPos][kingPos];
            }
            uint64_t pawnCaptures = getPawnAttackSquares(sideToMove, pawnPos) & pieceBB[opSide][Piece::any] & maskDirectionPin & interposingSquares;
            bool isStartingRank = sideToMove == Side::White ? (pawnPos >= 8 && pawnPos <= 15) : (pawnPos >= 48 && pawnPos <= 55);
            while (pawnCaptures) {
                int capturePos = BitScanForward64(pawnCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if(!isPinned|| (isPinned && getDirFromTwoSquares(pawnPos,capturePos)==getDirFromTwoSquares(pawnPos,kingPos)))
                if (isPromotion) {
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_QUEEN_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_KNIGHT_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_BISHOP_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::PROMOTION_ROOK_AND_CAPTURE, sideToMove, Piece::pawn, capturePiece));
                }
                else {
                    ans.emplace_back(Move(pawnPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::pawn, capturePiece));
                }
                pawnCaptures ^= (1ull << capturePos);
            }
            //TODO: add check promotion,promotionCaptures, en pasasant

            if (isPromotion && (!isPinned || (isPinned && getDirFromTwoSquares(pawnPos, pawnPos + moveOffset) == getDirFromTwoSquares(pawnPos, kingPos)))) {
                //if up 1 square is in interposing squares and the square does not have any piece
                if ((1ull << (pawnPos + moveOffset)) & (interposingSquares & ~occupied)) {
                    ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_QUEEN, sideToMove, Piece::pawn));
                    ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_KNIGHT, sideToMove, Piece::pawn));
                    ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_BISHOP, sideToMove, Piece::pawn));
                    ans.emplace_back(Move(pawnPos, pawnPos + moveOffset, MoveType::PROMOTION_ROOK, sideToMove, Piece::pawn));
                }

            }
            pawnBB ^= (1ull << pawnPos);
        }
        if (enPassantPos != 0) {
            uint64_t enPassantMask = arrPawnAttacks[opSide][enPassantPos] & pieceBB[sideToMove][Piece::pawn];
            while (enPassantMask) {
                int pawnPos = BitScanForward64(enPassantMask);
                Move pseudoLegal = Move(pawnPos, enPassantPos, MoveType::EN_PASSANT, sideToMove, Piece::pawn, Piece::pawn);
                move(pseudoLegal);
                if (!(getAllAttackSquares(opSide) & (1ull << kingPos))) {
                    ans.emplace_back(pseudoLegal);
                }
                undoMove(pseudoLegal);
                enPassantMask ^= (1ull << pawnPos);
            }
        }


        uint64_t knightBB = pieceBB[sideToMove][Piece::knight];
        while (knightBB) {
            int knightPos = BitScanForward64(knightBB);
            bool isPinned = (1ull << knightPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                knightBB ^= (1ull << knightPos);
                continue;
            }
            uint64_t knightAttacks = getKnightAttackSquares(knightPos) & ~pieceBB[sideToMove][Piece::any] & interposingSquares;
            uint64_t knightCaptures = knightAttacks & pieceBB[opSide][Piece::any];
            while (knightCaptures) {
                int capturePos = BitScanForward64(knightCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }
            knightBB ^= (1ull << knightPos);
        }

        uint64_t bishopBB = pieceBB[sideToMove][Piece::bishop];
        while (bishopBB) {
            int bishopPos = BitScanForward64(bishopBB);
            bool isPinned = (1ull << bishopPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[bishopPos][kingPos];
            }
            uint64_t bishopAttacks = getBishopAttackSquares(occupied, bishopPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t bishopCaptures = bishopAttacks & pieceBB[opSide][Piece::any];
            while (bishopCaptures) {
                int capturePos = BitScanForward64(bishopCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(bishopPos, capturePos) == getDirFromTwoSquares(bishopPos, kingPos)))
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::bishop, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }
            bishopBB ^= (1ull << bishopPos);
        }
        uint64_t rookBB = pieceBB[sideToMove][Piece::rook];
        while (rookBB) {
            int rookPos = BitScanForward64(rookBB);
            bool isPinned = (1ull << rookPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[rookPos][kingPos];
            }
            uint64_t rookAttacks = getRookAttackSquares(occupied, rookPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t rookCaptures = rookAttacks & pieceBB[opSide][Piece::any];
            while (rookCaptures) {
                int capturePos = BitScanForward64(rookCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(rookPos, capturePos) == getDirFromTwoSquares(rookPos, kingPos)))
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::rook, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }
            rookBB ^= (1ull << rookPos);
        }
        uint64_t queenBB = pieceBB[sideToMove][Piece::queen];
        while (queenBB) {
            int queenPos = BitScanForward64(queenBB);
            bool isPinned = (1ull << queenPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[queenPos][kingPos];
            }
            uint64_t queenAttacks = (getBishopAttackSquares(occupied, queenPos) | getRookAttackSquares(occupied, queenPos)) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t queenCaptures = queenAttacks & pieceBB[opSide][Piece::any];
            while (queenCaptures) {
                int capturePos = BitScanForward64(queenCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                //if (!isPinned || (isPinned && getDirFromTwoSquares(queenPos, capturePos) == getDirFromTwoSquares(queenPos, kingPos)))
                ans.emplace_back(Move(queenPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::queen, capturePiece));
                queenCaptures ^= (1ull << capturePos);
            }
            queenBB ^= (1ull << queenPos);
        }
        uint64_t kingBB = pieceBB[sideToMove][Piece::king];
        while (kingBB) {
            int kingPos = BitScanForward64(kingBB);
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            while (kingCaptures) {
                int capturePos = BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }
            kingBB ^= (1ull << kingPos);
        }
        //filter out absolute pinned piece


    }
    else {
        uint64_t kingBB = pieceBB[sideToMove][Piece::king];
        while (kingBB) {
            int kingPos = BitScanForward64(kingBB);
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            while (kingCaptures) {
                int capturePos = BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];
                //TODO: add move priority
                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }
            kingBB ^= (1ull << kingPos);
        }
    }
    return ans;

}

bool BitBoard::isKingInCheck() {
    Side sideToMove = isWhiteTurn ? Side::White : Side::Black;
    Side opSide = sideToMove == Side::White ? Side::Black : Side::White;
    int kingPos = BitScanForward64(pieceBB[sideToMove][Piece::king]);
    //std::cout << pieceBB[sideToMove][Piece::king] << std::endl;
    uint64_t occupied = pieceBB[sideToMove][Piece::any] | pieceBB[!sideToMove][Piece::any];
    return isSquareAttacked(occupied, kingPos, opSide);
}

uint64_t BitBoard::perft(int depth, int maxDepth) {
    std::vector<Move> moves;

    uint64_t nodes = 0;

    moves = getLegalMoves();
    cntNode++;
    if (depth == 1)
        return moves.size();

    for (int i = 0; i < moves.size(); i++) {
        move(moves[i]);
        uint64_t res = perft(depth - 1, maxDepth);
        if (depth == maxDepth)
            debugPerft[indexToSquare[moves[i].getStartPos()] + indexToSquare[moves[i].getEndPos()]] = res;
        nodes += res;
        undoMove(moves[i]);
    }
    return nodes;
}


bool BitBoard::isValidMove(Move& move) {
    Side sideToMove = move.getSideToMove();
    Piece movePiece = move.getMovePiece();
    Piece capturePiece = move.getCapturePiece();
    MoveType moveType = move.getMoveType();
    int startPos = move.getStartPos();
    int endPos = move.getEndPos();
    uint64_t occ = pieceBB[Side::White][Piece::any] | pieceBB[Side::Black][Piece::any];
    switch (moveType) {
    case MoveType::NORMAL:
        return pieceTable[startPos] == movePiece && pieceTable[endPos] == Piece::any;
        break;
    case MoveType::CAPTURE:

        return pieceTable[startPos] == movePiece && pieceTable[endPos] == capturePiece;
        break;
    case MoveType::EN_PASSANT:
    {
        return pieceTable[startPos] == movePiece && pieceTable[endPos] == Piece::any && enPassantPos == endPos;
    }
    break;
    case MoveType::CASTLE_KINGSIDE:
    {
        return pieceTable[startPos] == movePiece && pieceTable[endPos] == Piece::any;
    }
    break;
    case MoveType::CASTLE_QUEENSIDE:
    {
        return pieceTable[startPos] == movePiece && pieceTable[endPos] == Piece::any;
    }
    break;
    case MoveType::PROMOTION_BISHOP:
        return pieceTable[startPos] == movePiece && pieceTable[endPos] == Piece::any;
        break;
    case MoveType::PROMOTION_BISHOP_AND_CAPTURE:
        return pieceTable[startPos] == movePiece && pieceTable[endPos] == capturePiece;
        break;
    case MoveType::PROMOTION_KNIGHT:
        return pieceTable[startPos] == movePiece && pieceTable[endPos] == Piece::any;
        break;
    case MoveType::PROMOTION_KNIGHT_AND_CAPTURE:
        return pieceTable[startPos] == movePiece && pieceTable[endPos] == capturePiece;
        break;
    case MoveType::PROMOTION_QUEEN:
        return pieceTable[startPos] == movePiece && pieceTable[endPos] == Piece::any;
        break;
    case MoveType::PROMOTION_QUEEN_AND_CAPTURE:
        return pieceTable[startPos] == movePiece && pieceTable[endPos] == capturePiece;
        break;
    case MoveType::PROMOTION_ROOK:
        return pieceTable[startPos] == movePiece && pieceTable[endPos] == Piece::any;
        break;
    case MoveType::PROMOTION_ROOK_AND_CAPTURE:
        return pieceTable[startPos] == movePiece && pieceTable[endPos] == capturePiece;
        break;
    default:
        break;

    }
    return false;
}

bool BitBoard::SEE_GE(Move m, int threshold) {
    int from = m.getStartPos();
    int to = m.getEndPos();

    int swap = pieceValue[pieceTable[to]] - threshold;
    if (swap < 0)
        return false;

    swap = pieceValue[pieceTable[from]] - swap;
    if (swap <= 0)
        return true;
    uint64_t occupied = (pieceBB[Side::White][Piece::any] | pieceBB[Side::Black][Piece::any]) ^ (1ull << from) ^ (1ull << to);  // xoring to is important for pinned piece logic
    Side    stm = m.getSideToMove();
    uint64_t attackers = getAttackersOfSq(occupied, to);
    uint64_t stmAttackers, bb;
    int      res = 1;
    int kingPos[2] = { BitScanForward64(pieceBB[Side::White][Piece::king]),
                    BitScanForward64(pieceBB[Side::Black][Piece::king])
    };

    uint64_t pawns = pieceBB[Side::White][Piece::pawn] | pieceBB[Side::Black][Piece::pawn];
    uint64_t knights = pieceBB[Side::White][Piece::knight] | pieceBB[Side::Black][Piece::knight];
    uint64_t queens = pieceBB[Side::White][Piece::queen] | pieceBB[Side::Black][Piece::queen];
    uint64_t bishopQueens = pieceBB[Side::White][Piece::bishop] | pieceBB[Side::Black][Piece::bishop] | queens;
    uint64_t rookQueens = pieceBB[Side::White][Piece::rook] | pieceBB[Side::Black][Piece::rook] | queens;
    while (true)
    {

        stm = static_cast<Side>(!stm);
        attackers &= occupied;

        // If stm has no more attackers then give up: stm loses
        if (!(stmAttackers = attackers & pieceBB[stm][Piece::any]))
            break;

        // Don't allow pinned pieces to attack as long as there are
        // pinners on their original square.
        stmAttackers &= ~getPinnedPiece(occupied, kingPos[stm], stm);

        if (!stmAttackers)
            break;

        res ^= 1;

        // Locate and remove the next least valuable attacker, and add to
        // the bitboard 'attackers' any X-ray attackers behind it.
        if (bb = stmAttackers & pawns)
        {
            if ((swap = pieceValue[Piece::pawn] - swap) < res)
                break;
            occupied ^= (bb & (~bb + 1));

            attackers |= getBishopAttackSquares(occupied, to) & bishopQueens;

        }

        else if ((bb = stmAttackers & (pieceBB[Side::White][Piece::knight] | pieceBB[Side::Black][Piece::knight])))
        {
            if ((swap = pieceValue[Piece::knight] - swap) < res)
                break;
            occupied ^= (bb & (~bb + 1));

        }

        else if ((bb = stmAttackers & (pieceBB[Side::White][Piece::bishop] | pieceBB[Side::Black][Piece::bishop])))
        {
            if ((swap = pieceValue[Piece::bishop] - swap) < res)
                break;
            occupied ^= (bb & (~bb + 1));

            attackers |= getBishopAttackSquares(occupied, to) & bishopQueens;

        }

        else if ((bb = stmAttackers & (pieceBB[Side::White][Piece::rook] | pieceBB[Side::Black][Piece::rook])))
        {
            if ((swap = pieceValue[Piece::rook] - swap) < res)
                break;
            occupied ^= (bb & (~bb + 1));

            attackers |= getRookAttackSquares(occupied, to) & rookQueens;

        }

        else if ((bb = stmAttackers & (pieceBB[Side::White][Piece::queen] | pieceBB[Side::Black][Piece::queen])))
        {
            if ((swap = pieceValue[Piece::queen] - swap) < res)
                break;
            occupied ^= (bb & (~bb + 1));

            attackers |= (getBishopAttackSquares(occupied, to) & bishopQueens)
                | (getRookAttackSquares(occupied, to) & rookQueens);

        }

        else  // KING
              // If we "capture" with the king but the opponent still has attackers,
              // reverse the result.

            return (attackers & ~pieceBB[stm][Piece::any]) ? res ^ 1 : res;
    }

    return bool(res);
}

int BitBoard::evaluate(int alpha, int beta) {
    uint64_t occupied = (pieceBB[Side::White][Piece::any] | pieceBB[Side::Black][Piece::any]);
    int nPieces = popcount64(occupied);
    if (nPieces == 2) {
        return 0;
    }
    else if (nPieces == 3) {
        uint64_t drawPieces = pieceBB[Side::White][Piece::knight] | pieceBB[Side::White][Piece::bishop] | pieceBB[Side::Black][Piece::knight] | pieceBB[Side::Black][Piece::bishop];
        if (drawPieces) {
            return 0;
        }
    }
    else if (nPieces == 4) {
        if (pieceBB[Side::White][Piece::bishop] && pieceBB[Side::Black][Piece::bishop]) {
            return 0;
        }
        if ((pieceBB[Side::White][Piece::rook] && pieceBB[Side::Black][Piece::bishop]) || (pieceBB[Side::Black][Piece::rook] && pieceBB[Side::White][Piece::bishop])) {
            return 0;
        }
        if ((pieceBB[Side::White][Piece::bishop] && pieceBB[Side::Black][Piece::knight]) || (pieceBB[Side::Black][Piece::bishop] && pieceBB[Side::White][Piece::knight])) {
            return 0;
        }
    }
    int who2move = (isWhiteTurn ? 1 : -1);
    /*
    //Material for lazy eval
    int diffQueens = static_cast<int>(popcount64(pieceBB[Side::White][Piece::queen]) - popcount64(pieceBB[Side::Black][Piece::queen]))
        * pieceValue[Piece::queen];
    int diffRooks = static_cast<int>(popcount64(pieceBB[Side::White][Piece::rook]) - popcount64(pieceBB[Side::Black][Piece::rook]))
        * pieceValue[Piece::rook];
    int diffKnights = static_cast<int>(popcount64(pieceBB[Side::White][Piece::knight]) - popcount64(pieceBB[Side::Black][Piece::knight]))
        * pieceValue[Piece::knight];
    int diffBishops = static_cast<int>(popcount64(pieceBB[Side::White][Piece::bishop]) - popcount64(pieceBB[Side::Black][Piece::bishop]))
        * pieceValue[Piece::bishop];
    int diffPawns = static_cast<int>(popcount64(pieceBB[Side::White][Piece::pawn]) - popcount64(pieceBB[Side::Black][Piece::pawn]))
        * pieceValue[Piece::pawn];
    int diffMaterial = diffQueens + diffRooks + diffKnights + diffBishops + diffPawns;
    */

    int totalOcc = popcount64((pieceBB[Side::White][Piece::any] | pieceBB[Side::Black][Piece::any]));

    int score[2]{ 0 };
    int phase = TOTAL_PHASE;

    int kingPos[2] = {
        BitScanForward64(pieceBB[Side::White][Piece::king]),
        BitScanForward64(pieceBB[Side::Black][Piece::king])
    };
    int rankKing[2] = { posToRank(kingPos[0]) , posToRank(kingPos[1]) };
    int fileKing[2] = { posToFile(kingPos[0]) , posToFile(kingPos[1]) };
    int totalWeightedKingDistToOwnPawn[2]{ 0 };
    int sumOfWeightDist[2]{ 0 };


    //float knightValMobility = (static_cast<float>(MobilityOpening::KNIGHT) * phaseOpening + static_cast<float>(MobilityEndgame::KNIGHT) * phaseEndgame);
    int posFlipped = 0;
    uint64_t attacks = 0;
    uint64_t isDoublePawn = false;
    bool isIsolatedPawn = false;
    bool isOnlyKingAndPawn = isKingAndPawnEndgame();
    Side stm = isWhiteTurn ? Side::White : Side::Black;
    int mat[2] = { 1,1 };
    uint64_t pawnSet = pieceBB[0][Piece::pawn] | pieceBB[1][Piece::pawn];
    for (int side = 0; side < 2; side++) {

        uint64_t occupied2 = pieceBB[side][Piece::any];
        int opKingPos = kingPos[!side];
        int ownKingPos = kingPos[side];
        uint64_t opKingInnerRing = getKingAttackSquares(opKingPos);

        uint64_t opKingOuterRing = arrOuterRing[opKingPos];
        uint64_t kingInnerRing = getKingAttackSquares(ownKingPos);
        uint64_t kingOuterRing = arrOuterRing[ownKingPos];
        int flip = side == Side::White ? 56 : 0;
        uint64_t myPiece = occupied2;
        float nAttackers = 0;
        float nDefenders = static_cast<float>(popcount64(pieceBB[!side][Piece::pawn] & opKingInnerRing)) * 0.5f + static_cast<float>(popcount64(pieceBB[!side][Piece::pawn] & opKingOuterRing)) * 0.2f;
        uint64_t pieces = pieceBB[!side][Piece::any] ^ pieceBB[!side][Piece::pawn];
        nDefenders += popcount64(pieces & opKingInnerRing) * 1.0f + popcount64(pieces & opKingOuterRing) * 0.5f;
        int totalAtkUnits = 0;
        int nAtksOnInnerRing = 0;
        int nAtksOnOuterRing = 0;
        int nDefsOnInnerRing = 0;
        int pawnPushOffset = side == Side::White ? 8 : -8;
        bool sideIsSTM = side == stm;
        uint64_t opPawnSetAtkSq = side == Side::Black ? (pieceBB[!side][Piece::pawn] << 7ull | pieceBB[!side][Piece::pawn] << 9ull) : (pieceBB[!side][Piece::pawn] >> 7ull | pieceBB[!side][Piece::pawn] >> 9ull);
        if (popcount64(pieceBB[side][Piece::bishop]) == 2) {
            score[side] += bishopPairBonus;
        }
        while (occupied2) {
            int pos = BitScanForward64(occupied2);
            occupied2 ^= (1ull << pos);
            Side opSide = static_cast<Side>(!side);
            Piece piece = pieceTable[pos];
            //Material
            score[side] += materialValue[piece];

            posFlipped = pos ^ flip;
            if (piece >= 2) {
                //opponent's pawn attack own piece
                if (arrPawnAttacks[side][pos] & pieceBB[!side][Piece::pawn]) {
                    if (sideIsSTM)
                        score[side] -= penaltyAttackedByPawn[0];
                    else
                        score[side] -= penaltyAttackedByPawn[1];
                }
            }
            switch (piece)
            {
            case any:
                break;
            case pawn:
            {
                int rankPawn = posToRank(pos);
                int filePawn = posToFile(pos);
                int weight = 3;
                int manhattanDist = abs(rankKing[side] - rankPawn) + abs(fileKing[side] - filePawn);
                if (isPassedPawn(static_cast<Side>(side), static_cast<Side>(!side), pos)) {
                    //flip to white perspective
                    if (side == Side::Black)
                        rankPawn = 7 - rankPawn;
                    score[side] += bonusPassedPawnByRank[rankPawn];
                    if (isConnectedPawn(static_cast<Side>(side), pos)) {
                        score[side] += bonusConnectedPassedPawn;
                    }
                    //Check for piece in front of pawn

                    if (pieceTable[pos + pawnPushOffset] != Piece::any) {
                        score[side] -= penaltyBlockedPasserByRank[rankPawn];
                    }
                    if (filePawn == 0 || filePawn == 7)
                        score[side] += bonusOutsidePasser;
                    weight = 6;
                }
                totalWeightedKingDistToOwnPawn[side] += weight * manhattanDist;
                sumOfWeightDist[side] += weight;


                isDoublePawn = arrAfterPawn[side][pos] & pieceBB[side][Piece::pawn];
                isIsolatedPawn = !(arrNeighorPawn[pos] & pieceBB[side][Piece::pawn]);
                if (isDoublePawn) {
                    score[side] -= pawnDoublePenalty;
                }
                if (isIsolatedPawn) {
                    score[side] -= pawnIsolatedPenalty;
                }

                score[side] += pawnTable[posFlipped];
                attacks = arrPawnAttacks[side][Piece::pawn];
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 0.6f;
                else if (nAtksOnOuterRing)
                    nAttackers += 0.3f;
                totalAtkUnits += nAtksOnInnerRing * 2 + nAtksOnOuterRing;
            }
            break;
            case knight:
                phase--;
                score[side] += knightTable[posFlipped];
                attacks = getKnightAttackSquares(pos) & ~(myPiece | opPawnSetAtkSq);
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 1.0f;
                else if (nAtksOnOuterRing)
                    nAttackers += 0.5f;
                totalAtkUnits += nAtksOnInnerRing * 3 + nAtksOnOuterRing * 2;
                score[side] += popcount64(attacks) * mobilityValue[Piece::knight];
                break;
            case bishop:
                phase--;
                score[side] += bishopTable[posFlipped];
                attacks = getBishopAttackSquares(occupied, pos) & ~(myPiece | opPawnSetAtkSq);
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 1.0f;
                else if (nAtksOnOuterRing)
                    nAttackers += 0.5f;
                totalAtkUnits += nAtksOnInnerRing * 3 + nAtksOnOuterRing * 2;
                score[side] += popcount64(attacks) * mobilityValue[Piece::bishop];
                break;
            case rook:
                phase -= 2;
                score[side] += rookTable[posFlipped];
                //arrAfterPawn[side][pos] & pieceBB[side][Piece::pawn];
                //
                if (!(arrAfterPawn[side][pos] & pawnSet))
                    score[side] += bonusRookOnOpenFile;
                else if (!(arrAfterPawn[side][pos] & pieceBB[side][Piece::pawn]))
                    score[side] += bonusRookOnSemiOpenFile;
                attacks = getRookAttackSquares(occupied, pos) & ~(myPiece | opPawnSetAtkSq);
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 1.0f;
                else if (nAtksOnOuterRing)
                    nAttackers += 0.5f;
                totalAtkUnits += nAtksOnInnerRing * 4 + nAtksOnOuterRing * 3;
                score[side] += popcount64(attacks) * mobilityValue[Piece::rook];
                break;
            case queen:
                phase -= 4;
                score[side] += queenTable[posFlipped];
                attacks = (getRookAttackSquares(occupied, pos) | getBishopAttackSquares(occupied, pos)) & ~(myPiece | opPawnSetAtkSq);
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 2.0f;
                else if (nAtksOnOuterRing)
                    nAttackers += 1.0f;
                totalAtkUnits += nAtksOnInnerRing * 5 + nAtksOnOuterRing * 4;
                score[side] += popcount64(attacks) * mobilityValue[Piece::queen];
                break;
            case king:
                score[side] += kingTable[posFlipped];
                attacks = getKingAttackSquares(pos) & ~(myPiece | opPawnSetAtkSq);
                score[side] += popcount64(attacks) * mobilityValue[Piece::king];
                break;
            default:
                break;
            }
        }

        if (nAttackers>= 3) {
            score[side] += SafetyTable[totalAtkUnits]*(nAttackers-nDefenders);
        }
        /*
        atkDefsKing[side][0] = nAttackers;
        atkDefsKing[side][1] = nDefenders;
        atkDefsKing[side][2] = totalAtkUnits;
        */
    }
    /*
    float chanceAtkWhite = atkDefsKing[Side::White][0] - atkDefsKing[Side::Black][1];
    float chanceAtkBlack = atkDefsKing[Side::Black][0] - atkDefsKing[Side::White][1];
    if (chanceAtkWhite > 2) {
        uint32_t atkBonus = SafetyTable[static_cast<int>(atkDefsKing[Side::White][2])];
        uint32_t mg = atkBonus & 0xFFFFull;
        uint32_t eg = (atkBonus ^ mg) >> 16;
        float scaleAtk = chanceAtkWhite / 4.0f;
        score[Side::White] += static_cast<int>(static_cast<uint32_t>(static_cast<float>(eg)*scaleAtk) << 16) + static_cast<uint32_t>(static_cast<float>(mg)*scaleAtk);
    }
    if (chanceAtkBlack > 2) {
        uint32_t atkBonus = SafetyTable[static_cast<int>(atkDefsKing[Side::Black][2])];
        uint32_t mg = atkBonus & 0xFFFFull;
        uint32_t eg = (atkBonus ^ mg) >> 16;
        float scaleAtk = chanceAtkBlack / 4.0f;
        score[Side::Black] += static_cast<int>(static_cast<uint32_t>(static_cast<float>(eg) * scaleAtk) << 16) + static_cast<uint32_t>(static_cast<float>(mg) * scaleAtk);
    }
    */
    //king safety-related
    uint64_t pawnsBBAll = pieceBB[Side::White][Piece::pawn] | pieceBB[Side::Black][Piece::pawn];
    for (int side = 0; side < 2; side++) {
        uint64_t kingAttacks = getKingAttackSquares(kingPos[side]);
        int nPawnsSurroundKing = popcount64(kingAttacks & pieceBB[side][Piece::pawn]);
        int nOpenFilesKing = 0;
        int nSemiOpenFilesKing = 0;
        if (fileKing[side] == 0) {
            nOpenFilesKing = (!(arrAfterPawn[side][kingPos[side]] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[side][kingPos[side]] & pawnsBBAll));
            nSemiOpenFilesKing = (!(arrAfterPawn[side][kingPos[side]] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[side][kingPos[side] + 1] & pieceBB[side][Piece::pawn]));
        }
        else if (fileKing[side] == 7) {
            nOpenFilesKing = (!(arrAfterPawn[side][kingPos[side]] & pawnsBBAll)) + (!(arrAfterPawn[side][kingPos[side] - 1] & pawnsBBAll));
            nSemiOpenFilesKing = (!(arrAfterPawn[side][kingPos[side]] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[side][kingPos[side] - 1] & pieceBB[side][Piece::pawn]));
        }
        else {
            nOpenFilesKing = (!(arrAfterPawn[side][kingPos[side]] & pawnsBBAll)) + (!(arrAfterPawn[side][kingPos[side] - 1] & pawnsBBAll)) + (!(arrAfterPawn[side][kingPos[side] + 1] & pawnsBBAll));
            nSemiOpenFilesKing = (!(arrAfterPawn[side][kingPos[side]] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[side][kingPos[side] + 1] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[side][kingPos[side] - 1] & pieceBB[side][Piece::pawn]));
        }
        score[side] += nPawnsSurroundKing * bonusPawnSurroundKing;
        score[side] -= openFilesPenalty[nOpenFilesKing];
        score[side] -= semiOpenFilesPenalty[nSemiOpenFilesKing];
    }

    phase = (phase * 256 + (TOTAL_PHASE / 2)) / TOTAL_PHASE;

    //0: midgame,256:endgame
    if (sumOfWeightDist[0] > 0) {
        score[0] -= static_cast<int>((static_cast<float>(totalWeightedKingDistToOwnPawn[0]) / static_cast<float>(sumOfWeightDist[0]))) * kingPawnTropismFactor;
    }
    if (sumOfWeightDist[1] > 0) {
        score[1] -= static_cast<int>((static_cast<float>(totalWeightedKingDistToOwnPawn[1]) / static_cast<float>(sumOfWeightDist[1]))) * kingPawnTropismFactor;
    }
    score[stm] += tempo;

    int strongerSide = score[0] < score[1];
    int strongerSidePawnMissing = 8 - popcount64(pieceBB[strongerSide][Piece::pawn]);
    auto scale = (128 - strongerSidePawnMissing * strongerSidePawnMissing) / static_cast<double>(128);
    int eval = (score[0] - score[1]);

    int ans = (((short)eval * (256 - phase) + ((eval + 0x8000) >> 16) * scale * (phase)) / 256);

    //Mop-up eval
    if (pieceBB[0][Piece::pawn] == 0 && pieceBB[1][Piece::pawn] == 0) {
        float distBetweenKing = abs(fileKing[0] - fileKing[1]) + abs(rankKing[0] - rankKing[1]);
        if (ans >= 0) {
            ans += (4.7f * arrCenterManhattanDistance[kingPos[1]] + 1.6 * (14.0f - distBetweenKing));
        }
        else {
            ans -= (4.7f * arrCenterManhattanDistance[kingPos[0]] + 1.6 * (14.0f - distBetweenKing));
        }
    }

    return ans * who2move * 100;

}

Trace BitBoard::evaluate2() {
    Trace trace{};
    uint64_t occupied = (pieceBB[Side::White][Piece::any] | pieceBB[Side::Black][Piece::any]);

    int totalOcc = popcount64((pieceBB[Side::White][Piece::any] | pieceBB[Side::Black][Piece::any]));

    int score[2]{ 0 };
    int phase = TOTAL_PHASE;
    int kingPos[2] = {
        BitScanForward64(pieceBB[Side::White][Piece::king]),
        BitScanForward64(pieceBB[Side::Black][Piece::king])
    };
    int rankKing[2] = { posToRank(kingPos[0]) , posToRank(kingPos[1]) };
    int fileKing[2] = { posToFile(kingPos[0]) , posToFile(kingPos[1]) };
    int totalWeightedKingDistToOwnPawn[2]{ 0 };
    int sumOfWeightDist[2]{ 0 };


    //float knightValMobility = (static_cast<float>(MobilityOpening::KNIGHT) * phaseOpening + static_cast<float>(MobilityEndgame::KNIGHT) * phaseEndgame);

    int pos = 0;
    int posFlipped = 0;
    uint64_t attacks = 0;
    uint64_t isDoublePawn = false;
    bool isIsolatedPawn = false;

    for (int side = 0; side < 2; side++) {

        uint64_t occupied2 = pieceBB[side][Piece::any];
        int opKingPos = kingPos[!side];
        uint64_t opKingInnerRing = getKingAttackSquares(opKingPos);
        uint64_t opKingOuterRing = arrOuterRing[opKingPos];
        int flip = side == Side::White ? 56 : 0;
        uint64_t myPiece = occupied2;
        float nAttackers = 0;
        int totalAtkUnits = 0;
        int nAtksOnInnerRing = 0;
        int nAtksOnOuterRing = 0;

        if (popcount64(pieceBB[side][Piece::bishop]) == 2) {
            score[side] += bishopPairBonus;
            trace.bishop_pair[side]++;
        }

        while (occupied2) {
            int pos = BitScanForward64(occupied2);
            uint64_t maskPos = 1ull << pos;
            occupied2 ^= maskPos;
            Side opSide = static_cast<Side>(!side);
            Piece piece = pieceTable[pos];
            //Material
            score[side] += materialValue[piece];
            trace.material[piece][side]++;
            //Phase
            phase -= phases[piece];

            posFlipped = pos ^ flip;

            switch (piece)
            {
            case any:
                break;
            case pawn:
            {
                int rankPawn = posToRank(pos);
                int filePawn = posToFile(pos);
                int weight = 3;
                int manhattanDist = abs(rankKing[side] - rankPawn) + abs(fileKing[side] - filePawn);
                if (isPassedPawn(static_cast<Side>(side), static_cast<Side>(!side), pos)) {
                    score[side] += bonusPassedPawnByRank[rankPawn];
                    trace.passers[rankPawn][side]++;
                    if (isConnectedPawn(static_cast<Side>(side), pos)) {
                        score[side] += bonusConnectedPassedPawn;
                        trace.pawn_passed_protected[side]++;
                    }
                    weight = 6;
                }
                totalWeightedKingDistToOwnPawn[side] += weight * manhattanDist;
                sumOfWeightDist[side] += weight;


                isDoublePawn = arrAfterPawn[side][pos] & pieceBB[side][Piece::pawn];
                isIsolatedPawn = !(arrNeighorPawn[pos] & pieceBB[side][Piece::pawn]);
                if (isDoublePawn) {
                    score[side] -= pawnDoublePenalty;
                    trace.pawn_doubled_penalty[side]--;
                }
                if (isIsolatedPawn) {
                    score[side] -= pawnIsolatedPenalty;
                    trace.pawn_isolated_penalty[side]--;
                }

                score[side] += pawnTable[posFlipped];
                attacks = arrPawnAttacks[side][Piece::pawn];
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 1;
                if (nAtksOnOuterRing)
                    nAttackers += 0.5;
                totalAtkUnits += nAtksOnInnerRing * 2 + nAtksOnOuterRing;
            }
            break;
            case knight:
                score[side] += knightTable[posFlipped];
                attacks = getKnightAttackSquares(pos) & ~myPiece;
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 1;
                if (nAtksOnOuterRing)
                    nAttackers += 0.5;
                totalAtkUnits += nAtksOnInnerRing * 3 + nAtksOnOuterRing * 2;
                score[side] += popcount64(attacks) * mobilityValue[Piece::knight];
                trace.mobilities[Piece::knight][side] += popcount64(attacks);
                break;
            case bishop:
                score[side] += bishopTable[posFlipped];
                attacks = getBishopAttackSquares(occupied, pos) & ~myPiece;
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 1;
                if (nAtksOnOuterRing)
                    nAttackers += 0.5;
                totalAtkUnits += nAtksOnInnerRing * 3 + nAtksOnOuterRing * 2;
                score[side] += popcount64(attacks) * mobilityValue[Piece::bishop];
                trace.mobilities[Piece::bishop][side] += popcount64(attacks);
                break;
            case rook:
                score[side] += rookTable[posFlipped];
                attacks = getRookAttackSquares(occupied, pos) & ~myPiece;
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 1;
                if (nAtksOnOuterRing)
                    nAttackers += 0.5;
                totalAtkUnits += nAtksOnInnerRing * 4 + nAtksOnOuterRing * 3;
                score[side] += popcount64(attacks) * mobilityValue[Piece::rook];
                trace.mobilities[Piece::rook][side] += popcount64(attacks);
                break;
            case queen:
                score[side] += queenTable[posFlipped];
                attacks = (getRookAttackSquares(occupied, pos) | getBishopAttackSquares(occupied, pos)) & ~myPiece;
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 2;
                if (nAtksOnOuterRing)
                    nAttackers += 1;
                totalAtkUnits += nAtksOnInnerRing * 5 + nAtksOnOuterRing * 5;
                score[side] += popcount64(attacks) * mobilityValue[Piece::queen];
                trace.mobilities[Piece::queen][side] += popcount64(attacks);
                break;
            case king:
                score[side] += kingTable[posFlipped];
                attacks = getKingAttackSquares(pos) & ~myPiece;
                score[side] += popcount64(attacks) * mobilityValue[Piece::king];
                trace.mobilities[Piece::king][side] += popcount64(attacks);
                break;
            default:
                break;
            }
        }
        if (nAttackers >= 4) {
            score[side] += SafetyTable[totalAtkUnits];
            trace.king_attacks[totalAtkUnits][side]++;
        }
    }

    //king safety-related
    uint64_t pawnsBBAll = pieceBB[Side::White][Piece::pawn] | pieceBB[Side::Black][Piece::pawn];
    for (int side = 0; side < 2; side++) {
        uint64_t kingAttacks = getKingAttackSquares(kingPos[side]);
        int nPawnsSurroundKing = popcount64(kingAttacks & pieceBB[side][Piece::pawn]);
        int nOpenFilesKing = 0;
        int nSemiOpenFilesKing = 0;
        if (fileKing[side] == 0) {
            nOpenFilesKing = (!(arrAfterPawn[side][kingPos[side]] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[Side::White][kingPos[side]] & pawnsBBAll));
            nSemiOpenFilesKing = (!(arrAfterPawn[side][kingPos[side]] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[Side::White][kingPos[side] + 1] & pieceBB[side][Piece::pawn]));
        }
        else if (fileKing[side] == 7) {
            nOpenFilesKing = (!(arrAfterPawn[Side::White][kingPos[side]] & pawnsBBAll)) + (!(arrAfterPawn[Side::White][kingPos[side] - 1] & pawnsBBAll));
            nSemiOpenFilesKing = (!(arrAfterPawn[Side::White][kingPos[side]] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[Side::White][kingPos[side] - 1] & pieceBB[side][Piece::pawn]));
        }
        else {
            nOpenFilesKing = (!(arrAfterPawn[Side::White][kingPos[side]] & pawnsBBAll)) + (!(arrAfterPawn[Side::White][kingPos[side] - 1] & pawnsBBAll)) + (!(arrAfterPawn[Side::White][kingPos[side] + 1] & pawnsBBAll));
            nSemiOpenFilesKing = (!(arrAfterPawn[Side::White][kingPos[side]] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[Side::White][kingPos[side] + 1] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[Side::White][kingPos[side] - 1] & pieceBB[side][Piece::pawn]));
        }
        score[side] += nPawnsSurroundKing * bonusPawnSurroundKing;
        trace.king_shield[side] += nPawnsSurroundKing;
        score[side] -= openFilesPenalty[nOpenFilesKing];
        score[side] -= semiOpenFilesPenalty[nSemiOpenFilesKing];
        trace.open_files_penalty[nOpenFilesKing][side]--;
        trace.semi_open_files_penalty[nSemiOpenFilesKing][side]--;
    }

    phase = (phase * 256 + (TOTAL_PHASE / 2)) / TOTAL_PHASE;

    //0: midgame,256:endgame
    if (sumOfWeightDist[0] > 0) {
        score[0] -= static_cast<int>((static_cast<float>(totalWeightedKingDistToOwnPawn[0]) / static_cast<float>(sumOfWeightDist[0]))) * kingPawnTropismFactor;
        trace.tropism_factor[0] = -static_cast<int>((static_cast<float>(totalWeightedKingDistToOwnPawn[0]) / static_cast<float>(sumOfWeightDist[0])));
    }
    if (sumOfWeightDist[1] > 0) {
        score[1] -= static_cast<int>((static_cast<float>(totalWeightedKingDistToOwnPawn[1]) / static_cast<float>(sumOfWeightDist[1]))) * kingPawnTropismFactor;
        trace.tropism_factor[1] = -static_cast<int>((static_cast<float>(totalWeightedKingDistToOwnPawn[0]) / static_cast<float>(sumOfWeightDist[0])));
    }

    int eval = (score[0] - score[1]) * (isWhiteTurn ? 1 : -1);
    trace.score = (((short)eval * (256 - phase) + ((eval + 0x8000) >> 16) * (phase)) / 256);
    return trace;
}

bool BitBoard::isInsufficientMaterial() {
    uint64_t occupied = pieceBB[Side::White][Piece::any] | pieceBB[Side::Black][Piece::any];
    int nPieces = popcount64(occupied);
    if (nPieces == 2) {
        return true;
    }
    if (nPieces == 3) {
        uint64_t drawPieces = pieceBB[Side::White][Piece::knight] | pieceBB[Side::White][Piece::bishop] | pieceBB[Side::Black][Piece::knight] | pieceBB[Side::Black][Piece::bishop];
        if (drawPieces) {
            return true;
        }
    }
    if (nPieces == 4) {
        const uint64_t lightSquare = 0x55AA55AA55AA55AAull;
        const uint64_t darkSquare = 0xAA55AA55AA55AA55ull;
        uint64_t drawPieces = pieceBB[Side::White][Piece::bishop] | pieceBB[Side::Black][Piece::bishop];
        if (pieceBB[Side::White][Piece::bishop] && pieceBB[Side::Black][Piece::bishop] && (popcount64(drawPieces & lightSquare) == 2 || popcount64(drawPieces & darkSquare) == 2)) {
            return 0;
        }
    }
    return false;
}



TranspositionTable* ttable = new TranspositionTable();
std::atomic<int> maxThreadDepth = 0;
std::string finalBestMove;
class ChessEngine {
protected:
    bool startWithDefaultPosition;

    std::vector<Move> moveHistory;
    std::vector<uint64_t> hashMoveHistory;

    Move killerMoves[64][2];
    int historyTable[2][7][64];
    int mainNodeCnt = 0;
    int qNodeCnt = 0;
    int cnt[100]{ 0 };
    int nMovesOutOfBook;
    std::chrono::steady_clock::time_point searchStartTime;
    int timeLimitInMs;
public:
    std::string bestMove;
    int currEval;
    BitBoard bb;
    ChessEngine();
    ChessEngine(std::string fen);
    void addMove(Move move);
    void undoMove(Move move);
    void parseFEN(std::string fen);
    Move uciToMove(std::string uci);
    void selectionSort(MoveVector& moves, int currIndex);
    void parseMoves(std::vector<std::string> moves);
    int qSearch(BitBoard& bb, int alpha, int beta, int prevEndPos, int pliesFromLeaf = 0);
    int search(BitBoard& bb, SearchStack* const ss, int depth, int maxDepth, int alpha, int beta, uint64_t zHash, int pliesFromRoot, bool isPV = false, bool canNullMove = true);
    int iterativeDeepening(int depth, int timeLeft = -1, int moveTime = -1);
    void runMultithread();
    void moveOrdering(BitBoard& bb, MoveVector& moves, int pliesFromRoot, Move& currBestMove);
    void moveOrdering(BitBoard& bb, std::vector<Move>& moves, int pliesFromRoot, Move& currBestMove);
    bool hasWon() {
        return currEval > 1000000000;
    }
    bool hasLost() {
        return currEval < -1000000000;
    }
    bool hasDrawn() {
        return bb.isInsufficientMaterial();
    }
    std::string findBestMove(int depth, int timeLeft = -1, int moveTime = -1);
    void initTranspositionTable();
};

ChessEngine::ChessEngine() {
    startWithDefaultPosition = true;
    bb = BitBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    currEval = 1;
    hashMoveHistory.push_back(bb.getCurrentPositionHash());
    nMovesOutOfBook = 0;
}

ChessEngine::ChessEngine(std::string fen) {
    startWithDefaultPosition = fen == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" ? true : false;
    bb = BitBoard(fen);
    currEval = 1;
    hashMoveHistory.push_back(bb.getCurrentPositionHash());
    nMovesOutOfBook = 0;
}

void ChessEngine::initTranspositionTable() {

}

void ChessEngine::parseMoves(std::vector<std::string> moves) {
    if (moveHistory.size() > 0 && moves.size() >= 3 && moveHistory[moveHistory.size() - 1].toUci() == moves[moves.size() - 3]) {
        Move move = uciToMove(moves[moves.size() - 2]);
        addMove(move);
        move = uciToMove(moves[moves.size() - 1]);
        addMove(move);
        std::cout << "Add moves success" << std::endl;
    }
    else {
        for (std::string uciMove : moves) {
            Move move = uciToMove(uciMove);
            addMove(move);
        }
    }
}

Move ChessEngine::uciToMove(std::string uci) {
    int start = squareToIndex[uci.substr(0, 2)];
    int end = squareToIndex[uci.substr(2, 2)];
    char promoteTo = ' ';
    std::vector<Move> legalMovesInCurrPos = bb.getLegalMoves();
    for (Move move : legalMovesInCurrPos) {
        int startPos = move.getStartPos();
        int endPos = move.getEndPos();
        if (startPos == start && endPos == end) {
            if (uci.size() == 4) {
                return move;
            }
            else {
                bool matched = false;
                switch (move.getMoveType())
                {
                case PROMOTION_QUEEN:
                    matched = uci[4] == 'q';
                    break;
                case PROMOTION_ROOK:
                    matched = uci[4] == 'r';
                    break;
                case PROMOTION_KNIGHT:
                    matched = uci[4] == 'n';
                    break;
                case PROMOTION_BISHOP:
                    matched = uci[4] == 'b';
                    break;
                case PROMOTION_QUEEN_AND_CAPTURE:
                    matched = uci[4] == 'q';
                    break;
                case PROMOTION_ROOK_AND_CAPTURE:
                    matched = uci[4] == 'r';
                    break;
                case PROMOTION_KNIGHT_AND_CAPTURE:
                    matched = uci[4] == 'n';
                    break;
                case PROMOTION_BISHOP_AND_CAPTURE:
                    matched = uci[4] == 'b';
                    break;
                default:
                    break;
                }
                if (matched) {
                    return move;
                }
            }
        }
    }

}

void ChessEngine::parseFEN(std::string fen) {
    nMovesOutOfBook = 0;
    moveHistory.clear();
    hashMoveHistory.clear();
    startWithDefaultPosition = fen == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" ? true : false;
    currEval = 1;
    bb.parseFEN(fen);
    hashMoveHistory.push_back(bb.getCurrentPositionHash());

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 7; j++) {
            for (int k = 0; k < 64; k++) {
                historyTable[i][j][k] = 0;
            }
        }
    }

}

void ChessEngine::addMove(Move move) {
    moveHistory.push_back(move);
    uint64_t lastPosHash = hashMoveHistory[hashMoveHistory.size() - 1];
    uint64_t currPosHash = bb.move(move, lastPosHash);
    //std::cout << currPosHash << std::endl;
    //if (currPosHash != bb.getCurrentPositionHash()) {
    //    std::cout << "BUGG" << std::endl;
    //}
    hashMoveHistory.push_back(currPosHash);
}

void ChessEngine::undoMove(Move move) {
    bb.undoMove(move);
    moveHistory.pop_back();
    hashMoveHistory.pop_back();
}

std::string ChessEngine::findBestMove(int depth, int timeLeft, int moveTime) {
    const bool useOp = false;
    if (moveHistory.size() < 21 && startWithDefaultPosition && useOp) {
        std::ifstream in(PATH + "data/opening.txt");
        std::string opening;
        std::vector<std::string> possibleContinuations;
        while (std::getline(in, opening)) {
            std::istringstream ss(opening);

            std::string word; // for storing each word
            int currMove = 0;
            while (ss >> word)
            {
                if (currMove == moveHistory.size()) {
                    possibleContinuations.push_back(word);
                    break;

                }
                if (moveHistory[currMove].toUci() != word) {
                    break;
                }
                currMove++;
            }
        }
        in.close();
        if (possibleContinuations.size() == 0) {
            int evaluation = iterativeDeepening(depth, timeLeft, moveTime);
            return bestMove;
        }
        else {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(0, possibleContinuations.size() - 1);
            bestMove = possibleContinuations[dist(gen)];
            return bestMove;
        }
    }
    /*
    else if (popcount64(bb.pieceBB[Side::White][Piece::any] | bb.pieceBB[Side::Black][Piece::any]) >= 3 && popcount64(bb.pieceBB[Side::White][Piece::any] | bb.pieceBB[Side::Black][Piece::any]) <= 5) {
        if (bb.isKingInCheck() && bb.getLegalMoves().size() == 0) {
            currEval = -2000000000;
            return "a1a1";
        }
        if (bb.getLegalMoves().size() == 0) {
            currEval = 0;
            return "a1a1";
        }
        //syzygy
        uint64_t currHash = bb.getCurrentPositionHash();
        bool hashRepeated = false;
        for (uint64_t pastHash : hashMoveHistory) {
            if (currHash == pastHash)
                hashRepeated = true;
        }
        TbRootMoves movesList;
        tb_probe_root_wdl(
            bb.pieceBB[Side::White][Piece::any],
            bb.pieceBB[Side::Black][Piece::any],
            bb.pieceBB[Side::White][Piece::king] | bb.pieceBB[Side::Black][Piece::king],
            bb.pieceBB[Side::White][Piece::queen] | bb.pieceBB[Side::Black][Piece::queen],
            bb.pieceBB[Side::White][Piece::rook] | bb.pieceBB[Side::Black][Piece::rook],
            bb.pieceBB[Side::White][Piece::bishop] | bb.pieceBB[Side::Black][Piece::bishop],
            bb.pieceBB[Side::White][Piece::knight] | bb.pieceBB[Side::Black][Piece::knight],
            bb.pieceBB[Side::White][Piece::pawn] | bb.pieceBB[Side::Black][Piece::pawn],
            30,
            0,
            bb.enPassantPos,
            bb.isWhiteTurn,
            true,
            &movesList
        );
        TbRootMove highestRankMove;
        int highestRank = -2000000000;
        for (int i = 0; i < movesList.size; i++) {
            int fromSq = TB_MOVE_FROM(movesList.moves[i].move);
            int toSq = TB_MOVE_TO(movesList.moves[i].move);
            int promote = TB_MOVE_PROMOTES(movesList.moves[i].move);

            std::cout << indexToSquare[fromSq] + indexToSquare[toSq] << " " << movesList.moves[i].tbRank << std::endl;
            if (movesList.moves[i].tbRank > highestRank) {
                highestRankMove = movesList.moves[i];
                highestRank = movesList.moves[i].tbRank;
            }
        }
        int fromSq = TB_MOVE_FROM(highestRankMove.move);
        int toSq = TB_MOVE_TO(highestRankMove.move);
        int promote = TB_MOVE_PROMOTES(highestRankMove.move);
        switch (promote) {
        case 0:
            bestMove = indexToSquare[fromSq] + indexToSquare[toSq];
            break;
        case 1:
            bestMove = indexToSquare[fromSq] + indexToSquare[toSq] + 'q';
            break;
        case 2:
            bestMove = indexToSquare[fromSq] + indexToSquare[toSq] + 'r';
            break;
        case 3:
            bestMove = indexToSquare[fromSq] + indexToSquare[toSq] + 'b';
            break;
        case 4:
            bestMove = indexToSquare[fromSq] + indexToSquare[toSq] + 'n';
            break;
        default:
            break;
        }
        return bestMove;
    }
    */
    else {
        //TODO

        int evaluation = iterativeDeepening(depth, timeLeft, moveTime);
        //runMultithread();
        //std::cout << "EVALUATION: " << evaluation << std::endl;
        return bestMove;
    }
}

void ChessEngine::selectionSort(MoveVector& moves, int currIndex) {
    int maxIndex = 0;
    int currMaxPriority = -10000000;
    int currPriority;
    for (int i = currIndex; i < moves.size(); i++) {
        currPriority = moves[i].getMovePriority();
        if (currPriority > currMaxPriority) {
            currMaxPriority = currPriority;
            maxIndex = i;
        }
    }
    Move tmp = moves[maxIndex];
    moves[maxIndex] = moves[currIndex];
    moves[currIndex] = tmp;
}

void ChessEngine::moveOrdering(BitBoard& bb, MoveVector& moves, int pliesFromRoot, Move& currBestMove) {
    uint32_t first32Bit = currBestMove.getFirst32Bit();
    if (!killerMoves[pliesFromRoot][0].isNullMove()) {
        for (int i = 0; i < moves.size(); i++) {
            Piece movePiece = moves[i].getMovePiece();
            Piece capturePiece = moves[i].getCapturePiece();
            Side sideToMove = moves[i].getSideToMove();
            MoveType moveType = moves[i].getMoveType();
            int startPos = moves[i].getStartPos();
            int endPos = moves[i].getEndPos();
            uint32_t move32Bit = moves[i].getFirst32Bit();
            if (move32Bit == first32Bit) {
                moves[i].setMovePriority(1000000);
                continue;
            }
            else if (moves[i] == killerMoves[pliesFromRoot][0]) {
                moves[i].setMovePriority(15000);
                continue;
            }
            else if (moves[i] == killerMoves[pliesFromRoot][1]) {
                moves[i].setMovePriority(14000);
                continue;
            }
            switch (moveType)
            {
            case NORMAL:
            {
                moves[i].setMovePriority(historyTable[sideToMove][movePiece][endPos]);
            }
            break;
            case CAPTURE:
                if (pieceValue[capturePiece] >= pieceValue[movePiece])
                    moves[i].setMovePriority(pieceValue[capturePiece] * 110 - pieceValue[movePiece]);
                else {
                    if (bb.SEE_GE(moves[i], -2))
                        moves[i].setMovePriority(pieceValue[capturePiece] * 110 - pieceValue[movePiece]);
                    else {
                        moves[i].setMovePriority((pieceValue[capturePiece] - pieceValue[movePiece]) * 100);
                    }
                }
                break;
            case EN_PASSANT:
                moves[i].setMovePriority(10000);
                break;
            case CASTLE_KINGSIDE:
                moves[i].setMovePriority(historyTable[sideToMove][movePiece][endPos] );
                break;
            case CASTLE_QUEENSIDE:
                moves[i].setMovePriority(historyTable[sideToMove][movePiece][endPos]);
                break;
            case PROMOTION_QUEEN:
                moves[i].setMovePriority(45000);
                break;
            case PROMOTION_ROOK:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_KNIGHT:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_BISHOP:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_QUEEN_AND_CAPTURE:
                moves[i].setMovePriority(45000 + pieceValue[capturePiece] * 100);
                break;
            case PROMOTION_ROOK_AND_CAPTURE:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_KNIGHT_AND_CAPTURE:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_BISHOP_AND_CAPTURE:
                moves[i].setMovePriority(-30000);
                break;
            default:
                break;
            }


        }
    }
    else {
        for (int i = 0; i < moves.size(); i++) {
            Piece movePiece = moves[i].getMovePiece();
            Piece capturePiece = moves[i].getCapturePiece();
            Side sideToMove = moves[i].getSideToMove();
            MoveType moveType = moves[i].getMoveType();
            int startPos = moves[i].getStartPos();
            int endPos = moves[i].getEndPos();
            if (moves[i].getFirst32Bit() == first32Bit) {
                moves[i].setMovePriority(1000000);
                continue;
            }
            switch (moveType)
            {
            case NORMAL:
                moves[i].setMovePriority(historyTable[sideToMove][movePiece][endPos]);
                break;
            case CAPTURE:
                if (pieceValue[capturePiece] >= pieceValue[movePiece])
                    moves[i].setMovePriority(pieceValue[capturePiece] * 110 - pieceValue[movePiece]);
                else {
                    if (bb.SEE_GE(moves[i], -2))
                        moves[i].setMovePriority(pieceValue[capturePiece] * 110 - pieceValue[movePiece]);
                    else {
                        moves[i].setMovePriority((pieceValue[capturePiece] - pieceValue[movePiece]) * 100);
                    }
                }
                break;
            case EN_PASSANT:
                moves[i].setMovePriority(10000);
                break;
            case CASTLE_KINGSIDE:
                moves[i].setMovePriority(historyTable[sideToMove][movePiece][endPos]);
                break;
            case CASTLE_QUEENSIDE:
                moves[i].setMovePriority(historyTable[sideToMove][movePiece][endPos]);
                break;
            case PROMOTION_QUEEN:
                moves[i].setMovePriority(45000);
                break;
            case PROMOTION_ROOK:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_KNIGHT:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_BISHOP:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_QUEEN_AND_CAPTURE:
                moves[i].setMovePriority(45000 + pieceValue[capturePiece] * 100);
                break;
            case PROMOTION_ROOK_AND_CAPTURE:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_KNIGHT_AND_CAPTURE:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_BISHOP_AND_CAPTURE:
                moves[i].setMovePriority(-30000);
                break;
            default:
                break;
            }



        }
    }
}
void ChessEngine::moveOrdering(BitBoard& bb, std::vector<Move>& moves, int pliesFromRoot, Move& currBestMove) {
    uint32_t first32Bit = currBestMove.getFirst32Bit();
    if (!killerMoves[pliesFromRoot][0].isNullMove()) {
        for (int i = 0; i < moves.size(); i++) {
            Piece movePiece = moves[i].getMovePiece();
            Piece capturePiece = moves[i].getCapturePiece();
            Side sideToMove = moves[i].getSideToMove();
            MoveType moveType = moves[i].getMoveType();
            int startPos = moves[i].getStartPos();
            int endPos = moves[i].getEndPos();
            uint32_t move32Bit = moves[i].getFirst32Bit();
            if (move32Bit == first32Bit) {
                moves[i].setMovePriority(1000000);
                continue;
            }
            else if (moves[i] == killerMoves[pliesFromRoot][0]) {
                moves[i].setMovePriority(15000);
                continue;
            }
            else if (moves[i] == killerMoves[pliesFromRoot][1]) {
                moves[i].setMovePriority(14000);
                continue;
            }
            switch (moveType)
            {
            case NORMAL:
            {
                moves[i].setMovePriority(historyTable[sideToMove][movePiece][endPos]);
            }
            break;
            case CAPTURE:
                if (pieceValue[capturePiece] >= pieceValue[movePiece])
                    moves[i].setMovePriority(pieceValue[capturePiece] * 110 - pieceValue[movePiece]);
                else {
                    if (bb.SEE_GE(moves[i], -2))
                        moves[i].setMovePriority(pieceValue[capturePiece] * 110 - pieceValue[movePiece]);
                    else {
                        moves[i].setMovePriority((pieceValue[capturePiece] - pieceValue[movePiece]) * 100);
                    }
                }
                break;
            case EN_PASSANT:
                moves[i].setMovePriority(10000);
                break;
            case CASTLE_KINGSIDE:
                moves[i].setMovePriority(historyTable[sideToMove][movePiece][endPos]);
                break;
            case CASTLE_QUEENSIDE:
                moves[i].setMovePriority(historyTable[sideToMove][movePiece][endPos]);
                break;
            case PROMOTION_QUEEN:
                moves[i].setMovePriority(45000);
                break;
            case PROMOTION_ROOK:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_KNIGHT:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_BISHOP:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_QUEEN_AND_CAPTURE:
                moves[i].setMovePriority(45000 + pieceValue[capturePiece] * 100);
                break;
            case PROMOTION_ROOK_AND_CAPTURE:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_KNIGHT_AND_CAPTURE:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_BISHOP_AND_CAPTURE:
                moves[i].setMovePriority(-30000);
                break;
            default:
                break;
            }


        }
    }
    else {
        for (int i = 0; i < moves.size(); i++) {
            Piece movePiece = moves[i].getMovePiece();
            Piece capturePiece = moves[i].getCapturePiece();
            Side sideToMove = moves[i].getSideToMove();
            MoveType moveType = moves[i].getMoveType();
            int startPos = moves[i].getStartPos();
            int endPos = moves[i].getEndPos();
            switch (moveType)
            {
            case NORMAL:
                moves[i].setMovePriority(historyTable[sideToMove][movePiece][endPos]);
                break;
            case CAPTURE:
                if (pieceValue[capturePiece] >= pieceValue[movePiece])
                    moves[i].setMovePriority(pieceValue[capturePiece] * 110 - pieceValue[movePiece]);
                else {
                    if (bb.SEE_GE(moves[i], -2))
                        moves[i].setMovePriority(pieceValue[capturePiece] * 110 - pieceValue[movePiece]);
                    else {
                        moves[i].setMovePriority((pieceValue[capturePiece] - pieceValue[movePiece]) * 100);
                    }
                }
                break;
            case EN_PASSANT:
                moves[i].setMovePriority(10000);
                break;
            case CASTLE_KINGSIDE:
                moves[i].setMovePriority(historyTable[sideToMove][movePiece][endPos]);
                break;
            case CASTLE_QUEENSIDE:
                moves[i].setMovePriority(historyTable[sideToMove][movePiece][endPos]);
                break;
            case PROMOTION_QUEEN:
                moves[i].setMovePriority(45000);
                break;
            case PROMOTION_ROOK:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_KNIGHT:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_BISHOP:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_QUEEN_AND_CAPTURE:
                moves[i].setMovePriority(45000 + pieceValue[capturePiece] * 100);
                break;
            case PROMOTION_ROOK_AND_CAPTURE:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_KNIGHT_AND_CAPTURE:
                moves[i].setMovePriority(-30000);
                break;
            case PROMOTION_BISHOP_AND_CAPTURE:
                moves[i].setMovePriority(-30000);
                break;
            default:
                break;
            }
            /*
            if (moves[i].getFirst32Bit() == first32Bit) {
                moves[i].setMovePriority(1000000);
            }
            */
        }
    }
}
int ChessEngine::qSearch(BitBoard& bb, int alpha, int beta, int prevEndPos, int pliesFromLeaf) {
    //qNodeCnt++;
    int bestScore = -2000000000;
    bool isKingInCheck = bb.isKingInCheck();
    int score = !isKingInCheck?bb.evaluate(alpha, beta):bestScore;

    if (score > alpha) {
        if (score >= beta)
            return score;
        alpha = score;
    }
    bestScore = score;
    /*

    */
    int futilityBase = score + futilityMarginQSearch;
    if (pliesFromLeaf > maxPlyQSearch) {
        if (isKingInCheck && bb.getLegalMoves().size() == 0) {
            return -2000000000;
        }
        return bestScore;
    }
    std::vector<Move> moves = !isKingInCheck?bb.getLegalCaptures():bb.getLegalMoves();
    if (moves.size() == 0) {
        if (isKingInCheck) {
            return -2000000000;
        }
        return bestScore;
        
    }
    Move nullMove = Move();
    moveOrdering(bb, moves, 63, nullMove);

    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) {
        return a.getMovePriority() > b.getMovePriority();
        });

    bool isEndgame = bb.isEndgame();
    for (int i = 0; i < moves.size(); i++) {
        MoveType type = moves[i].getMoveType();
        int endPos = moves[i].getEndPos();
        Side sideToMove = moves[i].getSideToMove();
        if (type == MoveType::CAPTURE && (pieceValue[moves[i].getMovePiece()] - pieceValue[moves[i].getCapturePiece()] > 100) && (bb.getPawnAttackSquares(sideToMove, endPos) & bb.pieceBB[!sideToMove][Piece::pawn]) && !isEndgame) {
            continue;
        }
        bool isPawnEndgame = !((bb.pieceBB[Side::White][Piece::any] | bb.pieceBB[Side::Black][Piece::any]) ^ (bb.pieceBB[Side::White][Piece::pawn] | bb.pieceBB[Side::Black][Piece::pawn] | bb.pieceBB[Side::White][Piece::king] | bb.pieceBB[Side::Black][Piece::king]));
        if (bestScore > -1000000000 && !isPawnEndgame &&!isKingInCheck&& (endPos != prevEndPos || prevEndPos == -1) && type != MoveType::PROMOTION_QUEEN && type != MoveType::PROMOTION_QUEEN_AND_CAPTURE) {
            if (i > 2)
                continue;
            if (pieceValue[moves[i].getCapturePiece()] * 100 + 20000 + bestScore < alpha) {
                bestScore = std::max(bestScore, pieceValue[moves[i].getCapturePiece()] * 100 + 20000 + bestScore);
                continue;
            }
            if (pliesFromLeaf < maxPlyQSearch - 1) {

                if (futilityBase <= alpha && !bb.SEE_GE(moves[i], 2))
                {
                    bestScore = std::max(bestScore, futilityBase);
                    continue;
                }
                bool negativeSEE = !bb.SEE_GE(moves[i], SEEMarginQSearch);
                if (negativeSEE) {

                    continue;
                }
                /*
                if (futilityBase > alpha && !bb.SEE_GE(moves[i], ((alpha - futilityBase) * 4))/100)
                {
                    bestScore = alpha;
                    continue;
                }
                */
            }
        }
        /*
        if ((pieceValue[moves[i].getMovePiece()] - pieceValue[moves[i].getCapturePiece()] <= 200&& type==MoveType::CAPTURE) || type==MoveType::PROMOTION_QUEEN || type==MoveType::PROMOTION_QUEEN_AND_CAPTURE) {
            bb.move(moves[i]);
            score = -qSearch(bb, -beta, -alpha);
            bb.undoMove(moves[i]);
        }
        */

        bb.move(moves[i]);
        score = -qSearch(bb, -beta, -alpha, endPos, pliesFromLeaf + 1);
        bb.undoMove(moves[i]);
        if (score > bestScore)
            bestScore = score;
        if (score >= beta) {
            //std::cout << score << std::endl;
            return score;
        }
        alpha = std::max(alpha, score);
    }
    return bestScore;
}

int ChessEngine::search(BitBoard& bb, SearchStack* const ss, int depth, int maxDepth, int alpha, int beta, uint64_t zHash, int pliesFromRoot, bool isPV, bool canNullMove) {
    //assert(depth >= 0);
    mainNodeCnt++;
    if ((mainNodeCnt & 1023) == 1023) {
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - searchStartTime);
        if (duration.count() > timeLimitInMs) {
            throw 404;
        }
    }
    bool isKingInCheck = bb.isKingInCheck();
    if (isKingInCheck && pliesFromRoot < maxDepth*2) {
        depth++;
    }
    if (depth == 0) {
        return qSearch(bb, alpha, beta, -1);
    }
    int ithMove = bb.moveNum;
    Move potentialBestMove = Move();
    TableEntry* entry = ttable->find(zHash, ithMove);
    bool isPVNode = false;
    bool isFoundInTable = entry != nullptr;
    NodeFlag expectedNode = NodeFlag::ALLNODE;
    int entryDepth = 0;
    int entryScore = 0;
    int entryEval = 0;
    if (isFoundInTable) {
        entryDepth = entry->getDepth();
        expectedNode = entry->getNodeFlag();
        entryScore = entry->getScore();
        isPVNode = isPV && expectedNode == NodeFlag::EXACT;
        potentialBestMove = entry->getBestMove();
        entryEval = entry->getEval();
        if (bb.isValidMove(potentialBestMove)) {
            if (entryDepth >= depth && pliesFromRoot > 0) {
                if (expectedNode == NodeFlag::EXACT && !isPVNode) {
                    return entryScore;
                }
                if (expectedNode == NodeFlag::CUTNODE && entryScore >= beta) {
                    return entryScore;
                }
                if (expectedNode == NodeFlag::ALLNODE && entryScore <= alpha) {
                    return entryScore;
                }
            }
        }
        else {

            potentialBestMove = Move();
            isFoundInTable = false;
        }
    }
    if (depth >= 4 && !isFoundInTable) {
        depth -= IIDReductionDepth;
    }
    if (isPVNode && (pliesFromRoot & 0b111) == 7 && pliesFromRoot<maxDepth*2) {
        depth++;
    }
    int bestScore = -2000000000;
    int beginAlpha = alpha;
    bool isEndgame = bb.isEndgame();
    int evalDepthZero = isFoundInTable ? entryEval : bb.evaluate(alpha, beta);
    ss[pliesFromRoot].eval = evalDepthZero;
    bool improving = pliesFromRoot >= 2 && evalDepthZero > ss[pliesFromRoot - 2].eval;
    int staticEval = isFoundInTable ? entryScore : evalDepthZero;
    bool isRepeated = false;
    if (staticEval >= beta && !isKingInCheck && !isPVNode && canNullMove && depth > 4 && pliesFromRoot != 0 && !bb.isKingAndPawnEndgame()) {
        Move nullMove = Move();
        ss[pliesFromRoot].move = nullMove;
        uint64_t newHash = bb.move(nullMove, zHash);
        //int R = 2+(depth/8);
        //std::cout << std::max((depth * 100 + (beta - staticEval) / 100) / 186 - 1, 0) << std::endl;
        //std::cout << beta << " "<<staticEval<<std::endl;
        //std::cout << (beta - staticEval) / 100 << std::endl;
        //cnt[std::clamp(((depth * 100 + (beta - staticEval) / 100) / 186 )- 1, 1, depth - 1)]++;
        int evalNullMove = 0;
        if (!bb.isRepeatedPosition()) {
            evalNullMove = -search(bb, ss, std::clamp(((depth * 100 + (beta - staticEval) / 100) / nullMoveDivision) - 1, 1, depth) - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, false, false);
        }
        //std::cout << std::max((depth * 100 + (beta - staticEval) / 100) / 186 - 1, 0);

        bb.undoMove(nullMove);
        if (evalNullMove >= beta) {
            //transpositionTable[zHash] = TableEntry(Move(), NodeFlag::CUTNODE, depth, evalNullMove);

            return beta;
        }
        //cnt[depth]++;
        bestScore = std::max(bestScore, evalNullMove);
    }
    /*
    //probcut
    if (depth > 4 && abs(beta) < 1000000000 && !isKingInCheck && !isPVNode && canNullMove && pliesFromRoot != 0 && !bb.isKingAndPawnEndgame()) {
        int probCutBeta = beta + 5000 * depth;
        int probCutScore = 0;
        probCutScore = search(bb, ss,depth - 3, maxDepth, probCutBeta - 1, (probCutBeta), zHash, pliesFromRoot, isPV, canNullMove);
        if (probCutScore >= probCutBeta) {
            return probCutScore;
        }

    }
    */
    NodeFlag flag = NodeFlag::ALLNODE;
    Move currBestMove = potentialBestMove;
    bool delayedMoveGen = pliesFromRoot >= 1;
    //Delay move generation: test the best move from previous iteration first
    Side sideToMove = bb.isWhiteTurn ? Side::White : Side::Black;
    if (!potentialBestMove.isNullMove() && delayedMoveGen && bb.isValidMove(potentialBestMove)) {
        MoveType moveType = potentialBestMove.getMoveType();
        Piece movePiece = potentialBestMove.getMovePiece();
        int endPos = potentialBestMove.getEndPos();
        ss[pliesFromRoot].move = potentialBestMove;
        uint64_t newHash = bb.move(potentialBestMove, zHash);
        //prefetch(&ttable->arrEntry[zHash & MASK_HASH][0]);
        int score = 0;
        
        
        if (!bb.isRepeatedPosition()) {
            score = -search(bb, ss, depth - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, isPVNode);
        }
        else
            isRepeated = true;
        bb.undoMove(potentialBestMove);
        if (score >= beta) {
            if(!isRepeated)
                ttable->set(zHash, TableEntry(zHash, potentialBestMove, NodeFlag::CUTNODE, depth, score, ithMove, evalDepthZero));
            if (moveType == MoveType::NORMAL || moveType == MoveType::CASTLE_KINGSIDE || moveType == MoveType::CASTLE_QUEENSIDE) {
                int bonus = staticEval < alpha ? (depth + 1) * (depth + 1) : depth * depth;
                historyTable[sideToMove][movePiece][endPos] += (16 * bonus - historyTable[sideToMove][movePiece][endPos] * abs(bonus) / 512);
            }

            if (moveType == MoveType::NORMAL && !(potentialBestMove == killerMoves[pliesFromRoot][0])) {
                if (potentialBestMove == killerMoves[pliesFromRoot][1]) {
                    std::swap(killerMoves[pliesFromRoot][0], killerMoves[pliesFromRoot][1]);
                }
                else {
                    killerMoves[pliesFromRoot][1] = killerMoves[pliesFromRoot][0];
                    killerMoves[pliesFromRoot][0] = potentialBestMove;
                }
            }
            return score;
        }
        if (score > bestScore) {
            bestScore = score;
        }
        if (score > alpha) {
            alpha = score;
            flag = NodeFlag::EXACT;
        }

    }



    //generate legal moves
    auto& moves = ss[pliesFromRoot].moves;
    //std::vector<Move> moves = bb.getLegalMoves();
    bb.getLegalMovesAlt(moves);
    if (moves.size() == 0) {
        if (isKingInCheck)
            return -2000000000 + pliesFromRoot;
        return 0;
    }

    //reverse futility pruning
    if (!isPVNode && !isKingInCheck && staticEval >= beta && depth <= rfpDepthLimit && canNullMove && pliesFromRoot > 0 && staticEval < 1000000000) {
        int rfpScore = staticEval - rfpMargin * depth;
        if (rfpScore >= beta) {
            //cnt[depth]++;
            //transpositionTable[zHash] = TableEntry(Move(), NodeFlag::CUTNODE, depth, rfpScore);
            return beta > -1000000000 ? (staticEval + beta) / 2 : staticEval;
        }
    }
    /*
    if (!isPVNode && !isKingInCheck && staticEval <=alpha-25000 &&alpha==beta-1 && depth < rfpDepthLimit && pliesFromRoot > 0 && staticEval < 1000000000) {
        cnt[depth]++;
        return qSearch(bb, alpha, beta, -1, 0);
    }
    */
    moveOrdering(bb, moves, pliesFromRoot, currBestMove);

    //Sort move priority
    /*
    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) {
        return a.getMovePriority() > b.getMovePriority();
        });
    */
    //moves.sortByPriority();
    //main loop
    Side opSide = sideToMove == Side::White ? Side::Black : Side::White;
    bool lmr = !isPVNode  && depth >= 3;
    int futilityVal = 0;
    int i;
    cnt[0]++;
    int LMPLimit = depth <= 4 ? (improving?quietsToCheckTable[depth]+4: quietsToCheckTable[depth]) : 999;
    for (i = 0; i < moves.size(); i++) {
        selectionSort(moves, i);
        if (moves[i] == currBestMove && delayedMoveGen)
            continue;
        isRepeated = false;
        MoveType moveType = moves[i].getMoveType();
        Piece movePiece = moves[i].getMovePiece();
        int endPos = moves[i].getEndPos();
        ss[pliesFromRoot].move = moves[i];
        uint64_t newHash = bb.move(moves[i], zHash);
        //prefetch(&ttable->arrEntry[zHash&MASK_HASH][0]);
        int score = 0;

        //Pawn near promotion extension (the move is pawn push to 6th or 7th rank and the pawn is passed)
        if (!bb.isRepeatedPosition()) {
            //Pawn near promotion extension (the move is pawn push to 6th or 7th rank and the pawn is passed)
            bool canLMR = (lmr && i > 0 && moves[i].getMovePriority() < 15001 && moveType != MoveType::CAPTURE);
            //bool givesCheck = bb.giveCheck(moves[i]);

            int lmrDepth = depth;
            if (canLMR) {
                int reducedDepth = (i * lmrMoveCntFactor + depth * lmrDepthFactor) / 1000 + (historyTable[sideToMove][movePiece][endPos] / historyReductionFactor) + (!improving&& pliesFromRoot>=2) -isKingInCheck;
                lmrDepth = depth - std::clamp(reducedDepth, 1, depth);
            }

            //pruning
            if (!isKingInCheck && !isPVNode && pliesFromRoot > 0 && bestScore > -1000000000) {
                futilityVal = staticEval + (bestScore < staticEval - 5900 ? 14100 : 7800) + futilityMargin * lmrDepth + (improving ? 8000 : 0);
                if (lmrDepth <= futilityDepthLimit && futilityVal <= alpha && moveType == MoveType::NORMAL) {
                    if (bestScore <= futilityVal && abs(bestScore) < 1000000000) {
                        bestScore = (bestScore + futilityVal) / 2;
                    }
                    //cnt[depth - 1]++;
                    bb.undoMove(moves[i]);
                    break;
                }
                if (i > LMPLimit) {
                    bb.undoMove(moves[i]);
                    break;
                }

                /*
                if (moveType == MoveType::CAPTURE) {
                    if (!bb.SEE_GE(moves[i], SEEMargin * depth)) {
                        cnt[depth]++;
                        bb.undoMove(moves[i]);
                        continue;
                    }
                }
                */


            }
            //Late move reduction, reduce move not high in priority

            if (canLMR) {

                //int reducedDepth = depth>=4?2:1;

                //cnt[reducedDepth]++;
                //int reducedDepth = (depth >= 4 ? 2 : 1)+1;
                int reducedScore = -search(bb, ss, lmrDepth, maxDepth, -alpha - 1, -alpha, newHash, pliesFromRoot + 1);

                if (reducedScore > alpha) {
                    score = -search(bb, ss, depth - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1);
                }
                else {
                    score = reducedScore;
                }
            }
            else {

                //PVS
                if (i >= 1) {
                    score = -search(bb, ss, depth - 1, maxDepth, -alpha - 1, -alpha, newHash, pliesFromRoot + 1, isPVNode);
                    if (score > alpha) {
                        score = -search(bb, ss, depth - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, isPVNode);
                    }
                }
                else {
                    score = -search(bb, ss, depth - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, isPVNode);
                }
            }
        }
        else {
            isRepeated = true;
        }
        bb.undoMove(moves[i]);
        if (pliesFromRoot == 0 && score > alpha) {
            bestMove = moves[i].toUci();
        }
        if (score >= beta) {
            
            if (moveType == MoveType::NORMAL) {
                int bonus = staticEval < alpha ? (depth + 1) * (depth + 1) : depth * depth;
                
                int toAdd = (16 * bonus - historyTable[sideToMove][movePiece][endPos] * bonus / 512);
                historyTable[sideToMove][movePiece][endPos] += toAdd;
                for (int j = 0; j < i; j++) {
                    MoveType moveType2 = moves[j].getMoveType();
                    Piece movePiece2 = moves[j].getMovePiece();
                    int endPos2 = moves[j].getEndPos();
                    if (moveType2 == MoveType::NORMAL) {
                        int toMinus = (16 * bonus + historyTable[sideToMove][movePiece2][endPos2] * bonus / 512);
                        historyTable[sideToMove][movePiece2][endPos2] -= toMinus;
                    }
                }
            }
            

            cnt[1] += i;
            if(!isRepeated)
                ttable->set(zHash, TableEntry(zHash, moves[i], NodeFlag::CUTNODE, depth, score, ithMove, evalDepthZero));
            /**/
            //If the move is not high in priority and it is not in the killer move yet,save it to the killer move array 

            if (moves[i].getMovePriority() < 15001 && !(moves[i] == killerMoves[pliesFromRoot][0])) {
                if (moves[i] == killerMoves[pliesFromRoot][1]) {
                    std::swap(killerMoves[pliesFromRoot][0], killerMoves[pliesFromRoot][1]);
                }
                else {
                    killerMoves[pliesFromRoot][1] = killerMoves[pliesFromRoot][0];
                    killerMoves[pliesFromRoot][0] = moves[i];
                }
            }
            //std::cout << score << std::endl;
            return score;
        }
        if (score > bestScore) {
            bestScore = score;
            currBestMove = moves[i];
        }
        if (score > alpha) {
            alpha = score;
            flag = NodeFlag::EXACT;

        }

    }
    cnt[1] += i;
    //save to TTable
    if ((!isFoundInTable || entryDepth <= depth)) {
        if (flag == NodeFlag::ALLNODE) {
            ttable->set(zHash, TableEntry(zHash, potentialBestMove, flag, depth, bestScore, ithMove, evalDepthZero));
        }
        else if (flag == NodeFlag::EXACT) {
            ttable->set(zHash, TableEntry(zHash, currBestMove, flag, depth, bestScore, ithMove, evalDepthZero));
        }
    }
    return bestScore;
}


int ChessEngine::iterativeDeepening(int startDepth, int timeLeft, int moveTime) {
    //time management
    int time;
    if (timeLeft == -1) {
        if (moveTime != -1)
            time = moveTime;
        else {
            time = 10600;
        }
    }
    else {
        int nMoves = std::min(nMovesOutOfBook, 10);
        double factor = 2 - static_cast<double>(nMoves) / 10.0;
        double target = timeLeft / (popcount64(bb.pieceBB[Side::Black][Piece::any] | bb.pieceBB[Side::White][Piece::any]) * 2 + 10);
        time = factor * target;

    }
    timeLimitInMs = time;
    //std::cout << "Time allocated: " << time << std::endl;
    //init variable
    int currEval = 0;
    int prevEval = 1000000000;

    uint64_t zHash = bb.getCurrentPositionHash();
    /*
    for (int i = 0; i < hashMoveHistory.size() - 1; i++) {

        if(hashMoveHistory[i]!=zHash)
            transpositionTable[hashMoveHistory[i]] = TableEntry(true);
    }
    */
    //std::cout << zHash << std::endl;
    std::string lastBestMove;
    searchStartTime = std::chrono::steady_clock::now();
    SearchStack ss[128];
    const bool isEnableAspiration = true;
    for (int i = startDepth; i <= 63; i++) {

        //Aspiration window
        if (prevEval != 1000000000 && isEnableAspiration) {
            int beginWindow = 4000;
            int upperWindow = beginWindow;
            int lowerWindow = beginWindow;
            int power = 1;

            while (true) {
                int lowerBound = prevEval - lowerWindow;
                int upperBound = prevEval + upperWindow;
                //std::cout << "Start aspiration with window: " << prevEval - lowerWindow << " " << prevEval + upperWindow << std::endl;
                try
                {
                    currEval = search(bb, ss, i, i, prevEval - lowerWindow, prevEval + upperWindow, zHash, 0, true);
                }
                catch (int x)
                {
                    if (x == 404) {
                        bestMove = lastBestMove;
                        break;
                    }
                }
                //std::cout << "Eval: " << currEval << std::endl;
                prevEval = currEval;

                if (currEval >= upperBound) {
                    upperWindow *= 2;
                }

                else if (currEval <= lowerBound) {
                    lowerWindow *= 2;
                }
                else {
                    lastBestMove = bestMove;

                    break;
                }
                power++;

                //if the windows fail the 4th time,cancel it and do a full window search
                if (power >= 1) {
                    upperBound = 2000000000;
                    lowerBound = -2000000000;
                }

            }
        }
        //if this is the first iteration,do a full window search
        else {
            currEval = search(bb, ss, i, i, -2000000000, 2000000000, zHash, 0, true);
            lastBestMove = bestMove;
        }

        //currEval = search(bb, ss,i, i, -2000000000, 2000000000, zHash, 0, true);
        //std::cout << "Evaluation engine 1 at depth " << i << " " << ": " << currEval << std::endl;
        //std::cout << "Bestmove engine 1 at depth " << i << " " << ": " << bestMove << std::endl;
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 7; k++) {
                for (int l = 0; l < 64; l++) {
                    //std::cout << historyTable[i][j][k] << std::endl;
                    historyTable[j][k][l] /= 8;
                }
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - searchStartTime);
        double timeInSec = static_cast<double>(duration.count() + 1) / 1000.0f;
        double nps = (double)mainNodeCnt / (timeInSec);
        if (maxThreadDepth < i) {
            maxThreadDepth = i;
            finalBestMove = bestMove;
            std::cout << "info score cp " << currEval / 100 << " depth " << i << " time " << duration.count() << " nps " << (int)nps << " currmove " << bestMove << std::endl;
        }

        if (duration.count() > time)
            break;
        prevEval = currEval;

    }
    //std::cout << "N move repetition table: " << bb.repetitionTable.a << std::endl;
    //std::cout << "N undo repetition table: " << bb.repetitionTable.u << std::endl;
    //std::cout << "Bestmove engine 1 :" << bestMove << std::endl;
    this->currEval = currEval;
    for (int i = 0; i < 32; i++) {
        killerMoves[i][0] = Move();
        killerMoves[i][1] = Move();
    }

    for (int i = 0; i < 16; i++) {
        std::cout << "Depth " << i << " Node pruned: " << cnt[i] << std::endl;
    }
    std::cout << "Total node evaluated: "<<bb.cntNode << " Pawn hash found: " << bb.cntNode2 << std::endl;
    /*
    std::map<int, std::string> pieceMap = {
        {0,"any"},
        {1,"pawn"},
        {2,"knight"},
        {3,"bishop"},
        {4,"rook"},
        {5,"queen"},
        {6,"king"}
    };
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 7; j++) {
            for (int k = 0; k < 64; k++) {
                if(historyTable[i][j][k]!=0)
                    std::cout << historyTable[i][j][k] <<" "<<(i==0?"White ":"Black ")<<pieceMap[j]<<" "<<indexToSquare[k]<< std::endl;
                historyTable[i][j][k] = 0;
            }
        }
    }
    */
    nMovesOutOfBook++;
    for (int i = 0; i < 20; i++) {
        //std::cout << "Reduction " << i << " equal " << cnt[i] << std::endl;
    }
    //retrieve principal variation
    /*
    std::cout << std::endl << "Principal variation:" << std::endl;
    std::vector<TableEntry> v;
    int maxVariation = 0;
    while (true) {
        maxVariation++;
        if (maxVariation >= 40)
            break;
        if (!ttable->find(zHash))
            break;
        TableEntry entry = *(ttable->find(zHash));
        v.push_back(entry);
        std::cout << entry.getBestMove().toUci() << " " << static_cast<int>(entry.getNodeFlag()) << " " << entry.getScore() << std::endl;
        Move best = entry.getBestMove();
        zHash=bb.move(best, zHash);
    }
    for (int i = v.size() - 1; i > -1; i--) {
        Move m = v[i].getBestMove();
        bb.undoMove(m);
    }
    */
    //std::cout << "Main node count :" << mainNodeCnt << std::endl;
    //std::cout << "Q node count: " << qNodeCnt << std::endl;
    qNodeCnt = 0;
    mainNodeCnt = 0;
    //std::cout << cnt[0] << std::endl;
    /*
    std::cout << "Main node count :" << mainNodeCnt << std::endl;
    std::cout << "Q node count: " << qNodeCnt << std::endl;
    std::cout << "Number of collisions: " << ttable->nCollisions << std::endl;
    int fill = 0;
    int fillFull = 0;
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (ttable->arrEntry[i][0].zHash != 0)
            fill++;
        if (ttable->arrEntry[i][1].zHash != 0)
            fill++;
        if (ttable->arrEntry[i][0].zHash != 0 && ttable->arrEntry[i][1].zHash != 0 && ttable->arrEntry[i][2].zHash != 0)
            fillFull++;
    }
    std::cout << "Occupied: " << fill << std::endl;
    std::cout << "Fully Occupied: " << fillFull << std::endl;
    */
    return currEval;
}
void ChessEngine::runMultithread() {
    std::vector<std::thread> threads(4);
    for (int i = 0; i < 4; i++) {
        threads[i] = std::thread(&ChessEngine::iterativeDeepening, this, 64, -1, 7500);
    }
    for (int i = 0; i < 4; i++) {
        threads[i].join();
    }

}


class MultithreadChessEngine {
private:
    std::vector<std::thread>threads;
    ChessEngine* engines;
    int nThreads;
public:
    std::string bestMove;
    bool isWhiteTurn;
    MultithreadChessEngine(int nThreads) {
        threads = std::vector<std::thread>(nThreads);
        engines = new ChessEngine[nThreads];
        this->nThreads = nThreads;
    }
    void parseFEN(std::string fen) {
        for (int i = 0; i < nThreads; i++) {
            engines[i].parseFEN(fen);
        }
        isWhiteTurn = engines[0].bb.isWhiteTurn;
    }

    void parseMoves(std::vector<std::string> moves) {
        for (int i = 0; i < nThreads; i++) {
            engines[i].parseMoves(moves);
        }
        isWhiteTurn = engines[0].bb.isWhiteTurn;
    }

    void runMultithread() {

    }

    std::string findBestMove(int depth, int timeLeft = -1, int moveTime = -1) {
        for (int i = 0; i < nThreads; i++) {
            threads[i] = std::thread(&ChessEngine::iterativeDeepening, engines[i], i < 2 ? 1 : 2, timeLeft, moveTime);
        }
        for (int i = 0; i < nThreads; i++) {
            threads[i].join();
        }
        int currEval = -2e9;
        //std::cout<<"Final best: "<<bestMove<<std::endl;
        maxThreadDepth = 0;
        bestMove = finalBestMove;
        return bestMove;
    }
};




void runTest(BitBoard& bb, std::string fen, int highestDepth, int expectedResult) {
    bb.parseFEN(fen);
    auto startTime = std::chrono::steady_clock::now();
    int res = bb.perft(highestDepth, highestDepth);
    auto endTime = std::chrono::steady_clock::now();
    // Calculate the duration
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    if (res != expectedResult) {
        std::cout << ("Perft failed for fen string " + fen + " depth " + std::to_string(highestDepth) + " : Expected: " + std::to_string(expectedResult) +
            " Actual: " + std::to_string(res) + " Time: " + std::to_string(duration.count()) + " milliseconds") << std::endl;
    }
    else {
        std::cout << "Perft success for fen string " + fen + " depth " + std::to_string(highestDepth) + " : Expected: " + std::to_string(expectedResult) +
            " Actual: " + std::to_string(res) + " Time: " + std::to_string(duration.count()) + " milliseconds" << std::endl;
    }
}

#ifdef NDEBUG
void testMoveGen() {
    BitBoard bb = BitBoard();
    auto startTime = std::chrono::steady_clock::now();
    runTest(bb, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324);
    std::cout << bb.cntNode << std::endl;
    runTest(bb, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 5, 193690690);
    runTest(bb, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 7, 178633661);
    runTest(bb, "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 5, 15833292);
    runTest(bb, "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 89941194);
    runTest(bb, "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5, 164075551);
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "Run full test costs: " << std::to_string(duration.count()) << std::endl;

}

#else
void testMoveGen() {
    BitBoard bb = BitBoard();
    auto startTime = std::chrono::steady_clock::now();
    runTest(bb, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609);
    runTest(bb, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 4, 4085603);
    runTest(bb, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 5, 674624);
    runTest(bb, "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333);
    runTest(bb, "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487);
    runTest(bb, "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594);
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "Run full test costs: " << std::to_string(duration.count()) << std::endl;

}
#endif

void testEvaluation() {
    std::string fen;
    std::getline(std::cin, fen);
    BitBoard bb = BitBoard(fen);
    auto startTime = std::chrono::steady_clock::now();
    int eval = bb.evaluate(-2000000000, 2000000000);
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "Run full test costs: " << std::to_string(duration.count()) << std::endl;
    std::cout << eval << std::endl;
}
void testSearch() {
    ChessEngine engine = ChessEngine();
    std::string fen;
    std::getline(std::cin, fen);
    std::cout << fen;
    engine.parseFEN(fen);
    auto startTime = std::chrono::steady_clock::now();
    std::cout << engine.findBestMove(1) << std::endl;
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "Run search test costs: " << std::to_string(duration.count()) << std::endl;
}
enum class command
{
    search_moves,
    ponder,
    white_time,
    black_time,
    white_increment,
    black_increment,
    moves_to_go,
    depth,
    nodes,
    mate,
    move_time,
    infinite
};
void uciCommunication()
{
    MultithreadChessEngine engine = MultithreadChessEngine(1);
    const std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::string line;
    auto   running = true;
    //engine.parseFEN("r1bq1rk1/p4p2/1pn1p1p1/3pP1N1/3P3Q/2PB4/P4PKP/4RR2 b - - 1 18");
    //std::cout << engine.findBestMove(6) << std::endl;
    //std::ofstream out("log_engine.txt");
    while (running && getline(std::cin, line))
    {
        //out << line << std::endl;
        line += " ";
        std::istringstream iss(line);
        std::string        token;
        iss >> std::skipws >> token;



        if (token == "uci")
        {
            std::cout << "option name maxPlyQSearch type spin default 6 min 1 max 12" << std::endl;
            std::cout << "option name futilityMarginQSearch type spin default 22000 min 5000 max 50000" << std::endl;
            std::cout << "option name SEEMarginQSearch type spin default -34 min -400 max 400" << std::endl;
            std::cout << "option name IIDReductionDepth type spin default 2 min 0 max 4" << std::endl;
            std::cout << "option name nullMoveDivision type spin default 186 min 120 max 320" << std::endl;
            std::cout << "option name rfpDepthLimit type spin default 4 min 1 max 10" << std::endl;
            std::cout << "option name rfpMargin type spin default 6000 min 0 max 40000" << std::endl;
            std::cout << "option name lmrMoveCntFactor type spin default 93 min 1 max 400" << std::endl;
            std::cout << "option name lmrDepthFactor type spin default 144 min 1 max 500" << std::endl;
            std::cout << "option name historyReductionFactor type spin default -3500 min -8250 max 8250" << std::endl;
            std::cout << "option name futilityMargin type spin default 12500 min 1 max 90000" << std::endl;
            std::cout << "option name futilityDepthLimit type spin default 6 min 0 max 12" << std::endl;
            std::cout << "option name SEEMargin type spin default -101 min -500 max 500" << std::endl;
            std::cout << "uciok" << std::endl;
        }
        else if (token == "isready")
        {

            std::cout << "readyok" << std::endl;
        }
        else if (token == "ucinewgame")
        {

        }
        else if (token == "position")
        {

            std::string              fen;
            std::vector<std::string> moves;
            iss >> token;
            if (token == "startpos")
            {
                fen = start_fen;
                iss >> token;
            }
            else if (token == "fen") {
                while (iss >> token && token != "moves")
                    fen += token + " ";
            }
            else
                continue;
            while (iss >> token)
                moves.push_back(token);

            engine.parseFEN(fen);
            engine.parseMoves(moves);


        }
        else if (token == "go")
        {
            std::map<command, std::string> commands;
            while (iss >> token)
                if (token == "searchmoves")
                    while (iss >> token)
                        commands[command::search_moves] += std::string(" ", commands[command::search_moves].empty() ? 0 : 1) + token;
                else if (token == "ponder")
                    commands[command::ponder];
                else if (token == "wtime")
                    iss >> commands[command::white_time];
                else if (token == "btime")
                    iss >> commands[command::black_time];
                else if (token == "winc")
                    iss >> commands[command::white_increment];
                else if (token == "binc")
                    iss >> commands[command::black_increment];
                else if (token == "movestogo")
                    iss >> commands[command::moves_to_go];
                else if (token == "depth")
                    iss >> commands[command::depth];
                else if (token == "nodes")
                    iss >> commands[command::nodes];
                else if (token == "mate")
                    iss >> commands[command::mate];
                else if (token == "movetime") {
                    iss >> commands[command::move_time];

                }
                else if (token == "infinite") {
                    commands[command::infinite];
                }
            if (commands.find(command::move_time) != commands.end()) {
                engine.findBestMove(1, -1, std::stoi(commands[command::move_time]));
            }
            else {
                if (engine.isWhiteTurn) {
                    engine.findBestMove(1, std::stoi(commands[command::white_time]));
                }
                else {
                    engine.findBestMove(1, std::stoi(commands[command::black_time]));
                }
            }
            std::cout << "bestmove " << engine.bestMove << std::endl;

        }
        else if (token == "stop")
        {
        }
        else if (token == "ponderhit")
        {
        }
        else if (token == "quit")
        {
            running = false;
        }
        else if (token == "setoption") {
            /*
            std::string tmp;
            std::string optionName;
            std::string value;
            iss >> tmp;
            iss >> optionName;
            iss >> tmp;
            iss >> value;
            if (optionName == "maxPlyQSearch") {
                maxPlyQSearch = std::stoi(value);
            }
            else if (optionName == "futilityMarginQSearch") {
                futilityMarginQSearch = std::stoi(value);
            }
            else if (optionName == "SEEMarginQSearch") {
                SEEMarginQSearch = std::stoi(value);
            }
            else if (optionName == "IIDReductionDepth") {
                IIDReductionDepth = std::stoi(value);
            }
            else if (optionName == "nullMoveDivision") {
                nullMoveDivision = std::stoi(value);
            }
            else if (optionName == "rfpDepthLimit") {
                rfpDepthLimit = std::stoi(value);
            }
            else if (optionName == "rfpMargin") {
                rfpMargin = std::stoi(value);
            }
            else if (optionName == "historyReductionFactor") {
                historyReductionFactor = std::stoi(value);
            }
            else if (optionName == "lmrMoveCntFactor") {
                lmrMoveCntFactor = std::stoi(value);
            }
            else if (optionName == "lmrDepthFactor") {
                lmrDepthFactor = std::stoi(value);
            }
            else if (optionName == "futilityMargin") {
                futilityMargin = std::stoi(value);
            }
            else if (optionName == "futilityDepthLimit") {
                futilityDepthLimit = std::stoi(value);
            }
            else if (optionName == "SEEMargin") {
                SEEMargin = std::stoi(value);
            }
        */
        }
        else
        {
            //std::cout << "Unrecognized command: " << line << std::endl;

        }

    }

}

void testEPD() {

    std::string line;
    std::string ans;
    std::string fen;
    std::ifstream epd("test.epd");
    std::ofstream err("fail.txt");
    int success = 0;
    int fail = 0;
    int progress = 0;
    ChessEngine engine = ChessEngine();

    while (std::getline(epd, line)) {
        std::cout << progress << '\r';
        std::istringstream ss(line);
        std::getline(ss, fen, ';');
        std::getline(ss, ans, ';');
        engine.parseFEN(fen);
        std::string engineBestMove = engine.findBestMove(1);
        if (ans == engineBestMove) {
            success++;
        }
        else {
            fail++;
            err << fen << ";" << ans << ";" << engineBestMove << std::endl;
        }
        progress++;
    }
    std::cout << "Success: " << success << "/" << success + fail << std::endl;
    std::cout << "Failed: " << fail << "/" << success + fail << std::endl;

}

void testSEE() {
    std::string fen;
    std::getline(std::cin, fen);
    BitBoard bb = BitBoard(fen);
    auto v = bb.getLegalCaptures();
    for (Move m : v) {
        if (m.getMoveType() == MoveType::CAPTURE) {
            bool evalSEE = bb.SEE_GE(m, 2);
            if (evalSEE) {
                std::cout << m.toUci() << std::endl;
            }
        }
    }
}


int main(int argn, char** argv) {
    //tb_init((PATH + "syzygy").c_str());
    //BoardUI boardUI = BoardUI("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    //boardUI.run();
    //boardUI.matchMaking(5);
    //testEvaluation();
    //testSearch();
    //testSEE();
    //testEPD();
    //std::cout << popcount64(0);
    //testMoveGen();
    //testMoveSorting();
    //std::cout << BitScanForward64(0b11010000) << std::endl;
    uciCommunication();
    //MultithreadChessEngine engine=MultithreadChessEngine(4);
    //engine.parseFEN("r1b1k2r/pp1nbp1p/1q3p2/4p3/3p1B2/2PB1N2/PP2QPPP/R4RK1 w kq - 2 12");
    //engine.findBestMove(64);
    //getchar();
    return 0;
}