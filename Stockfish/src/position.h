#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include "types.h"
#include "../../include/mybitboard.h"
#include <array>

namespace Stockfish {

class Position {
    BitBoard* bb;
    std::array<Piece, SQUARE_NB> board;
public:
    Position() = default;
    Position(BitBoard* bb) : bb(bb) {
        board.fill(NO_PIECE);
        for(int c=0; c<2; c++) {
            Color color = c == 0 ? WHITE : BLACK;
            Side side = c == 0 ? Side::White : Side::Black;
            for(int pt=1; pt<=6; pt++) {
                ::Piece my_pt = static_cast<::Piece>(pt);
                uint64_t mask = bb->pieceBB[side][my_pt];
                while(mask) {
                    int i = my_BitScanForward64(mask);
                    mask &= mask - 1;
                    board[i] = make_piece(color, static_cast<PieceType>(pt));
                }
            }
        }
    }

    Color side_to_move() const { return bb->isWhiteTurn ? WHITE : BLACK; }

    template<PieceType Pt> int count(Color c) const {
        return popcount64(bb->pieceBB[c == WHITE ? Side::White : Side::Black][Pt == PAWN ? ::Piece::pawn : Pt == KNIGHT ? ::Piece::knight : Pt == BISHOP ? ::Piece::bishop : Pt == ROOK ? ::Piece::rook : Pt == QUEEN ? ::Piece::queen : Pt == KING ? ::Piece::king : ::Piece::any]);
    }
    template<PieceType Pt> int count() const { return count<Pt>(WHITE) + count<Pt>(BLACK); }

    int non_pawn_material(Color c) const {
        int val = 0;
        val += count<KNIGHT>(c) * KnightValue;
        val += count<BISHOP>(c) * BishopValue;
        val += count<ROOK>(c) * RookValue;
        val += count<QUEEN>(c) * QueenValue;
        return val;
    }
    int non_pawn_material() const { return non_pawn_material(WHITE) + non_pawn_material(BLACK); }

    int rule50_count() const { return 0; }
    bool checkers() const { return bb->isKingInCheck(); }

    template<PieceType Pt> Square square(Color c) const {
        uint64_t mask = bb->pieceBB[c == WHITE ? Side::White : Side::Black][Pt == PAWN ? ::Piece::pawn : Pt == KNIGHT ? ::Piece::knight : Pt == BISHOP ? ::Piece::bishop : Pt == ROOK ? ::Piece::rook : Pt == QUEEN ? ::Piece::queen : Pt == KING ? ::Piece::king : ::Piece::any];
        return mask ? static_cast<Square>(my_BitScanForward64(mask)) : SQ_NONE;
    }

    Bitboard pieces() const { return bb->pieceBB[Side::White][::Piece::any] | bb->pieceBB[Side::Black][::Piece::any]; }
    Bitboard pieces(PieceType pt) const {
        auto pieceMapping = pt == PAWN ? ::Piece::pawn : pt == KNIGHT ? ::Piece::knight : pt == BISHOP ? ::Piece::bishop : pt == ROOK ? ::Piece::rook : pt == QUEEN ? ::Piece::queen : pt == KING ? ::Piece::king : ::Piece::any;
        return bb->pieceBB[Side::White][pieceMapping] | bb->pieceBB[Side::Black][pieceMapping];
    }
    Bitboard pieces(Color c) const {
        return bb->pieceBB[c == WHITE ? Side::White : Side::Black][::Piece::any];
    }
    Bitboard pieces(Color c, PieceType pt) const {
        auto pieceMapping = pt == PAWN ? ::Piece::pawn : pt == KNIGHT ? ::Piece::knight : pt == BISHOP ? ::Piece::bishop : pt == ROOK ? ::Piece::rook : pt == QUEEN ? ::Piece::queen : pt == KING ? ::Piece::king : ::Piece::any;
        return bb->pieceBB[c == WHITE ? Side::White : Side::Black][pieceMapping];
    }

    Piece piece_on_calc(Square s) const {
        ::Piece p = bb->pieceTable[s];
        if (p == ::Piece::any) return NO_PIECE;
        Color c = (bb->pieceBB[Side::White][p] & (1ULL << s)) ? WHITE : BLACK;
        PieceType pt = p == ::Piece::pawn ? PAWN : p == ::Piece::knight ? KNIGHT : p == ::Piece::bishop ? BISHOP : p == ::Piece::rook ? ROOK : p == ::Piece::queen ? QUEEN : KING;
        return make_piece(c, pt);
    }
    
    Piece piece_on(Square s) const {
        return board[s];
    }

    const std::array<Piece, SQUARE_NB>& piece_array() const { return board; }
    
    Key pawn_key() const { return bb->getPawnHash(); }
    Key non_pawn_key() const { return bb->getCurrentPositionHash(); }
    Key minor_piece_key() const { return bb->getCurrentPositionHash(); }
};

} // namespace Stockfish

#endif