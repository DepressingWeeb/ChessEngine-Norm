#pragma once
#include "bitboard.h"
#include "constant_bitboard.h"
#include "constant_hash.h"
#include <sstream>

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
                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }

            while (knightNormalMoves) {
                int movePos = BitScanForward64(knightNormalMoves);
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
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, movePiece, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }

            while (bishopNormalMoves) {
                int movePos = BitScanForward64(bishopNormalMoves);
                bishopNormalMoves ^= (1ull << movePos);
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
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, movePiece, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }

            while (rookNormalMoves) {
                int movePos = BitScanForward64(rookNormalMoves);
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
                int capturePos = BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = BitScanForward64(kingNormalMoves);
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

                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }

            while (knightNormalMoves) {
                int movePos = BitScanForward64(knightNormalMoves);

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
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::bishop, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }

            while (bishopNormalMoves) {
                int movePos = BitScanForward64(bishopNormalMoves);
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
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::rook, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }
            while (rookNormalMoves) {
                int movePos = BitScanForward64(rookNormalMoves);
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
                ans.emplace_back(Move(queenPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::queen, capturePiece));
                queenCaptures ^= (1ull << capturePos);
            }

            while (queenNormalMoves) {
                int movePos = BitScanForward64(queenNormalMoves);
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

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = BitScanForward64(kingNormalMoves);

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

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = BitScanForward64(kingNormalMoves);

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
    uint64_t occupied = pieceBB[sideToMove][Piece::any] | pieceBB[!sideToMove][Piece::any];

    int checks = isSquareAttacked(occupied, kingPos, opSide);
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

                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }

            while (knightNormalMoves) {
                int movePos = BitScanForward64(knightNormalMoves);

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
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, movePiece, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }

            while (bishopNormalMoves) {
                int movePos = BitScanForward64(bishopNormalMoves);
                bishopNormalMoves ^= (1ull << movePos);
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
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, movePiece, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }

            while (rookNormalMoves) {
                int movePos = BitScanForward64(rookNormalMoves);
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
                int capturePos = BitScanForward64(kingCaptures);
                Piece capturePiece = pieceTable[capturePos];

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = BitScanForward64(kingNormalMoves);

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

                if (!isPinned)
                    ans.emplace_back(Move(knightPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::knight, capturePiece));
                knightCaptures ^= (1ull << capturePos);
            }

            while (knightNormalMoves) {
                int movePos = BitScanForward64(knightNormalMoves);

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
                ans.emplace_back(Move(bishopPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::bishop, capturePiece));
                bishopCaptures ^= (1ull << capturePos);
            }

            while (bishopNormalMoves) {
                int movePos = BitScanForward64(bishopNormalMoves);
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
                ans.emplace_back(Move(rookPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::rook, capturePiece));
                rookCaptures ^= (1ull << capturePos);
            }

            while (rookNormalMoves) {
                int movePos = BitScanForward64(rookNormalMoves);
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
                ans.emplace_back(Move(queenPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::queen, capturePiece));
                queenCaptures ^= (1ull << capturePos);
            }

            while (queenNormalMoves) {
                int movePos = BitScanForward64(queenNormalMoves);
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

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = BitScanForward64(kingNormalMoves);

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

                ans.emplace_back(Move(kingPos, capturePos, MoveType::CAPTURE, sideToMove, Piece::king, capturePiece));
                kingCaptures ^= (1ull << capturePos);
            }

            while (kingNormalMoves) {
                int movePos = BitScanForward64(kingNormalMoves);

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

    int kingPos = BitScanForward64(pieceBB[sideToMove][Piece::king]);
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
    int kingPos = BitScanForward64(pieceBB[sideToMove][Piece::king]);
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
bool BitBoard::isThreat(const Move& move) {
    const Side     sideToMove = move.getSideToMove();
    const Side     opSide =  sideToMove == Side::White ? Side::Black : Side::White;
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
    uint64_t occupied = pieceBB[0][Piece::any] | pieceBB[1][Piece::any];
    const int who2move = isWhiteTurn ? 1 : -1;
    const int totalOcc = popcount64(occupied);

    int score[2]{ 0 };
    int phase = TOTAL_PHASE;

    const int kingPos[2] = {
        BitScanForward64(pieceBB[0][Piece::king]),
        BitScanForward64(pieceBB[1][Piece::king])
    };
    const int rankKing[2] = { posToRank(kingPos[0]), posToRank(kingPos[1]) };
    const int fileKing[2] = { posToFile(kingPos[0]), posToFile(kingPos[1]) };

    // ── Halve detection ───────────────────────────────────────────────────────
    bool halve = false;
    if (totalOcc <= 6
        && (pieceBB[0][Piece::queen] | pieceBB[1][Piece::queen]) == 0
        && (pieceBB[0][Piece::pawn] | pieceBB[1][Piece::pawn]) == 0)
    {
        const int diffMaterial =
            (popcount64(pieceBB[0][Piece::rook]) - popcount64(pieceBB[1][Piece::rook])) * pieceValue[Piece::rook] +
            (popcount64(pieceBB[0][Piece::knight]) - popcount64(pieceBB[1][Piece::knight])) * pieceValue[Piece::knight] +
            (popcount64(pieceBB[0][Piece::bishop]) - popcount64(pieceBB[1][Piece::bishop])) * pieceValue[Piece::bishop];
        if (abs(diffMaterial) <= 300)
            halve = true;
    }

    // ── Pawn hash ─────────────────────────────────────────────────────────────
    const uint64_t pawnHash = getPawnHash();
    int pawnScore[2] = { 0, 0 };
    const bool pawnCacheHit = probePawnHash(pawnHash, pawnScore);

    // ── Shared precomputed masks ──────────────────────────────────────────────
    const Side stm = isWhiteTurn ? Side::White : Side::Black;
    const uint64_t pawnSet = pieceBB[0][Piece::pawn] | pieceBB[1][Piece::pawn];

    const uint64_t allPieces[2] = {
        pieceBB[0][Piece::any] & ~pieceBB[0][Piece::pawn],
        pieceBB[1][Piece::any] & ~pieceBB[1][Piece::pawn]
    };
    const uint64_t allHeavyPieces[2] = {
        allPieces[0] & ~(pieceBB[0][Piece::bishop] | pieceBB[0][Piece::knight]),
        allPieces[1] & ~(pieceBB[1][Piece::bishop] | pieceBB[1][Piece::knight])
    };

    int rawKingSafety[2] = { 0, 0 };
    int totalWeightedKingDistToOwnPawn[2] = { 0, 0 };
    int sumOfWeightDist[2] = { 0, 0 };

    // ── Per-side setup ────────────────────────────────────────────────────────
    // Precompute everything that is side-invariant within the piece loops below.
    // Indexed [side]: opponent pawn attacks, king rings, defender counts, etc.

    uint64_t opPawnAttacks[2];
    uint64_t opPawnSetAtkSq[2];
    uint64_t opInner[2];       // king inner ring of the opponent
    uint64_t opOuter[2];       // king outer ring of the opponent
    int      nDef20[2];        // defender weight around own king, scaled ×20
    int      nAtk20[2] = { 0, 0 };
    int      totalAtkUnits[2] = { 0, 0 };

    for (int side = 0; side < 2; side++) {
        const int op = side ^ 1;

        opInner[side] = getKingAttackSquares(kingPos[op]);
        opOuter[side] = arrOuterRing[kingPos[op]];

        const uint64_t opPawns = pieceBB[op][Piece::pawn];
        const uint64_t opNonPawns = pieceBB[op][Piece::any] ^ opPawns;

        // side==0 (White) is attacked by Black (op==1) pawns that push downward
        opPawnAttacks[side] = (side == 1)
            ? (((opPawns << 7) & ~0x8080808080808080ULL) | ((opPawns << 9) & ~0x0101010101010101ULL))
            : (((opPawns >> 7) & ~0x0101010101010101ULL) | ((opPawns >> 9) & ~0x8080808080808080ULL));

        opPawnSetAtkSq[side] = opPawnAttacks[side] & ~allPieces[op];

        // Defenders around the king being attacked, scaled ×20
        // 0.5 per inner pawn → 10, 0.2 per outer pawn → 4,
        // 1.0 per inner piece → 20, 0.5 per outer piece → 10
        nDef20[side] =
            popcount64(opPawns & opInner[side]) * 10 +
            popcount64(opPawns & opOuter[side]) * 4 +
            popcount64(opNonPawns & opInner[side]) * 20 +
            popcount64(opNonPawns & opOuter[side]) * 10;
    }

    // ═════════════════════════════════════════════════════════════════════════
    // Per-piece-type loops — one tight loop per piece, both sides interleaved
    // so all shared tables (PST, attack tables) stay hot in L1/L2.
    // ═════════════════════════════════════════════════════════════════════════

    // ── PAWNS ─────────────────────────────────────────────────────────────────
    // Tables touched: pawnTable, arrPawnAttacks, arrAfterPawn,
    //                 arrNeighorPawn, arrFrontPawn, bonusPassedPawnByRank
    for (int side = 0; side < 2; side++) {
        const int      op = side ^ 1;
        const int      flip = (side == 0) ? 56 : 0;
        const int      pushOff = (side == 0) ? 8 : -8;
        const bool     isSTM = (side == (int)stm);
        const uint64_t myBB = pieceBB[side][Piece::pawn];

        score[side] += popcount64(myBB) * materialValue[Piece::pawn];

        uint64_t bb = myBB;
        while (bb) {
            const int pos = BitScanForward64(bb);
            bb &= bb - 1;

            const int rankPawn = posToRank(pos);
            const int filePawn = posToFile(pos);
            const bool passed = isPassedPawn(static_cast<Side>(side),
                static_cast<Side>(op), pos);

            // ── Pawn structure (cached) ──────────────────────────────────────
            if (!pawnCacheHit) {
                pawnScore[side] += pawnTable[pos ^ flip];

                if (arrAfterPawn[side][pos] & myBB)
                    pawnScore[side] -= pawnDoublePenalty;

                const bool isolated = !(arrNeighorPawn[pos] & myBB);
                if (isolated)
                    pawnScore[side] -= pawnIsolatedPenalty;
                const int rankFlip = (side == 1) ? (7 - rankPawn) : rankPawn;
                uint64_t adjSameRank = 0ULL;
                if (filePawn > 0) adjSameRank |= (1ULL << (pos - 1));
                if (filePawn < 7) adjSameRank |= (1ULL << (pos + 1));

                if (myBB & adjSameRank)
                    pawnScore[side] += phalanxBonus[rankFlip];
                if (passed) {
                    
                    pawnScore[side] += bonusPassedPawnByRank[rankFlip];

                    if (isConnectedPawn(static_cast<Side>(side), pos))
                        pawnScore[side] += bonusConnectedPassedPawn;

                    const int   frontSq = pos + pushOff;
                    const Piece frontPce = pieceTable[frontSq];
                    if (frontPce != Piece::any) {
                        pawnScore[side] -= ((1ULL << frontSq) & pieceBB[op][Piece::any])
                            ? penaltyPasserBlockedByEnemy
                            : penaltyPasserBlockedBySelf;
                        pawnScore[side] -= penaltyBlockedPasserByRank[rankFlip];
                    }

                    if (rankFlip >= 4) {
                        if (arrAfterPawn[op][pos] & pieceBB[side][Piece::rook])
                            pawnScore[side] += bonusRookSupportPasser;
                        if (arrAfterPawn[side][pos] & pieceBB[op][Piece::rook])
                            pawnScore[side] -= penaltyEnemyRookBlockPasser;
                    }
                    if (rankFlip >= 5 && (arrAfterPawn[side][pos] & pieceBB[op][Piece::any]))
                        pawnScore[side] -= penaltyHasBlockade;
                    if (filePawn == 0 || filePawn == 7)
                        pawnScore[side] += bonusOutsidePasser;

                }
                else if (!isolated) {
                    const uint64_t adj = arrNeighorPawn[pos] & myBB;
                    if ((adj & ~arrFrontPawn[side][pos]) == 0) {
                        if (arrPawnAttacks[side][pos + pushOff] & pieceBB[op][Piece::pawn])
                            pawnScore[side] -= pawnBackwardPenalty;
                    }
                }
            }

            // ── Promo-square king distances (EG, outside cache) ─────────────
            if (passed) {
                const int rankFlip = (side == 1) ? (7 - rankPawn) : rankPawn;
                const int promFile = filePawn;
                const int promRank = (side == 0) ? 7 : 0;
                const int opDistPromo = std::max(abs(posToFile(kingPos[op]) - promFile),
                    abs(posToRank(kingPos[op]) - promRank));
                const int myDistPromo = std::max(abs(posToFile(kingPos[side]) - promFile),
                    abs(posToRank(kingPos[side]) - promRank));
                score[side] += opDistPromo * promoDistOpWeight;
                score[side] -= myDistPromo * promoDistMyWeight;
            }

            // ── King–pawn tropism ────────────────────────────────────────────
            const int weight = passed ? 6 : 3;
            const int mDist = abs(rankKing[side] - rankPawn) + abs(fileKing[side] - filePawn);
            totalWeightedKingDistToOwnPawn[side] += weight * mDist;
            sumOfWeightDist[side] += weight;

            // ── King attack contribution ─────────────────────────────────────
            const uint64_t atkSq = arrPawnAttacks[side][pos];
            const int nAtksI = popcount64(atkSq & opInner[side]);
            const int nAtksO = popcount64(atkSq & opOuter[side]);
            if (nAtksI) nAtk20[side] += 10;   // 0.50 × 20
            else if (nAtksO) nAtk20[side] += 5;   // 0.25 × 20
            totalAtkUnits[side] += nAtksI * 2 + nAtksO;

            if (isSTM && (atkSq & allPieces[op]))
                score[side] += bonusThreatOnHigherValuePiece[1];
        }
    }

    // ── KNIGHTS ──────────────────────────────────────────────────────────────
    // Tables touched: knightTable, getKnightAttackSquares (magic/array),
    //                 arrPawnAttacks, arrFrontPawn, arrNeighorPawn
    for (int side = 0; side < 2; side++) {
        const int      op = side ^ 1;
        const int      flip = (side == 0) ? 56 : 0;
        const bool     isSTM = (side == (int)stm);
        const uint64_t myBB = pieceBB[side][Piece::any];

        uint64_t bb = pieceBB[side][Piece::knight];
        while (bb) {
            const int pos = BitScanForward64(bb);
            bb &= bb - 1;

            --phase;
            score[side] += materialValue[Piece::knight];
            score[side] += knightTable[pos ^ flip];

            const uint64_t atkSq = getKnightAttackSquares(pos) & ~myBB;
            const int nAtksI = popcount64(atkSq & opInner[side]);
            const int nAtksO = popcount64(atkSq & opOuter[side]);
            if (nAtksI) nAtk20[side] += 20;   // 1.0 × 20
            else if (nAtksO) nAtk20[side] += 10;   // 0.5 × 20
            totalAtkUnits[side] += nAtksI * 3 + nAtksO * 2;

            if (isSTM && (atkSq & allHeavyPieces[op]))
                score[side] += bonusThreatOnHigherValuePiece[1];

            // Outpost
            if ((arrPawnAttacks[op][pos] & pieceBB[side][Piece::pawn]) &&
                !(pieceBB[op][Piece::pawn] & arrFrontPawn[side][pos] & arrNeighorPawn[pos]))
                score[side] += bonusOutpostKnight;

            const int mob = popcount64(atkSq & ~opPawnAttacks[side]);
            score[side] += mob * mobilityValue[Piece::knight];
            if (mob == 0)
                score[side] -= penaltyNoMobility[Piece::knight];
        }
    }

    // ── BISHOPS ──────────────────────────────────────────────────────────────
    // Tables touched: bishopTable, getBishopAttackSquares (magic),
    //                 arrPawnAttacks, arrFrontPawn, arrNeighorPawn
    for (int side = 0; side < 2; side++) {
        const int      op = side ^ 1;
        const int      flip = (side == 0) ? 56 : 0;
        const bool     isSTM = (side == (int)stm);
        const uint64_t myBB = pieceBB[side][Piece::any];

        if (popcount64(pieceBB[side][Piece::bishop]) == 2)
            score[side] += bishopPairBonus;

        uint64_t bb = pieceBB[side][Piece::bishop];
        while (bb) {
            const int pos = BitScanForward64(bb);
            bb &= bb - 1;

            --phase;
            score[side] += materialValue[Piece::bishop];
            score[side] += bishopTable[pos ^ flip];

            const uint64_t atkSq = getBishopAttackSquares(occupied, pos) & ~myBB;
            const int nAtksI = popcount64(atkSq & opInner[side]);
            const int nAtksO = popcount64(atkSq & opOuter[side]);
            if (nAtksI) nAtk20[side] += 20;
            else if (nAtksO) nAtk20[side] += 10;
            totalAtkUnits[side] += nAtksI * 3 + nAtksO * 2;

            if (isSTM && (atkSq & allHeavyPieces[op]))
                score[side] += bonusThreatOnHigherValuePiece[1];

            // Outpost
            if ((arrPawnAttacks[op][pos] & pieceBB[side][Piece::pawn]) &&
                !(pieceBB[op][Piece::pawn] & arrFrontPawn[side][pos] & arrNeighorPawn[pos]))
                score[side] += bonusOutpostBishop;

            // Bad bishop: own pawns on same-color squares
            const bool     onLight = (posToFile(pos) + posToRank(pos)) % 2 == 0;
            const uint64_t sameColor = onLight ? 0x55AA55AA55AA55AAULL
                : 0xAA55AA55AA55AA55ULL;
            score[side] -= popcount64(pieceBB[side][Piece::pawn] & sameColor) * penaltyBadBishop;

            score[side] += popcount64(atkSq & ~opPawnAttacks[side]) * mobilityValue[Piece::bishop];
        }
    }

    // ── ROOKS ────────────────────────────────────────────────────────────────
    // Tables touched: rookTable, getRookAttackSquares (magic),
    //                 arrAfterPawn, pawnSet
    for (int side = 0; side < 2; side++) {
        const int      op = side ^ 1;
        const int      flip = (side == 0) ? 56 : 0;
        const bool     isSTM = (side == (int)stm);
        const uint64_t myBB = pieceBB[side][Piece::any];
        const int      rank7 = (side == 0) ? 6 : 1;
        const int      rank8 = (side == 0) ? 7 : 0;
        const uint64_t r7mask = 0xFFULL << (rank7 * 8);

        uint64_t bb = pieceBB[side][Piece::rook];
        while (bb) {
            const int pos = BitScanForward64(bb);
            bb &= bb - 1;

            phase -= 2;
            score[side] += materialValue[Piece::rook];
            score[side] += rookTable[pos ^ flip];

            // Open / semi-open file
            if (!(arrAfterPawn[side][pos] & pawnSet))
                score[side] += bonusRookOnOpenFile;
            else if (!(arrAfterPawn[side][pos] & pieceBB[side][Piece::pawn]))
                score[side] += bonusRookOnSemiOpenFile;

            // Rook on 7th rank
            if (posToRank(pos) == rank7) {
                if ((pieceBB[op][Piece::pawn] & r7mask) || posToRank(kingPos[op]) == rank8) {
                    score[side] += bonusRookOn7th;
                    if (pieceBB[side][Piece::rook] & r7mask & ~(1ULL << pos))
                        score[side] += bonusRookOn7thWithAnother;
                }
            }

            // Connected rooks (only the lower-indexed rook pays the bonus)
            const uint64_t otherRooks = pieceBB[side][Piece::rook] & ~(1ULL << pos);
            if (otherRooks && BitScanForward64(pieceBB[side][Piece::rook]) == pos) {
                if (getRookAttackSquares(occupied, pos) & otherRooks)
                    score[side] += bonusConnectedRooks;
            }

            const uint64_t atkSq = getRookAttackSquares(occupied, pos) & ~myBB;
            const int nAtksI = popcount64(atkSq & opInner[side]);
            const int nAtksO = popcount64(atkSq & opOuter[side]);
            if (nAtksI) nAtk20[side] += 20;
            else if (nAtksO) nAtk20[side] += 10;
            totalAtkUnits[side] += nAtksI * 4 + nAtksO * 3;

            if (isSTM && (atkSq & pieceBB[op][Piece::queen]))
                score[side] += bonusThreatOnHigherValuePiece[1];

            score[side] += popcount64(atkSq & ~opPawnAttacks[side]) * mobilityValue[Piece::rook];
        }
    }

    // ── QUEENS ───────────────────────────────────────────────────────────────
    // Tables touched: queenTable, getRookAttackSquares + getBishopAttackSquares
    for (int side = 0; side < 2; side++) {
        const int      op = side ^ 1;
        const int      flip = (side == 0) ? 56 : 0;
        const uint64_t myBB = pieceBB[side][Piece::any];

        uint64_t bb = pieceBB[side][Piece::queen];
        while (bb) {
            const int pos = BitScanForward64(bb);
            bb &= bb - 1;

            phase -= 4;
            score[side] += materialValue[Piece::queen];
            score[side] += queenTable[pos ^ flip];

            const uint64_t atkSq = (getRookAttackSquares(occupied, pos)
                | getBishopAttackSquares(occupied, pos)) & ~myBB;
            const int nAtksI = popcount64(atkSq & opInner[side]);
            const int nAtksO = popcount64(atkSq & opOuter[side]);
            if (nAtksI) nAtk20[side] += 40;  // 2.0 × 20
            else if (nAtksO) nAtk20[side] += 20;  // 1.0 × 20
            totalAtkUnits[side] += nAtksI * 5 + nAtksO * 4;

            score[side] += popcount64(atkSq & ~opPawnAttacks[side]) * mobilityValue[Piece::queen];
        }
    }

    // ── KINGS ────────────────────────────────────────────────────────────────
    // Tables touched: kingTable, getKingAttackSquares
    for (int side = 0; side < 2; side++) {
        const int      flip = (side == 0) ? 56 : 0;
        const uint64_t myBB = pieceBB[side][Piece::any];

        const int pos = kingPos[side];
        score[side] += kingTable[pos ^ flip];

        const uint64_t atkSq = getKingAttackSquares(pos) & ~myBB & ~opPawnAttacks[side];
        const int mob = popcount64(atkSq);
        score[side] += mob * mobilityValue[Piece::king];
        if (mob == 0)
            score[side] -= penaltyNoMobility[Piece::king];
    }

    // ── SPACE ─────────────────────────────────────────────────────────────────
    for (int side = 0; side < 2; side++) {
        const uint64_t myPawns = pieceBB[side][Piece::pawn];

        uint64_t spaceMask;
        if (side == 0) {            // White: forward is +8 => << 8
            spaceMask = myPawns << 8;
            spaceMask |= spaceMask << 8;
            spaceMask |= spaceMask << 16;
            spaceMask |= spaceMask << 32;
        }
        else {                    // Black: forward is -8 => >> 8
            spaceMask = myPawns >> 8;
            spaceMask |= spaceMask >> 8;
            spaceMask |= spaceMask >> 16;
            spaceMask |= spaceMask >> 32;
        }

        // central files/ranks mask (your original)
        spaceMask &= 0x003C3C3C3C3C3C00ULL;

        // strongly recommended: count only empty space
        spaceMask &= ~occupied;

        // don’t count squares controlled by enemy pawns
        spaceMask &= ~opPawnAttacks[side];

        score[side] += popcount64(spaceMask) * bonusSpacePerSquare;
    }

    // ── KING SAFETY ───────────────────────────────────────────────────────────
    // Finalise attacker danger and structural king-shelter penalties.
    for (int side = 0; side < 2; side++) {
        // Danger: attackers outweigh defenders → apply SafetyTable penalty
        if (nAtk20[side] > 0) {
            const int danger20 = nAtk20[side] - nDef20[side];
            if (danger20 > 0)
                rawKingSafety[side] = SafetyTable[totalAtkUnits[side]] * danger20 / 20;
        }

        // Pawn shelter around king
        const int kp = kingPos[side];
        const int kf = fileKing[side];
        const int nPawnsSurround = popcount64(getKingAttackSquares(kp) & pieceBB[side][Piece::pawn]);

        const bool missingCenter = !(arrAfterPawn[side][kp] & pieceBB[side][Piece::pawn]);
        const bool missingRight = (kf < 7) && !(arrAfterPawn[side][kp + 1] & pieceBB[side][Piece::pawn]);
        const bool missingLeft = (kf > 0) && !(arrAfterPawn[side][kp - 1] & pieceBB[side][Piece::pawn]);

        int nSemiOpen;
        if (kf == 0) nSemiOpen = missingCenter + missingRight;
        else if (kf == 7) nSemiOpen = missingCenter + missingLeft;
        else              nSemiOpen = missingCenter + missingLeft + missingRight;

        score[side] += nPawnsSurround * bonusPawnSurroundKing;
        score[side] -= semiOpenFilesPenalty[nSemiOpen];
    }

    // ── PAWN CACHE WRITEBACK ──────────────────────────────────────────────────
    if (!pawnCacheHit)
        storePawnHash(pawnHash, pawnScore);
    score[0] += pawnScore[0];
    score[1] += pawnScore[1];

    // ── PHASE TAPERING ────────────────────────────────────────────────────────
    phase = (phase * 256 + (TOTAL_PHASE / 2)) / TOTAL_PHASE;

    score[0] += rawKingSafety[0];
    score[1] += rawKingSafety[1];

    // ── KING–PAWN TROPISM ─────────────────────────────────────────────────────
    if (sumOfWeightDist[0] > 0)
        score[0] -= (totalWeightedKingDistToOwnPawn[0] / sumOfWeightDist[0]) * kingPawnTropismFactor;
    if (sumOfWeightDist[1] > 0)
        score[1] -= (totalWeightedKingDistToOwnPawn[1] / sumOfWeightDist[1]) * kingPawnTropismFactor;

    score[(int)stm] += tempo;

    // ── FINAL BLEND (MG/EG taper + pawn scaling) ──────────────────────────────
    const int strongerSide = (score[0] < score[1]) ? 1 : 0;
    const int pawnsMissing = 8 - (int)popcount64(pieceBB[strongerSide][Piece::pawn]);
    const int scaleNum = 128 - pawnsMissing * pawnsMissing;  // /128 denominator

    const int eval = score[0] - score[1];
    const int mg = (short)eval;
    const int eg = (eval + 0x8000) >> 16;

    // (mg*(256-phase) + eg*scaleNum*phase/128) / 256
    int ans = (mg * (256 - phase) + eg * scaleNum * phase / 128) / 256;

    if (halve)
        ans >>= 1;

    // ── MOP-UP EVAL (pawnless endgame) ───────────────────────────────────────
    // 4.7f → 47/10, 1.6f → 16/10, combined into one integer division
    if ((pieceBB[0][Piece::pawn] | pieceBB[1][Piece::pawn]) == 0) {
        const int distBetween = abs(fileKing[0] - fileKing[1]) + abs(rankKing[0] - rankKing[1]);
        if (ans >= 0)
            ans += (47 * arrCenterManhattanDistance[kingPos[1]] + 16 * (14 - distBetween)) / 10;
        else
            ans -= (47 * arrCenterManhattanDistance[kingPos[0]] + 16 * (14 - distBetween)) / 10;
    }

    return ans * who2move * 100;
}