/******************************************************************************/
/*                                                                            */
/* src/msh/Termmng.h                                                          */
/*                                                                 2020/08/23 */
/* Copyright (C) 2020 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
#ifndef TERMMNG_H
#define TERMMNG_H
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stddef.h>


/******************************************************************************/
/* グローバル関数宣言                                                         */
/******************************************************************************/
/* ターミナル管理初期化 */
extern void TermmngInit( void );
/* 読込み */
extern size_t TermmngRead( void   *pBuffer,
                           size_t size      );
/* 書込み */
extern size_t TermmngWrite( void    *pBuffer,
                            size_t  size      );


/******************************************************************************/
#endif
