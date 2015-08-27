#include "renderer.h"

// TODO: delete
#include <windows.h>


// These are saved as global vars on the first call of GameUpdateAndRender
global game_offscreen_buffer *GameBackBuffer;
global game_memory *GameMemory;

global entity *Players;
global entity *Ball;


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


internal void *
ReadModelFromFile(char *Filename)
{
    file_read_result FileReadResult = GameMemory->DEBUGPlatformReadEntireFile(Filename);

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

    DrawLine(-10, -100, 300, 250, 0x00777777);
}


