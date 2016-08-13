#include <stdio.h>
#include <stdlib.h>

int swap_endian(int n)
{
	return (n << 24 & 0xff000000) | ((n << 8) & 0x00ff0000) | ((n >> 8) & 0x0000ff00) | (n >> 24 & 0x000000ff);
}

short swap_endian(short n)
{
	return ((n << 8) & 0x0000ff00) | ((n >> 8) & 0x000000ff);
}

#pragma pack(push, 1) // アラインメント調整
struct HeaderChunk
{
	int id_; // "MThd" 4D 54 68 64
	int size_; // データ長
	short format_;
	short track_; // トラック数
	short timeUnit_; // 時間単位 ( 四分音符あたりの分解能

	void SwapEndian()
	{
		id_ = swap_endian(id_);
		size_ = swap_endian(size_);
		format_ = swap_endian(format_);
		track_ = swap_endian(track_);
		timeUnit_ = swap_endian(timeUnit_);
	}
};

struct TrackChunk
{
	int id_; // "MTrk" 4D 54 72 6B
	int size_; // データ長

	void SwapEndian()
	{
		id_ = swap_endian(id_);
		size_ = swap_endian(size_);
	}
};
#pragma pack(pop)

void main()
{
	FILE* fp;
	const char* name = "dat/batof.mid";

	fopen_s(&fp, name, "rb");

	if (fp == NULL)
	{
		printf("mapが　ありません！\n");
		//assert(NULL);
		getchar();
		return;
	}

	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	printf("file_size : %d\n", size);
	fseek(fp, 0, SEEK_SET);


	int as = 0;


	HeaderChunk h;
	fread(&h, sizeof(h), 1, fp);
	h.SwapEndian();

	printf("--header-------------------------------\n");
	printf("id_ : %x\n", h.id_);
	printf("size_ : %d\n", h.size_);
	printf("format_ : %d\n", h.format_);
	printf("track_ : %d\n", h.track_);
	printf("timeUnit_ : %x\n", h.timeUnit_);

	as += sizeof(h);


	int tempo; // 四分音符の長さをマイクロ秒単位で表現


	//for (int i = 0; i < h.track_; i++)
	for (int i = 0; i < 2; i++)
	{
		TrackChunk t;
		fread(&t, sizeof(t), 1, fp);
		t.SwapEndian();

		printf("--track--------------------------------\n");
		printf("id_ : %x\n", t.id_);
		printf("size_ : %d\n", t.size_);

		unsigned char* buf = (unsigned char*)malloc(t.size_);
		fread(buf, t.size_, 1, fp);

		int it = 0;

		/*  */
		// 処理

#if 1
		for (int i = 0; i < t.size_; i++) // debug 一覧表示
			printf("%02x ", buf[i]);
		printf("\n");
#endif

		// デルタタイム ( 可変 めんどい
		while (buf[it++] != /**/ 0xff /**/); // 条件式　今はテキトー

		// イベント
		// 1. MIDIイベント
		// 2. SysExイベント		f0, f7 始まり
		// 3. メタイベント		ff     始まり

		if (buf[it++] == 0x51) // テンポ
		{
			it++; // 0x03
			// 3 byte
			tempo = (buf[it] << 16) | (buf[it + 1] << 8) | buf[it + 2];
			// 60 * 10⁶ / <?> = tempo(μsec)
			printf("tempo : %d\n", 60 * 1000000 / tempo); // 有効桁数以下切り捨て
			it += 3;
		}
		else if (buf[it] == 0x58) // 拍子
		{

		}
		else if (buf[it] == 0x59) // キー(調)
		{

		}
		else if (buf[it] == 0x2f) // トラックチャンクの終わり
		{
			//break;
		}

		printf("%02x ", buf[it++]);
		printf("%02x ", buf[it++]);
		printf("%02x ", buf[it++]);


		/*  */

		free(buf);

		as += sizeof(t);
		as += t.size_;
		printf("\n");
	}

	printf("as : %d\n", as); // file_size 一致

	fclose(fp);

	getchar();
}