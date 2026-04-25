#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <cassert>
#include <algorithm>
#include "utils.h"
const int CHESS_MAX_PRIORITY = 1e6;
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
    uint32_t encodedMove;
public:
    Move() {
        encodedMove = 0;
    }
    Move(uint32_t encoded) {
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
        return (encodedMove & 0b11111111111111111111111) == (m.encodedMove & 0b11111111111111111111111);
    }
    const bool operator != (const Move& m) {
        return (encodedMove & 0b11111111111111111111111) != (m.encodedMove & 0b11111111111111111111111);
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
        encodedMove &= ~(0b111111ULL); // Clear the previous value
        encodedMove |= (startPos);
    }

    void setEndPos(int endPos) {
        encodedMove &= ~(0b111111ULL << 6); // Clear the previous value
        encodedMove |= (static_cast<uint32_t>(endPos) << 6);
    }

    void setMovePiece(Piece movePiece) {
        encodedMove &= ~(0b111ULL << 12); // Clear the previous value
        encodedMove |= (static_cast<uint32_t>(movePiece) << 12);
    }

    void setCapturePiece(Piece capturePiece) {
        encodedMove &= ~(0b111LL << 15); // Clear the previous value
        encodedMove |= (static_cast<uint32_t>(capturePiece) << 15);
    }

    void setSideToMove(Side sideToMove) {
        encodedMove &= ~(0x1ULL << 18); // Clear the previous value
        encodedMove |= (static_cast<uint32_t>(sideToMove) << 18);
    }

    void setMoveType(MoveType moveType) {
        encodedMove &= ~(0xFULL << 19); // Clear the previous value
        encodedMove |= (static_cast<uint32_t>(moveType) << 19);
    }

    void setCastleRightBefore(bool r1, bool r2) {
        encodedMove &= ~(0x3ULL << 23); // Clear the previous value
        encodedMove |= (static_cast<uint32_t>(r1) << 23) | (static_cast<uint32_t>(r2) << 24);
    }

    void setEnpassantPosBefore(int pos) {
        encodedMove &= ~(0b111111ULL << 25); // Clear the previous value
        encodedMove |= (static_cast<uint32_t>(pos) << 25);
    }

    uint32_t getFirst32Bit() {
        return encodedMove;
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
    int movePriority[218];
};

enum class NodeFlag :int {
    EXACT,
    CUTNODE,//beta cutoff
    ALLNODE//alpha cannot raise
};
