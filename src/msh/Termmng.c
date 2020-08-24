/******************************************************************************/
/*                                                                            */
/* src/msh/Termmng.c                                                          */
/*                                                                 2020/08/24 */
/* Copyright (C) 2020 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ライブラリヘッダ */
#include <libmvfs.h>
#include <libmk.h>

/* モジュール内ヘッダ */
#include "Termmng.h"


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/** ターミナルファイルFD */
static uint32_t gTermFD;


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       ターミナル管理初期化
 * @details     ターミナルファイルをopenする。
 */
/******************************************************************************/
void TermmngInit( void )
{
    LibMvfsErr_t errLibMvfs;    /* libmvfsエラー要因 */
    LibMvfsRet_t retLibMvfs;    /* libmvfs戻り値     */

    /* 初期化 */
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    /* ターミナルファイルオープン */
    retLibMvfs = LibMvfsOpen( &gTermFD, "/ttyS1", &errLibMvfs );

    /* オープン結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        /* [TODO]異常終了 */
        while ( true ) {
            LibMkTimerSleep( 10000000, NULL );
        }
    }

    return;
}


/******************************************************************************/
/**
 * @brief       読込み
 * @details     ターミナルファイルが読込み可能になるまで待ち合わせて読込みを行
 *              う。
 *
 * @param[in]   *pBuffer 読込み先バッファ
 * @param[in]   size     読込サイズ
 *
 * @return      実際に読み込んだサイズを返す。
 */
/******************************************************************************/
size_t TermmngRead( void   *pBuffer,
                    size_t size      )
{
    size_t       ret;           /* 戻り値             */
    LibMvfsFds_t fds;           /* 読込み監視FDリスト */
    LibMvfsErr_t errLibMvfs;    /* libmvfsエラー要因  */
    LibMvfsRet_t retLibMvfs;    /* libmvfs戻り値      */

    /* 初期化 */
    ret        = 0;
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;
    memset( &fds, 0, sizeof ( LibMvfsFds_t ) );

    /* 読込み監視FD設定 */
    LIBMVFS_FDS_SET( &fds, gTermFD );

    /* 読込み可能待ち合わせ */
    retLibMvfs = LibMvfsSelect( &fds, NULL, 0, &errLibMvfs );

    /* 待ち合わせ結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        return 0;
    }

    /* ターミナルファイルread */
    retLibMvfs = LibMvfsRead( gTermFD, pBuffer, size, &ret, &errLibMvfs );

    /* read結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        return 0;
    }

    return ret;
}


/******************************************************************************/
/**
 * @brief       書込み
 * @details     ターミナルファイルに書込みを行う。
 *
 * @param[in]   *pBuffer 書込み元バッファ
 * @param[in]   size     書込みサイズ
 *
 * @return      実際に書き込んだサイズを返す。
 */
/******************************************************************************/
size_t TermmngWrite( void   *pBuffer,
                     size_t size      )
{
    size_t       ret;           /* 戻り値            */
    LibMvfsErr_t errLibMvfs;    /* libmvfsエラー要因 */
    LibMvfsRet_t retLibMvfs;    /* libmvfs戻り値     */

    /* 初期化 */
    ret        = 0;
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    /* ターミナルファイルwrite */
    retLibMvfs = LibMvfsWrite( gTermFD, pBuffer, size, &ret, &errLibMvfs );

    /* write結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        return 0;
    }

    return ret;
}


/******************************************************************************/
