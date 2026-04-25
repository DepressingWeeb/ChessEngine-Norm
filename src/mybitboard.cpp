#pragma once
#include "bitboard.h"
#include "constant_bitboard.h"
#include "constant_hash.h"
#include <sstream>
#include "../Stockfish/src/types.h"
#include "../Stockfish/src/position.h"
#include "../Stockfish/src/evaluate.h"
#include "../Stockfish/src/nnue/network.h"
#include "../Stockfish/src/nnue/nnue_accumulator.h"
extern Stockfish::Eval::NNUE::Networks* g_nnue_networks;
extern thread_local Stockfish::Eval::NNUE::AccumulatorCaches* g_nnue_caches;
thread_local Stockfish::Eval::NNUE::AccumulatorStack g_nnue_acc;

BitBoard::BitBoard() {
    initConstant();
    initMagic();
    parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    memset(pawnHashTable, 0, sizeof(pawnHashTable));
}
std::pair<int, int> BitBoard::posToRowCol(int pos) {
    return std::make_pair(pos / 8, pos % 8);
}

int BitBoard::rowColToPos(int row, int col) {
    return row * 8 + col;
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
    memset(pawnHashTable, 0, sizeof(pawnHashTable));
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
    for (int i = 0; i < v[0].size(); i++) {
        Side pieceSide = islower(v[0][i]) ? Side::Black : Side::White;
        if (v[0][i] == '/') {
            currPos -= 16;
            continue;
        }
        char lowercasePiece = tolower(v[0][i]);
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
            break;
        default:
            currPos += (v[0][i] - '1');
            break;
        }
        currPos++;
    }
    isWhiteTurn = v[1][0] == 'w';
    castingRight[Side::White][0] = false;
    castingRight[Side::White][1] = false;
    castingRight[Side::Black][0] = false;
    castingRight[Side::Black][1] = false;
    for (int i = 0; i < v[2].size(); i++) {
        switch (v[2][i])
        {
        case 'Q':
            castingRight[Side::White][1] = true;
            break;
        case 'K':
            castingRight[Side::White][0] = true;
            break;
        case 'q':
            castingRight[Side::Black][1] = true;
            break;
        case 'k':
            castingRight[Side::Black][0] = true;
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
            maskNeighbor = getMaskOfFile(pawnFile + 1);
        }
        else if (pawnFile == 7) {
            mask = getMaskOfFile(pawnFile) | getMaskOfFile(pawnFile - 1);
            maskNeighbor = getMaskOfFile(pawnFile - 1);
        }
        else {
            mask = getMaskOfFile(pawnFile) | getMaskOfFile(pawnFile + 1) | getMaskOfFile(pawnFile - 1);
            maskNeighbor = getMaskOfFile(pawnFile + 1) | getMaskOfFile(pawnFile - 1);
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
        int pawnPos = my_BitScanForward64(pawnBB);
        uint64_t pawnAttacks = getPawnAttackSquares(side, pawnPos);
        allAttackSquares |= pawnAttacks;
        pawnBB ^= (1ull << pawnPos);
    }
    uint64_t knightBB = pieceBB[side][Piece::knight];
    while (knightBB) {
        int knightPos = my_BitScanForward64(knightBB);
        uint64_t knightAttacks = getKnightAttackSquares(knightPos);
        allAttackSquares |= knightAttacks;
        knightBB ^= (1ull << knightPos);
    }
    uint64_t bishopQueenBB = pieceBB[side][Piece::bishop] | pieceBB[side][Piece::queen];
    while (bishopQueenBB) {
        int bishopPos = my_BitScanForward64(bishopQueenBB);
        uint64_t bishopAttacks = getBishopAttackSquares(occ, bishopPos);
        allAttackSquares |= bishopAttacks;
        bishopQueenBB ^= (1ull << bishopPos);
    }
    uint64_t rookQueenBB = pieceBB[side][Piece::rook] | pieceBB[side][Piece::queen];
    while (rookQueenBB) {
        int rookPos = my_BitScanForward64(rookQueenBB);
        uint64_t rookAttacks = getRookAttackSquares(occ, rookPos);
        allAttackSquares |= rookAttacks;
        rookQueenBB ^= (1ull << rookPos);
    }
    uint64_t kingPos = my_BitScanForward64(pieceBB[side][Piece::king]);
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
        int sq = my_BitScanForward64(pinner);
        pinned |= (arrRectangular[sq][kingPos] & pieceBB[side][Piece::any]) ^ (1ull << kingPos);
        pinner &= pinner - 1;
    }
    pinner = getBishopXrayAttackSquares(occ, pieceBB[side][Piece::any], kingPos) & opBQ;
    while (pinner) {
        int sq = my_BitScanForward64(pinner);
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
        int checkingPieceIndex = my_BitScanForward64(maskCheckingPieceBishop);
        return arrRectangular[kingPos][checkingPieceIndex];
    }
    uint64_t rooksQueens = pieceBB[!side][Piece::queen] | pieceBB[!side][Piece::rook];
    uint64_t maskCheckingPieceRook = getRookAttackSquares(occ, kingPos) & rooksQueens;
    if (maskCheckingPieceRook) {
        int checkingPieceIndex = my_BitScanForward64(maskCheckingPieceRook);
        return arrRectangular[kingPos][checkingPieceIndex];
    }
    return 0;
}


Stockfish::Piece mapPiece(Side side, Piece piece) {
    Stockfish::Color c = side == Side::White ? Stockfish::WHITE : Stockfish::BLACK;
    Stockfish::PieceType pt;
    switch(piece) {
        case Piece::pawn: pt = Stockfish::PAWN; break;
        case Piece::knight: pt = Stockfish::KNIGHT; break;
        case Piece::bishop: pt = Stockfish::BISHOP; break;
        case Piece::rook: pt = Stockfish::ROOK; break;
        case Piece::queen: pt = Stockfish::QUEEN; break;
        case Piece::king: pt = Stockfish::KING; break;
        default: return Stockfish::NO_PIECE;
    }
    return Stockfish::make_piece(c, pt);
}

uint64_t BitBoard::move(Move& move, uint64_t zHash) {
    moveNum++;
    assert(moveNum >= 0);
    Side sideToMove = move.getSideToMove();
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

    if (g_nnue_networks) {
        
        Stockfish::DirtyPiece dp;

        dp.pc = mapPiece(sideToMove, movePiece);
        dp.from = static_cast<Stockfish::Square>(startPos);
        dp.to = static_cast<Stockfish::Square>(endPos);
        dp.remove_sq = Stockfish::SQ_NONE;
        dp.add_sq = Stockfish::SQ_NONE;
        
        if (moveType == MoveType::CAPTURE) {
            dp.remove_sq = static_cast<Stockfish::Square>(endPos);
            dp.remove_pc = mapPiece(sideToMove == Side::White ? Side::Black : Side::White, capturePiece);
        } else if (moveType == MoveType::EN_PASSANT) {
            dp.remove_sq = static_cast<Stockfish::Square>(sideToMove == Side::White ? endPos - 8 : endPos + 8);
            dp.remove_pc = mapPiece(sideToMove == Side::White ? Side::Black : Side::White, capturePiece);
        } else if (moveType == MoveType::CASTLE_KINGSIDE || moveType == MoveType::CASTLE_QUEENSIDE) {
            dp.remove_sq = static_cast<Stockfish::Square>(moveType == MoveType::CASTLE_KINGSIDE ? (sideToMove == Side::White ? 7 : 63) : (sideToMove == Side::White ? 0 : 56));
            dp.remove_pc = mapPiece(sideToMove, Piece::rook);
            dp.add_sq = static_cast<Stockfish::Square>(moveType == MoveType::CASTLE_KINGSIDE ? (sideToMove == Side::White ? 5 : 61) : (sideToMove == Side::White ? 3 : 59));
            dp.add_pc = mapPiece(sideToMove, Piece::rook);
        } else if (moveType >= MoveType::PROMOTION_BISHOP) {
            dp.to = Stockfish::SQ_NONE;
            dp.add_sq = static_cast<Stockfish::Square>(endPos);
            Piece promPiece;
            if (moveType == MoveType::PROMOTION_QUEEN || moveType == MoveType::PROMOTION_QUEEN_AND_CAPTURE) promPiece = Piece::queen;
            else if (moveType == MoveType::PROMOTION_ROOK || moveType == MoveType::PROMOTION_ROOK_AND_CAPTURE) promPiece = Piece::rook;
            else if (moveType == MoveType::PROMOTION_BISHOP || moveType == MoveType::PROMOTION_BISHOP_AND_CAPTURE) promPiece = Piece::bishop;
            else promPiece = Piece::knight;
            dp.add_pc = mapPiece(sideToMove, promPiece);
            if (moveType == MoveType::PROMOTION_QUEEN_AND_CAPTURE || moveType == MoveType::PROMOTION_ROOK_AND_CAPTURE || moveType == MoveType::PROMOTION_BISHOP_AND_CAPTURE || moveType == MoveType::PROMOTION_KNIGHT_AND_CAPTURE) {
                dp.remove_sq = static_cast<Stockfish::Square>(endPos);
                dp.remove_pc = mapPiece(sideToMove == Side::White ? Side::Black : Side::White, capturePiece);
            }
        }
        g_nnue_acc.push(dp);
    }

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
    if (!move.isNullMove() && g_nnue_networks) {
        g_nnue_acc.pop();
    }

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

    int kingPos = my_BitScanForward64(pieceBB[sideToMove][Piece::king]);
    uint64_t occupied = pieceBB[sideToMove][Piece::any] | pieceBB[!sideToMove][Piece::any];

    int checks = isSquareAttacked(occupied, kingPos, opSide);
    uint64_t opAttackSquares = getAllAttackSquares(opSide);
    std::vector<Move> ans;
    ans.reserve(40);
    if (checks == 0) {
        uint64_t pinnedPiece = getPinnedPiece(occupied, kingPos, sideToMove);
        //pawn pseudo legal move
        uint64_t pawnBB = pieceBB[sideToMove][Piece::pawn];
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        while (pawnBB) {
            int pawnPos = my_BitScanForward64(pawnBB);
            bool isPinned = (1ull << pawnPos) & pinnedPiece;
            bool isPromotion = (1ull << pawnPos) & mask7thRank;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[pawnPos][kingPos];
            }
            uint64_t pawnCaptures = getPawnAttackSquares(sideToMove, pawnPos) & pieceBB[opSide][Piece::any] & maskDirectionPin;
            bool isStartingRank = sideToMove == Side::White ? (pawnPos >= 8 && pawnPos <= 15) : (pawnPos >= 48 && pawnPos <= 55);
            while (pawnCaptures) {
                int capturePos = my_BitScanForward64(pawnCaptures);
                Piece capturePiece = pieceTable[capturePos];
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
                int pawnPos = my_BitScanForward64(enPassantMask);
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
            int knightPos = my_BitScanForward64(knightBB);
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
                int capturePos = my_BitScanForward64(knightCaptures);
                Piece capturePiece = pieceTable[capturePos];
                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }

            while (knightNormalMoves) {
                int movePos = my_BitScanForward64(knightNormalMoves);
                ans.emplace_back(Move(knightPos, movePos, MoveType::NORMAL, sideToMove, Piece::knight));
                knightNormalMoves ^= (1ull << movePos);
            }
            knightBB ^= (1ull << knightPos);
        }

        uint64_t bishopBB = pieceBB[sideToMove][Piece::bishop] | pieceBB[sideToMove][Piece::queen];
        while (bishopBB) {
            int bishopPos = my_BitScanForward64(bishopBB);
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
                int capturePos = my_BitScanForward64(bishopCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, movePiece, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }

            while (bishopNormalMoves) {
                int movePos = my_BitScanForward64(bishopNormalMoves);
                bishopNormalMoves ^= (1ull << movePos);
                ans.emplace_back(Move(bishopPos, movePos, MoveType::NORMAL, sideToMove, movePiece));

            }
            bishopBB ^= (1ull << bishopPos);
        }
        uint64_t rookBB = pieceBB[sideToMove][Piece::rook] | pieceBB[sideToMove][Piece::queen];
        while (rookBB) {
            int rookPos = my_BitScanForward64(rookBB);
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
                int capturePos = my_BitScanForward64(rookCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, movePiece, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }

            while (rookNormalMoves) {
                int movePos = my_BitScanForward64(rookNormalMoves);
                rookNormalMoves ^= (1ull << movePos);
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
                int capturePos = my_BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = my_BitScanForward64(kingNormalMoves);
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
        //pawn pseudo legal move
        uint64_t pawnBB = pieceBB[sideToMove][Piece::pawn];
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        while (pawnBB) {
            int pawnPos = my_BitScanForward64(pawnBB);
            bool isPinned = (1ull << pawnPos) & pinnedPiece;
            bool isPromotion = (1ull << pawnPos) & mask7thRank;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[pawnPos][kingPos];
            }
            uint64_t pawnCaptures = getPawnAttackSquares(sideToMove, pawnPos) & pieceBB[opSide][Piece::any] & maskDirectionPin & interposingSquares;
            bool isStartingRank = sideToMove == Side::White ? (pawnPos >= 8 && pawnPos <= 15) : (pawnPos >= 48 && pawnPos <= 55);
            while (pawnCaptures) {
                int capturePos = my_BitScanForward64(pawnCaptures);
                Piece capturePiece = pieceTable[capturePos];
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
                int pawnPos = my_BitScanForward64(enPassantMask);
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
            int knightPos = my_BitScanForward64(knightBB);
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
                int capturePos = my_BitScanForward64(knightCaptures);
                Piece capturePiece = pieceTable[capturePos];

                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }

            while (knightNormalMoves) {
                int movePos = my_BitScanForward64(knightNormalMoves);

                ans.emplace_back(Move(knightPos, movePos, MoveType::NORMAL, sideToMove, Piece::knight));
                knightNormalMoves ^= (1ull << movePos);
            }
            knightBB ^= (1ull << knightPos);
        }

        uint64_t bishopBB = pieceBB[sideToMove][Piece::bishop];
        while (bishopBB) {
            int bishopPos = my_BitScanForward64(bishopBB);
            bool isPinned = (1ull << bishopPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[bishopPos][kingPos];
            }
            uint64_t bishopAttacks = getBishopAttackSquares(occupied, bishopPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t bishopCaptures = bishopAttacks & pieceBB[opSide][Piece::any];
            uint64_t bishopNormalMoves = bishopAttacks ^ bishopCaptures;
            while (bishopCaptures) {
                int capturePos = my_BitScanForward64(bishopCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::bishop, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }

            while (bishopNormalMoves) {
                int movePos = my_BitScanForward64(bishopNormalMoves);
                ans.emplace_back(Move(bishopPos, movePos, MoveType::NORMAL, sideToMove, Piece::bishop));
                bishopNormalMoves ^= (1ull << movePos);
            }
            bishopBB ^= (1ull << bishopPos);
        }
        uint64_t rookBB = pieceBB[sideToMove][Piece::rook];
        while (rookBB) {
            int rookPos = my_BitScanForward64(rookBB);
            bool isPinned = (1ull << rookPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[rookPos][kingPos];
            }
            uint64_t rookAttacks = getRookAttackSquares(occupied, rookPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t rookCaptures = rookAttacks & pieceBB[opSide][Piece::any];
            uint64_t rookNormalMoves = rookAttacks ^ rookCaptures;
            while (rookCaptures) {
                int capturePos = my_BitScanForward64(rookCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::rook, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }
            while (rookNormalMoves) {
                int movePos = my_BitScanForward64(rookNormalMoves);
                ans.emplace_back(Move(rookPos, movePos, MoveType::NORMAL, sideToMove, Piece::rook));
                rookNormalMoves ^= (1ull << movePos);
            }
            rookBB ^= (1ull << rookPos);
        }
        uint64_t queenBB = pieceBB[sideToMove][Piece::queen];
        while (queenBB) {
            int queenPos = my_BitScanForward64(queenBB);
            bool isPinned = (1ull << queenPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[queenPos][kingPos];
            }
            uint64_t queenAttacks = (getBishopAttackSquares(occupied, queenPos) | getRookAttackSquares(occupied, queenPos)) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t queenCaptures = queenAttacks & pieceBB[opSide][Piece::any];
            uint64_t queenNormalMoves = queenAttacks ^ queenCaptures;
            while (queenCaptures) {
                int capturePos = my_BitScanForward64(queenCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(queenPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::queen, capturePiece));
                queenCaptures ^= (1ull << capturePos);
            }

            while (queenNormalMoves) {
                int movePos = my_BitScanForward64(queenNormalMoves);
                ans.emplace_back(Move(queenPos, movePos, MoveType::NORMAL, sideToMove, Piece::queen));
                queenNormalMoves ^= (1ull << movePos);
            }
            queenBB ^= (1ull << queenPos);
        }
        uint64_t kingBB = pieceBB[sideToMove][Piece::king];
        while (kingBB) {
            int kingPos = my_BitScanForward64(kingBB);
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            uint64_t kingNormalMoves = kingAttacks ^ kingCaptures;
            while (kingCaptures) {
                int capturePos = my_BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = my_BitScanForward64(kingNormalMoves);

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
            int kingPos = my_BitScanForward64(kingBB);
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            uint64_t kingNormalMoves = kingAttacks ^ kingCaptures;
            while (kingCaptures) {
                int capturePos = my_BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = my_BitScanForward64(kingNormalMoves);

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

    int kingPos = my_BitScanForward64(pieceBB[sideToMove][Piece::king]);
    uint64_t occupied = pieceBB[sideToMove][Piece::any] | pieceBB[!sideToMove][Piece::any];

    int checks = isSquareAttacked(occupied, kingPos, opSide);
    uint64_t opAttackSquares = getAllAttackSquares(opSide);
    if (checks == 0) {
        uint64_t pinnedPiece = getPinnedPiece(occupied, kingPos, sideToMove);
        //pawn pseudo legal move
        uint64_t pawnBB = pieceBB[sideToMove][Piece::pawn];
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        while (pawnBB) {
            int pawnPos = my_BitScanForward64(pawnBB);
            bool isPinned = (1ull << pawnPos) & pinnedPiece;
            bool isPromotion = (1ull << pawnPos) & mask7thRank;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[pawnPos][kingPos];
            }
            uint64_t pawnCaptures = getPawnAttackSquares(sideToMove, pawnPos) & pieceBB[opSide][Piece::any] & maskDirectionPin;
            bool isStartingRank = sideToMove == Side::White ? (pawnPos >= 8 && pawnPos <= 15) : (pawnPos >= 48 && pawnPos <= 55);
            while (pawnCaptures) {
                int capturePos = my_BitScanForward64(pawnCaptures);
                Piece capturePiece = pieceTable[capturePos];


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
                int pawnPos = my_BitScanForward64(enPassantMask);
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
            int knightPos = my_BitScanForward64(knightBB);
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
                int capturePos = my_BitScanForward64(knightCaptures);
                Piece capturePiece = pieceTable[capturePos];

                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }

            while (knightNormalMoves) {
                int movePos = my_BitScanForward64(knightNormalMoves);

                ans.emplace_back(Move(knightPos, movePos, MoveType::NORMAL, sideToMove, Piece::knight));
                knightNormalMoves ^= (1ull << movePos);
            }
            knightBB ^= (1ull << knightPos);
        }

        uint64_t bishopBB = pieceBB[sideToMove][Piece::bishop] | pieceBB[sideToMove][Piece::queen];
        while (bishopBB) {
            int bishopPos = my_BitScanForward64(bishopBB);
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
                int capturePos = my_BitScanForward64(bishopCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, movePiece, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }

            while (bishopNormalMoves) {
                int movePos = my_BitScanForward64(bishopNormalMoves);
                bishopNormalMoves ^= (1ull << movePos);
                ans.emplace_back(Move(bishopPos, movePos, MoveType::NORMAL, sideToMove, movePiece));

            }
            bishopBB ^= (1ull << bishopPos);
        }
        uint64_t rookBB = pieceBB[sideToMove][Piece::rook] | pieceBB[sideToMove][Piece::queen];
        while (rookBB) {
            int rookPos = my_BitScanForward64(rookBB);
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
                int capturePos = my_BitScanForward64(rookCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, movePiece, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }

            while (rookNormalMoves) {
                int movePos = my_BitScanForward64(rookNormalMoves);
                rookNormalMoves ^= (1ull << movePos);
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
                int capturePos = my_BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = my_BitScanForward64(kingNormalMoves);

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
        //pawn pseudo legal move
        uint64_t pawnBB = pieceBB[sideToMove][Piece::pawn];
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        while (pawnBB) {
            int pawnPos = my_BitScanForward64(pawnBB);
            bool isPinned = (1ull << pawnPos) & pinnedPiece;
            bool isPromotion = (1ull << pawnPos) & mask7thRank;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[pawnPos][kingPos];
            }
            uint64_t pawnCaptures = getPawnAttackSquares(sideToMove, pawnPos) & pieceBB[opSide][Piece::any] & maskDirectionPin & interposingSquares;
            bool isStartingRank = sideToMove == Side::White ? (pawnPos >= 8 && pawnPos <= 15) : (pawnPos >= 48 && pawnPos <= 55);
            while (pawnCaptures) {
                int capturePos = my_BitScanForward64(pawnCaptures);
                Piece capturePiece = pieceTable[capturePos];
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
                int pawnPos = my_BitScanForward64(enPassantMask);
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
            int knightPos = my_BitScanForward64(knightBB);
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
                int capturePos = my_BitScanForward64(knightCaptures);
                Piece capturePiece = pieceTable[capturePos];

                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }

            while (knightNormalMoves) {
                int movePos = my_BitScanForward64(knightNormalMoves);

                ans.emplace_back(Move(knightPos, movePos, MoveType::NORMAL, sideToMove, Piece::knight));
                knightNormalMoves ^= (1ull << movePos);
            }
            knightBB ^= (1ull << knightPos);
        }

        uint64_t bishopBB = pieceBB[sideToMove][Piece::bishop];
        while (bishopBB) {
            int bishopPos = my_BitScanForward64(bishopBB);
            bool isPinned = (1ull << bishopPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[bishopPos][kingPos];
            }
            uint64_t bishopAttacks = getBishopAttackSquares(occupied, bishopPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t bishopCaptures = bishopAttacks & pieceBB[opSide][Piece::any];
            uint64_t bishopNormalMoves = bishopAttacks ^ bishopCaptures;
            while (bishopCaptures) {
                int capturePos = my_BitScanForward64(bishopCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::bishop, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }

            while (bishopNormalMoves) {
                int movePos = my_BitScanForward64(bishopNormalMoves);
                ans.emplace_back(Move(bishopPos, movePos, MoveType::NORMAL, sideToMove, Piece::bishop));
                bishopNormalMoves ^= (1ull << movePos);
            }
            bishopBB ^= (1ull << bishopPos);
        }
        uint64_t rookBB = pieceBB[sideToMove][Piece::rook];
        while (rookBB) {
            int rookPos = my_BitScanForward64(rookBB);
            bool isPinned = (1ull << rookPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[rookPos][kingPos];
            }
            uint64_t rookAttacks = getRookAttackSquares(occupied, rookPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t rookCaptures = rookAttacks & pieceBB[opSide][Piece::any];
            uint64_t rookNormalMoves = rookAttacks ^ rookCaptures;
            while (rookCaptures) {
                int capturePos = my_BitScanForward64(rookCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::rook, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }

            while (rookNormalMoves) {
                int movePos = my_BitScanForward64(rookNormalMoves);
                ans.emplace_back(Move(rookPos, movePos, MoveType::NORMAL, sideToMove, Piece::rook));
                rookNormalMoves ^= (1ull << movePos);
            }
            rookBB ^= (1ull << rookPos);
        }
        uint64_t queenBB = pieceBB[sideToMove][Piece::queen];
        while (queenBB) {
            int queenPos = my_BitScanForward64(queenBB);
            bool isPinned = (1ull << queenPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[queenPos][kingPos];
            }
            uint64_t queenAttacks = (getBishopAttackSquares(occupied, queenPos) | getRookAttackSquares(occupied, queenPos)) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t queenCaptures = queenAttacks & pieceBB[opSide][Piece::any];
            uint64_t queenNormalMoves = queenAttacks ^ queenCaptures;
            while (queenCaptures) {
                int capturePos = my_BitScanForward64(queenCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(queenPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::queen, capturePiece));
                queenCaptures ^= (1ull << capturePos);
            }

            while (queenNormalMoves) {
                int movePos = my_BitScanForward64(queenNormalMoves);
                ans.emplace_back(Move(queenPos, movePos, MoveType::NORMAL, sideToMove, Piece::queen));
                queenNormalMoves ^= (1ull << movePos);
            }
            queenBB ^= (1ull << queenPos);
        }
        uint64_t kingBB = pieceBB[sideToMove][Piece::king];
        while (kingBB) {
            int kingPos = my_BitScanForward64(kingBB);
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            uint64_t kingNormalMoves = kingAttacks ^ kingCaptures;
            while (kingCaptures) {
                int capturePos = my_BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = my_BitScanForward64(kingNormalMoves);

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
            int kingPos = my_BitScanForward64(kingBB);
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            uint64_t kingNormalMoves = kingAttacks ^ kingCaptures;
            while (kingCaptures) {
                int capturePos = my_BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = my_BitScanForward64(kingNormalMoves);

                ans.emplace_back(Move(kingPos, movePos, MoveType::NORMAL, sideToMove, Piece::king));
                kingNormalMoves ^= (1ull << movePos);
            }
            kingBB ^= (1ull << kingPos);
        }
    }
    //For move-ordering
}
void BitBoard::getLegalCapturesAlt(MoveVector& ans) {
    ans.reset();
    Side sideToMove = isWhiteTurn ? Side::White : Side::Black;
    Side opSide = sideToMove == Side::White ? Side::Black : Side::White;

    int kingPos = my_BitScanForward64(pieceBB[sideToMove][Piece::king]);
    uint64_t occupied = pieceBB[sideToMove][Piece::any] | pieceBB[!sideToMove][Piece::any];

    int checks = isSquareAttacked(occupied, kingPos, opSide);
    uint64_t opAttackSquares = getAllAttackSquares(opSide);
    if (checks == 0) {
        uint64_t pinnedPiece = getPinnedPiece(occupied, kingPos, sideToMove);
        //pawn pseudo legal move
        uint64_t pawnBB = pieceBB[sideToMove][Piece::pawn];
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        int moveOffset = sideToMove == Side::White ? 8 : -8;
        while (pawnBB) {
            int pawnPos = my_BitScanForward64(pawnBB);
            bool isPinned = (1ull << pawnPos) & pinnedPiece;
            bool isPromotion = (1ull << pawnPos) & mask7thRank;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[pawnPos][kingPos];
            }
            uint64_t pawnCaptures = getPawnAttackSquares(sideToMove, pawnPos) & pieceBB[opSide][Piece::any] & maskDirectionPin;
            bool isStartingRank = sideToMove == Side::White ? (pawnPos >= 8 && pawnPos <= 15) : (pawnPos >= 48 && pawnPos <= 55);
            while (pawnCaptures) {
                int capturePos = my_BitScanForward64(pawnCaptures);
                Piece capturePiece = pieceTable[capturePos];


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
                int pawnPos = my_BitScanForward64(enPassantMask);
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
            int knightPos = my_BitScanForward64(knightBB);
            bool isPinned = (1ull << knightPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                knightBB ^= (1ull << knightPos);
                continue;
            }
            uint64_t knightAttacks = getKnightAttackSquares(knightPos) & ~pieceBB[sideToMove][Piece::any];
            uint64_t knightCaptures = knightAttacks & pieceBB[opSide][Piece::any];
            while (knightCaptures) {
                int capturePos = my_BitScanForward64(knightCaptures);
                Piece capturePiece = pieceTable[capturePos];

                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }
            knightBB ^= (1ull << knightPos);
        }

        uint64_t bishopBB = pieceBB[sideToMove][Piece::bishop];
        while (bishopBB) {
            int bishopPos = my_BitScanForward64(bishopBB);
            bool isPinned = (1ull << bishopPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[bishopPos][kingPos];
            }
            uint64_t bishopAttacks = getBishopAttackSquares(occupied, bishopPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin;
            uint64_t bishopCaptures = bishopAttacks & pieceBB[opSide][Piece::any];
            while (bishopCaptures) {
                int capturePos = my_BitScanForward64(bishopCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::bishop, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }
            bishopBB ^= (1ull << bishopPos);
        }
        uint64_t rookBB = pieceBB[sideToMove][Piece::rook];
        while (rookBB) {
            int rookPos = my_BitScanForward64(rookBB);
            bool isPinned = (1ull << rookPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[rookPos][kingPos];
            }
            uint64_t rookAttacks = getRookAttackSquares(occupied, rookPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin;
            uint64_t rookCaptures = rookAttacks & pieceBB[opSide][Piece::any];
            while (rookCaptures) {
                int capturePos = my_BitScanForward64(rookCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::rook, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }
            rookBB ^= (1ull << rookPos);
        }
        uint64_t queenBB = pieceBB[sideToMove][Piece::queen];
        while (queenBB) {
            int queenPos = my_BitScanForward64(queenBB);
            bool isPinned = (1ull << queenPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[queenPos][kingPos];
            }
            uint64_t queenAttacks = (getBishopAttackSquares(occupied, queenPos) | getRookAttackSquares(occupied, queenPos)) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin;
            uint64_t queenCaptures = queenAttacks & pieceBB[opSide][Piece::any];
            while (queenCaptures) {
                int capturePos = my_BitScanForward64(queenCaptures);
                Piece capturePiece = pieceTable[capturePos];
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
                int capturePos = my_BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }
            kingBB ^= (1ull << kingPos);
        }

    }
    else if (checks == 1) {
        uint64_t pinnedPiece = getPinnedPiece(occupied, kingPos, sideToMove);
        uint64_t interposingSquares = getSingleCheckInterposingSquares(occupied, kingPos, sideToMove);
        //pawn pseudo legal move
        uint64_t pawnBB = pieceBB[sideToMove][Piece::pawn];
        uint64_t mask7thRank = sideToMove == Side::White ? 0x00ff000000000000 : 0x000000000000ff00;
        int moveOffset = sideToMove == Side::White ? 8 : -8;
        while (pawnBB) {
            int pawnPos = my_BitScanForward64(pawnBB);
            bool isPinned = (1ull << pawnPos) & pinnedPiece;
            bool isPromotion = (1ull << pawnPos) & mask7thRank;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[pawnPos][kingPos];
            }
            uint64_t pawnCaptures = getPawnAttackSquares(sideToMove, pawnPos) & pieceBB[opSide][Piece::any] & maskDirectionPin & interposingSquares;
            bool isStartingRank = sideToMove == Side::White ? (pawnPos >= 8 && pawnPos <= 15) : (pawnPos >= 48 && pawnPos <= 55);
            while (pawnCaptures) {
                int capturePos = my_BitScanForward64(pawnCaptures);
                Piece capturePiece = pieceTable[capturePos];
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
                int pawnPos = my_BitScanForward64(enPassantMask);
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
            int knightPos = my_BitScanForward64(knightBB);
            bool isPinned = (1ull << knightPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                knightBB ^= (1ull << knightPos);
                continue;
            }
            uint64_t knightAttacks = getKnightAttackSquares(knightPos) & ~pieceBB[sideToMove][Piece::any] & interposingSquares;
            uint64_t knightCaptures = knightAttacks & pieceBB[opSide][Piece::any];
            while (knightCaptures) {
                int capturePos = my_BitScanForward64(knightCaptures);
                Piece capturePiece = pieceTable[capturePos];

                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }
            knightBB ^= (1ull << knightPos);
        }

        uint64_t bishopBB = pieceBB[sideToMove][Piece::bishop];
        while (bishopBB) {
            int bishopPos = my_BitScanForward64(bishopBB);
            bool isPinned = (1ull << bishopPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[bishopPos][kingPos];
            }
            uint64_t bishopAttacks = getBishopAttackSquares(occupied, bishopPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t bishopCaptures = bishopAttacks & pieceBB[opSide][Piece::any];
            while (bishopCaptures) {
                int capturePos = my_BitScanForward64(bishopCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::bishop, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }
            bishopBB ^= (1ull << bishopPos);
        }
        uint64_t rookBB = pieceBB[sideToMove][Piece::rook];
        while (rookBB) {
            int rookPos = my_BitScanForward64(rookBB);
            bool isPinned = (1ull << rookPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[rookPos][kingPos];
            }
            uint64_t rookAttacks = getRookAttackSquares(occupied, rookPos) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t rookCaptures = rookAttacks & pieceBB[opSide][Piece::any];
            while (rookCaptures) {
                int capturePos = my_BitScanForward64(rookCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::rook, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }
            rookBB ^= (1ull << rookPos);
        }
        uint64_t queenBB = pieceBB[sideToMove][Piece::queen];
        while (queenBB) {
            int queenPos = my_BitScanForward64(queenBB);
            bool isPinned = (1ull << queenPos) & pinnedPiece;
            uint64_t maskDirectionPin = UINT64_MAX;
            if (isPinned) {
                maskDirectionPin = dirMask[queenPos][kingPos];
            }
            uint64_t queenAttacks = (getBishopAttackSquares(occupied, queenPos) | getRookAttackSquares(occupied, queenPos)) & ~pieceBB[sideToMove][Piece::any] & maskDirectionPin & interposingSquares;
            uint64_t queenCaptures = queenAttacks & pieceBB[opSide][Piece::any];
            while (queenCaptures) {
                int capturePos = my_BitScanForward64(queenCaptures);
                Piece capturePiece = pieceTable[capturePos];
                ans.emplace_back(Move(queenPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::queen, capturePiece));
                queenCaptures ^= (1ull << capturePos);
            }
            queenBB ^= (1ull << queenPos);
        }
        uint64_t kingBB = pieceBB[sideToMove][Piece::king];
        while (kingBB) {
            int kingPos = my_BitScanForward64(kingBB);
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            while (kingCaptures) {
                int capturePos = my_BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];

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
            int kingPos = my_BitScanForward64(kingBB);
            uint64_t kingAttacks = getKingAttackSquares(kingPos) & ~(pieceBB[sideToMove][Piece::any]) & (~opAttackSquares);
            uint64_t kingCaptures = kingAttacks & pieceBB[opSide][Piece::any];
            while (kingCaptures) {
                int capturePos = my_BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }
            kingBB ^= (1ull << kingPos);
        }
    }
}
bool BitBoard::isKingInCheck() {
    Side sideToMove = isWhiteTurn ? Side::White : Side::Black;
    Side opSide = sideToMove == Side::White ? Side::Black : Side::White;
    int kingPos = my_BitScanForward64(pieceBB[sideToMove][Piece::king]);
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
    int kingPos[2] = { my_BitScanForward64(pieceBB[Side::White][Piece::king]),
                    my_BitScanForward64(pieceBB[Side::Black][Piece::king])
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
bool BitBoard::isThreat(const Move& move) {
    const Side     sideToMove = move.getSideToMove();
    const Side     opSide = sideToMove == Side::White ? Side::Black : Side::White;
    const int      startPos = move.getStartPos();
    const int      endPos = move.getEndPos();
    const MoveType mt = move.getMoveType();
    const Piece    movePiece = move.getMovePiece();

    // Queens can never threaten a higher-value piece — bail immediately.
    // This also covers all queen-promotion moves.
    if (movePiece == Piece::queen)
        return false;

    // ── Effective piece after non-queen promotion ─────────────────────────────
    Piece effectivePiece = movePiece;
    switch (mt) {
    case MoveType::PROMOTION_ROOK:
    case MoveType::PROMOTION_ROOK_AND_CAPTURE:    effectivePiece = Piece::rook;   break;
    case MoveType::PROMOTION_BISHOP:
    case MoveType::PROMOTION_BISHOP_AND_CAPTURE:  effectivePiece = Piece::bishop; break;
    case MoveType::PROMOTION_KNIGHT:
    case MoveType::PROMOTION_KNIGHT_AND_CAPTURE:  effectivePiece = Piece::knight; break;
    default: break;
    }

    // ── Post-move occupancy ───────────────────────────────────────────────────
    uint64_t occ = pieceBB[0][Piece::any] | pieceBB[1][Piece::any];
    occ ^= (1ull << startPos);
    occ |= (1ull << endPos);
    if (mt == MoveType::EN_PASSANT) {
        const int capturedPawnSq = (sideToMove == Side::White) ? endPos - 8 : endPos + 8;
        occ ^= (1ull << capturedPawnSq);
    }

    // ── Attack squares from destination ──────────────────────────────────────
    uint64_t atkSq = 0;
    switch (effectivePiece) {
    case Piece::pawn:   atkSq = arrPawnAttacks[sideToMove][endPos];                              break;
    case Piece::knight: atkSq = arrKnightAttacks[endPos];                                        break;
    case Piece::bishop: atkSq = getBishopAttackSquares(occ, endPos);                             break;
    case Piece::rook:   atkSq = getRookAttackSquares(occ, endPos);                               break;
    case Piece::king:   atkSq = arrKingAttacks[endPos];                                          break;
    default:            return false;
    }

    // ── Hit any opponent piece worth strictly more than the mover? ────────────
    const int myValue = pieceValue[effectivePiece];
    for (int p = Piece::pawn; p <= Piece::queen; p++) {
        if (pieceValue[p] > myValue && (atkSq & pieceBB[opSide][p])) {

            return true;
        }
    }
    return false;
}
int BitBoard::quickMaterialScore() {
    int score = 0;
    for (int p = Piece::pawn; p <= Piece::queen; p++) {
        score += popcount64(pieceBB[Side::White][p]) * materialValue[p];
        score -= popcount64(pieceBB[Side::Black][p]) * materialValue[p];
    }
    return score * (isWhiteTurn ? 1 : -1);
}
bool BitBoard::isEndgame() {
    // Mirror the phase calculation from evaluate():
    //   knights  → 1 each
    //   bishops  → 1 each
    //   rooks    → 2 each
    //   queens   → 4 each
    // TOTAL_PHASE is the starting value (full middlegame).
    // When the remaining phase falls to ≤ 50% of TOTAL_PHASE, call it endgame.

    int phase = TOTAL_PHASE;

    phase -= popcount64(pieceBB[0][Piece::knight] | pieceBB[1][Piece::knight]);
    phase -= popcount64(pieceBB[0][Piece::bishop] | pieceBB[1][Piece::bishop]);
    phase -= popcount64(pieceBB[0][Piece::rook] | pieceBB[1][Piece::rook]) * 2;
    phase -= popcount64(pieceBB[0][Piece::queen] | pieceBB[1][Piece::queen]) * 4;

    // phase==0 → pure endgame, phase==TOTAL_PHASE → pure opening/middlegame.
    // Threshold at 25% of TOTAL_PHASE remaining (≈ one rook + one minor piece left).
    return phase <= TOTAL_PHASE / 3;
}
uint64_t BitBoard::getPawnHash() {
    // Simple XOR hash of pawn positions — fast and sufficient
    uint64_t h = pieceBB[Side::White][Piece::pawn] * 0x9e3779b97f4a7c15ULL;
    h ^= pieceBB[Side::Black][Piece::pawn] * 0xbf58476d1ce4e5b9ULL;
    return h;
}

int BitBoard::probePawnHash(uint64_t pawnHash, int* score) {
    PawnEntry& entry = pawnHashTable[pawnHash & (PAWN_TABLE_SIZE - 1)];
    if (entry.valid && entry.pawnHash == pawnHash) {
        score[0] = entry.score[0];
        score[1] = entry.score[1];
        return 1; // hit
    }
    return 0; // miss
}

void BitBoard::storePawnHash(uint64_t pawnHash, int score[2]) {
    PawnEntry& entry = pawnHashTable[pawnHash & (PAWN_TABLE_SIZE - 1)];
    entry.pawnHash = pawnHash;
    entry.score[0] = score[0];
    entry.score[1] = score[1];
    entry.valid = true;
}
int BitBoard::evaluate(int alpha, int beta) {
    if (!g_nnue_networks) return 0;
    
    Stockfish::Position pos(this);
    Stockfish::Eval::NNUE::AccumulatorCaches& caches = *g_nnue_caches;
    
    int eval = Stockfish::Eval::evaluate(*g_nnue_networks, pos, g_nnue_acc, caches, 0);
    return eval*25;
}

    