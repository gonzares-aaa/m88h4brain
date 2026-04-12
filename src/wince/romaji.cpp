/***********************************************************************
 *
 * ローマ字→カナ変換処理
 *
 ************************************************************************/

// ★ m88h.hはインクルードしません（依存関係エラーを断ち切るため）
#include "romaji.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define P0      0
#define P1      1
#define P2      2
#define P3      3
#define P4      4
#define P5      5
#define P6      6
#define P7      7
#define P8      8
#define P9      9
#define Pa      10
#define Pb      11
#define Pc      12
#define Pd      13
#define Pe      14

// --- M88コアを呼び出すためのブリッジ関数（HpcKeyIF.cpp側で実装） ---
extern "C" void RomajiHook_KeyDown(void* pKeyIF, int port, int bit);
extern "C" void RomajiHook_KeyUp(void* pKeyIF, int port, int bit);

/* キューに蓄える値。ポート情報がパックしてある */
#define RJ(port, bit, shift)	((unsigned char)((port<<4) | (shift<<3) | bit))

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*----------------------------------------------------------------------
 * ローマ字 → カナ 変換テーブル構造体
 *----------------------------------------------------------------------*/
typedef struct {
	const char *s;
	unsigned char      list[4];
} romaji_list;

#include "romaji-table.h"

/*----------------------------------------------------------------------
 * ワーク (サスペンド情報には残す必要なし)
 *----------------------------------------------------------------------*/

static char input_buf[4];				/* 入力済みの文字のバッファ */
static int  input_size;					/* 入力済みの文字の数 */

#define ROMAJI_QUE_SIZE		(64)
static int   romaji_set;
static int   romaji_ptr;
static unsigned char romaji_que[ ROMAJI_QUE_SIZE ];

static int press_timer;					/* キーオン・オフのタイマー */
#define KEY_ON_OFF_INTERVAL	(2)			/* キーオン・オフの時間 */

static romaji_list list[280];
static int         nr_list;

static int romajicmp(const void *p1, const void *p2)
{
	return strcmp(((const romaji_list *)p1)->s, ((const romaji_list *)p2)->s);
}

void romaji_init(void)
{
	int i, nr_p;
	const romaji_list *p;

	romaji_clear(0);
	nr_list = 0;

	p = list_msime;
	nr_p = COUNTOF(list_msime);
	for (i = 0; i < nr_p; i++) {
		list[ nr_list ++ ] = *p ++;
		if (nr_list >= COUNTOF(list)) break;
	}

	qsort(&list, nr_list, sizeof(romaji_list), romajicmp);

	p    = list_mark;
	nr_p = COUNTOF(list_mark);
	for (i = 0; i < nr_p; i++) {
		list[ nr_list ++ ] = *p ++;
		if (nr_list >= COUNTOF(list)) break;
	}
}

void romaji_clear(void* pKeyIF)
{
	if (0 < press_timer && press_timer <= KEY_ON_OFF_INTERVAL) {
		if (pKeyIF) {
			unsigned char c = romaji_que[romaji_ptr];
			// ★ キャストをやめ、ブリッジ関数を呼ぶ
			if (c & 0x08) RomajiHook_KeyUp(pKeyIF, 8, 6);
			RomajiHook_KeyUp(pKeyIF, c >> 4, c & 7);
		}
	}
	
	romaji_set  = 0;
	romaji_ptr  = 0;
	press_timer = 0;
	input_size  = 0;
}

static void set_romaji_que(const unsigned char *p)
{
	unsigned char c;
	while ((c = *p++)) {
		romaji_que[ romaji_set++ ] = c;
		romaji_set &= (ROMAJI_QUE_SIZE - 1);
	}
}

int romaji_input(int key, void* pKeyIF)
{
	int i, j;

	if (key == ' ' || key == '@' || key == '[' || key == '/' ||
		key == '-' || key == '{' || key == '}' || key == '.' ||
		key == ',' || key == '\'') {
		;
	} else if (islower(key)) {
		key = toupper(key);
	} else if (isupper(key)) {
		;
	} else {
		romaji_clear(pKeyIF);
		return key;
	}

	input_buf[ input_size ] = key;
	input_size ++;

	while (input_size) {
		int          list_size = nr_list;
		romaji_list *list_p   = list;
		int same   = FALSE;
		int nearly = FALSE;

		for (i = 0; i < list_size; i++, list_p++) {
			const char *s1 = input_buf;
			const char *s2 = list_p->s;

			for (j = 0; j < input_size; j++, s1++, s2++) {
				if (*s1 != *s2) {
					j = 0;
					break;
				}
			}

			if (j == 0) {
				if (nearly) break;
			} else {
				if (*s2 == '\0') {
					same   = TRUE;
					break;
				} else {
					nearly = TRUE;
				}
			}
		}

		if (same) {
			set_romaji_que(list_p->list);
			input_size = 0;
			break;
		} else if (nearly) {
			break;
		} else {
			if (input_buf[0] == 'N') {
				set_romaji_que(list_NN.list);
			} else if (input_size >= 2 && input_buf[0] == input_buf[1]) {
				set_romaji_que(list_tu.list);
			}
			input_size --;
			memmove(&input_buf[0], &input_buf[1], input_size);
		}
	}

	if (input_size >= (int)sizeof(input_buf)) {
		input_size = 0;
	}

	return 0;
}

void romaji_output(void* pKeyIF)
{
	if (!pKeyIF) return;
	unsigned char c;

	switch (press_timer) {
	case 0:
		if (romaji_ptr != romaji_set) {
			c = romaji_que[ romaji_ptr ];
			// ★ キャストをやめ、ブリッジ関数を呼ぶ
			if (c & 0x08) RomajiHook_KeyDown(pKeyIF, 8, 6);
			RomajiHook_KeyDown(pKeyIF, c >> 4, c & 7);
			press_timer ++;
		}
		break;

	case KEY_ON_OFF_INTERVAL:
		c = romaji_que[ romaji_ptr ];
		// ★ キャストをやめ、ブリッジ関数を呼ぶ
		if (c & 0x08) RomajiHook_KeyUp(pKeyIF, 8, 6);
		RomajiHook_KeyUp(pKeyIF, c >> 4, c & 7);

		romaji_ptr++;
		romaji_ptr &= (ROMAJI_QUE_SIZE - 1);
		press_timer ++;
		break;

	case KEY_ON_OFF_INTERVAL * 2:
		press_timer = 0;
		break;

	default:
		press_timer ++;
	}
}