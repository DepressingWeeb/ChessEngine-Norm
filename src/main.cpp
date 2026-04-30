#pragma once
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
#include "constant_search.h"
#include "bitboard.h"
#include <thread>
#include <unordered_map>
#include <functional>
#include "chessengine.h"

class MultithreadChessEngine {
private:
    std::vector<std::thread>threads;

public:
    int nThreads;
    ChessEngine* engines;
    std::string bestMove;
    bool isWhiteTurn;
    MultithreadChessEngine(int nThreads) {
        this->nThreads = nThreads;
        threads = std::vector<std::thread>(nThreads);
        engines = new ChessEngine[nThreads];
        auto shared_tt = std::make_shared<TranspositionTable>();
        for (int i = 0; i < nThreads; i++) {
            engines[i].initTranspositionTable(shared_tt);
        }
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

    std::string findBestMove(int depth, int timeLeft = -1, int moveTime = -1, int movesToGo = -1, int endDepth = -1) {
        for (int i = 0; i < nThreads; i++) {
            threads[i] = std::thread(&ChessEngine::iterativeDeepening, &engines[i], i < nThreads / 2 ? 1 : 2, timeLeft, moveTime, movesToGo, endDepth);
        }
        for (int i = 0; i < nThreads; i++) {
            threads[i].join();
        }
        int currEval = -1e9;
        for (int i = 0; i < nThreads; i++) {
            engines[i].maxThreadDepth = 0;
        }

        bestMove = engines[0].finalBestMove;
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
// --- Macros to reduce boilerplate ---
#define REG_INT(m, name, target)   m[name]       = [](int v){ target = v; }
#define REG_SMG(m, name, target)   m[name "Mg"]  = [](int v){ setMg(target, v); }
#define REG_SEG(m, name, target)   m[name "Eg"]  = [](int v){ setEg(target, v); }
#define REG_S(m, name, target)     REG_SMG(m, name, target); REG_SEG(m, name, target)

// --- Build option map once (static) ---
static std::unordered_map<std::string, std::function<void(int)>> buildOptionMap() {
    std::unordered_map<std::string, std::function<void(int)>> m;

    // ── Existing search params ──────────────────────────────────────────────
    REG_INT(m, "LMP1", quietsToCheckTable[1]);
    REG_INT(m, "LMP2", quietsToCheckTable[2]);
    REG_INT(m, "LMP3", quietsToCheckTable[3]);
    REG_INT(m, "LMP4", quietsToCheckTable[4]);
    REG_INT(m, "SEMargin", SEMargin);
    REG_INT(m, "SEDoubleMargin", SEDoubleMargin);
    REG_INT(m, "SEDepth", SEDepth);
    REG_INT(m, "historyReductionFactor", historyReductionFactor);
    REG_INT(m, "captureHistoryReductionFactor", captureHistoryReductionFactor);
    REG_INT(m, "counterMoveHistoryReductionFactor", counterMoveHistoryReductionFactor);
    REG_INT(m, "followUpHistoryReductionFactor", followUpHistoryReductionFactor);
    REG_INT(m, "probCutBetaMargin", probCutBetaMargin);
    REG_INT(m, "probCutDepthLimit", probCutDepthLimit);
    REG_INT(m, "nullMoveDepthLimit", nullMoveDepthLimit);
    REG_INT(m, "IIDDepthLimit", IIDDepthLimit);
    REG_INT(m, "LMPDepthLimit", LMPDepthLimit);
    REG_INT(m, "SEENormalDepthLimit", SEENormalDepthLimit);
    REG_INT(m, "singularBetaMargin", singularBetaMargin);
    REG_INT(m, "futilityBaseVal1", futilityBaseVal1);
    REG_INT(m, "futilityBaseVal2", futilityBaseVal2);
    REG_INT(m, "futilityImprovingBonus", futilityImprovingBonus);
    REG_INT(m, "futilityMarginQSearch", futilityMarginQSearch);
    REG_INT(m, "SEEMarginQSearch", SEEMarginQSearch);
    REG_INT(m, "IIDReductionDepth", IIDReductionDepth);
    REG_INT(m, "nullMoveDivision", nullMoveDivision);
    REG_INT(m, "rfpDepthLimit", rfpDepthLimit);
    REG_INT(m, "SEENormalMargin", SEENormalMargin);
    REG_INT(m, "rfpMargin", rfpMargin);
    REG_INT(m, "futilityMargin", futilityMargin);
    REG_INT(m, "futilityDepthLimit", futilityDepthLimit);
    REG_INT(m, "SEEMargin", SEEMargin);
    // LMR params need /100 scaling — keep as special lambdas
    m["lmrDivMid"] = [](int v) { lmrDivMid = v / 100.f; };
    m["lmrBaseMid"] = [](int v) { lmrBaseMid = v / 100.f; };
    m["lmrDivEnd"] = [](int v) { lmrDivEnd = v / 100.f; };
    m["lmrBaseEnd"] = [](int v) { lmrBaseEnd = v / 100.f; };

    // ── Eval: Material (Tier 1) ─────────────────────────────────────────────
    REG_S(m, "matPawn", materialValue[1]);
    REG_S(m, "matKnight", materialValue[2]);
    REG_S(m, "matBishop", materialValue[3]);
    REG_S(m, "matRook", materialValue[4]);
    REG_S(m, "matQueen", materialValue[5]);

    // ── Eval: Mobility (Tier 1) ─────────────────────────────────────────────
    REG_S(m, "mobKnight", mobilityValue[2]);
    REG_S(m, "mobBishop", mobilityValue[3]);
    REG_S(m, "mobRook", mobilityValue[4]);
    REG_S(m, "mobQueen", mobilityValue[5]);
    REG_S(m, "mobKing", mobilityValue[6]);

    // ── Eval: Passed pawn by rank (Tier 1) ─────────────────────────────────
    REG_S(m, "passedR1", bonusPassedPawnByRank[1]);
    REG_S(m, "passedR2", bonusPassedPawnByRank[2]);
    REG_S(m, "passedR3", bonusPassedPawnByRank[3]);
    REG_S(m, "passedR4", bonusPassedPawnByRank[4]);
    REG_S(m, "passedR5", bonusPassedPawnByRank[5]);
    REG_S(m, "passedR6", bonusPassedPawnByRank[6]);
    REG_S(m, "passedR7", bonusPassedPawnByRank[7]);

    // ── Eval: Blocked passer by rank (Tier 2) ──────────────────────────────
    REG_S(m, "blockedPassedR1", penaltyBlockedPasserByRank[1]);
    REG_S(m, "blockedPassedR2", penaltyBlockedPasserByRank[2]);
    REG_S(m, "blockedPassedR3", penaltyBlockedPasserByRank[3]);
    REG_S(m, "blockedPassedR4", penaltyBlockedPasserByRank[4]);
    REG_S(m, "blockedPassedR5", penaltyBlockedPasserByRank[5]);
    REG_S(m, "blockedPassedR6", penaltyBlockedPasserByRank[6]);

    // ── Eval: Pawn structure (Tier 2) ───────────────────────────────────────
    REG_S(m, "pawnDouble", pawnDoublePenalty);
    REG_S(m, "pawnIsolated", pawnIsolatedPenalty);
    REG_S(m, "pawnBackward", pawnBackwardPenalty);

    // ── Eval: King safety (Tier 3) ──────────────────────────────────────────
    REG_S(m, "semiOpenFile0", semiOpenFilesPenalty[0]);
    REG_S(m, "semiOpenFile1", semiOpenFilesPenalty[1]);
    REG_S(m, "semiOpenFile2", semiOpenFilesPenalty[2]);
    REG_S(m, "semiOpenFile3", semiOpenFilesPenalty[3]);
    REG_S(m, "pawnSurroundKing", bonusPawnSurroundKing);

    // ── Eval: Piece bonuses (Tier 4) ────────────────────────────────────────
    REG_S(m, "bishopPair", bishopPairBonus);
    REG_S(m, "outpostKnight", bonusOutpostKnight);
    REG_S(m, "outpostBishop", bonusOutpostBishop);

    // ── Eval: Passed pawn detail + Rooks (Tier 5) ───────────────────────────
    REG_S(m, "connectedPassed", bonusConnectedPassedPawn);
    REG_S(m, "passerBlockedByEnemy", penaltyPasserBlockedByEnemy);
    REG_S(m, "passerBlockedBySelf", penaltyPasserBlockedBySelf);
    REG_S(m, "rookOnOpenFile", bonusRookOnOpenFile);
    REG_S(m, "rookOn7th", bonusRookOn7th);

    return m;
}

static const auto optionMap = buildOptionMap();
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
            std::cout << "option name SEENormalDepthLimit type spin default 8 min 4 max 12" << std::endl;  // ADDED
            std::cout << "option name IIDReductionDepth type spin default 2 min 0 max 4" << std::endl;
            std::cout << "option name IIDDepthLimit type spin default 4 min 1 max 8" << std::endl;         // ADDED
            std::cout << "option name nullMoveDivision type spin default 186 min 120 max 320" << std::endl;
            std::cout << "option name nullMoveDepthLimit type spin default 4 min 1 max 8" << std::endl;    // ADDED
            std::cout << "option name rfpDepthLimit type spin default 4 min 1 max 10" << std::endl;
            std::cout << "option name rfpMargin type spin default 6000 min 0 max 40000" << std::endl;
            std::cout << "option name lmrDivMid type spin default 342 min 150 max 555" << std::endl;
            std::cout << "option name lmrBaseMid type spin default 98 min 1 max 150" << std::endl;
            std::cout << "option name lmrDivEnd type spin default 342 min 150 max 555" << std::endl;
            std::cout << "option name lmrBaseEnd type spin default 98 min 1 max 150" << std::endl;
            std::cout << "option name historyReductionFactor type spin default -3500 min -8250 max 8250" << std::endl;
            std::cout << "option name captureHistoryReductionFactor type spin default -3750 min -8250 max 8250" << std::endl;      // ADDED
            std::cout << "option name counterMoveHistoryReductionFactor type spin default -3750 min -8250 max 8250" << std::endl;  // ADDED
            std::cout << "option name followUpHistoryReductionFactor type spin default -3750 min -8250 max 8250" << std::endl;     // ADDED
            std::cout << "option name futilityMargin type spin default 12500 min 1 max 90000" << std::endl;
            std::cout << "option name futilityDepthLimit type spin default 6 min 0 max 12" << std::endl;
            std::cout << "option name futilityBaseVal1 type spin default 12500 min 1 max 90000" << std::endl;  // ADDED
            std::cout << "option name futilityBaseVal2 type spin default 7250 min 1 max 90000" << std::endl;   // ADDED
            std::cout << "option name futilityImprovingBonus type spin default 8000 min 0 max 30000" << std::endl; // ADDED
            std::cout << "option name SEEMargin type spin default -101 min -500 max 500" << std::endl;
            std::cout << "option name SEMargin type spin default 400 min 100 max 1000" << std::endl;
            std::cout << "option name SEDoubleMargin type spin default 5000 min 1000 max 15000" << std::endl;
            std::cout << "option name SEDepth type spin default 6 min 4 max 7" << std::endl;
            std::cout << "option name singularBetaMargin type spin default 6000 min 1000 max 15000" << std::endl; // ADDED
            std::cout << "option name probCutBetaMargin type spin default 7000 min 1000 max 15000" << std::endl;  // ADDED
            std::cout << "option name probCutDepthLimit type spin default 5 min 1 max 10" << std::endl;           // ADDED
            std::cout << "option name LMPDepthLimit type spin default 4 min 1 max 8" << std::endl;                // ADDED
            std::cout << "option name LMP1 type spin default 5 min 2 max 8" << std::endl;
            std::cout << "option name LMP2 type spin default 7 min 3 max 11" << std::endl;
            std::cout << "option name LMP3 type spin default 14 min 7 max 21" << std::endl;
            std::cout << "option name LMP4 type spin default 29 min 15 max 40" << std::endl;
            // ── Eval: Material ──────────────────────────────────────────────────────────
            std::cout << "option name matPawnMg   type spin default 100  min 50   max 150" << std::endl;
            std::cout << "option name matPawnEg   type spin default 137  min 80   max 200" << std::endl;
            std::cout << "option name matKnightMg type spin default 381  min 250  max 500" << std::endl;
            std::cout << "option name matKnightEg type spin default 328  min 200  max 450" << std::endl;
            std::cout << "option name matBishopMg type spin default 385  min 250  max 500" << std::endl;
            std::cout << "option name matBishopEg type spin default 381  min 250  max 500" << std::endl;
            std::cout << "option name matRookMg   type spin default 527  min 380  max 680" << std::endl;
            std::cout << "option name matRookEg   type spin default 690  min 500  max 880" << std::endl;
            std::cout << "option name matQueenMg  type spin default 1142 min 900  max 1400" << std::endl;
            std::cout << "option name matQueenEg  type spin default 1213 min 950  max 1500" << std::endl;

            // ── Eval: Mobility ──────────────────────────────────────────────────────────
            std::cout << "option name mobKnightMg type spin default 8  min -5  max 25" << std::endl;
            std::cout << "option name mobKnightEg type spin default 13 min -5  max 30" << std::endl;
            std::cout << "option name mobBishopMg type spin default 7  min -5  max 20" << std::endl;
            std::cout << "option name mobBishopEg type spin default 8  min -5  max 20" << std::endl;
            std::cout << "option name mobRookMg   type spin default 4  min -5  max 15" << std::endl;
            std::cout << "option name mobRookEg   type spin default 5  min -5  max 15" << std::endl;
            std::cout << "option name mobQueenMg  type spin default 1  min -10 max 15" << std::endl;
            std::cout << "option name mobQueenEg  type spin default 9  min -5  max 25" << std::endl;
            std::cout << "option name mobKingMg   type spin default -7 min -25 max 10" << std::endl;
            std::cout << "option name mobKingEg   type spin default 13 min -5  max 30" << std::endl;

            // ── Eval: Passed pawn by rank ────────────────────────────────────────────────
            std::cout << "option name passedR1Mg type spin default 8   min -20  max 40" << std::endl;
            std::cout << "option name passedR1Eg type spin default -2  min -30  max 30" << std::endl;
            std::cout << "option name passedR2Mg type spin default 7   min -20  max 40" << std::endl;
            std::cout << "option name passedR2Eg type spin default -7  min -30  max 30" << std::endl;
            std::cout << "option name passedR3Mg type spin default -3  min -30  max 40" << std::endl;
            std::cout << "option name passedR3Eg type spin default 21  min -10  max 70" << std::endl;
            std::cout << "option name passedR4Mg type spin default 15  min -10  max 70" << std::endl;
            std::cout << "option name passedR4Eg type spin default 58  min 10   max 130" << std::endl;
            std::cout << "option name passedR5Mg type spin default 44  min 0    max 130" << std::endl;
            std::cout << "option name passedR5Eg type spin default 149 min 70   max 260" << std::endl;
            std::cout << "option name passedR6Mg type spin default 93  min 30   max 190" << std::endl;
            std::cout << "option name passedR6Eg type spin default 254 min 140  max 370" << std::endl;
            std::cout << "option name passedR7Mg type spin default 0   min -60  max 120" << std::endl;
            std::cout << "option name passedR7Eg type spin default 100 min 40   max 210" << std::endl;

            // ── Eval: Blocked passer by rank ────────────────────────────────────────────
            std::cout << "option name blockedPassedR1Mg type spin default 14  min -10 max 50" << std::endl;
            std::cout << "option name blockedPassedR1Eg type spin default 5   min -10 max 30" << std::endl;
            std::cout << "option name blockedPassedR2Mg type spin default 2   min -10 max 30" << std::endl;
            std::cout << "option name blockedPassedR2Eg type spin default 2   min -10 max 30" << std::endl;
            std::cout << "option name blockedPassedR3Mg type spin default -1  min -20 max 25" << std::endl;
            std::cout << "option name blockedPassedR3Eg type spin default 9   min -10 max 45" << std::endl;
            std::cout << "option name blockedPassedR4Mg type spin default 3   min -10 max 45" << std::endl;
            std::cout << "option name blockedPassedR4Eg type spin default 17  min -10 max 65" << std::endl;
            std::cout << "option name blockedPassedR5Mg type spin default -4  min -40 max 35" << std::endl;
            std::cout << "option name blockedPassedR5Eg type spin default 48  min 5   max 110" << std::endl;
            std::cout << "option name blockedPassedR6Mg type spin default -12 min -60 max 25" << std::endl;
            std::cout << "option name blockedPassedR6Eg type spin default 89  min 30  max 160" << std::endl;

            // ── Eval: Pawn structure ────────────────────────────────────────────────────
            std::cout << "option name pawnDoubleMg           type spin default 3  min -10 max 50" << std::endl;
            std::cout << "option name pawnDoubleEg           type spin default 37 min 5   max 80" << std::endl;
            std::cout << "option name pawnIsolatedMg         type spin default 27 min 0   max 65" << std::endl;
            std::cout << "option name pawnIsolatedEg         type spin default 22 min 0   max 65" << std::endl;
            std::cout << "option name pawnBackwardMg         type spin default 20 min 0   max 55" << std::endl;
            std::cout << "option name pawnBackwardEg         type spin default 15 min 0   max 55" << std::endl;
            std::cout << "option name pawnBackwardOpenFileMg type spin default 18 min 0   max 50" << std::endl;
            std::cout << "option name pawnBackwardOpenFileEg type spin default 12 min 0   max 50" << std::endl;

            // ── Eval: King safety ───────────────────────────────────────────────────────
            std::cout << "option name semiOpenFile0Mg  type spin default -37 min -100 max 10" << std::endl;
            std::cout << "option name semiOpenFile0Eg  type spin default 28  min -10  max 80" << std::endl;
            std::cout << "option name semiOpenFile1Mg  type spin default -6  min -50  max 30" << std::endl;
            std::cout << "option name semiOpenFile1Eg  type spin default 9   min -20  max 50" << std::endl;
            std::cout << "option name semiOpenFile2Mg  type spin default 26  min -10  max 80" << std::endl;
            std::cout << "option name semiOpenFile2Eg  type spin default -5  min -40  max 30" << std::endl;
            std::cout << "option name semiOpenFile3Mg  type spin default 85  min 20   max 160" << std::endl;
            std::cout << "option name semiOpenFile3Eg  type spin default -29 min -90  max 10" << std::endl;
            std::cout << "option name pawnSurroundKingMg type spin default 9 min 0    max 35" << std::endl;
            std::cout << "option name pawnSurroundKingEg type spin default 9 min 0    max 35" << std::endl;

            // ── Eval: Piece bonuses ─────────────────────────────────────────────────────
            std::cout << "option name bishopPairMg    type spin default 44 min 10  max 90" << std::endl;
            std::cout << "option name bishopPairEg    type spin default 75 min 20  max 130" << std::endl;
            std::cout << "option name outpostKnightMg type spin default 30 min 0   max 70" << std::endl;
            std::cout << "option name outpostKnightEg type spin default 20 min 0   max 60" << std::endl;
            std::cout << "option name outpostBishopMg type spin default 20 min 0   max 60" << std::endl;
            std::cout << "option name outpostBishopEg type spin default 15 min 0   max 50" << std::endl;

            // ── Eval: Passed pawn detail + Rooks ────────────────────────────────────────
            std::cout << "option name connectedPassedMg      type spin default 21 min 0   max 70" << std::endl;
            std::cout << "option name connectedPassedEg      type spin default 30 min 0   max 90" << std::endl;
            std::cout << "option name passerBlockedByEnemyMg type spin default 20 min 0   max 70" << std::endl;
            std::cout << "option name passerBlockedByEnemyEg type spin default 50 min 5   max 110" << std::endl;
            std::cout << "option name passerBlockedBySelfMg  type spin default 10 min 0   max 45" << std::endl;
            std::cout << "option name passerBlockedBySelfEg  type spin default 10 min 0   max 45" << std::endl;
            std::cout << "option name rookOnOpenFileMg        type spin default 22 min 0   max 70" << std::endl;
            std::cout << "option name rookOnOpenFileEg        type spin default 35 min 0   max 90" << std::endl;
            std::cout << "option name rookOn7thMg             type spin default 35 min 0   max 80" << std::endl;
            std::cout << "option name rookOn7thEg             type spin default 20 min 0   max 70" << std::endl;
            std::cout << "uciok" << std::endl;
        }
        else if (token == "isready")
        {

            std::cout << "readyok" << std::endl;
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
            commands[command::depth] = "63";
            commands[command::white_time] = "-1";
            commands[command::black_time] = "-1";
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
            int movesToGo = -1;
            if (commands.find(command::moves_to_go) != commands.end()) {
                movesToGo = std::stoi(commands[command::moves_to_go]);

            }
            if (commands.find(command::move_time) != commands.end()) {
                engine.findBestMove(1, -1, std::stoi(commands[command::move_time]), movesToGo, std::stoi(commands[command::depth]));
            }
            else {
                if (engine.isWhiteTurn) {
                    engine.findBestMove(1, std::stoi(commands[command::white_time]), -1, movesToGo, std::stoi(commands[command::depth]));
                }
                else {
                    engine.findBestMove(1, std::stoi(commands[command::black_time]), -1, movesToGo, std::stoi(commands[command::depth]));
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

            std::string tmp, optionName, value;
            iss >> tmp >> optionName >> tmp >> value;

            if (auto it = optionMap.find(optionName); it != optionMap.end()) {
                it->second(std::stoi(value));
            }

        }
        else if (token == "ucinewgame") {
            engine.engines[0].ttable->clear();
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