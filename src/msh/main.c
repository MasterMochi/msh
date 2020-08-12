/******************************************************************************/
/*                                                                            */
/* src/msh/main.c                                                             */
/*                                                                 2020/08/12 */
/* Copyright (C) 2020 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
#include <string.h>

/* ライブラリヘッダ */
#include <libmvfs.h>
#include <libmk.h>


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
void main( void )
{
    char         *pPrompt;      /* プロンプト文字列   */
    size_t       promptSize;    /* プロンプト文字列長 */
    size_t       writeSize;     /* 書込みサイズ       */
    uint32_t     fd;            /* ターミナルFD       */
    LibMvfsErr_t errLibMvfs;    /* libmvfsエラー要因  */
    LibMvfsRet_t retLibMvfs;    /* libmvfs戻り値      */

    /* 初期化 */
    pPrompt    = "\e[1m\e[92m[msh]\e[97m$\e[0m ";
    promptSize = strlen( pPrompt );
    writeSize  = 0;
    fd         = 0;
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    /* TODO: 起動待ち合わせ */
    LibMkTimerSleep( 10000000, NULL );

    /* ターミナルファイルオープン */
    retLibMvfs = LibMvfsOpen( &fd, "/ttyS1", &errLibMvfs );

    /* オープン結果判定 */
    if ( retLibMvfs == LIBMVFS_RET_SUCCESS ) {
        /* 成功 */

        /* プロンプト出力 */
        LibMvfsWrite( fd, pPrompt, promptSize, &writeSize, NULL );
    }

    /* 無限ループ */
    while ( true ) {
        LibMkTimerSleep( 10000000, NULL );
    }
}


/******************************************************************************/
