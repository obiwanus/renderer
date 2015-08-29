#include "renderer.h"

#include <windows.h>


// These are saved as global vars on the first call of GameUpdateAndRender
global game_offscreen_buffer *GameBackBuffer;
global game_memory *GameMemory;
global model Model;


inline int
TruncateReal32(r32 Value)
{
    int Result = (int) Value;
    return Result;
}


inline int
RoundReal32(r32 Value)
{
    // TODO: think about overflow
    return TruncateReal32(Value + 0.5f);
}


void *
GameMemoryAlloc(int SizeInBytes)
{
    void *Result = GameMemory->Free;

    GameMemory->Free = (void *)((u8 *)GameMemory->Free + SizeInBytes);
    i64 CurrentSize = ((u8 *)GameMemory->Free - (u8 *) GameMemory->Start);
    Assert(CurrentSize < GameMemory->MemorySize);

    return Result;
}


// Super awesome code below

internal int
ReadLine(wchar_t *FileData, int i, wchar_t *Result)
{
    int j = 0;
    while (j < 100 && FileData[i+j] != L'\n')
    {
        Result[j] = FileData[i+j];
        j++;
    }

    Result[j] = 0;

    return i + j + 1;
}


internal int
ReadWord(wchar_t *Word, wchar_t **Cursor, wchar_t Delimiter)
{
    int i = 0;
    wchar_t *Line = *Cursor;

    while ((*Word++ = *Line++) != Delimiter && i++ < 20)
    {
        if (*Line == 0)
        {
            *Word = 0;
            return -1;
        }
    }
    Word--;
    *Word = 0;
    *Cursor = Line;
    return ++i;
}


internal void
ReadModelFromFile(char *Filename, model *Result)
{
    file_read_result FileRead = GameMemory->DEBUGPlatformReadEntireFile(Filename);
    GameMemory->ConvertBytesToString(FileRead.Memory, (int)FileRead.MemorySize, &Result->File);

    Result->FileCharCount = (int)FileRead.MemorySize;

    wchar_t Line[100];
    int i = 0;

    // Count faces and vertices
    while (i < Result->FileCharCount)
    {
        i = ReadLine(Result->File, i, Line);

        wchar_t Words[10][20] = {};  // max 10 words 20 symbols long
        int j = 0;
        wchar_t *Cursor = Line;
        while (ReadWord(Words[j++], &Cursor, L' ') != -1)
            ;

        if (Words[0][0] == L'v' && Words[0][1] == 0)
        {
            Result->VertCount++;
        }
        else if (Words[0][0] == L'f' && Words[0][1] == 0)
        {
            Result->FaceCount++;
        }

    }

    // Allocate some space (too lazy to calculate)
    Result->Verts = (v3 *) GameMemoryAlloc((Result->VertCount + 1) * sizeof(v3));
    Result->Faces = (v3 *) GameMemoryAlloc((Result->FaceCount + 1) * sizeof(v3));

    // Fill faces and vertices
    i = 0;
    int vert_i = 0;
    int face_i = 0;
    while (i < Result->FileCharCount)
    {
        i = ReadLine(Result->File, i, Line);

        wchar_t Words[10][20] = {};  // max 10 words 20 symbols long
        int j = 0;
        wchar_t *Cursor = Line;
        while (ReadWord(Words[j++], &Cursor, L' ') != -1)
            ;

        if (Words[0][0] == L'v' && Words[0][1] == 0)
        {
            v3 *Vertex = &Result->Verts[vert_i++];
            Vertex->x = (r32)(_wtof(Words[1]) + 1.0f) * GameBackBuffer->Width / 4.0f;
            Vertex->y = (r32)(_wtof(Words[2]) + 1.0f) * GameBackBuffer->Width / 4.0f;
            Vertex->z = (r32)(_wtof(Words[3]) + 1.0f) * GameBackBuffer->Width / 4.0f;
        }
        else if (Words[0][0] == L'f' && Words[0][1] == 0)
        {
            v3 *Face = &Result->Faces[face_i++];

            // Split each word
            wchar_t FaceWord[10] = {};

            Cursor = Words[1];
            ReadWord(FaceWord, &Cursor, L'/');
            Face->x = (r32)_wtof(FaceWord);

            Cursor = Words[2];
            ReadWord(FaceWord, &Cursor, L'/');
            Face->y = (r32)_wtof(FaceWord);

            Cursor = Words[3];
            ReadWord(FaceWord, &Cursor, L'/');
            Face->z = (r32)_wtof(FaceWord);
        }

    }
}


inline void
SetPixel(int X, int Y, u32 Color)
{
    // Point 0, 0 is in the left bottom corner
    int Pitch = GameBackBuffer->Width * GameBackBuffer->BytesPerPixel;
    u8 *Row = (u8 *)GameBackBuffer->Memory + (GameBackBuffer->Height - 1) * Pitch
              - Pitch * Y
              + X * GameBackBuffer->BytesPerPixel;
    u32 *Pixel = (u32 *)Row;
    *Pixel = Color;
}


internal void
DrawLine(r32 StartX, r32 StartY, r32 EndX, r32 EndY, u32 Color)
{
    // Bresenham's algorithm
    r32 DeltaX = EndX - StartX;
    r32 DeltaY = EndY - StartY;
    int SignX = (DeltaX < 0) ? -1: 1;
    int SignY = (DeltaY < 0) ? -1: 1;
    int X = (int)StartX;
    int Y = (int)StartY;
    r32 Error = 0;

    if (DeltaX)
    {
        r32 DeltaErr = Abs(DeltaY / DeltaX);
        while (X != (int)EndX)
        {
            SetPixel(X, Y, Color);
            X += SignX;
            Error += DeltaErr;
            while (Error >= 0.5f)
            {
                SetPixel(X, Y, Color);
                Y += SignY;
                Error -= 1.0f;
            }
        }
    }
    else if (DeltaY)
    {
        while (Y != (int)EndY)
        {
            SetPixel(X, Y, Color);
            Y += SignY;
        }
    }

}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    // Update global vars
    GameBackBuffer = Buffer;
    GameMemory = Memory;

    if (!Model.FileCharCount)
    {
        ReadModelFromFile("african_head.model", &Model);
    }

    u32 Color = 0x00777777;
    for (int i = 0; i < Model.FaceCount; i++)
    {
        v3 *Face = &Model.Faces[i];

        v3 *Vert1 = &Model.Verts[(int)Face->x];
        v3 *Vert2 = &Model.Verts[(int)Face->y];
        v3 *Vert3 = &Model.Verts[(int)Face->z];
        DrawLine(Vert1->x, Vert1->y, Vert2->x, Vert2->y, Color);
        // DrawLine(Vert1->x, Vert1->y, Vert3->x, Vert3->y, Color);
        // DrawLine(Vert2->x, Vert2->y, Vert3->x, Vert3->y, Color);
    }
}


