#include <iostream>
#include <fstream>
#include <sstream>
#include <random>

#include "chessengine.h"
#include "constant_search.h"

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

void ChessEngine::initTranspositionTable(std::shared_ptr<TranspositionTable> shared_tt) {
    if (shared_tt) {
        ttable = shared_tt; // Use the provided shared TT
    }
    else {
        ttable = std::make_shared<TranspositionTable>(); // Allocate a new isolated one
    }
}

void ChessEngine::parseMoves(std::vector<std::string> moves) {
    for (std::string uciMove : moves) {
        Move move = uciToMove(uciMove);
        addMove(move);
    }
}

Move ChessEngine::uciToMove(std::string uci) {
    MoveVector legalMovesInCurrPos;
    bb.getLegalMovesAlt(legalMovesInCurrPos);
    for (int i = 0; i < legalMovesInCurrPos.size(); i++) {
        if (legalMovesInCurrPos[i].toUci() == uci) {
            return legalMovesInCurrPos[i];
        }
    }
    throw std::runtime_error("No move found in this position");

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
    memset(counterMoveHistory, 0, sizeof(counterMoveHistory));
    memset(followUpHistory, 0, sizeof(followUpHistory));
    memset(captureHistory, 0, sizeof(captureHistory));

}

void ChessEngine::addMove(Move move) {
    moveHistory.push_back(move);
    uint64_t lastPosHash = hashMoveHistory[hashMoveHistory.size() - 1];
    uint64_t currPosHash = bb.move(move, lastPosHash);
    hashMoveHistory.push_back(currPosHash);
}

void ChessEngine::undoMove(Move move) {
    bb.undoMove(move);
    moveHistory.pop_back();
    hashMoveHistory.pop_back();
}

std::string ChessEngine::findBestMove(int depth, int timeLeft, int moveTime, int endDepth) {
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
            int evaluation = iterativeDeepening(depth, timeLeft, moveTime, endDepth);
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

    else {
        int evaluation = iterativeDeepening(depth, timeLeft, moveTime, -1, endDepth);
        return bestMove;
    }
}

void ChessEngine::selectionSort(MoveVector& moves, int* movePriority, int currIndex) {
    int maxIndex = 0;
    int currMaxPriority = -10000000;
    int currPriority;
    for (int i = currIndex; i < moves.size(); i++) {
        currPriority = movePriority[i];
        if (currPriority > currMaxPriority) {
            currMaxPriority = currPriority;
            maxIndex = i;
        }
    }
    std::swap(moves[currIndex], moves[maxIndex]);
    std::swap(movePriority[currIndex], movePriority[maxIndex]);
}

void ChessEngine::moveOrdering(BitBoard& bb, SearchStack* const ss, MoveVector& moves, int pliesFromRoot, Move& currBestMove, int* movePriority) {

    if (!killerMoves[pliesFromRoot][0].isNullMove()) {
        for (int i = 0; i < moves.size(); i++) {
            Piece movePiece = moves[i].getMovePiece();
            Piece capturePiece = moves[i].getCapturePiece();
            Side sideToMove = moves[i].getSideToMove();
            MoveType moveType = moves[i].getMoveType();
            int startPos = moves[i].getStartPos();
            int endPos = moves[i].getEndPos();
            if (moves[i] == currBestMove) {
                movePriority[i] = 1000000;
                continue;
            }
            else if (moves[i] == killerMoves[pliesFromRoot][0]) {
                movePriority[i] = 15000;
                continue;
            }
            else if (moves[i] == killerMoves[pliesFromRoot][1]) {
                movePriority[i] = 14000;
                continue;
            }
            switch (moveType)
            {
            case NORMAL:
            {
                int score = historyTable[sideToMove][movePiece][endPos];
                Move prevMove = pliesFromRoot >= 1 ? ss[pliesFromRoot - 1].move : Move();
                if (!prevMove.isNullMove()) {
                    score += counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos];
                }
                Move prevPrevMove = pliesFromRoot >= 2 ? ss[pliesFromRoot - 2].move : Move();
                if (!prevPrevMove.isNullMove()) {
                    score += followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos];
                }
                movePriority[i] = score;
            }
            break;
            case CAPTURE:
            {
                int capScore = pieceValue[capturePiece] * 110 - pieceValue[movePiece];
                capScore += captureHistory[sideToMove][movePiece][endPos][capturePiece];
                if (pieceValue[capturePiece] >= pieceValue[movePiece])
                    movePriority[i] = capScore;
                else {
                    if (bb.SEE_GE(moves[i], 0))
                        movePriority[i] = capScore;
                    else {
                        movePriority[i] = (pieceValue[capturePiece] - pieceValue[movePiece]) * 100 + captureHistory[sideToMove][movePiece][endPos][capturePiece];
                    }
                }
            }
            break;
            case EN_PASSANT:
                movePriority[i] = 10000;
                break;
            case CASTLE_KINGSIDE:
                movePriority[i] = historyTable[sideToMove][movePiece][endPos];
                break;
            case CASTLE_QUEENSIDE:
                movePriority[i] = historyTable[sideToMove][movePiece][endPos];
                break;
            case PROMOTION_QUEEN:
                movePriority[i] = 45000;
                break;
            case PROMOTION_ROOK:
                movePriority[i] = -30000;
                break;
            case PROMOTION_KNIGHT:
                movePriority[i] = -30000;
                break;
            case PROMOTION_BISHOP:
                movePriority[i] = -30000;
                break;
            case PROMOTION_QUEEN_AND_CAPTURE:
                movePriority[i] = 45000 + pieceValue[capturePiece] * 100;
                break;
            case PROMOTION_ROOK_AND_CAPTURE:
                movePriority[i] = -30000;
                break;
            case PROMOTION_KNIGHT_AND_CAPTURE:
                movePriority[i] = -30000;
                break;
            case PROMOTION_BISHOP_AND_CAPTURE:
                movePriority[i] = -30000;
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
            if (moves[i] == currBestMove) {
                movePriority[i] = 1000000;
                continue;
            }
            switch (moveType)
            {
            case NORMAL:
            {
                int score = historyTable[sideToMove][movePiece][endPos];
                Move prevMove = pliesFromRoot >= 1 ? ss[pliesFromRoot - 1].move : Move();
                if (!prevMove.isNullMove()) {
                    score += counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos];
                }
                Move prevPrevMove = pliesFromRoot >= 2 ? ss[pliesFromRoot - 2].move : Move();
                if (!prevPrevMove.isNullMove()) {
                    score += followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos];
                }
                movePriority[i] = score;
                if (bb.isThreat(moves[i])) {
                    movePriority[i] += 4000;
                }
            }
            break;
            case CAPTURE:
            {
                int capScore = pieceValue[capturePiece] * 110 - pieceValue[movePiece];
                capScore += captureHistory[sideToMove][movePiece][endPos][capturePiece];
                if (pieceValue[capturePiece] >= pieceValue[movePiece])
                    movePriority[i] = capScore;
                else {
                    if (bb.SEE_GE(moves[i], 0))
                        movePriority[i] = capScore;
                    else {
                        movePriority[i] = (pieceValue[capturePiece] - pieceValue[movePiece]) * 100 + captureHistory[sideToMove][movePiece][endPos][capturePiece];
                    }
                }
            }
            break;
            case EN_PASSANT:
                movePriority[i] = 10000;
                break;
            case CASTLE_KINGSIDE:
                movePriority[i] = historyTable[sideToMove][movePiece][endPos];
                break;
            case CASTLE_QUEENSIDE:
                movePriority[i] = historyTable[sideToMove][movePiece][endPos];
                break;
            case PROMOTION_QUEEN:
                movePriority[i] = 45000;
                break;
            case PROMOTION_ROOK:
                movePriority[i] = -30000;
                break;
            case PROMOTION_KNIGHT:
                movePriority[i] = -30000;
                break;
            case PROMOTION_BISHOP:
                movePriority[i] = -30000;
                break;
            case PROMOTION_QUEEN_AND_CAPTURE:
                movePriority[i] = 45000 + pieceValue[capturePiece] * 100;
                break;
            case PROMOTION_ROOK_AND_CAPTURE:
                movePriority[i] = -30000;
                break;
            case PROMOTION_KNIGHT_AND_CAPTURE:
                movePriority[i] = -30000;
                break;
            case PROMOTION_BISHOP_AND_CAPTURE:
                movePriority[i] = -30000;
                break;
            default:
                break;
            }
        }
    }
}
int ChessEngine::qSearch(BitBoard& bb, SearchStack* const ss, int alpha, int beta, int prevEndPos, int pliesFromLeaf, int pliesFromRoot) {
    //qNodeCnt++;
    if (pliesFromRoot > selDepth) selDepth = pliesFromRoot;
    int bestScore = -1000000000;
    bool isKingInCheck = bb.isKingInCheck();
    int standPat = !isKingInCheck ? bb.evaluate(alpha, beta) : bestScore;

    if (standPat > alpha) {
        if (standPat >= beta)
            return standPat;
        alpha = standPat;
    }
    bestScore = standPat;
    int futilityBase = isKingInCheck ? -1000000000 : standPat + futilityMarginQSearch;

    if (pliesFromLeaf > maxPlyQSearch) {
        if (isKingInCheck) {
            auto legalMoves = bb.getLegalMoves();
            if (legalMoves.size() == 0)
                return -1000000000 + pliesFromRoot;
            return bb.evaluate(alpha, beta); // or a rough static eval — position is unclear, assume neutral
        }
        return bestScore; // standPat, which is valid here when not in check
    }
    auto& moves = ss[pliesFromRoot].moves;
    auto& movePriority = ss[pliesFromRoot].movePriority;
    if (isKingInCheck) {
        bb.getLegalMovesAlt(moves);
    }
    else {
        bb.getLegalCapturesAlt(moves);
    }
    if (moves.size() == 0) {
        if (isKingInCheck) {
            return -1000000000 + pliesFromRoot;
        }
        return bestScore;

    }
    Move nullMove = Move();
    moveOrdering(bb, ss, moves, pliesFromRoot, nullMove, movePriority);
    bool isEndgame = bb.isEndgame();
    for (int i = 0; i < moves.size(); i++) {
        selectionSort(moves, movePriority, i);
        MoveType type = moves[i].getMoveType();
        int endPos = moves[i].getEndPos();
        Side sideToMove = moves[i].getSideToMove();
        int priority = movePriority[i];
        if (type == MoveType::CAPTURE && !isKingInCheck && !bb.SEE_GE(moves[i], 0)) {
            continue;
        }
        bool isPawnEndgame = !((bb.pieceBB[Side::White][Piece::any] | bb.pieceBB[Side::Black][Piece::any]) ^ (bb.pieceBB[Side::White][Piece::pawn] | bb.pieceBB[Side::Black][Piece::pawn] | bb.pieceBB[Side::White][Piece::king] | bb.pieceBB[Side::Black][Piece::king]));
        bool pruningCond1 = bestScore > -900000000 && !isPawnEndgame && !isKingInCheck && (endPos != prevEndPos || prevEndPos == -1) &&
            type != MoveType::PROMOTION_QUEEN && type != MoveType::PROMOTION_QUEEN_AND_CAPTURE;


        if (pruningCond1) {
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



            }
            if (priority < 0)
                continue;

            bool pruningCond2 = !(type == MoveType::NORMAL && bb.isThreat(moves[i]));
            if (i > 2 && pruningCond2 && priority < 0)
                continue;

        }
        bb.move(moves[i]);
        int score = -qSearch(bb, ss, -beta, -alpha, endPos, pliesFromLeaf + 1, pliesFromRoot + 1);
        bb.undoMove(moves[i]);
        if (score > bestScore)
            bestScore = score;
        if (score >= beta) {
            return score;
        }
        alpha = std::max(alpha, score);
    }
    return bestScore;
}
int ChessEngine::singularSearch(BitBoard& bb, SearchStack* const ss, int depth, int maxDepth, int alpha, int beta, uint64_t zHash, int pliesFromRoot, bool isPV, Move excluded) {
    bool isKingInCheck = bb.isKingInCheck();
    //generate legal moves
    auto& moves = ss[pliesFromRoot].moves;
    auto& movePriority = ss[pliesFromRoot].movePriority;
    bb.getLegalMovesAlt(moves);
    if (moves.size() == 0) {
        if (isKingInCheck)
            return -1000000000 + pliesFromRoot;
        return 0;
    }
    moveOrdering(bb, ss, moves, pliesFromRoot, excluded, movePriority);
    int staticEval = ss[pliesFromRoot].eval;
    int bestScore = -1e9;
    bool improving = pliesFromRoot >= 2 && staticEval > ss[pliesFromRoot - 2].eval;
    ss[pliesFromRoot].eval = staticEval;
    Side sideToMove = bb.isWhiteTurn ? Side::White : Side::Black;
    Side opSide = sideToMove == Side::White ? Side::Black : Side::White;
    bool lmr = depth >= 3;
    int futilityVal = 0;
    int i;
    int LMPLimit = depth <= 4 ? (improving ? quietsToCheckTable[depth] + 5 : quietsToCheckTable[depth]) : 999;
    int quietMoves = 0;
    bool isRepeated = false;
    bool isEndgame = bb.isEndgame();
    for (i = 0; i < moves.size(); i++) {
        selectionSort(moves, movePriority, i);
        if (i == 0)
            continue;
        isRepeated = false;
        MoveType moveType = moves[i].getMoveType();
        Piece movePiece = moves[i].getMovePiece();
        int endPos = moves[i].getEndPos();
        uint64_t newHash = bb.move(moves[i], zHash);
        ss[pliesFromRoot].move = moves[i];
        int priority = movePriority[i];
        int score = 0;
        if (moveType == MoveType::NORMAL) {
            quietMoves++;
        }
        if (!bb.isRepeatedPosition()) {
            bool canLMR = (lmr && i > 0 && priority < 15001);
            int lmrDepth = depth;
            if (canLMR) {
                float lmrB = isEndgame ? lmrBaseEnd : lmrBaseMid;
                float lmrD = isEndgame ? lmrDivEnd : lmrDivMid;
                int reducedDepth = (log2(i) * log2(depth) / lmrD + lmrB) + (!improving && pliesFromRoot >= 2) - isKingInCheck;

                if (moveType == MoveType::CAPTURE) {
                    Piece capturePiece = moves[i].getCapturePiece();
                    reducedDepth += captureHistory[sideToMove][movePiece][endPos][capturePiece] / captureHistoryReductionFactor;
                    reducedDepth -= bb.SEE_GE(moves[i], 0) ? 1 : -1;
                }
                else {
                    reducedDepth += historyTable[sideToMove][movePiece][endPos] / historyReductionFactor;
                    Move prevMove = pliesFromRoot >= 1 ? ss[pliesFromRoot - 1].move : Move();
                    if (!prevMove.isNullMove()) {
                        reducedDepth += counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] / counterMoveHistoryReductionFactor;
                    }
                    Move prevPrevMove = pliesFromRoot >= 2 ? ss[pliesFromRoot - 2].move : Move();
                    if (!prevPrevMove.isNullMove()) {
                        reducedDepth += followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] / followUpHistoryReductionFactor;
                    }
                }
                lmrDepth = depth - std::clamp(reducedDepth, 1, depth);
            }

            //pruning at low depth
            if (!isKingInCheck && pliesFromRoot > 0 && bestScore > -900000000) {
                futilityVal = staticEval + (bestScore < staticEval ? futilityBaseVal1 : futilityBaseVal2) + futilityMargin * lmrDepth + (improving ? futilityImprovingBonus : 0);
                if (lmrDepth <= futilityDepthLimit && futilityVal <= alpha && moveType == MoveType::NORMAL) {
                    if (bestScore <= futilityVal && abs(bestScore) < 900000000) {
                        bestScore = (bestScore + futilityVal) / 2;
                    }
                    bb.undoMove(moves[i]);
                    break;
                }
                if (quietMoves > LMPLimit) {
                    bb.undoMove(moves[i]);
                    break;
                }
                if (!isKingInCheck && moveType == MoveType::NORMAL && lmrDepth < 8) {
                    bb.undoMove(moves[i]);
                    if (!bb.SEE_GE(moves[i], SEENormalMargin * lmrDepth * lmrDepth)) {
                        continue;
                    }
                    bb.move(moves[i], zHash);
                }


            }
            //Late move reduction, reduce move not high in priority

            if (canLMR) {
                int reducedScore = -search(bb, ss, lmrDepth, maxDepth, -alpha - 1, -alpha, newHash, pliesFromRoot + 1);

                if (reducedScore > alpha && lmrDepth < depth - 1) {
                    score = -search(bb, ss, depth - 1, maxDepth, -alpha - 1, -alpha, newHash, pliesFromRoot + 1, false);
                }
                else {
                    score = reducedScore;
                }
            }
            else {

                //PVS
                score = -search(bb, ss, depth - 1, maxDepth, -alpha - 1, -alpha, newHash, pliesFromRoot + 1, false);
            }
        }
        else {
            isRepeated = true;
        }
        bb.undoMove(moves[i]);
        if (pliesFromRoot == 0 && score > bestScore) {
            bestMove = moves[i].toUci();
            finalBestMoveObj = moves[i];
        }
        if (score >= beta) {

            if (moveType == MoveType::NORMAL) {
                int bonus = staticEval < alpha ? (depth + 1) * (depth + 1) : depth * depth;
                int toAdd = (16 * bonus - historyTable[sideToMove][movePiece][endPos] * bonus / 512);
                historyTable[sideToMove][movePiece][endPos] += toAdd;
                Move prevMove = pliesFromRoot >= 1 ? ss[pliesFromRoot - 1].move : Move();
                if (!prevMove.isNullMove()) {
                    int toAddCM = (16 * bonus - counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] * bonus / 512);
                    counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] += toAddCM;
                }
                Move prevPrevMove = pliesFromRoot >= 2 ? ss[pliesFromRoot - 2].move : Move();
                if (!prevPrevMove.isNullMove()) {
                    int toAddFM = (16 * bonus - followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] * bonus / 512);
                    followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] += toAddFM;
                }
                for (int j = 0; j < i; j++) {
                    MoveType moveType2 = moves[j].getMoveType();
                    Piece movePiece2 = moves[j].getMovePiece();
                    int endPos2 = moves[j].getEndPos();
                    if (moveType2 == MoveType::NORMAL) {
                        int toMinus = (16 * bonus + historyTable[sideToMove][movePiece2][endPos2] * bonus / 512);
                        historyTable[sideToMove][movePiece2][endPos2] -= toMinus;
                        if (!prevMove.isNullMove()) {
                            int toMinusCM = (16 * bonus + counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece2][endPos2] * bonus / 512);
                            counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece2][endPos2] -= toMinusCM;
                        }
                        if (!prevPrevMove.isNullMove()) {
                            int toMinusFM = (16 * bonus + followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece2][endPos2] * bonus / 512);
                            followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece2][endPos2] -= toMinusFM;
                        }
                    }
                }
            }
            else if (moveType == MoveType::CAPTURE) {
                int bonus = staticEval < alpha ? (depth + 1) * (depth + 1) : depth * depth;
                Piece capturePiece = moves[i].getCapturePiece();
                int toAdd = (16 * bonus - captureHistory[sideToMove][movePiece][endPos][capturePiece] * bonus / 512);
                captureHistory[sideToMove][movePiece][endPos][capturePiece] += toAdd;
                for (int j = 0; j < i; j++) {
                    MoveType moveType2 = moves[j].getMoveType();
                    Piece movePiece2 = moves[j].getMovePiece();
                    int endPos2 = moves[j].getEndPos();
                    Piece capturePiece2 = moves[j].getCapturePiece();
                    if (moveType2 == MoveType::CAPTURE) {
                        int toMinus = (16 * bonus + captureHistory[sideToMove][movePiece2][endPos2][capturePiece2] * bonus / 512);
                        captureHistory[sideToMove][movePiece2][endPos2][capturePiece2] -= toMinus;
                    }
                }
            }
            //If the move is not high in priority and it is not in the killer move yet,save it to the killer move array 

            if (priority < 15001 && !(moves[i] == killerMoves[pliesFromRoot][0])) {
                if (moves[i] == killerMoves[pliesFromRoot][1]) {
                    std::swap(killerMoves[pliesFromRoot][0], killerMoves[pliesFromRoot][1]);
                }
                else {
                    killerMoves[pliesFromRoot][1] = killerMoves[pliesFromRoot][0];
                    killerMoves[pliesFromRoot][0] = moves[i];
                }
            }
            return score;
        }
        if (score > bestScore) {
            bestScore = score;
        }
        if (score > alpha) {
            alpha = score;

        }

    }
    return bestScore;

}

int ChessEngine::search(BitBoard& bb, SearchStack* const ss, int depth, int maxDepth, int alpha, int beta, uint64_t zHash, int pliesFromRoot, bool isPV, bool canNullMove) {
    mainNodeCnt++;
    if (pliesFromRoot > selDepth) selDepth = pliesFromRoot; // <--- Add this line
    if ((mainNodeCnt & 0b1111111111) == 0b1111111111) {
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - searchStartTime);
        if (duration.count() > timeLimitInMs) {
            throw 403;
        }
    }
    bool isKingInCheck = bb.isKingInCheck();
    if (depth <= 0 || pliesFromRoot >= maxDepth * 2) {
        return qSearch(bb, ss, alpha, beta, -1, 0, pliesFromRoot);
    }

    Move potentialBestMove = Move();
    TableEntry* entry = ttable->find(zHash);

    bool isPVNode = pliesFromRoot == 0 || (isPV && ss[pliesFromRoot - 1].move == PV[maxDepth - 1][pliesFromRoot - 1]);
    if (isPVNode && (pliesFromRoot & 0b111) == 7 && pliesFromRoot < maxDepth * 2) {
        depth++;
    }
    bool isFoundInTable = entry != nullptr;

    NodeFlag expectedNode = NodeFlag::ALLNODE;
    int entryDepth = 0;
    int entryScore = -1e9;
    int entryEval = 0;
    if (isFoundInTable) {
        tthits++;
        entryDepth = entry->getDepth();
        expectedNode = entry->getNodeFlag();
        entryScore = entry->getScore();
        ttable->adjustScore(entryScore, pliesFromRoot);
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

    if (depth >= IIDDepthLimit && !isFoundInTable) {
        depth -= IIDReductionDepth;
    }
    int bestScore = -1000000000;
    int beginAlpha = alpha;
    bool isEndgame = bb.isEndgame();
    int evalDepthZero = isFoundInTable ? entryEval : bb.evaluate(alpha, beta);
    ss[pliesFromRoot].eval = evalDepthZero;
    bool improving = pliesFromRoot >= 2 && evalDepthZero > ss[pliesFromRoot - 2].eval;
    int staticEval = isFoundInTable ? entryScore : evalDepthZero;
    bool isRepeated = false;
    //reverse futility pruning
    if (!isPVNode && !isKingInCheck && staticEval >= beta && depth <= rfpDepthLimit && canNullMove && pliesFromRoot > 0 && beta > -9e8) {
        int rfpScore = staticEval - rfpMargin * (depth);
        if (rfpScore >= beta) {
            return beta > -900000000 ? (staticEval + beta) / 2 : staticEval;
        }
    }

    
    if (staticEval >= beta && !isKingInCheck && !isPVNode && canNullMove && depth > 4 && pliesFromRoot != 0 && bb.isNullMoveEnable() && beta > -9e8) {
        Move nullMove = Move();
        uint64_t newHash = bb.move(nullMove, zHash);
        ss[pliesFromRoot].move = nullMove;
        int evalNullMove = 0;
        if (!bb.isRepeatedPosition()) {
            evalNullMove = -search(bb, ss, std::clamp(((depth * 100 + (beta - staticEval) / 100) / nullMoveDivision) - 1, 1, depth) - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, false, false);
        }

        bb.undoMove(nullMove);
        if (evalNullMove >= beta) {
            return abs(evalNullMove) > 9e8 ? beta : evalNullMove;
        }
    }
    
    // ProbCut
    if (!isPVNode && !isKingInCheck && depth >= probCutDepthLimit && abs(beta) < 900000000) {
        int probCutBeta = beta + probCutBetaMargin;
        MoveVector probCutMoves;
        bb.getLegalCapturesAlt(probCutMoves);
        Move nullMove2 = Move();
        int probCutPriority[256];
        moveOrdering(bb, ss, probCutMoves, pliesFromRoot, nullMove2, probCutPriority);
        for (int i = 0; i < probCutMoves.size(); i++) {
            selectionSort(probCutMoves, probCutPriority, i);
            MoveType pcMoveType = probCutMoves[i].getMoveType();
            // Only consider captures that pass a quick SEE check
            if (!bb.SEE_GE(probCutMoves[i], (probCutBeta - staticEval) / 100))
                continue;
            uint64_t pcHash = bb.move(probCutMoves[i], zHash);
            ss[pliesFromRoot].move = probCutMoves[i];
            if (!bb.isRepeatedPosition()) {
                // Quick verification search at reduced depth

                // After
                int pcScore = -qSearch(bb, ss, -probCutBeta, -probCutBeta + 1, -1, 0, pliesFromRoot + 1);
                if (pcScore >= probCutBeta)
                    pcScore = -search(bb, ss, depth - 4, maxDepth, -probCutBeta, -probCutBeta + 1, pcHash, pliesFromRoot + 1, false, false);
                if (pcScore >= probCutBeta) {
                    bb.undoMove(probCutMoves[i]);
                    ttable->set(zHash, TableEntry(zHash, probCutMoves[i], NodeFlag::CUTNODE, depth - 3, pcScore, evalDepthZero, pliesFromRoot));
                    return pcScore;
                }
            }
            bb.undoMove(probCutMoves[i]);
        }
    }
    NodeFlag flag = NodeFlag::ALLNODE;
    Move currBestMove = potentialBestMove;
    bool delayedMoveGen = pliesFromRoot >= 1;
    //Delay move generation: test the best move from previous iteration first
    Side sideToMove = bb.isWhiteTurn ? Side::White : Side::Black;
    if (!potentialBestMove.isNullMove() && delayedMoveGen && bb.isValidMove(potentialBestMove)) {
        MoveType moveType = potentialBestMove.getMoveType();
        Piece movePiece = potentialBestMove.getMovePiece();
        int endPos = potentialBestMove.getEndPos();
        int extension = 0;

        if (depth >= SEDepth + isPVNode && expectedNode == NodeFlag::CUTNODE && entryDepth >= depth - 3 && abs(staticEval) < 9e8) {
            int singularBeta = entryScore - depth * SEMargin;
            int singularScore = singularSearch(bb, ss, (depth - 1) / 2, maxDepth, singularBeta - 1, singularBeta, zHash, pliesFromRoot, isPV, potentialBestMove);
            if (singularScore < singularBeta) {
                if (!isPVNode && singularScore < singularBeta - SEDoubleMargin && pliesFromRoot < maxDepth) {
                    extension = 2;
                }
                else {
                    extension = 1;
                }
            }
            else if (singularBeta >= beta) {
                return singularBeta;
            }
            else if (entryScore >= beta) {
                extension = -1 - (entryScore >= beta + singularBetaMargin);
            }

        }

        uint64_t newHash = bb.move(potentialBestMove, zHash);
        ss[pliesFromRoot].move = potentialBestMove;
        int score = 0;

        if (!bb.isRepeatedPosition()) {
            score = -search(bb, ss, depth - 1 + extension, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, isPVNode);
        }
        else
            isRepeated = true;
        bb.undoMove(potentialBestMove);
        if (score >= beta) {
            if (!isRepeated)
                ttable->set(zHash, TableEntry(zHash, potentialBestMove, NodeFlag::CUTNODE, depth, score, evalDepthZero, pliesFromRoot));
            if (moveType == MoveType::NORMAL || moveType == MoveType::CASTLE_KINGSIDE || moveType == MoveType::CASTLE_QUEENSIDE) {
                int depthBonus = depth + (staticEval < alpha);
                int bonus = depthBonus * depthBonus;
                historyTable[sideToMove][movePiece][endPos] += (16 * bonus - historyTable[sideToMove][movePiece][endPos] * bonus / 512);
                Move prevMove = pliesFromRoot >= 1 ? ss[pliesFromRoot - 1].move : Move();
                if (!prevMove.isNullMove()) {
                    int toAddCM = (16 * bonus - counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] * bonus / 512);
                    counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] += toAddCM;
                }
                Move prevPrevMove = pliesFromRoot >= 2 ? ss[pliesFromRoot - 2].move : Move();
                if (!prevPrevMove.isNullMove()) {
                    int toAddFM = (16 * bonus - followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] * bonus / 512);
                    followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] += toAddFM;
                }
            }
            else if (moveType == MoveType::CAPTURE) {
                int depthBonus = depth + (staticEval < alpha);
                int bonus = depthBonus * depthBonus;
                Piece capturePiece = potentialBestMove.getCapturePiece();
                int toAdd = (16 * bonus - captureHistory[sideToMove][movePiece][endPos][capturePiece] * bonus / 512);
                captureHistory[sideToMove][movePiece][endPos][capturePiece] += toAdd;
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
    auto& movePriority = ss[pliesFromRoot].movePriority;
    bb.getLegalMovesAlt(moves);
    if (moves.size() == 0) {
        if (isKingInCheck)
            return -1000000000 + pliesFromRoot;
        return 0;
    }

    moveOrdering(bb, ss, moves, pliesFromRoot, currBestMove, movePriority);

    Side opSide = sideToMove == Side::White ? Side::Black : Side::White;
    bool lmr = !isPVNode && depth >= 3;
    int futilityVal = 0;
    int i;
    int LMPLimit = depth <= LMPDepthLimit ? (improving ? quietsToCheckTable[depth] + 5 : quietsToCheckTable[depth]) : 999;
    int quietMoves = 0;
    for (i = 0; i < moves.size(); i++) {
        selectionSort(moves, movePriority, i);
        if (moves[i] == currBestMove && delayedMoveGen)
            continue;
        isRepeated = false;
        MoveType moveType = moves[i].getMoveType();
        Piece movePiece = moves[i].getMovePiece();
        int endPos = moves[i].getEndPos();
        uint64_t newHash = bb.move(moves[i], zHash);
        ss[pliesFromRoot].move = moves[i];
        int priority = movePriority[i];
        int score = 0;
        if (moveType == MoveType::NORMAL) {
            quietMoves++;
        }
        if (!bb.isRepeatedPosition()) {
            bool canLMR = (lmr && i > 0 && priority < 15001);
            int lmrDepth = depth;
            if (canLMR) {
                float lmrB = isEndgame ? lmrBaseEnd : lmrBaseMid;
                float lmrD = isEndgame ? lmrDivEnd : lmrDivMid;
                int reducedDepth = (log2(i) * log2(depth) / lmrD + lmrB) + (!improving && pliesFromRoot >= 2) - isKingInCheck;

                if (moveType == MoveType::CAPTURE) {
                    Piece capturePiece = moves[i].getCapturePiece();
                    reducedDepth += captureHistory[sideToMove][movePiece][endPos][capturePiece] / captureHistoryReductionFactor;
                    reducedDepth -= bb.SEE_GE(moves[i], 0) ? 1 : -1;
                }
                else {
                    reducedDepth += historyTable[sideToMove][movePiece][endPos] / historyReductionFactor;
                    Move prevMove = pliesFromRoot >= 1 ? ss[pliesFromRoot - 1].move : Move();
                    if (!prevMove.isNullMove()) {
                        reducedDepth += counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] / counterMoveHistoryReductionFactor;
                    }
                    Move prevPrevMove = pliesFromRoot >= 2 ? ss[pliesFromRoot - 2].move : Move();
                    if (!prevPrevMove.isNullMove()) {
                        reducedDepth += followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] / followUpHistoryReductionFactor;
                    }
                }

                lmrDepth = depth - std::clamp(reducedDepth, 1, depth);
            }

            //pruning at low depth
            if (!isKingInCheck && !isPVNode && pliesFromRoot > 0 && abs(bestScore) < 900000000) {
                futilityVal = staticEval + (bestScore < staticEval ? futilityBaseVal1 : futilityBaseVal2) + futilityMargin * lmrDepth + (improving ? futilityImprovingBonus : 0);
                if (lmrDepth <= futilityDepthLimit && futilityVal <= alpha && moveType == MoveType::NORMAL) {
                    if (bestScore <= futilityVal && abs(bestScore) < 900000000) {
                        bestScore = (bestScore + futilityVal) / 2;
                    }
                    bb.undoMove(moves[i]);
                    break;
                }

                if (quietMoves > LMPLimit && priority < 0) {
                    bb.undoMove(moves[i]);
                    break;
                }
                if (!isPVNode && !isKingInCheck && moveType == MoveType::NORMAL && lmrDepth < SEENormalDepthLimit) {
                    bb.undoMove(moves[i]);
                    if (!bb.SEE_GE(moves[i], SEENormalMargin * lmrDepth * lmrDepth)) {
                        continue;
                    }
                    bb.move(moves[i], zHash);
                }



            }
            //Late move reduction, reduce move not high in priority

            if (canLMR) {
                int reducedScore = -search(bb, ss, lmrDepth, maxDepth, -alpha - 1, -alpha, newHash, pliesFromRoot + 1);

                if (reducedScore > alpha && !(beta == alpha + 1 && lmrDepth == depth - 1)) {
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
                    if (score > alpha && beta != alpha + 1) {
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
            finalBestMoveObj = moves[i];
        }
        if (score >= beta) {

            if (moveType == MoveType::NORMAL) {
                int depthBonus = depth + (staticEval < alpha);
                int bonus = depthBonus * depthBonus;
                int toAdd = (16 * bonus - historyTable[sideToMove][movePiece][endPos] * bonus / 512);
                historyTable[sideToMove][movePiece][endPos] += toAdd;
                Move prevMove = pliesFromRoot >= 1 ? ss[pliesFromRoot - 1].move : Move();
                if (!prevMove.isNullMove()) {
                    int toAddCM = (16 * bonus - counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] * bonus / 512);
                    counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] += toAddCM;
                }
                Move prevPrevMove = pliesFromRoot >= 2 ? ss[pliesFromRoot - 2].move : Move();
                if (!prevPrevMove.isNullMove()) {
                    int toAddFM = (16 * bonus - followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] * bonus / 512);
                    followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] += toAddFM;
                }
                for (int j = 0; j < i; j++) {
                    MoveType moveType2 = moves[j].getMoveType();
                    Piece movePiece2 = moves[j].getMovePiece();
                    int endPos2 = moves[j].getEndPos();
                    if (moveType2 == MoveType::NORMAL) {
                        int toMinus = (16 * bonus + historyTable[sideToMove][movePiece2][endPos2] * bonus / 512);
                        historyTable[sideToMove][movePiece2][endPos2] -= toMinus;
                        if (!prevMove.isNullMove()) {
                            int toMinusCM = (16 * bonus + counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece2][endPos2] * bonus / 512);
                            counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece2][endPos2] -= toMinusCM;
                        }
                        if (!prevPrevMove.isNullMove()) {
                            int toMinusFM = (16 * bonus + followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece2][endPos2] * bonus / 512);
                            followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece2][endPos2] -= toMinusFM;
                        }
                    }
                }
            }
            else if (moveType == MoveType::CAPTURE) {
                int depthBonus = depth + (staticEval < alpha);
                int bonus = depthBonus * depthBonus;
                Piece capturePiece = moves[i].getCapturePiece();
                int toAdd = (16 * bonus - captureHistory[sideToMove][movePiece][endPos][capturePiece] * bonus / 512);
                captureHistory[sideToMove][movePiece][endPos][capturePiece] += toAdd;
                for (int j = 0; j < i; j++) {
                    MoveType moveType2 = moves[j].getMoveType();
                    Piece movePiece2 = moves[j].getMovePiece();
                    int endPos2 = moves[j].getEndPos();
                    Piece capturePiece2 = moves[j].getCapturePiece();
                    if (moveType2 == MoveType::CAPTURE) {
                        int toMinus = (16 * bonus + captureHistory[sideToMove][movePiece2][endPos2][capturePiece2] * bonus / 512);
                        captureHistory[sideToMove][movePiece2][endPos2][capturePiece2] -= toMinus;
                    }
                }
            }
            if (!isRepeated)
                ttable->set(zHash, TableEntry(zHash, moves[i], NodeFlag::CUTNODE, depth, score, evalDepthZero, pliesFromRoot));
            //If the move is not high in priority and it is not in the killer move yet,save it to the killer move array 

            if (priority < 15001 && !(moves[i] == killerMoves[pliesFromRoot][0])) {
                if (moves[i] == killerMoves[pliesFromRoot][1]) {
                    std::swap(killerMoves[pliesFromRoot][0], killerMoves[pliesFromRoot][1]);
                }
                else {
                    killerMoves[pliesFromRoot][1] = killerMoves[pliesFromRoot][0];
                    killerMoves[pliesFromRoot][0] = moves[i];
                }
            }
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
    //save to TTable
    if ((!isFoundInTable || entryDepth <= depth)) {
        if (flag == NodeFlag::ALLNODE) {
            ttable->set(zHash, TableEntry(zHash, potentialBestMove, flag, depth, bestScore, evalDepthZero, pliesFromRoot));
        }
        else if (flag == NodeFlag::EXACT) {
            ttable->set(zHash, TableEntry(zHash, currBestMove, flag, depth, bestScore, evalDepthZero, pliesFromRoot));
        }
    }
    if (flag == NodeFlag::EXACT) {
        PV[maxDepth][pliesFromRoot] = currBestMove;
    }
    return bestScore;
}

int ChessEngine::searchDebug(BitBoard& bb, SearchStack* const ss, int depth, int maxDepth, int alpha, int beta, uint64_t zHash, int pliesFromRoot, bool isPV, bool canNullMove) {
    mainNodeCnt++;
    if (pliesFromRoot > selDepth) selDepth = pliesFromRoot; // <--- Add this line
    if ((mainNodeCnt & 0b1111111111) == 0b1111111111) {
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - searchStartTime);
        if (duration.count() > timeLimitInMs) {
            throw 403;
        }
    }
    bool isDebugNode = false;
    if (debugLine.size() == pliesFromRoot) {
        isDebugNode = true;
        for (int j = 0; j < pliesFromRoot; j++) {
            if (ss[j].move.toUci() != debugLine[j]) {
                isDebugNode = false;
                break;
            }
        }
    }

    bool isKingInCheck = bb.isKingInCheck();
    if (isKingInCheck && pliesFromRoot < maxDepth * 2) {
        depth++;
    }
    if (depth <= 0 || pliesFromRoot >= maxDepth * 2) {
        return qSearch(bb, ss, alpha, beta, -1, 0, pliesFromRoot);
    }

    Move potentialBestMove = Move();
    TableEntry* entry = ttable->find(zHash);

    bool isPVNode = pliesFromRoot == 0 || (isPV && ss[pliesFromRoot - 1].move == PV[maxDepth - 1][pliesFromRoot - 1]);
    if (isPVNode && (pliesFromRoot & 0b111) == 7 && pliesFromRoot < maxDepth * 2) {
        depth++;
    }
    bool isFoundInTable = entry != nullptr;

    NodeFlag expectedNode = NodeFlag::ALLNODE;
    int entryDepth = 0;
    int entryScore = -1e9;
    int entryEval = 0;
    if (isFoundInTable) {
        tthits++;
        entryDepth = entry->getDepth();
        expectedNode = entry->getNodeFlag();
        entryScore = entry->getScore();
        ttable->adjustScore(entryScore, pliesFromRoot);
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
    int bestScore = -1000000000;
    int beginAlpha = alpha;
    bool isEndgame = bb.isEndgame();
    int evalDepthZero = isFoundInTable ? entryEval : bb.evaluate(alpha, beta);
    ss[pliesFromRoot].eval = evalDepthZero;
    bool improving = pliesFromRoot >= 2 && evalDepthZero > ss[pliesFromRoot - 2].eval;
    int staticEval = isFoundInTable ? entryScore : evalDepthZero;
    bool isRepeated = false;
    //reverse futility pruning
    if (!isPVNode && !isKingInCheck && staticEval >= beta && depth <= rfpDepthLimit && canNullMove && pliesFromRoot > 0 && beta > -9e8) {
        int rfpScore = staticEval - rfpMargin * (depth);
        if (rfpScore >= beta) {
            return beta > -900000000 ? (staticEval + beta) / 2 : staticEval;
        }
    }


    if (staticEval >= beta && !isKingInCheck && !isPVNode && canNullMove && depth > 4 && pliesFromRoot != 0 && bb.isNullMoveEnable() && beta > -9e8) {
        Move nullMove = Move();
        uint64_t newHash = bb.move(nullMove, zHash);
        ss[pliesFromRoot].move = nullMove;
        int evalNullMove = 0;
        if (!bb.isRepeatedPosition()) {
            evalNullMove = -searchDebug(bb, ss, std::clamp(((depth * 100 + (beta - staticEval) / 100) / nullMoveDivision) - 1, 1, depth) - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, false, false);
        }

        bb.undoMove(nullMove);
        if (evalNullMove >= beta) {
            return abs(evalNullMove) > 9e8 ? beta : evalNullMove;
        }
    }
    // ProbCut
    if (!isPVNode && !isKingInCheck && depth >= 5 && abs(beta) < 900000000) {
        int probCutBeta = beta + 20000;
        MoveVector probCutMoves;
        bb.getLegalCapturesAlt(probCutMoves);
        Move nullMove2 = Move();
        int probCutPriority[256];
        moveOrdering(bb, ss, probCutMoves, pliesFromRoot, nullMove2, probCutPriority);
        for (int i = 0; i < probCutMoves.size(); i++) {
            selectionSort(probCutMoves, probCutPriority, i);
            MoveType pcMoveType = probCutMoves[i].getMoveType();
            // Only consider captures that pass a quick SEE check
            if (!bb.SEE_GE(probCutMoves[i], (probCutBeta - staticEval) / 100))
                continue;
            uint64_t pcHash = bb.move(probCutMoves[i], zHash);
            ss[pliesFromRoot].move = probCutMoves[i];
            if (!bb.isRepeatedPosition()) {
                // Quick verification search at reduced depth

                // After
                int pcScore = -qSearch(bb, ss, -probCutBeta, -probCutBeta + 1, -1, 0, pliesFromRoot + 1);
                if (pcScore >= probCutBeta)
                    pcScore = -searchDebug(bb, ss, depth - 4, maxDepth, -probCutBeta, -probCutBeta + 1, pcHash, pliesFromRoot + 1, false, false);
                if (pcScore >= probCutBeta) {
                    bb.undoMove(probCutMoves[i]);
                    ttable->set(zHash, TableEntry(zHash, probCutMoves[i], NodeFlag::CUTNODE, depth - 3, pcScore, evalDepthZero, pliesFromRoot));
                    return pcScore;
                }
            }
            bb.undoMove(probCutMoves[i]);
        }
    }
    NodeFlag flag = NodeFlag::ALLNODE;
    Move currBestMove = potentialBestMove;
    bool delayedMoveGen = pliesFromRoot >= 1;

    if (isDebugNode) {
        std::cout << "--- DEBUG NODE ---" << std::endl;
        std::cout << "Alpha: " << alpha << " Beta: " << beta << " Depth: " << depth << std::endl;
        std::cout << "Static Eval: " << staticEval << std::endl;
    }
    //Delay move generation: test the best move from previous iteration first
    Side sideToMove = bb.isWhiteTurn ? Side::White : Side::Black;
    if (!potentialBestMove.isNullMove() && delayedMoveGen && bb.isValidMove(potentialBestMove)) {
        MoveType moveType = potentialBestMove.getMoveType();
        Piece movePiece = potentialBestMove.getMovePiece();
        int endPos = potentialBestMove.getEndPos();
        int extension = 0;

        if (depth >= SEDepth + isPVNode && expectedNode == NodeFlag::CUTNODE && entryDepth >= depth - 3 && abs(staticEval) < 9e8) {
            int singularBeta = entryScore - depth * SEMargin;
            int singularScore = singularSearch(bb, ss, (depth - 1) / 2, maxDepth, singularBeta - 1, singularBeta, zHash, pliesFromRoot, isPV, potentialBestMove);
            if (singularScore < singularBeta) {
                if (!isPVNode && singularScore < singularBeta - SEDoubleMargin && pliesFromRoot < maxDepth) {
                    extension = 2;
                }
                else {
                    extension = 1;
                }
            }
            else if (singularBeta >= beta) {
                return singularBeta;
            }
            else if (entryScore >= beta) {
                extension = -1 - (entryScore >= beta + 6000);
            }

        }


        if (isDebugNode) {
            std::cout << "Delayed Move: " << potentialBestMove.toUci() << std::endl;
            if (extension > 0) std::cout << "  Extension: " << extension << std::endl;
            else if (extension < 0) std::cout << "  Reduction: " << -extension << std::endl;
        }
        uint64_t newHash = bb.move(potentialBestMove, zHash);
        ss[pliesFromRoot].move = potentialBestMove;
        int score = 0;

        if (!bb.isRepeatedPosition()) {
            score = -searchDebug(bb, ss, depth - 1 + extension, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, isPVNode);
        }
        else
            isRepeated = true;
        bb.undoMove(potentialBestMove);

        if (isDebugNode) {
            std::cout << "  Score: " << score << std::endl;
        }
        if (score >= beta) {
            if (!isRepeated)
                ttable->set(zHash, TableEntry(zHash, potentialBestMove, NodeFlag::CUTNODE, depth, score, evalDepthZero, pliesFromRoot));
            if (moveType == MoveType::NORMAL || moveType == MoveType::CASTLE_KINGSIDE || moveType == MoveType::CASTLE_QUEENSIDE) {
                int depthBonus = depth + (staticEval < alpha);
                int bonus = depthBonus * depthBonus;
                historyTable[sideToMove][movePiece][endPos] += (16 * bonus - historyTable[sideToMove][movePiece][endPos] * bonus / 512);
                Move prevMove = pliesFromRoot >= 1 ? ss[pliesFromRoot - 1].move : Move();
                if (!prevMove.isNullMove()) {
                    int toAddCM = (16 * bonus - counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] * bonus / 512);
                    counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] += toAddCM;
                }
                Move prevPrevMove = pliesFromRoot >= 2 ? ss[pliesFromRoot - 2].move : Move();
                if (!prevPrevMove.isNullMove()) {
                    int toAddFM = (16 * bonus - followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] * bonus / 512);
                    followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] += toAddFM;
                }
            }
            else if (moveType == MoveType::CAPTURE) {
                int depthBonus = depth + (staticEval < alpha);
                int bonus = depthBonus * depthBonus;
                Piece capturePiece = potentialBestMove.getCapturePiece();
                int toAdd = (16 * bonus - captureHistory[sideToMove][movePiece][endPos][capturePiece] * bonus / 512);
                captureHistory[sideToMove][movePiece][endPos][capturePiece] += toAdd;
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
    auto& movePriority = ss[pliesFromRoot].movePriority;
    bb.getLegalMovesAlt(moves);
    if (moves.size() == 0) {
        if (isKingInCheck)
            return -1000000000 + pliesFromRoot;
        return 0;
    }

    moveOrdering(bb, ss, moves, pliesFromRoot, currBestMove, movePriority);

    if (isDebugNode) {
        std::cout << "Killer Moves: " << killerMoves[pliesFromRoot][0].toUci() << ", " << killerMoves[pliesFromRoot][1].toUci() << std::endl;
        std::cout << "Moves List Priorities (Unsorted):" << std::endl;
        for (int debug_i = 0; debug_i < moves.size(); debug_i++) {
            std::cout << "  " << moves[debug_i].toUci() << ": " << movePriority[debug_i] << std::endl;
        }
    }

    Side opSide = sideToMove == Side::White ? Side::Black : Side::White;
    bool lmr = !isPVNode && depth >= 3;
    int futilityVal = 0;
    int i;
    int LMPLimit = depth <= 4 ? (improving ? quietsToCheckTable[depth] + 5 : quietsToCheckTable[depth]) : 999;
    int quietMoves = 0;
    for (i = 0; i < moves.size(); i++) {
        selectionSort(moves, movePriority, i);
        if (isDebugNode) {
            std::cout << "Move: " << moves[i].toUci() << " Priority: " << movePriority[i] << std::endl;
            MoveType moveType = moves[i].getMoveType();
            Piece movePiece = moves[i].getMovePiece();
            int endPos = moves[i].getEndPos();
            if (moveType == MoveType::NORMAL) {
                std::cout << "  History: " << historyTable[sideToMove][movePiece][endPos] << std::endl;
                Move prevMove = pliesFromRoot >= 1 ? ss[pliesFromRoot - 1].move : Move();
                if (!prevMove.isNullMove()) {
                    std::cout << "  CounterMoveHistory: " << counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] << std::endl;
                }
                Move prevPrevMove = pliesFromRoot >= 2 ? ss[pliesFromRoot - 2].move : Move();
                if (!prevPrevMove.isNullMove()) {
                    std::cout << "  FollowUpHistory: " << followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] << std::endl;
                }
            }
            else if (moveType == MoveType::CAPTURE) {
                Piece capturePiece = moves[i].getCapturePiece();
                std::cout << "  CaptureHistory: " << captureHistory[sideToMove][movePiece][endPos][capturePiece] << std::endl;
            }
        }
        if (moves[i] == currBestMove && delayedMoveGen) {
            if (isDebugNode) std::cout << "  (Skipped, already searched in delayed move gen)" << std::endl;
        }

        if (moves[i] == currBestMove && delayedMoveGen)
            continue;
        isRepeated = false;
        MoveType moveType = moves[i].getMoveType();
        Piece movePiece = moves[i].getMovePiece();
        int endPos = moves[i].getEndPos();
        uint64_t newHash = bb.move(moves[i], zHash);
        ss[pliesFromRoot].move = moves[i];
        int priority = movePriority[i];
        int score = 0;
        if (moveType == MoveType::NORMAL) {
            quietMoves++;
        }
        if (!bb.isRepeatedPosition()) {
            bool canLMR = (lmr && i > 0 && priority < 15001);
            int lmrDepth = depth;
            if (canLMR) {
                float lmrB = isEndgame ? lmrBaseEnd : lmrBaseMid;
                float lmrD = isEndgame ? lmrDivEnd : lmrDivMid;
                int reducedDepth = (log2(i) * log2(depth) / lmrD + lmrB) + (!improving && pliesFromRoot >= 2) - isKingInCheck;

                if (moveType == MoveType::CAPTURE) {
                    Piece capturePiece = moves[i].getCapturePiece();
                    reducedDepth += captureHistory[sideToMove][movePiece][endPos][capturePiece] / captureHistoryReductionFactor;
                    reducedDepth -= bb.SEE_GE(moves[i], 0) ? 1 : -1;
                }
                else {
                    reducedDepth += historyTable[sideToMove][movePiece][endPos] / historyReductionFactor;
                    Move prevMove = pliesFromRoot >= 1 ? ss[pliesFromRoot - 1].move : Move();
                    if (!prevMove.isNullMove()) {
                        reducedDepth += counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] / counterMoveHistoryReductionFactor;
                    }
                    Move prevPrevMove = pliesFromRoot >= 2 ? ss[pliesFromRoot - 2].move : Move();
                    if (!prevPrevMove.isNullMove()) {
                        reducedDepth += followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] / followUpHistoryReductionFactor;
                    }
                }

                lmrDepth = depth - std::clamp(reducedDepth, 1, depth);
            }
            if (isDebugNode) {
                if (canLMR) {
                    std::cout << "  LMR Depth: " << lmrDepth << " (Reduced by " << depth - lmrDepth << ")" << std::endl;
                }
                else {
                    std::cout << "  LMR Depth: None (Full Depth " << depth << ")" << std::endl;
                }
            }


            //pruning at low depth
            if (!isKingInCheck && !isPVNode && pliesFromRoot > 0 && abs(bestScore) < 900000000) {
                futilityVal = staticEval + (bestScore < staticEval ? futilityBaseVal1 : futilityBaseVal2) + futilityMargin * lmrDepth + (improving ? futilityImprovingBonus : 0);
                if (lmrDepth <= futilityDepthLimit && futilityVal <= alpha && moveType == MoveType::NORMAL) {
                    if (bestScore <= futilityVal && abs(bestScore) < 900000000) {
                        bestScore = (bestScore + futilityVal) / 2;
                    }
                    bb.undoMove(moves[i]);
                    if (isDebugNode) {
                        if (isRepeated) std::cout << "  Score: Repeated Position (0)" << std::endl;
                        else std::cout << "  Score: " << score << std::endl;
                    }

                    break;
                }

                if (quietMoves > LMPLimit && priority < 0) {
                    bb.undoMove(moves[i]);
                    break;
                }
                if (!isPVNode && !isKingInCheck && moveType == MoveType::NORMAL && lmrDepth < SEENormalDepthLimit) {
                    bb.undoMove(moves[i]);
                    if (!bb.SEE_GE(moves[i], SEENormalMargin * lmrDepth * lmrDepth)) {
                        continue;
                    }
                    bb.move(moves[i], zHash);
                }
            }
            //Late move reduction, reduce move not high in priority

            if (canLMR) {
                int reducedScore = -searchDebug(bb, ss, lmrDepth, maxDepth, -alpha - 1, -alpha, newHash, pliesFromRoot + 1);

                if (reducedScore > alpha && !(beta == alpha + 1 && lmrDepth == depth - 1)) {
                    score = -searchDebug(bb, ss, depth - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1);
                }
                else {
                    score = reducedScore;
                }
            }
            else {

                //PVS
                if (i >= 1) {
                    score = -searchDebug(bb, ss, depth - 1, maxDepth, -alpha - 1, -alpha, newHash, pliesFromRoot + 1, isPVNode);
                    if (score > alpha && beta != alpha + 1) {
                        score = -searchDebug(bb, ss, depth - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, isPVNode);
                    }
                }
                else {
                    score = -searchDebug(bb, ss, depth - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, isPVNode);
                }
            }
        }
        else {
            isRepeated = true;

        }
        bb.undoMove(moves[i]);

        if (isDebugNode) {
            std::cout << "  -> Score for move " << moves[i].toUci() << ": " << score << std::endl;
        }

        if (pliesFromRoot == 0 && score > alpha) {
            bestMove = moves[i].toUci();
            finalBestMoveObj = moves[i];
        }
        if (score >= beta) {

            if (moveType == MoveType::NORMAL) {
                int depthBonus = depth + (staticEval < alpha);
                int bonus = depthBonus * depthBonus;
                int toAdd = (16 * bonus - historyTable[sideToMove][movePiece][endPos] * bonus / 512);
                historyTable[sideToMove][movePiece][endPos] += toAdd;
                Move prevMove = pliesFromRoot >= 1 ? ss[pliesFromRoot - 1].move : Move();
                if (!prevMove.isNullMove()) {
                    int toAddCM = (16 * bonus - counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] * bonus / 512);
                    counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece][endPos] += toAddCM;
                }
                Move prevPrevMove = pliesFromRoot >= 2 ? ss[pliesFromRoot - 2].move : Move();
                if (!prevPrevMove.isNullMove()) {
                    int toAddFM = (16 * bonus - followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] * bonus / 512);
                    followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece][endPos] += toAddFM;
                }
                for (int j = 0; j < i; j++) {
                    MoveType moveType2 = moves[j].getMoveType();
                    Piece movePiece2 = moves[j].getMovePiece();
                    int endPos2 = moves[j].getEndPos();
                    if (moveType2 == MoveType::NORMAL) {
                        int toMinus = (16 * bonus + historyTable[sideToMove][movePiece2][endPos2] * bonus / 512);
                        historyTable[sideToMove][movePiece2][endPos2] -= toMinus;
                        if (!prevMove.isNullMove()) {
                            int toMinusCM = (16 * bonus + counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece2][endPos2] * bonus / 512);
                            counterMoveHistory[prevMove.getMovePiece()][prevMove.getEndPos()][movePiece2][endPos2] -= toMinusCM;
                        }
                        if (!prevPrevMove.isNullMove()) {
                            int toMinusFM = (16 * bonus + followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece2][endPos2] * bonus / 512);
                            followUpHistory[prevPrevMove.getMovePiece()][prevPrevMove.getEndPos()][movePiece2][endPos2] -= toMinusFM;
                        }
                    }
                }
            }
            else if (moveType == MoveType::CAPTURE) {
                int depthBonus = depth + (staticEval < alpha);
                int bonus = depthBonus * depthBonus;
                Piece capturePiece = moves[i].getCapturePiece();
                int toAdd = (16 * bonus - captureHistory[sideToMove][movePiece][endPos][capturePiece] * bonus / 512);
                captureHistory[sideToMove][movePiece][endPos][capturePiece] += toAdd;
                for (int j = 0; j < i; j++) {
                    MoveType moveType2 = moves[j].getMoveType();
                    Piece movePiece2 = moves[j].getMovePiece();
                    int endPos2 = moves[j].getEndPos();
                    Piece capturePiece2 = moves[j].getCapturePiece();
                    if (moveType2 == MoveType::CAPTURE) {
                        int toMinus = (16 * bonus + captureHistory[sideToMove][movePiece2][endPos2][capturePiece2] * bonus / 512);
                        captureHistory[sideToMove][movePiece2][endPos2][capturePiece2] -= toMinus;
                    }
                }
            }
            if (!isRepeated)
                ttable->set(zHash, TableEntry(zHash, moves[i], NodeFlag::CUTNODE, depth, score, evalDepthZero, pliesFromRoot));
            //If the move is not high in priority and it is not in the killer move yet,save it to the killer move array 

            if (priority < 15001 && !(moves[i] == killerMoves[pliesFromRoot][0])) {
                if (moves[i] == killerMoves[pliesFromRoot][1]) {
                    std::swap(killerMoves[pliesFromRoot][0], killerMoves[pliesFromRoot][1]);
                }
                else {
                    killerMoves[pliesFromRoot][1] = killerMoves[pliesFromRoot][0];
                    killerMoves[pliesFromRoot][0] = moves[i];
                }
            }
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
    //save to TTable
    if ((!isFoundInTable || entryDepth <= depth)) {
        if (flag == NodeFlag::ALLNODE) {
            ttable->set(zHash, TableEntry(zHash, potentialBestMove, flag, depth, bestScore, evalDepthZero, pliesFromRoot));
        }
        else if (flag == NodeFlag::EXACT) {
            ttable->set(zHash, TableEntry(zHash, currBestMove, flag, depth, bestScore, evalDepthZero, pliesFromRoot));
        }
    }
    if (flag == NodeFlag::EXACT) {
        PV[maxDepth][pliesFromRoot] = currBestMove;
    }
    if (isDebugNode) std::cout << "Returned Score: " << bestScore << std::endl;
    return bestScore;
}

int ChessEngine::iterativeDeepening(int startDepth, int timeLeft, int moveTime, int movesToGo, int endDepth) {
    //originalBb = bb;
    selDepth = 0;
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
        int baseTime = 0.025 * static_cast<double>(timeLeft);
        int nPieces = popcount64(bb.pieceBB[Side::Black][Piece::any] | bb.pieceBB[Side::White][Piece::any]);
        int bonusMiddleGame = 0.002 * (static_cast<float>(nPieces) / 5.0) * static_cast<double>(timeLeft);
        int bonusEvalDrawish = currEval != 0 ? 0.0025 * (6.0 - static_cast<float>(abs(currEval)) / 5000) * static_cast<double>(timeLeft) : 0;
        int movesToGoBonus = 0;
        if (movesToGo >= 10 && movesToGo < 15)
            movesToGoBonus = baseTime;
        if (movesToGo >= 5 && movesToGo < 10)
            movesToGoBonus = baseTime * 3;
        if (movesToGo >= 0 && movesToGo < 5)
            movesToGoBonus = baseTime * 7;
        bonusEvalDrawish = std::max(bonusEvalDrawish, 0);
        time = baseTime + bonusMiddleGame + bonusEvalDrawish + (movesToGo < 15 ? movesToGoBonus : 0);
    }
    timeLimitInMs = time;
    int currEval = 0;
    int prevEval = 1000000002;
    tthits = 0;
    uint64_t zHash = bb.getCurrentPositionHash();
    ttable->currRootAge = bb.moveNum;
    std::string lastBestMove;
    searchStartTime = std::chrono::steady_clock::now();
    SearchStack ss[128];
    const bool isEnableAspiration = true;
    for (int i = startDepth; i <= std::min(63, endDepth); i++) {
        bool timeout = false;
        //Aspiration window
        if (prevEval != 1000000002 && isEnableAspiration) {
            int beginWindow = 4000;
            int upperWindow = beginWindow;
            int lowerWindow = beginWindow;
            int power = 1;
            int lowerBound = prevEval - lowerWindow;
            int upperBound = prevEval + upperWindow;
            while (true) {
                try
                {
                    if (debugLine.size() > 0) {
                        currEval = searchDebug(bb, ss, i, i, lowerBound, upperBound, zHash, 0, true);
                    }
                    else {
                        currEval = search(bb, ss, i, i, lowerBound, upperBound, zHash, 0, true);
                    }
                }
                catch (...)
                {
                    timeout = true;
                    //bestMove = lastBestMove;
                    break;
                }
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
                //std::cout << std::endl;
                lowerBound = prevEval - lowerWindow;
                upperBound = prevEval + upperWindow;
                if (power >= 4) {
                    upperBound = 1000000001;
                    lowerBound = -1000000001;
                }

            }
        }
        //if this is the first iteration,do a full window search
        else {
            if (debugLine.size() > 0) {
                currEval = searchDebug(bb, ss, i, i, -1000000001, 1000000001, zHash, 0, true);
            }
            else {
                currEval = search(bb, ss, i, i, -1000000001, 1000000001, zHash, 0, true);
            }
            lastBestMove = bestMove;
        }
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 7; k++) {
                for (int l = 0; l < 64; l++) {
                    historyTable[j][k][l] /= 8;
                }
            }
        }
        // 1) Decay captureHistory[side][movePiece][toSq][capturedPiece]
        for (int s = 0; s < 2; s++) {
            for (int mp = 0; mp < 7; mp++) {
                for (int to = 0; to < 64; to++) {
                    for (int cp = 0; cp < 7; cp++) {
                        captureHistory[s][mp][to][cp] /= 8;
                    }
                }
            }
        }

        // 2) Decay counterMoveHistory[prevPiece][prevTo][movePiece][to]
        for (int p1 = 0; p1 < 7; p1++) {
            for (int sq1 = 0; sq1 < 64; sq1++) {
                for (int p2 = 0; p2 < 7; p2++) {
                    for (int sq2 = 0; sq2 < 64; sq2++) {
                        counterMoveHistory[p1][sq1][p2][sq2] /= 8;
                    }
                }
            }
        }

        // 3) Decay followUpHistory[prevPrevPiece][prevPrevTo][movePiece][to]
        for (int p1 = 0; p1 < 7; p1++) {
            for (int sq1 = 0; sq1 < 64; sq1++) {
                for (int p2 = 0; p2 < 7; p2++) {
                    for (int sq2 = 0; sq2 < 64; sq2++) {
                        followUpHistory[p1][sq1][p2][sq2] /= 8;
                    }
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
            //finalBestMoveObj = PV[i][0];
            //mate eval info 
            std::string infoEval = "";
            if (currEval > 1e8) {
                infoEval = "mate " + std::to_string((100 - currEval % 100) / 2);
            }
            else if (currEval < -1e8) {
                infoEval = "mate " + std::to_string((-100 - currEval % 100) / 2);
            }
            else {
                infoEval = "cp " + std::to_string(currEval / 100);
            }
            std::cout << "info score " << infoEval << " depth " << i << " seldepth " << selDepth << " time " << duration.count() << " nps " << (int)nps << " tthits " << tthits << " nodes " << mainNodeCnt << " hashfull " << ttable->hashfull();
            /*
            std::cout << " pv ";
            for (int j = 0; j < 128; j++) {
                if (PV[i][j].isNullMove())
                    break;
                std::cout << PV[i][j].toUci() << " ";
            }
            */
            std::cout << std::endl;
        }

        if (duration.count() > time)
            break;
        //bonus time for variation,only when depth > 5 and not in "go movetime" command
        else if (i >= 5 && moveTime == -1 && PV[i][0] != PV[i - 1][0]) {
            timeLimitInMs += (0.006) * static_cast<double>(timeLeft);
        }
        prevEval = currEval;

    }
    this->currEval = currEval;
    for (int i = 0; i < 32; i++) {
        killerMoves[i][0] = Move();
        killerMoves[i][1] = Move();
    }
    nMovesOutOfBook++;
    qNodeCnt = 0;
    mainNodeCnt = 0;
    //bb = originalBb;
    return currEval;
}
