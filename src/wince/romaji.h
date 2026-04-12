#ifndef ROMAJI_H_INCLUDED
#define ROMAJI_H_INCLUDED


#define COUNTOF(arr)	(int)(sizeof(arr)/sizeof((arr)[0]))

/* ローマ字入力処理 */

void romaji_init(void);
void romaji_clear(void* pKeyIF = 0);           // ★ void* に変更
int  romaji_input(int key, void* pKeyIF = 0);  // ★ void* に変更
void romaji_output(void* pKeyIF);              // ★ void* に変更

#endif /* ROMAJI_H_INCLUDED */

