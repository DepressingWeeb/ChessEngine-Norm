#pragma once
#include "types.h"
#include <stdlib.h>
#include <iostream>
const uint64_t TABLE_SIZE = 0x100000;
const uint64_t MASK_HASH = TABLE_SIZE-1;
const int MATE_VALUE = 2e9;
const int BUCKET_SIZE = 3;
class TableEntry {
public:
    uint64_t data1;
    uint64_t data2;
    TableEntry() {
        data1 = 0;
        data2 = 0;
    }
    TableEntry(uint64_t zHash,Move best, NodeFlag flag, int depth, int score,int age,int staticEval) {
        setKey(zHash);
        setBestMove(best);
        setNodeFlag(flag);
        setDepth(depth);
        setScore(score);
        setAge(age);
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
        uint64_t scoreConverted = static_cast<uint64_t>(score + MATE_VALUE);
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
        return static_cast<int>(((data2 >> 18) & 0xFFFFFFFFull)-MATE_VALUE);
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

class EvalEntry {
public:
    uint64_t data1;
    EvalEntry() {
        data1 = 0;
    }
    EvalEntry(uint64_t zHash,int eval) {
        setKey(zHash);
        setEval(eval);
    }
    bool isValid() {
        return data1;
    }
    void setKey(uint64_t zHash) {
        data1 &= ~(0xFFFFFFFFull); // Clear the previous value
        data1 |= ((zHash>>32));
    }
    void setEval(int eval) {
        data1 &= ~(0xFFFFFFFFull << 32); // Clear the previous value
        data1 |= static_cast<uint64_t>(eval) << 32;
    }

    int getEval() const {
        return static_cast<int>((data1 >> 32) & 0xFFFFFFFFull);
    }

    uint32_t getHash() const {
        return static_cast<uint32_t>(data1 & 0xFFFFFFFFull);
    }
};

class TranspositionTable {
private:
    TableEntry arrEntry[TABLE_SIZE][BUCKET_SIZE];
public:
    
    int nCollisions = 0;
    TranspositionTable() {  


    }
    TableEntry* find(uint64_t zHash,int ithMove) {
        uint64_t key = zHash & MASK_HASH;
        if (arrEntry[key][0].getHash() == static_cast<uint32_t>(zHash>>32) && ithMove == arrEntry[key][0].getAge()) {
            return &arrEntry[key][0];
        }
        else if (arrEntry[key][1].getHash() == static_cast<uint32_t>(zHash >> 32) && ithMove == arrEntry[key][1].getAge()) {
            return &arrEntry[key][1];
        }
        else if (arrEntry[key][2].getHash() == static_cast<uint32_t>(zHash >> 32) && ithMove == arrEntry[key][2].getAge()) {
            return &arrEntry[key][2];
        }
        return nullptr;
    }
    void set(uint64_t zHash, TableEntry t) {
        uint64_t key = zHash & MASK_HASH;
        //if (zHash != arrEntry[key][0].zHash && zHash != arrEntry[key][1].zHash && zHash != arrEntry[key][2].zHash&& arrEntry[key][2].zHash!=0)
            //nCollisions++;
        arrEntry[key][2] = arrEntry[key][1];
        arrEntry[key][1] = arrEntry[key][0];
        arrEntry[key][0] = t;
    }
};

class EvalTable {
private:
    EvalEntry arrEntry[TABLE_SIZE][BUCKET_SIZE-1];
public:
    
    int nCollisions = 0;
    EvalTable() {  

    }
    EvalEntry* find(uint64_t zHash) {
        uint64_t key = zHash & MASK_HASH;
        if (arrEntry[key][0].getHash() == static_cast<uint32_t>(zHash>>32)) {
            return &arrEntry[key][0];
        }
        else if (arrEntry[key][1].getHash() == static_cast<uint32_t>(zHash >> 32)) {
            return &arrEntry[key][1];
        }
        return nullptr;
    }
    void set(uint64_t zHash, EvalEntry t) {
        uint64_t key = zHash & MASK_HASH;
        //if (zHash != arrEntry[key][0].zHash && zHash != arrEntry[key][1].zHash && zHash != arrEntry[key][2].zHash&& arrEntry[key][2].zHash!=0)
            //nCollisions++;
        arrEntry[key][1] = arrEntry[key][0];
        arrEntry[key][0] = t;
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

struct PawnEntry {
    uint64_t key;
    uint64_t passedPawnsSet;
    int kingPos[2];
    int scorePawn[2];
    float attackersPawn;
    int atkUnitsPawn;
    PawnEntry() {
        key = 0;
        passedPawnsSet = 0;
        kingPos[0] = 0;
        kingPos[1] = 0;
        scorePawn[0] = 0;
        scorePawn[1] = 0;
        attackersPawn = 0.0f;
        atkUnitsPawn = 0;
    }
    void reset() {
        key = 0;
        passedPawnsSet = 0;
        kingPos[0] = 0;
        kingPos[1] = 0;
        scorePawn[0] = 0;
        scorePawn[1] = 0;
        attackersPawn = 0.0f;
        atkUnitsPawn = 0;
    }
};
const int PAWN_HASH_SIZE = 0x400;
const int MASK_PAWN = PAWN_HASH_SIZE - 1;
PawnEntry pawnEntryDefault = PawnEntry();
class PawnHashTable {
private:
    PawnEntry arrEntry[PAWN_HASH_SIZE];
public:
    PawnEntry find(uint64_t orgKey,int kingPos1,int kingPos2) const {
        uint64_t key = orgKey & MASK_PAWN;
        if (arrEntry[key].key == orgKey)
            return arrEntry[key];
        return pawnEntryDefault;
    }
    PawnEntry& find(uint64_t orgKey) {
        return arrEntry[orgKey & MASK_PAWN];
    }
    void set(uint64_t orgKey,PawnEntry& entry) {
        arrEntry[orgKey & MASK_PAWN] = entry;
    }
};
