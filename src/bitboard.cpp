#include "bitboard.h"
#include "constant_bitboard.h"
#include "constant_eval.h"
#include "constant_hash.h"
#include <sstream>

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

int BitBoard::evaluate(int alpha, int beta) {
    uint64_t occupied = (pieceBB[Side::White][Piece::any] | pieceBB[Side::Black][Piece::any]);

    int who2move = (isWhiteTurn ? 1 : -1);
    int totalOcc = popcount64((pieceBB[Side::White][Piece::any] | pieceBB[Side::Black][Piece::any]));


    int score[2]{ 0 };
    int phase = TOTAL_PHASE;

    int kingPos[2] = {
        BitScanForward64(pieceBB[Side::White][Piece::king]),
        BitScanForward64(pieceBB[Side::Black][Piece::king])
    };
    int rankKing[2] = { posToRank(kingPos[0]) , posToRank(kingPos[1]) };
    int fileKing[2] = { posToFile(kingPos[0]) , posToFile(kingPos[1]) };
    bool halve = false;
    if (totalOcc <= 6 && (pieceBB[0][Piece::queen] | pieceBB[1][Piece::queen]) == 0) {
        int diffQueens = static_cast<int>(popcount64(pieceBB[Side::White][Piece::queen]) - popcount64(pieceBB[Side::Black][Piece::queen]))
            * pieceValue[Piece::queen];
        int diffRooks = static_cast<int>(popcount64(pieceBB[Side::White][Piece::rook]) - popcount64(pieceBB[Side::Black][Piece::rook]))
            * pieceValue[Piece::rook];
        int diffKnights = static_cast<int>(popcount64(pieceBB[Side::White][Piece::knight]) - popcount64(pieceBB[Side::Black][Piece::knight]))
            * pieceValue[Piece::knight];
        int diffBishops = static_cast<int>(popcount64(pieceBB[Side::White][Piece::bishop]) - popcount64(pieceBB[Side::Black][Piece::bishop]))
            * pieceValue[Piece::bishop];
        int diffMaterial = diffQueens + diffRooks + diffKnights + diffBishops;
        if ((pieceBB[0][Piece::pawn] | pieceBB[1][Piece::pawn]) == 0) {
            if (abs(diffMaterial) <= 300) {
                halve = true;
            }
        }
    }
    int totalWeightedKingDistToOwnPawn[2]{ 0 };
    int sumOfWeightDist[2]{ 0 };

    uint64_t attacks = 0;
    uint64_t isDoublePawn = false;
    bool isIsolatedPawn = false;
    Side stm = isWhiteTurn ? Side::White : Side::Black;
    uint64_t pawnSet = pieceBB[0][Piece::pawn] | pieceBB[1][Piece::pawn];
    uint64_t allPieces[2] = {
        pieceBB[0][Piece::any] & ~pieceBB[0][Piece::pawn],
        pieceBB[1][Piece::any] & ~pieceBB[1][Piece::pawn]
    };
    uint64_t allHeavyPieces[2] = {
        allPieces[0] & ~(pieceBB[0][Piece::bishop] | pieceBB[0][Piece::knight]),
        allPieces[1] & ~(pieceBB[1][Piece::bishop] | pieceBB[1][Piece::knight])
    };
    int nMobility;
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
        float nDefenders = static_cast<float>(popcount64(pieceBB[!side][Piece::pawn] & opKingInnerRing)) * 0.5f
            + static_cast<float>(popcount64(pieceBB[!side][Piece::pawn] & opKingOuterRing)) * 0.2f;
        uint64_t pieces = pieceBB[!side][Piece::any] ^ pieceBB[!side][Piece::pawn];
        nDefenders += popcount64(pieces & opKingInnerRing) * 1.0f + popcount64(pieces & opKingOuterRing) * 0.5f;
        int totalAtkUnits = 0;
        int nAtksOnInnerRing = 0;
        int nAtksOnOuterRing = 0;
        int nDefsOnInnerRing = 0;
        int pawnPushOffset = side == Side::White ? 8 : -8;
        bool sideIsSTM = side == stm;
        Side opSide = static_cast<Side>(!side);
        uint64_t opPawnSetAtkSq = side == Side::Black ?
            (((pieceBB[!side][Piece::pawn] << 7ull) & ~0x8080808080808080ull) | ((pieceBB[!side][Piece::pawn] << 9ull) & ~0x0101010101010101ull)) :
            (((pieceBB[!side][Piece::pawn] >> 7ull) & ~0x0101010101010101ull) | ((pieceBB[!side][Piece::pawn] >> 9ull) & ~0x8080808080808080ull));
        opPawnSetAtkSq = opPawnSetAtkSq & ~allPieces[opSide];
        if (popcount64(pieceBB[side][Piece::bishop]) == 2) {
            score[side] += bishopPairBonus;
        }
        while (occupied2) {
            int pos = BitScanForward64(occupied2);
            occupied2 ^= (1ull << pos);

            Piece piece = pieceTable[pos];
            //Material
            score[side] += materialValue[piece];

            int posFlipped = pos ^ flip;
            if (piece == Piece::pawn) {
                int rankPawn = posToRank(pos);
                int filePawn = posToFile(pos);
                int weight = 3;
                int manhattanDist = std::abs(rankKing[side] - rankPawn) + std::abs(fileKing[side] - filePawn);
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
                attacks = arrPawnAttacks[side][pos];
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 0.5f;
                else if (nAtksOnOuterRing)
                    nAttackers += 0.25f;
                totalAtkUnits += nAtksOnInnerRing * 2 + nAtksOnOuterRing;
                if (sideIsSTM && (attacks & allPieces[opSide])) {
                    score[side] += bonusThreatOnHigherValuePiece[1];
                }
                if (popcount64(attacks) == 0) {
                    score[side] -= penaltyNoMobility[Piece::pawn];
                }
            }
            else if (piece == Piece::knight) {
                phase--;
                score[side] += knightTable[posFlipped];
                attacks = getKnightAttackSquares(pos) & ~(myPiece | opPawnSetAtkSq);
                if (sideIsSTM && (attacks & allHeavyPieces[opSide])) {
                    score[side] += bonusThreatOnHigherValuePiece[1];
                }
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 1.0f;
                else if (nAtksOnOuterRing)
                    nAttackers += 0.5f;
                totalAtkUnits += nAtksOnInnerRing * 3 + nAtksOnOuterRing * 2;
                nMobility = popcount64(attacks);
                score[side] += nMobility * mobilityValue[Piece::knight];
                if (nMobility == 0) {
                    score[side] -= penaltyNoMobility[Piece::knight];
                }
            }
            else if (piece == Piece::bishop) {
                phase--;
                score[side] += bishopTable[posFlipped];
                attacks = getBishopAttackSquares(occupied, pos) & ~(myPiece | opPawnSetAtkSq);
                if (sideIsSTM && (attacks & allHeavyPieces[opSide])) {
                    score[side] += bonusThreatOnHigherValuePiece[1];
                }
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 1.0f;
                else if (nAtksOnOuterRing)
                    nAttackers += 0.5f;
                totalAtkUnits += nAtksOnInnerRing * 3 + nAtksOnOuterRing * 2;
                score[side] += popcount64(attacks) * mobilityValue[Piece::bishop];
            }
            else if (piece == Piece::rook) {
                phase -= 2;
                score[side] += rookTable[posFlipped];
                if (!(arrAfterPawn[side][pos] & pawnSet))
                    score[side] += bonusRookOnOpenFile;
                else if (!(arrAfterPawn[side][pos] & pieceBB[side][Piece::pawn]))
                    score[side] += bonusRookOnSemiOpenFile;
                attacks = getRookAttackSquares(occupied, pos) & ~(myPiece | opPawnSetAtkSq);
                if (sideIsSTM && (attacks & pieceBB[opSide][Piece::queen])) {
                    score[side] += bonusThreatOnHigherValuePiece[1];
                }
                nAtksOnInnerRing = popcount64(attacks & opKingInnerRing);
                nAtksOnOuterRing = popcount64(attacks & opKingOuterRing);
                if (nAtksOnInnerRing)
                    nAttackers += 1.0f;
                else if (nAtksOnOuterRing)
                    nAttackers += 0.5f;
                totalAtkUnits += nAtksOnInnerRing * 4 + nAtksOnOuterRing * 3;
                score[side] += popcount64(attacks) * mobilityValue[Piece::rook];
            }
            else if (piece == Piece::queen) {
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
            }
            else if (piece == Piece::king) {
                score[side] += kingTable[posFlipped];
                attacks = getKingAttackSquares(pos) & ~(myPiece | opPawnSetAtkSq);
                nMobility = popcount64(attacks);
                score[side] += nMobility * mobilityValue[Piece::king];
                if (nMobility == 0) {
                    score[side] -= penaltyNoMobility[Piece::king];
                }
            }

        }

        if (nAttackers >= 3) {
            score[side] += SafetyTable[totalAtkUnits] * (nAttackers - nDefenders);
        }
    }
    //king safety-related
    uint64_t pawnsBBAll = pieceBB[Side::White][Piece::pawn] | pieceBB[Side::Black][Piece::pawn];
    for (int side = 0; side < 2; side++) {
        uint64_t kingAttacks = getKingAttackSquares(kingPos[side]);
        int nPawnsSurroundKing = popcount64(kingAttacks & pieceBB[side][Piece::pawn]);
        int nSemiOpenFilesKing = 0;
        if (fileKing[side] == 0) {
            nSemiOpenFilesKing = (!(arrAfterPawn[side][kingPos[side]] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[side][kingPos[side] + 1] & pieceBB[side][Piece::pawn]));
        }
        else if (fileKing[side] == 7) {
            nSemiOpenFilesKing = (!(arrAfterPawn[side][kingPos[side]] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[side][kingPos[side] - 1] & pieceBB[side][Piece::pawn]));
        }
        else {
            nSemiOpenFilesKing = (!(arrAfterPawn[side][kingPos[side]] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[side][kingPos[side] + 1] & pieceBB[side][Piece::pawn])) + (!(arrAfterPawn[side][kingPos[side] - 1] & pieceBB[side][Piece::pawn]));
        }
        score[side] += nPawnsSurroundKing * bonusPawnSurroundKing;
        score[side] -= semiOpenFilesPenalty[nSemiOpenFilesKing];
    }

    phase = (phase * 256 + (TOTAL_PHASE / 2)) / TOTAL_PHASE;
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
    if (halve)
        ans >>= 1;
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