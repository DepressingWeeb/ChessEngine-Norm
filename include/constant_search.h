#pragma once

inline int futilityMarginQSearch = 13000;
inline int SEEMarginQSearch = 0;


inline int nullMoveDivision = 237;
inline int nullMoveDepthLimit = 7;
inline int nullMoveDepthMargin = 2300;
inline int nullMoveMargin = 17700;

inline int rfpDepthLimit = 3;
inline int rfpMargin = 1500;
inline int historyReductionFactor = -3750;
inline int captureHistoryReductionFactor = -3750;
inline int counterMoveHistoryReductionFactor = -3750;
inline int followUpHistoryReductionFactor = -3750;
inline float lmrBaseMid = 0.98f;
inline float lmrBaseEnd = 0.98f;
inline float lmrDivMid = 3.42f;
inline float lmrDivEnd = 3.42f;
inline int futilityMargin = 23000;
inline int futilityDepthLimit = 9;
inline int futilityBaseVal1 = 13500;
inline int futilityBaseVal2 = 4500;
inline int futilityImprovingBonus = 15500;
inline int SEEMargin = -72; //curently not used
inline int SEENormalMargin = -32;
inline int SEENormalDepthLimit = 8;
inline int SEMargin = 125;
inline int SEDoubleMargin = 9500;
inline int SEDepth = 6;
inline int singularBetaMargin = 7500;
inline int probCutBetaMargin = 6000;
inline int probCutDepthLimit = 9;
inline int razoringDepthLimit = 3;
inline int razoringConstant = 9700;
inline int deeperDepthMargin = 4000;
inline int shallowerDepthMargin = 1000;
inline int razoringDepthConstant = 23900;
inline int lmrDepthLimit = 3;


inline int skipMovesQsearch = 5;
inline int deltaPruningMargin = 35000;

inline int IIDDepthLimit = 4;
inline int IIDReductionDepth = 1;
inline int quietsToCheckTable[5] = { 0, 12, 7, 16, 21 };
inline int LMPDepthLimit = 8;
inline int LMPImprovingBonusMove = 16;
inline int maxPlyQSearch = 5;