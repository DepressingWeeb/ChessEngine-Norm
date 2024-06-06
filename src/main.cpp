#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <array>
#include <chrono>
#include <random>
#include <algorithm>
#include "constant_eval.h"
#include "constant_search.h"
#include "bitboard.h"
#include <thread>
const std::string PATH = "./";

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
    Move PV[64][128];
    int mainNodeCnt = 0;
    int qNodeCnt = 0;
    int cnt[100]{ 0 };
    int nMovesOutOfBook;
    std::chrono::steady_clock::time_point searchStartTime;
    int timeLimitInMs;

public:
    std::string bestMove;
    int currEval;
    std::vector<std::string> debugLine;
    BitBoard bb;
    ChessEngine();
    ChessEngine(std::string fen);
    void addMove(Move move);
    void undoMove(Move move);
    void parseFEN(std::string fen);
    Move uciToMove(std::string uci);
    void selectionSort(MoveVector& moves,int* movePriority, int currIndex);
    void parseMoves(std::vector<std::string> moves);
    int qSearch(BitBoard& bb, SearchStack* const ss, int alpha, int beta, int prevEndPos, int pliesFromLeaf, int pliesFromRoot);
    int search(BitBoard& bb, SearchStack* const ss, int depth, int maxDepth, int alpha, int beta, uint64_t zHash, int pliesFromRoot, bool isPV = false, bool canNullMove = true);
    int iterativeDeepening(int depth, int timeLeft = -1, int moveTime = -1);
    int singularSearch(BitBoard& bb, SearchStack* const ss, int depth, int maxDepth, int alpha, int beta, uint64_t zHash, int pliesFromRoot, bool isPV, Move excluded);
    void moveOrdering(BitBoard& bb, MoveVector& moves, int pliesFromRoot, Move& currBestMove,int* movePriority);
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

    else {
        int evaluation = iterativeDeepening(depth, timeLeft, moveTime);
        return bestMove;
    }
}

void ChessEngine::selectionSort(MoveVector& moves,int* movePriority, int currIndex) {
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

void ChessEngine::moveOrdering(BitBoard& bb, MoveVector& moves, int pliesFromRoot, Move& currBestMove, int* movePriority) {

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
                movePriority[i] = historyTable[sideToMove][movePiece][endPos];
            }
            break;
            case CAPTURE:
                if (pieceValue[capturePiece] >= pieceValue[movePiece])
                    movePriority[i] = pieceValue[capturePiece] * 110 - pieceValue[movePiece];
                else {
                    if (bb.SEE_GE(moves[i], -2))
                        movePriority[i] = pieceValue[capturePiece] * 110 - pieceValue[movePiece];
                    else {
                        movePriority[i] = (pieceValue[capturePiece] - pieceValue[movePiece]) * 100;
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
                movePriority[i] = historyTable[sideToMove][movePiece][endPos];
                break;
            case CAPTURE:
                if (pieceValue[capturePiece] >= pieceValue[movePiece])
                    movePriority[i] = pieceValue[capturePiece] * 110 - pieceValue[movePiece];
                else {
                    if (bb.SEE_GE(moves[i], -2))
                        movePriority[i] = pieceValue[capturePiece] * 110 - pieceValue[movePiece];
                    else {
                        movePriority[i] = (pieceValue[capturePiece] - pieceValue[movePiece]) * 100;
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
    int bestScore = -2000000000;
    bool isKingInCheck = bb.isKingInCheck();
    int standPat = !isKingInCheck ? bb.evaluate(alpha, beta) : bestScore;

    if (standPat > alpha) {
        if (standPat >= beta)
            return standPat;
        alpha = standPat;
    }
    bestScore = standPat;
    int futilityBase = standPat + futilityMarginQSearch;
    if (pliesFromLeaf > maxPlyQSearch) {
        if (isKingInCheck && bb.getLegalMoves().size() == 0) {
            return -2000000000;
        }
        return bestScore;
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
            return -2000000000;
        }
        return bestScore;

    }
    Move nullMove = Move();
    moveOrdering(bb, moves, 63, nullMove,movePriority);
    bool isEndgame = bb.isEndgame();
    for (int i = 0; i < moves.size(); i++) {
        selectionSort(moves, movePriority,i);
        MoveType type = moves[i].getMoveType();
        int endPos = moves[i].getEndPos();
        Side sideToMove = moves[i].getSideToMove();
        int priority = movePriority[i];
        if (type == MoveType::CAPTURE && (pieceValue[moves[i].getMovePiece()] - pieceValue[moves[i].getCapturePiece()] > 100) && (bb.getPawnAttackSquares(sideToMove, endPos) & bb.pieceBB[!sideToMove][Piece::pawn]) && !isEndgame) {
            continue;
        }
        bool isPawnEndgame = !((bb.pieceBB[Side::White][Piece::any] | bb.pieceBB[Side::Black][Piece::any]) ^ (bb.pieceBB[Side::White][Piece::pawn] | bb.pieceBB[Side::Black][Piece::pawn] | bb.pieceBB[Side::White][Piece::king] | bb.pieceBB[Side::Black][Piece::king]));
        if (bestScore > -1000000000 && !isPawnEndgame && !isKingInCheck && (endPos != prevEndPos || prevEndPos == -1) && type != MoveType::PROMOTION_QUEEN && type != MoveType::PROMOTION_QUEEN_AND_CAPTURE) {
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
                if (priority < 0)
                    continue;
                

            }
            
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
            return -2000000000 + pliesFromRoot;
        return 0;
    }
    moveOrdering(bb, moves, pliesFromRoot, excluded, movePriority);
    int staticEval = ss[pliesFromRoot].eval;
    int bestScore = -2e9;
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
            bool canLMR = (lmr && i > 0 && priority < 15001 && moveType != MoveType::CAPTURE);
            int lmrDepth = depth;
            if (canLMR) {
                int reducedDepth = (log2(i) * log2(depth) / lmrDiv + lmrBase) + historyTable[sideToMove][movePiece][endPos] / historyReductionFactor + (!improving && pliesFromRoot >= 2) - isKingInCheck;
                lmrDepth = depth - std::clamp(reducedDepth, 1, depth);
            }

            //pruning at low depth
            if (!isKingInCheck && pliesFromRoot > 0 && bestScore > -1000000000) {
                futilityVal = staticEval + (bestScore < staticEval ? 12500 : 7250) + futilityMargin * lmrDepth + (improving ? 8000 : 0);
                if (lmrDepth <= futilityDepthLimit && futilityVal <= alpha && moveType == MoveType::NORMAL) {
                    if (bestScore <= futilityVal && abs(bestScore) < 1000000000) {
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
                    score = -search(bb, ss, depth - 1, maxDepth, -alpha - 1, -alpha, newHash, pliesFromRoot + 1, false);
                    if (score > alpha) {
                        score = -search(bb, ss, depth - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, false);
                    }
                }
                else {
                    score = -search(bb, ss, depth - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, false);
                }
            }
        }
        else {
            isRepeated = true;
        }
        bb.undoMove(moves[i]);
        if (pliesFromRoot == 0 && score > bestScore) {
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
    if ((mainNodeCnt & 1023) == 1023) {
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - searchStartTime);
        if (duration.count() > timeLimitInMs) {
            throw 404;
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
    //bool isPVNode = false;
    bool isPVNode = pliesFromRoot==0 ||(isPV && ss[pliesFromRoot-1].move == PV[maxDepth-1][pliesFromRoot-1]);
    if (isPVNode && (pliesFromRoot & 0b111) == 7 && pliesFromRoot < maxDepth * 2) {
        depth++;
        //cnt[2]++;
    }
    /*
    if (pliesFromRoot == 1) {
        std::cout << ss[pliesFromRoot - 1].move.toUci() << " " << PV[maxDepth - 1][pliesFromRoot - 1].toUci()
            <<" "<< ss[pliesFromRoot - 1].move.getFirst32Bit() << " " << PV[maxDepth - 1][pliesFromRoot - 1].getFirst32Bit()<<std::endl;
    }
    */
    bool isFoundInTable = entry != nullptr;
    NodeFlag expectedNode = NodeFlag::ALLNODE;
    int entryDepth = 0;
    int entryScore = -2e9;
    int entryEval = 0;
    if (isFoundInTable) {
        entryDepth = entry->getDepth();
        expectedNode = entry->getNodeFlag();
        entryScore = entry->getScore();
        //isPVNode = isPV && expectedNode == NodeFlag::EXACT;
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
    /*
    if (debugLine.size() != 0 && pliesFromRoot == debugLine.size()+1) {
        bool matched = true;
        for (int i = 0; i < debugLine.size(); i++) {
            if (ss[i].move.toUci() != debugLine[i]) {
                matched = false;
            }
        }
        if (matched) {
            std::cout << "Checking variation ";
            for (int i = 0; i < pliesFromRoot; i++) {
                std::cout << ss[i].move.toUci() << " ";
            }
            std::cout << "Depth: " << depth << std::endl;
            std::cout << std::endl;
        }
    }
    */
    int bestScore = -2000000000;
    int beginAlpha = alpha;
    bool isEndgame = bb.isEndgame();
    int evalDepthZero = isFoundInTable ? entryEval : bb.evaluate(alpha,beta);
    /*
    if (isFoundInTable && mainNodeCnt%10000==9999) {
        std::cout << "----------------Node Info----------------" << std::endl;
        std::cout << entryDepth << std::endl;
        std::cout << evalDepthZero << std::endl;
        std::cout << entryScore << std::endl;
        std::cout << potentialBestMove.toUci() << std::endl;
        std::cout << static_cast<int>(expectedNode) << std::endl;
        std::cout << "----------------End Info----------------" << std::endl;

    }
    */
    ss[pliesFromRoot].eval = evalDepthZero;
    bool improving = pliesFromRoot >= 2 && evalDepthZero > ss[pliesFromRoot - 2].eval;
    int staticEval = isFoundInTable ? entryScore : evalDepthZero;
    bool isRepeated = false;
    //reverse futility pruning
    if (!isPVNode && !isKingInCheck && staticEval >= beta && depth <= rfpDepthLimit && canNullMove && pliesFromRoot > 0) {
        int rfpScore = staticEval - rfpMargin * (depth);
        if (rfpScore >= beta) {
            return beta > -1000000000 ? (staticEval + beta) / 2 : staticEval;
        }
    }
    if (staticEval >= beta && !isKingInCheck && !isPVNode && canNullMove && depth > 4 && pliesFromRoot != 0 && !bb.isKingAndPawnEndgame() && abs(staticEval) < 1000000000) {
        Move nullMove = Move();
        uint64_t newHash = bb.move(nullMove, zHash);
        ss[pliesFromRoot].move = nullMove;
        int evalNullMove = 0;
        if (!bb.isRepeatedPosition()) {
            evalNullMove = -search(bb, ss, std::clamp(((depth * 100 + (beta - staticEval) / 100) / nullMoveDivision) - 1, 1, depth) - 1, maxDepth, -beta, -alpha, newHash, pliesFromRoot + 1, false, false);
        }

        bb.undoMove(nullMove);
        if (evalNullMove >= beta) {
            return evalNullMove;
        }
        bestScore = std::max(bestScore, evalNullMove);
    }
    NodeFlag flag = NodeFlag::ALLNODE;
    Move currBestMove = potentialBestMove;
    bool delayedMoveGen = pliesFromRoot>=1;

    //Delay move generation: test the best move from previous iteration first
    Side sideToMove = bb.isWhiteTurn ? Side::White : Side::Black;
    if (!potentialBestMove.isNullMove() && delayedMoveGen && bb.isValidMove(potentialBestMove)) {
        MoveType moveType = potentialBestMove.getMoveType();
        Piece movePiece = potentialBestMove.getMovePiece();
        int endPos = potentialBestMove.getEndPos();
        int extension = 0;

        if (depth >= SEDepth + isPVNode && expectedNode == NodeFlag::CUTNODE && entryDepth >= depth - 3 && abs(staticEval) < 1e9) {
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
                ttable->set(zHash, TableEntry(zHash, potentialBestMove, NodeFlag::CUTNODE, depth, score, evalDepthZero));
            if (moveType == MoveType::NORMAL || moveType == MoveType::CASTLE_KINGSIDE || moveType == MoveType::CASTLE_QUEENSIDE) {
                int depthBonus = depth + (staticEval < alpha);
                int bonus = depthBonus * depthBonus;
                historyTable[sideToMove][movePiece][endPos] += (16 * bonus - historyTable[sideToMove][movePiece][endPos] * bonus / 512);
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
            return -2000000000 + pliesFromRoot;
        return 0;
    }
    
    moveOrdering(bb, moves, pliesFromRoot, currBestMove,movePriority);

    Side opSide = sideToMove == Side::White ? Side::Black : Side::White;
    bool lmr = !isPVNode && depth >= 3;
    int futilityVal = 0;
    int i;
    int LMPLimit = depth <= 4 ? (improving ? quietsToCheckTable[depth] + 5 : quietsToCheckTable[depth]) : 999;
    int quietMoves = 0;
    for (i = 0; i < moves.size(); i++) {
        selectionSort(moves,movePriority, i);
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
            bool canLMR = (lmr && i > 0 && priority < 15001 && moveType != MoveType::CAPTURE);
            int lmrDepth = depth;
            if (canLMR) {
                int reducedDepth = (log2(i) * log2(depth) / lmrDiv + lmrBase) + historyTable[sideToMove][movePiece][endPos] / historyReductionFactor + (!improving && pliesFromRoot >= 2) - isKingInCheck;
 
                lmrDepth = depth - std::clamp(reducedDepth, 1, depth);
            }

            //pruning at low depth
            if (!isKingInCheck && !isPVNode && pliesFromRoot > 0 && bestScore > -1000000000) {
                futilityVal = staticEval + (bestScore < staticEval ? 12500 : 7250) + futilityMargin * lmrDepth + (improving ? 8000 : 0);
                if (lmrDepth <= futilityDepthLimit && futilityVal <= alpha && moveType == MoveType::NORMAL) {
                    if (bestScore <= futilityVal && abs(bestScore) < 1000000000) {
                        bestScore = (bestScore + futilityVal) / 2;
                    }
                    bb.undoMove(moves[i]);
                    break;
                }

                if (quietMoves > LMPLimit && priority<0) {
                    bb.undoMove(moves[i]);
                    break;
                }
                if (!isPVNode && !isKingInCheck && moveType == MoveType::NORMAL && lmrDepth < 8) {
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

                if (reducedScore > alpha && !(beta==alpha+1 && lmrDepth == depth-1)) {
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
                    if (score > alpha && beta!=alpha+1) {
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
                int depthBonus = depth + (staticEval < alpha);
                int bonus = depthBonus * depthBonus;
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
            if (!isRepeated)
                ttable->set(zHash, TableEntry(zHash, moves[i], NodeFlag::CUTNODE, depth, score, evalDepthZero));
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
            ttable->set(zHash, TableEntry(zHash, potentialBestMove, flag, depth, bestScore, evalDepthZero));
        }
        else if (flag == NodeFlag::EXACT) {
            ttable->set(zHash, TableEntry(zHash, currBestMove, flag, depth, bestScore, evalDepthZero));
        }
    }
    
    if (flag == NodeFlag::EXACT) {
        PV[maxDepth][pliesFromRoot] = currBestMove;
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
    int currEval = 0;
    int prevEval = 1000000000;

    uint64_t zHash = bb.getCurrentPositionHash();
    ttable->currRootAge = bb.moveNum;
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
                try
                {
                    currEval = search(bb, ss, i, i, prevEval - lowerWindow, prevEval + upperWindow, zHash, 0, true);
                }
                catch (int x)
                {
                    if (x == 404) {
                        //bestMove = lastBestMove;
                        break;
                    }
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
                if (power >= 1) {
                    upperBound = 2000000001;
                    lowerBound = -2000000001;
                }

            }
        }
        //if this is the first iteration,do a full window search
        else {
            currEval = search(bb, ss, i, i, -2000000001, 2000000001, zHash, 0, true);
            lastBestMove = bestMove;
        }
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 7; k++) {
                for (int l = 0; l < 64; l++) {
                    historyTable[j][k][l] /= 2;
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
            std::cout << "info score cp " << currEval / 100 << " depth " << i << " time " << duration.count() << " nps " << (int)nps << " pv " ;
            /*
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
        prevEval = currEval;

    }
    this->currEval = currEval;
    for (int i = 0; i < 32; i++) {
        killerMoves[i][0] = Move();
        killerMoves[i][1] = Move();
    }

    for (int i = 0; i < 41; i++) {
        std::cout << "Depth " << i << " Node pruned: " << cnt[i] << std::endl;
    }
    std::cout << "Total node evaluated: " << bb.cntNode << " Pawn hash found: " << bb.cntNode2 << std::endl;
    nMovesOutOfBook++;
    qNodeCnt = 0;
    mainNodeCnt = 0;
    return currEval;
}


class MultithreadChessEngine {
private:
    std::vector<std::thread>threads;
    
    int nThreads;
public:
    ChessEngine* engines;
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

    std::string findBestMove(int depth, int timeLeft = -1, int moveTime = -1) {
        for (int i = 0; i < nThreads; i++) {
            threads[i] = std::thread(&ChessEngine::iterativeDeepening, engines[i], i < nThreads / 2 ? 1 : 2, timeLeft, moveTime);
        }
        for (int i = 0; i < nThreads; i++) {
            threads[i].join();
        }
        int currEval = -2e9;
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
    runTest(bb, "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 6, 71179139);
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
    while (running && getline(std::cin, line))
    {
        line += " ";
        std::istringstream iss(line);
        std::string        token;
        iss >> std::skipws >> token;



        if (token == "uci")
        {
            std::cout << "option name maxPlyQSearch type spin default 6 min 1 max 12" << std::endl;
            std::cout << "option name futilityMarginQSearch type spin default 23500 min 5000 max 50000" << std::endl;
            std::cout << "option name SEEMarginQSearch type spin default -34 min -400 max 400" << std::endl;
            std::cout << "option name SEENormalMargin type spin default -17 min -25 max -10" << std::endl;
            std::cout << "option name IIDReductionDepth type spin default 2 min 0 max 4" << std::endl;
            std::cout << "option name nullMoveDivision type spin default 186 min 120 max 320" << std::endl;
            std::cout << "option name rfpDepthLimit type spin default 4 min 1 max 10" << std::endl;
            std::cout << "option name rfpMargin type spin default 6000 min 0 max 40000" << std::endl;
            std::cout << "option name lmrDiv type spin default 367 min 343 max 555" << std::endl;
            std::cout << "option name lmrBase type spin default 43 min 1 max 100" << std::endl;
            std::cout << "option name historyReductionFactor type spin default -3500 min -8250 max 8250" << std::endl;
            std::cout << "option name futilityMargin type spin default 12500 min 1 max 90000" << std::endl;
            std::cout << "option name futilityDepthLimit type spin default 6 min 0 max 12" << std::endl;
            std::cout << "option name SEEMargin type spin default -101 min -500 max 500" << std::endl;
            std::cout << "option name LMP1 type spin default 5 min 2 max 8" << std::endl;
            std::cout << "option name LMP2 type spin default 7 min 3 max 11" << std::endl;
            std::cout << "option name LMP3 type spin default 14 min 7 max 21" << std::endl;
            std::cout << "option name LMP4 type spin default 29 min 15 max 40" << std::endl;
            std::cout << "option name SEMargin type spin default 400 min 100 max 1000" << std::endl;
            std::cout << "option name SEDoubleMargin type spin default 5000 min 1000 max 15000" << std::endl;
            std::cout << "option name SEDepth type spin default 6 min 4 max 7" << std::endl;
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
                else if (token == "debug") {
                    while (iss >> token) {
                        engine.engines[0].debugLine.push_back(token);
                    }
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

            std::string tmp;
            std::string optionName;
            std::string value;
            iss >> tmp;
            iss >> optionName;
            iss >> tmp;
            iss >> value;
            if (optionName == "LMP1") {
                quietsToCheckTable[1] = std::stoi(value);
            }
            else if (optionName == "LMP2") {
                quietsToCheckTable[2] = std::stoi(value);
            }
            else if (optionName == "LMP3") {
                quietsToCheckTable[3] = std::stoi(value);
            }
            else if (optionName == "LMP4") {
                quietsToCheckTable[4] = std::stoi(value);
            }
            else if (optionName == "lmrDiv") {
                lmrDiv = std::stoi(value);
                lmrDiv /= 100.f;
            }
            else if (optionName == "lmrBase") {
                lmrBase = std::stoi(value);
                lmrBase /= 100.f;
            }
            else if (optionName == "SEMargin") {
                SEMargin = std::stoi(value);
            }
            else if (optionName == "SEDoubleMargin") {
                SEDoubleMargin = std::stoi(value);
            }
            else if (optionName == "SEDepth") {
                SEDepth = std::stoi(value);
            }
            else if (optionName == "historyReductionFactor") {
                historyReductionFactor = std::stoi(value);
            }
            /*
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

            else if (optionName == "SEENormalMargin") {
                SEENormalMargin = std::stoi(value);
            }
            else if (optionName == "rfpMargin") {
                rfpMargin = std::stoi(value);
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
int main(int argn, char** argv) {
    //tb_init((PATH + "syzygy").c_str());
    //testMoveGen();
    //testMoveSorting();
    //testEvaluation();
    uciCommunication();
    return 0;
}