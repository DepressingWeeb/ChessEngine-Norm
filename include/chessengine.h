#pragma once
#include <thread>
#include <unordered_map>
#include <functional>
#include <types.h>
#include <ttable.h>
#include <bitboard.h>
const std::string PATH = "./";
class ChessEngine {
protected:
    bool startWithDefaultPosition;
    std::vector<Move> moveHistory;
    std::vector<uint64_t> hashMoveHistory;
    Move killerMoves[64][2];
    int historyTable[2][7][64];
    int counterMoveHistory[7][64][7][64];
    int followUpHistory[7][64][7][64];
    int captureHistory[2][7][64][7];
    Move PV[64][128];

    int mainNodeCnt = 0;
    int selDepth = 0;
    int qNodeCnt = 0;
    int cnt[100]{ 0 };
    int nMovesOutOfBook;
    std::chrono::steady_clock::time_point searchStartTime;
    int timeLimitInMs;
    int tthits = 0;

public:
    int maxThreadDepth = 0;
    std::string finalBestMove;
    Move finalBestMoveObj;
    std::shared_ptr<TranspositionTable> ttable;
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
    void selectionSort(MoveVector& moves, int* movePriority, int currIndex);
    void parseMoves(std::vector<std::string> moves);
    int qSearch(BitBoard& bb, SearchStack* const ss, int alpha, int beta, int prevEndPos, int pliesFromLeaf, int pliesFromRoot);
    int search(BitBoard& bb, SearchStack* const ss, int depth, int maxDepth, int alpha, int beta, uint64_t zHash, int pliesFromRoot, bool isPV = false, bool canNullMove = true);
    int searchDebug(BitBoard& bb, SearchStack* const ss, int depth, int maxDepth, int alpha, int beta, uint64_t zHash, int pliesFromRoot, bool isPV = false, bool canNullMove = true);
    int iterativeDeepening(int depth, int timeLeft = -1, int moveTime = -1, int movesToGo = -1, int endDepth = 63);
    int singularSearch(BitBoard& bb, SearchStack* const ss, int depth, int maxDepth, int alpha, int beta, uint64_t zHash, int pliesFromRoot, bool isPV, Move excluded);
    void moveOrdering(BitBoard& bb, SearchStack* const ss, MoveVector& moves, int pliesFromRoot, Move& currBestMove, int* movePriority);
    std::string findBestMove(int depth, int timeLeft = -1, int moveTime = -1, int endDepth = 63);
    void initTranspositionTable(std::shared_ptr<TranspositionTable> shared_tt = nullptr);
};
