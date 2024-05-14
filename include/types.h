#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <cassert>
#include <algorithm>
std::map<std::string, int> squareToIndex = {
    {"a1", 0}, {"b1", 1}, {"c1", 2}, {"d1", 3},
    {"e1", 4}, {"f1", 5}, {"g1", 6}, {"h1", 7},
    {"a2", 8}, {"b2", 9}, {"c2", 10}, {"d2", 11},
    {"e2", 12}, {"f2", 13}, {"g2", 14}, {"h2", 15},
    {"a3", 16}, {"b3", 17}, {"c3", 18}, {"d3", 19},
    {"e3", 20}, {"f3", 21}, {"g3", 22}, {"h3", 23},
    {"a4", 24}, {"b4", 25}, {"c4", 26}, {"d4", 27},
    {"e4", 28}, {"f4", 29}, {"g4", 30}, {"h4", 31},
    {"a5", 32}, {"b5", 33}, {"c5", 34}, {"d5", 35},
    {"e5", 36}, {"f5", 37}, {"g5", 38}, {"h5", 39},
    {"a6", 40}, {"b6", 41}, {"c6", 42}, {"d6", 43},
    {"e6", 44}, {"f6", 45}, {"g6", 46}, {"h6", 47},
    {"a7", 48}, {"b7", 49}, {"c7", 50}, {"d7", 51},
    {"e7", 52}, {"f7", 53}, {"g7", 54}, {"h7", 55},
    {"a8", 56}, {"b8", 57}, {"c8", 58}, {"d8", 59},
    {"e8", 60}, {"f8", 61}, {"g8", 62}, {"h8", 63}
};
std::map<int, std::string> indexToSquare = {
    {0, "a1"}, {1, "b1"}, {2, "c1"}, {3, "d1"},
    {4, "e1"}, {5, "f1"}, {6, "g1"}, {7, "h1"},
    {8, "a2"}, {9, "b2"}, {10, "c2"}, {11, "d2"},
    {12, "e2"}, {13, "f2"}, {14, "g2"}, {15, "h2"},
    {16, "a3"}, {17, "b3"}, {18, "c3"}, {19, "d3"},
    {20, "e3"}, {21, "f3"}, {22, "g3"}, {23, "h3"},
    {24, "a4"}, {25, "b4"}, {26, "c4"}, {27, "d4"},
    {28, "e4"}, {29, "f4"}, {30, "g4"}, {31, "h4"},
    {32, "a5"}, {33, "b5"}, {34, "c5"}, {35, "d5"},
    {36, "e5"}, {37, "f5"}, {38, "g5"}, {39, "h5"},
    {40, "a6"}, {41, "b6"}, {42, "c6"}, {43, "d6"},
    {44, "e6"}, {45, "f6"}, {46, "g6"}, {47, "h6"},
    {48, "a7"}, {49, "b7"}, {50, "c7"}, {51, "d7"},
    {52, "e7"}, {53, "f7"}, {54, "g7"}, {55, "h7"},
    {56, "a8"}, {57, "b8"}, {58, "c8"}, {59, "d8"},
    {60, "e8"}, {61, "f8"}, {62, "g8"}, {63, "h8"}
};
const int MAX_PRIORITY = 1e6;
enum MoveType {
    NORMAL,
    CAPTURE,
    EN_PASSANT,
    CASTLE_KINGSIDE,
    CASTLE_QUEENSIDE,
    PROMOTION_QUEEN,
    PROMOTION_ROOK,
    PROMOTION_KNIGHT,
    PROMOTION_BISHOP,
    PROMOTION_QUEEN_AND_CAPTURE,
    PROMOTION_ROOK_AND_CAPTURE,
    PROMOTION_KNIGHT_AND_CAPTURE,
    PROMOTION_BISHOP_AND_CAPTURE
};
enum Side :int {
    White,
    Black
};
enum Piece
{
    any,
    pawn,
    knight,
    bishop,
    rook,
    queen,
    king
};

enum Direction {
    Horizontal,
    Vertical,
    DiagonalMain,
    DiagonalCounter,
    None
};
class Move {
private:
    uint64_t encodedMove;
public:
    Move() {
        encodedMove = 0;
    }
    Move(uint64_t encoded) {
        encodedMove = encoded;
    }
    Move(int startPos, int endPos, MoveType moveType, Side sideToMove, Piece movePiece, Piece capturePiece = Piece::any) {
        encodedMove = 0;
        setStartPos(startPos);//6 bit
        setEndPos(endPos);//6 bit
        setMovePiece(movePiece);//3 bit
        setCapturePiece(capturePiece);//3 bit
        setSideToMove(sideToMove);//1 bit
        setMoveType(moveType);//4 bit
    }
    const bool operator == (const Move& m) {
        return (encodedMove & ~(0xffffffff00000000ull)) == (m.encodedMove & ~(0xffffffff00000000ull));
    }
    const bool operator != (const Move& m) {
        return (encodedMove & ~(0xffffffff00000000ull)) != (m.encodedMove & ~(0xffffffff00000000ull));
    }
    bool isNullMove() {
        return getStartPos() == getEndPos();
    }
    std::string toUci() {
        std::string ans = indexToSquare[getStartPos()] + indexToSquare[getEndPos()];
        switch (getMoveType())
        {
        case NORMAL:
            break;
        case CAPTURE:
            break;
        case EN_PASSANT:
            break;
        case CASTLE_KINGSIDE:
            break;
        case CASTLE_QUEENSIDE:
            break;
        case PROMOTION_QUEEN:
            ans += 'q';
            break;
        case PROMOTION_ROOK:
            ans += 'r';
            break;
        case PROMOTION_KNIGHT:
            ans += 'n';
            break;
        case PROMOTION_BISHOP:
            ans += 'b';
            break;
        case PROMOTION_QUEEN_AND_CAPTURE:
            ans += 'q';
            break;
        case PROMOTION_ROOK_AND_CAPTURE:
            ans += 'r';
            break;
        case PROMOTION_KNIGHT_AND_CAPTURE:
            ans += 'n';
            break;
        case PROMOTION_BISHOP_AND_CAPTURE:
            ans += 'b';
            break;
        default:
            break;
        }
        return ans;
    }
    // Setters
    void setStartPos(int startPos) {
        //encodedMove &= ~(0b111111ULL); // Clear the previous value
        encodedMove |= (startPos);
    }

    void setEndPos(int endPos) {
        //encodedMove &= ~(0b111111ULL << 6); // Clear the previous value
        encodedMove |= (static_cast<uint64_t>(endPos) << 6);
    }

    void setMovePiece(Piece movePiece) {
        //encodedMove &= ~(0b111ULL << 12); // Clear the previous value
        encodedMove |= (static_cast<uint64_t>(movePiece) << 12);
    }

    void setCapturePiece(Piece capturePiece) {
        //encodedMove &= ~(0b111LL << 15); // Clear the previous value
        encodedMove |= (static_cast<uint64_t>(capturePiece) << 15);
    }

    void setSideToMove(Side sideToMove) {
        //encodedMove &= ~(0x1ULL << 18); // Clear the previous value
        encodedMove |= (static_cast<uint64_t>(sideToMove) << 18);
    }

    void setMoveType(MoveType moveType) {
        //encodedMove &= ~(0xFULL << 19); // Clear the previous value
        encodedMove |= (static_cast<uint64_t>(moveType) << 19);
    }

    void setCastleRightBefore(bool r1, bool r2) {
        //encodedMove &= ~(0x3ULL << 23); // Clear the previous value
        encodedMove |= (static_cast<uint64_t>(r1) << 23) | (static_cast<uint64_t>(r2) << 24);
    }

    void setEnpassantPosBefore(int pos) {
        //encodedMove &= ~(0b111111ULL << 25); // Clear the previous value
        encodedMove |= (static_cast<uint64_t>(pos) << 25);
    }

    void setMovePriority(int movePriority) {
        uint64_t scoreConverted = static_cast<uint64_t>(movePriority + MAX_PRIORITY);
        encodedMove &= ~(0xFFFFFFFFULL << 32); // Clear the previous value
        encodedMove |= scoreConverted << 32;
    }

    uint32_t getFirst32Bit() {
        return encodedMove & 0xFFFFFFFFULL;
    }

    // Getters
    int getStartPos() const {
        return static_cast<int>((encodedMove) & 0b111111);
    }

    int getEndPos() const {
        return static_cast<int>((encodedMove >> 6) & 0b111111);
    }

    Piece getMovePiece() const {
        return static_cast<Piece>((encodedMove >> 12) & 0b111);
    }

    Piece getCapturePiece() const {
        return static_cast<Piece>((encodedMove >> 15) & 0b111);
    }

    Side getSideToMove() const {
        return static_cast<Side>((encodedMove >> 18) & 0x1);
    }

    MoveType getMoveType() const {
        return static_cast<MoveType>((encodedMove >> 19) & 0xF);
    }

    bool getCastleRightBefore(int idx) const {
        return static_cast<bool>((encodedMove >> (23 + idx)) & 0x1);
    }
    int getEnpassantPosBefore() const {
        return static_cast<int>((encodedMove >> 25) & 0b111111);
    }
    int getMovePriority() const {
        return static_cast<int>(((encodedMove >> 32) & 0xFFFFFFFFull) - MAX_PRIORITY);
    }
};

class MoveVector {
private:
    Move arr[218];
    int currPos;
public:

    MoveVector() {
        currPos = 0;
    }
    int size() {
        return currPos;
    }
    void emplace_back(Move move) {
        arr[currPos] = move;
        currPos++;
    }
    void reset() {
        currPos = 0;
    }
    void sortByPriority() {
        std::sort(arr,arr+currPos, [](const Move& a, const Move& b) {
            return a.getMovePriority() > b.getMovePriority();
            });
    }
    Move operator [] (int i) const { return arr[i]; }
    Move& operator [] (int i) { return arr[i]; }
};
class PlyInfo{
public:
    
};

class SearchStack {
public:
    int eval;
    Move move;
    MoveVector moves;
};

enum class NodeFlag :int {
    EXACT,
    CUTNODE,//beta cutoff
    ALLNODE//alpha cannot raise
};