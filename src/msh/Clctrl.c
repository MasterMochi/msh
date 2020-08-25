/******************************************************************************/
/*                                                                            */
/* src/msh/Clctrl.c                                                           */
/*                                                                 2020/08/25 */
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
#include <MLib/MLibState.h>

/* モジュール内ヘッダ */
#include "Clctrl.h"
#include "Termmng.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* 状態 */
#define STATE_INIT   ( 1 )  /**< 初期状態       */
#define STATE_ESCAPE ( 2 )  /**< エスケープ状態 */
#define STATE_CSI    ( 3 )  /**< CSI状態        */

/** 状態遷移タスクパラメータ */
typedef struct {
    char *pBuffer;  /**< コマンドラインバッファ       */
    char c;         /**< 入力文字                     */
    bool end;       /**< コマンドライン編集終了フラグ */
} editParam_t;


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* 1文字前消去 */
static MLibStateNo_t DoBackspace( void *pArg );
/* コマンドライン編集終了 */
static MLibStateNo_t DoCR( void *pArg );
/* CUB */
static MLibStateNo_t DoCUB( void *pArg );
/* CUF */
static MLibStateNo_t DoCUF( void *pArg );
/* 1文字消去 */
static MLibStateNo_t DoDelete( void *pArg );
/* 文字挿入 */
static MLibStateNo_t DoInsert( void *pArg );

/* コマンドライン編集 */
static bool Edit( char *pBuffer,
                  char c         );


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/** 状態遷移ハンドル */
static MLibState_t gState;
/** 状態遷移表 */
static const MLibStateTransition_t gStt[] =
    {
        /*-------------+--------+-------------+----------------------*/
        /* 状態        | 入力   | タスク      | 遷移先状態           */
        /*-------------+--------+-------------+----------------------*/
        { STATE_INIT   , '\b'   , DoBackspace , { STATE_INIT   , 0 } },
        { STATE_INIT   , '\r'   , DoCR        , { STATE_INIT   , 0 } },
        { STATE_INIT   , '\e'   , NULL        , { STATE_ESCAPE , 0 } },
        { STATE_INIT   , '\x7F' , DoDelete    , { STATE_INIT   , 0 } },
        { STATE_INIT   , 0      , DoInsert    , { STATE_INIT   , 0 } },
        /*-------------+--------+-------------+----------------------*/
        { STATE_ESCAPE , '['    , NULL        , { STATE_CSI    , 0 } },
        { STATE_ESCAPE , 0      , NULL        , { STATE_INIT   , 0 } },
        /*-------------+--------+-------------+----------------------*/
        { STATE_CSI    , 'C'    , DoCUF       , { STATE_INIT   , 0 } },
        { STATE_CSI    , 'D'    , DoCUB       , { STATE_INIT   , 0 } },
        { STATE_CSI    , 0      , NULL        , { STATE_INIT   , 0 } }
        /*-------------+--------+-------------+----------------------*/
    };

/** プロンプト */
static const char pgPrompt[] = "\e[1m\e[92m[msh]\e[97m$\e[0m ";

/** バッファカーソル */
static uint32_t gCursor;
/** コマンド文字数 */
static size_t   gLength;


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       コマンドライン制御初期化
 * @details     コマンドライン編集用状態遷移を初期化する。
 */
/******************************************************************************/
void ClctrlInit( void )
{
    /* 状態遷移初期化 */
    ( void ) MLibStateInit( &gState, gStt, sizeof ( gStt ), STATE_INIT, NULL );

    return;
}


/******************************************************************************/
/**
 * @brief       コマンド入力
 * @details     プロンプトを出力しコマンド入力を受け付ける。
 *
 * @param[in]   *pBuffer コマンドラインバッファ
 * @param[in]   size     コマンドラインバッファサイズ
 *
 * @return      コマンド入力結果を返す。
 * @retval      true  コマンド入力
 * @retval      false キャンセル
 */
/******************************************************************************/
bool ClctrlInput( char   *pBuffer,
                  size_t size      )
{
    bool   end;         /* コマンドライン編集終了フラグ   */
    char   c;           /* 入力文字                       */
    size_t readSize;    /* ターミナルファイル読込みサイズ */

    /* 初期化 */
    end      = false;
    c        = 0;
    readSize = 0;

    /* プロンプト出力 */
    TermmngWrite( ( void * ) pgPrompt, strlen( pgPrompt ) );

    /* コマンドライン初期化 */
    gCursor = 1;                /* バッファカーソル */
    gLength = 0;                /* コマンド文字数   */

    do {
        /* 読込み */
        readSize = TermmngRead( &c, 1 );

        /* 読込み結果判定 */
        if ( readSize != 1 ) {
            /* 失敗 */
            continue;
        }

        /* コマンドライン編集 */
        end = Edit( pBuffer, c );

    /* コマンドライン編集終了判定 */
    } while ( end == false );

    /* コマンド文字列数判定 */
    if ( gLength == 0 ) {
        /* 無し */

        return false;
    }

    return true;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       1文字前消去
 * @details     コマンドラインのカーソル位置の1つ前の文字を消去する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT 初期状態
 */
/******************************************************************************/
static MLibStateNo_t DoBackspace( void *pArg )
{
    uint32_t    idx;        /* インデックス */
    editParam_t *pParam;    /* パラメータ   */

    /* 初期化 */
    idx    = 0;
    pParam = ( editParam_t * ) pArg;

    /* カーソル位置判定 */
    if ( gCursor == 1 ) {
        /* バッファ先頭 */

        /* 処理無し */

    } else if ( gCursor == ( gLength + 1 ) ) {
        /* バッファ末尾文字 + 1 */

        /* カーソル位置を1文字戻す */
        gCursor -= 1;

        /* カーソル位置を1文字削除する */
        pParam->pBuffer[ gCursor - 1 ]  = 0;
        gLength                        -= 1;

        /* ターミナル反映 */
        TermmngWrite( "\e[D\e[X", 6 );

    } else {
        /* 上記以外 */

        /* カーソル位置を1文字戻す */
        gCursor -= 1;

        /* 1文字前にずらす */
        for ( idx = ( gCursor - 1 ); idx < ( gLength - 1 ); idx++ ) {
            pParam->pBuffer[ idx ] = pParam->pBuffer[ idx + 1 ];
        }
        gLength -= 1;

        /* ターミナル反映 */
        TermmngWrite( "\e[D\e[P", 6 );
    }

    return STATE_INIT;
}

/******************************************************************************/
/**
 * @brief       コマンドライン編集終了
 * @details     コマンドラインバッファに終端文字を設定し、ターミナルファイルに
 *              '\ee'を書込み、コマンドライン編集を終了する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT 初期状態
 */
/******************************************************************************/
static MLibStateNo_t DoCR( void *pArg )
{
    editParam_t *pParam;    /* パラメータ */

    /* 初期化 */
    pParam = ( editParam_t * ) pArg;

    /* コマンドラインバッファに終端文字設定 */
    pParam->pBuffer[ gLength ] = 0;

    /* ターミナルファイル改行 */
    TermmngWrite( "\eE", 2 );

    /* ターミナルファイル編集終了 */
    pParam->end = true;

    return STATE_INIT;
}

/******************************************************************************/
/**
 * @brief       CUB
 * @details     コマンドラインのカーソル位置を1文字進める。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT 初期状態
 */
/******************************************************************************/
static MLibStateNo_t DoCUB( void *pArg )
{
    editParam_t *pParam;    /* パラメータ */

    /* 初期化 */
    pParam = ( editParam_t * ) pArg;

    /* カーソル位置判定 */
    if ( gCursor == 1 ) {
        /* バッファ先頭 */

        /* 処理無し */

    } else {
        /* 上記以外 */

        /* カーソル位置を1文字戻す */
        gCursor -= 1;

        /* ターミナル反映 */
        TermmngWrite( "\e[D", 3 );
    }

    return STATE_INIT;
}

/******************************************************************************/
/**
 * @brief       CUF
 * @details     コマンドラインのカーソル位置を1文字戻す。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT 初期状態
 */
/******************************************************************************/
static MLibStateNo_t DoCUF( void *pArg )
{
    editParam_t *pParam;    /* パラメータ */

    /* 初期化 */
    pParam = ( editParam_t * ) pArg;

    /* カーソル位置判定 */
    if ( gCursor == ( gLength + 1 ) ) {
        /* バッファ末尾文字+1 */

        /* 処理無し */

    } else {
        /* 上記以外 */

        /* バッファのカーソル位置を1文字進める */
        gCursor += 1;

        /* ターミナル反映 */
        TermmngWrite( "\e[C", 3 );
    }

    return STATE_INIT;
}

/******************************************************************************/
/**
 * @brief       1文字消去
 * @details     コマンドラインのカーソル位置の文字を消去する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT 初期状態
 */
/******************************************************************************/
static MLibStateNo_t DoDelete( void *pArg )
{
    uint32_t    idx;        /* インデックス */
    editParam_t *pParam;    /* パラメータ   */

    /* 初期化 */
    idx    = 0;
    pParam = ( editParam_t * ) pArg;

    /* カーソル位置判定 */
    if ( gCursor == ( gLength + 1 ) ) {
        /* バッファ末尾文字+1 */

        /* 処理無し */

    } else {
        /* 上記以外 */

        /* 1文字前ずらす */
        for ( idx = ( gCursor - 1 ); idx < ( gLength - 1 ); idx++ ) {
            pParam->pBuffer[ idx ] = pParam->pBuffer[ idx + 1 ];
        }
        gLength -= 1;

        /* ターミナル反映 */
        TermmngWrite( "\e[P", 3 );
    }

    return STATE_INIT;
}

/******************************************************************************/
/**
 * @brief       文字挿入
 * @details     コマンドラインのカーソル位置に文字を挿入する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT 初期状態
 */
/******************************************************************************/
static MLibStateNo_t DoInsert( void *pArg )
{
    char        buffer[] = "\e[@ "; /* ターミナル書込みバッファ */
    uint32_t    idx;                /* インデックス             */
    editParam_t *pParam;            /* パラメータ               */

    /* 初期化 */
    idx    = 0;
    pParam = ( editParam_t * ) pArg;

    /* カーソル位置判定 */
    if ( gCursor == ( gLength + 1 ) ) {
        /* バッファ末尾文字+1 */

        /* 文字挿入 */
        pParam->pBuffer[ gCursor - 1 ]  = pParam->c;
        gCursor                        += 1;
        gLength                        += 1;

        /* ターミナル反映 */
        TermmngWrite( &( pParam->c ), 1 );

    } else {
        /* 上記以外 */

        /* 1文字後にずらす */
        for ( idx = gLength; idx > ( gCursor - 1 ); idx-- ) {
            pParam->pBuffer[ idx ] = pParam->pBuffer[ idx - 1 ];
        }
        gLength += 1;

        /* 文字挿入 */
        pParam->pBuffer[ gCursor - 1 ]  = pParam->c;
        gCursor                        += 1;

        /* ターミナル反映 */
        buffer[ 3 ] = pParam->c;
        TermmngWrite( buffer, 4 );
    }

    return STATE_INIT;
}


/******************************************************************************/
/**
 * @brief       コマンドライン編集
 * @details     入力文字をイベント番号として状態遷移を実行する。
 *
 * @param[in]   *pBuffer コマンドラインバッファ
 * @param[in]   c        入力文字
 *
 * @return      コマンドライン編集終了か否かを返す。
 * @retval      true  コマンドライン編集終了
 * @retval      false コマンドライン編集継続
 */
/******************************************************************************/
static bool Edit( char *pBuffer,
                  char c         )
{
    editParam_t param;

    /* 初期化 */
    param.pBuffer = pBuffer;
    param.c       = c;
    param.end     = false;

    /* 状態遷移実行 */
    ( void ) MLibStateExec( &gState,
                            ( MLibStateEvent_t ) c,
                            &param,
                            NULL,
                            NULL,
                            NULL                    );

    return param.end;
}


/******************************************************************************/
