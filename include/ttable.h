#pragma once
#include "chess_types.h"
#include <stdlib.h>
#include <iostream>
#include <nmmintrin.h>
const uint64_t TABLE_SIZE = 0x100000;
const uint64_t MASK_HASH = TABLE_SIZE-1;
const int MATE_VALUE = 1e9;
const int BUCKET_SIZE = 4;
class TableEntry {
public:
    uint64_t data1;
    uint64_t data2;
    TableEntry() {
        data1 = 0;
        data2 = 0;
    }
    TableEntry(uint64_t zHash,Move best, NodeFlag flag, int depth, int score,int staticEval,int pliesFromRoot) {
        if (score > 900000000) {
            score += pliesFromRoot;
        }
        else if (score < -900000000) {
            score -= pliesFromRoot;
        }
        setKey(zHash);
        setBestMove(best);
        setNodeFlag(flag);
        setDepth(depth);
        setScore(score);
        setEval(staticEval);
    }
    bool isValid() {
        return data1;
    }
    void setKey(uint64_t zHash) {
        data1 &= ~(0xFFFFFFFFull); // Clear the previous value
        data1 |= ((zHash>>32));
    }
    void setBestMove(Move m) {
        data1 &= ~(0xFFFFFFFFull << 32); // Clear the previous value
        data1 |= static_cast<uint64_t>(m.getFirst32Bit()) << 32;
    }
    void setNodeFlag(NodeFlag flag) {
        data2 &= ~(0b11ull); // Clear the previous value
        data2 |= static_cast<uint64_t>(flag);
    }
    void setDepth(int depth) {
        data2 &= ~(0b111111ull << 2); // Clear the previous value
        data2 |= static_cast<uint64_t>(depth) << 2;
    }
    void setAge(int age) {
        data2 &= ~(0b1111111111ull << 8); // Clear the previous value
        data2 |= static_cast<uint64_t>(age) << 8;
        //assert(getScore() == score);
    }
    void setScore(int score) {
        uint64_t scoreConverted = static_cast<uint64_t>(score + MATE_VALUE) ;
        data2 &= ~(0xFFFFFFFFull << 18); // Clear the previous value
        data2 |= scoreConverted << 18;
        //assert(getScore() == score);
    }
    void setEval(int staticEval){
        staticEval>>=6;
        uint64_t scoreConverted = static_cast<uint64_t>(staticEval+4000);
        data2 &= ~(0b1111111111111ull << 50); // Clear the previous value
        data2 |= scoreConverted << 50;
    }

    

    int getScore() const {
        return static_cast<int>(((data2 >> 18) & 0xFFFFFFFFull)- MATE_VALUE);
    }
    int getEval() const{
        return static_cast<int>(((data2 >> 50) & 0b1111111111111ull)-4000)<<6;
    }
    int getDepth() const {
        return static_cast<int>((data2 >> 2) & 0b111111);
    }
    int getAge() const {
        return static_cast<int>((data2 >> 8) & 0b1111111111ull);
    }
    NodeFlag getNodeFlag() const{
        return static_cast<NodeFlag>(data2 & 0b11);
    }
    Move getBestMove() const {
        return static_cast<Move>((data1 >> 32) & 0xFFFFFFFFull);
    }

    uint32_t getHash() const {
        return static_cast<uint32_t>(data1 & 0xFFFFFFFFull);
    }
};

class TranspositionTable {
private:
    
public:
    TableEntry arrEntry[TABLE_SIZE][BUCKET_SIZE];
    int nCollisions = 0;
    int currRootAge = 0;
    TranspositionTable() {  


    }
    TableEntry* find(uint64_t zHash) {
        uint64_t key = zHash & MASK_HASH;
        for (int i = 0; i < BUCKET_SIZE; i++) {
            if (arrEntry[key][i].getHash() == static_cast<uint32_t>(zHash >> 32)) {
                return &arrEntry[key][i];
            }
        }
        return nullptr;
    }
    void set(uint64_t zHash, TableEntry t) {
        uint64_t key = zHash & MASK_HASH;
        for (int i = 0; i < BUCKET_SIZE; i++) {
            if (arrEntry[key][i].getHash() == (zHash >> 32)) {
                arrEntry[key][i] = t;
                return;
            }
        }
        for (int i = BUCKET_SIZE - 1; i > 0; i--) {
            arrEntry[key][i] = arrEntry[key][i - 1];
        }
        arrEntry[key][0] = t;
    }
    int hashfull() const {
        // Sample first 1000 indices — standard UCI permille convention
        constexpr int SAMPLE_SIZE = 1000;
        int used = 0;

        for (int i = 0; i < SAMPLE_SIZE; i++) {
            for (int j = 0; j < BUCKET_SIZE; j++) {
                if (arrEntry[i][j].getHash() != 0) {
                    used++;
                }
            }
        }
        return used/BUCKET_SIZE; // out of 1000 = permille
    }
    static void adjustScore(int& score,int ply) {
        if (score > 900000000)
            score-=ply;
        else if (score < -900000000)
            score+=ply;
    }
    void clear() {
        memset(arrEntry, 0, sizeof(arrEntry)); // Efficiently sets all memory to 0
        nCollisions = 0;
        currRootAge = 0;
    }
};

class RepetitionTable {
private:
    uint64_t hashHistory[1024];
    int lastIrreversible[1024];
    int curr;//also the current size of table
public:
    RepetitionTable() {
        curr = 1;
        for (int i = 0; i < 1024; i++) {
            lastIrreversible[i] = 0;
            hashHistory[i] = 0;
        }
    }
    void add(uint64_t zHash,bool isIrreversible) {
        hashHistory[curr] = zHash;
        lastIrreversible[curr] = isIrreversible ? curr : lastIrreversible[curr - 1];
        //std::cout << "Ply "<<curr<<": "<<lastIrreversible[curr] << std::endl;
        curr++;
        
    }
    void pop() {
        curr--;
    }
    //called after call makemove
    bool isRepetition() {
        int start = lastIrreversible[curr - 1];
        int cnt = 0;
        uint64_t hashToCmp = hashHistory[curr - 1];
        for (int i = curr - 5; i >= start; i -=2) {
            if (hashHistory[i] == hashToCmp)
                return true;
        }
        return false;
    }

};
