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
	void Print()
	{
		printf("--header-------------------------------\n");
		printf("id_ : %x\n", id_);
		printf("size_ : %d\n", size_);
		printf("format_ : %d\n", format_);
		printf("track_ : %d\n", track_);
		printf("timeUnit_ : %x\n", timeUnit_);
		printf("--end----------------------------------\n\n");
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
	void Print()
	{
		printf("--track--------------------------------\n");
		printf("id_ : %x\n", id_);
		printf("size_ : %d\n", size_);
	}
};
#pragma pack(pop)

enum MidiStatus
{
	NoteOff,				// 8n kk vv (3 byte)
	NoteOn,					// 9n kk vv (3 byte)
	PolyphonicKeyPressure,	// An kk vv (3 byte)
	ControllChange,			// Bn cc vv (3 byte or 4 byte)
	ProgramChange,			// Cn pp (2 byte)
	ChannelPressure,		// Dn vv (2 byte)
	PitchBend,				// En mm ll (3 byte) little endian!!
};

bool isMidiEvent(unsigned char buf, MidiStatus& midiStatus)
{
	if (0x80 <= buf && buf <= 0x8f)
	{
		midiStatus = NoteOff; return true;
	}
	else if (0x90 <= buf && buf <= 0x9f)
	{
		midiStatus = NoteOn; return true;
	}
	else if (0xa0 <= buf && buf <= 0xaf)
	{
		midiStatus = PolyphonicKeyPressure; return true;
	}
	else if (0xb0 <= buf && buf <= 0xbf)
	{
		midiStatus = ControllChange; return true;
	}
	else if (0xc0 <= buf && buf <= 0xcf)
	{
		midiStatus = ProgramChange;	return true;
	}
	else if (0xd0 <= buf && buf <= 0xdf)
	{
		midiStatus = ChannelPressure; return true;
	}
	else if (0xe0 <= buf && buf <= 0xef)
	{
		midiStatus = PitchBend; return true;
	}

	return false;
}

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
	printf("file_size : %d\n\n", size);
	fseek(fp, 0, SEEK_SET);


	int as = 0;


	HeaderChunk h;
	fread(&h, sizeof(h), 1, fp);
	h.SwapEndian();
	h.Print();


	as += sizeof(h);


	int tempo; // 四分音符の長さをマイクロ秒単位で表現


	//for (int i = 0; i < h.track_; i++)
	for (int i = 0; i < 3; i++)
	{
		TrackChunk t;
		fread(&t, sizeof(t), 1, fp);
		t.SwapEndian();
		t.Print();

		unsigned char* buf = (unsigned char*)malloc(t.size_);
		fread(buf, t.size_, 1, fp);

		int it = 0;

		/*  */
		// 処理

#if 0
		for (int i = 0; i < t.size_; i++) // debug 一覧表示
			printf("%02x ", buf[i]);
		printf("\n");
#endif

		while (1)
		{
			MidiStatus midiStatus;

			// デルタタイム ( 可変
			int deltaSize = 0;
			while (1)
			{
				if (isMidiEvent(buf[it], midiStatus)) // 1. MIDIイベント
				{
					printf("deltaSize : %d\n", deltaSize);
					goto MidiEvent;
				}
				else if (buf[it] == 0xf0 || buf[it] == 0xf7) // 2. SysExイベント
				{
					printf("deltaSize : %d\n", deltaSize);
					goto SysExEvent;
				}
				else if (buf[it] == 0xff) // 3. メタイベント
				{
					it++;
					printf("deltaSize : %d\n", deltaSize);
					goto MetaEvent;
				}

				deltaSize++;
				it++;
			}


		MidiEvent: //----------------------------------------------------------------
			if (midiStatus == NoteOff) // 8n kk vv (3 byte)
			{
				//printf("--Midi[NoteOff]\n");
				// 9n kk 00 とも表現できる
				unsigned char channel = buf[it] - 0x80;
				printf("チャンネル(%02x)で鳴っている、ノート(%02x)の音を消音。\n", channel, buf[it + 1]);
				it += 3;
			}
			else if (midiStatus == NoteOn) // 9n kk vv (3 byte)
			{
				//printf("--Midi[NoteOn]\n");
				unsigned char channel = buf[it] - 0x90;
				printf("チャンネル(%02x)で、ノート(%02x)の音を、ベロシティ(%02x)で発音。\n", channel, buf[it + 1], buf[it + 2]);
				it += 3;
			}
			else if (midiStatus == PolyphonicKeyPressure) // An kk vv (3 byte)
			{
				printf("--Midi[PolyphonicKeyPressure]\n");
				unsigned char channel = buf[it] - 0xa0;
				printf("チャンネル(%02x)で、発音中のノート(%02x)の音に対し、ベロシティ(%02x)のプレッシャー情報を与える。\n",
					channel, buf[it + 1], buf[it + 2]);
				it += 3;
			}
			else if (midiStatus == ControllChange) // Bn cc vv (3 byte or 4 byte) ???
			{
				printf("--Midi[ControllChange]\n");
				unsigned char channel = buf[it] - 0xb0;
				printf("チャンネル(%02x)で、コントローラナンバー(%02x)に、値(%02x)を送る。\n", channel, buf[it + 1], buf[it + 2]);
				it += 3;
			}
			else if (midiStatus == ProgramChange) // Cn pp (2 byte)
			{
				printf("--Midi[ProgramChange]\n");
				unsigned char channel = buf[it] - 0xc0;
				printf("チャンネル(%02x)で、プログラム(音色)を(%02x)に変える。\n", channel, buf[it + 1]);
				it += 2;
			}
			else if (midiStatus == ChannelPressure) // Dn vv (2 byte)
			{
				printf("--Midi[ChannelPressure]\n");
				unsigned char channel = buf[it] - 0xd0;
				printf("チャンネル(%02x)に、プレッシャー情報(%02x)を送信する。\n", channel, buf[it + 1]);
				// PolyphonicKeyPressure の、チャンネル内全音版
				it += 2;
			}
			else if (midiStatus == PitchBend) // En mm ll (3 byte) little endian!!
			{
				printf("--Midi[PitchBend]\n");
				unsigned char channel = buf[it] - 0xe0;
				unsigned short bend = (buf[it + 2] << 8) | buf[it + 1]; // ll mm
				printf("チャンネル(%02x)に対し、ピッチベンド値(%02x)を送る。\n", channel, bend);
				it += 3;
			}

			continue; // 次のイベントへ

		SysExEvent: //---------------------------------------------------------------
			if (buf[it] == 0xf0)
			{
				printf("--SysEx[0xf0]\n");
				unsigned char len = buf[++it];
				printf("len : %d\n", len);
				it += len + 1;
			}
			else if (buf[it] == 0xf7)
			{
				printf("--SysEx[0xf7]\n");
				unsigned char len = buf[++it];
				printf("len : %d\n", len);
				it += len + 1;
			}

			continue; // 次のイベントへ

		MetaEvent: //----------------------------------------------------------------
			if (buf[it] == 0x03) // シーケンス名(曲タイトル)・トラック名
			{
				printf("--Meta[0x03]\n");
				unsigned char len = buf[++it];
				printf("len : %d\n", len);
				it += len + 1;
			}
			else if (buf[it] == 0x2f) // トラックチャンクの終わり
			{
				printf("--end----------------------------------\n\n");
				break;
			}
			else if (buf[it] == 0x51) // テンポ
			{
				printf("--Meta[0x51]\n");
				it++;
				it++; // 0x03
				// 3 byte
				tempo = (buf[it] << 16) | (buf[it + 1] << 8) | buf[it + 2];
				// 60 * 10⁶ / <?> = tempo(μsec)
				printf("tempo : %d\n", 60 * 1000000 / tempo); // 有効桁数以下切り捨て
				it += 3;
			}
			else if (buf[it] == 0x58) // 拍子
			{
				printf("--Meta[0x58]\n");
				it++;
				it++; // 0x04
				// 4 byte
				printf("n : %02x\n", buf[it++]); // nn = 分子
				printf("d : %02x\n", buf[it++]); // dd = 分母(二のマイナス乗で表す) 2:四分音符 3 : 八分音符 4 : 十六分音符
				printf("c : %02x\n", buf[it++]); // cc = メトロノーム間隔(四分音符間隔なら0x18(= 24))
				printf("b : %02x\n", buf[it++]); // bb = 四分音符あたりの三十二分音符の数
			}
			else if (buf[it] == 0x59) // キー(調)
			{
				printf("--Meta[0x59]\n");
				it++;
				it++; // 0x02
				// 2 byte
				it += 2;
			}
			else if (buf[it] == 0x7f) // シーケンサ特定メタ
			{
				printf("--Meta[0x7f]\n");
				unsigned char len = buf[++it];
				printf("len : %d\n", len);
				it += len + 1;
			}


		} // while (1)



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