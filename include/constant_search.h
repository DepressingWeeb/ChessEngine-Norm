const int maxPlyQSearch = 6;
const int futilityMarginQSearch = 23500;
const int SEEMarginQSearch = -24;


const int IIDReductionDepth = 1;
const int nullMoveDivision = 167;
const int rfpDepthLimit = 6;
const int rfpMargin = 7500;
int historyReductionFactor = -3750;
float lmrBase = 0.98;
float lmrDiv = 3.42;
const int futilityMargin = 12500;
const int futilityDepthLimit = 6;
const int SEEMargin = -72;
const int SEENormalMargin = -17;
int SEMargin = 400;
int SEDoubleMargin = 5000;
int SEDepth = 6;
int quietsToCheckTable[5] = { 0, 6, 4, 13, 23};