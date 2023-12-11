#include "hand.h"

//handの構成
// rook bishop gold silver knight lance pawn
// 2bit 2bit   3bit  3bit  3bit   3bit   5bit
const u32 Hand::hand_shift[HAND_PIECE_SIZE] = {
  14, 0, 5, 8, 11, 17, 19
};
const u32 Hand::hand_mask[HAND_PIECE_SIZE] = {
  0x7u  << hand_shift[GOLD_H],
  0x1fu << hand_shift[PAWN_H],
  0x7u  << hand_shift[LANCE_H],
  0x7u  << hand_shift[KNIGHT_H],
  0x7u  << hand_shift[SILVER_H],
  0x3u  << hand_shift[BISHOP_H],
  0x3u  << hand_shift[ROOK_H],
};
const u32 Hand::hand_inc[HAND_PIECE_SIZE] = {
  1u << hand_shift[GOLD_H],
  1u << hand_shift[PAWN_H],
  1u << hand_shift[LANCE_H],
  1u << hand_shift[KNIGHT_H],
  1u << hand_shift[SILVER_H],
  1u << hand_shift[BISHOP_H],
  1u << hand_shift[ROOK_H],
};
