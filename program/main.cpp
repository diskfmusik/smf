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


	for (int i = 0; i < h.track_; i++)
	{
		TrackChunk t;
		fread(&t, sizeof(t), 1, fp);
		t.SwapEndian();

		printf("--track--------------------------------\n");
		printf("id_ : %x\n", t.id_);
		printf("size_ : %d\n", t.size_);

		char* buf = (char*)malloc(t.size_);
		fread(buf, t.size_, 1, fp);

		/*  */
		// 処理
		/*  */

		free(buf);

		as += sizeof(t);
		as += t.size_;
	}

	printf("as : %d\n", as); // file_size 一致

	fclose(fp);

	getchar();
}