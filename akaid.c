#include "raylib.h"  //
#include "raymath.h" //
#include <stdio.h>   //
#include <math.h>    //
#include <stdbool.h> //

#define Max_loons 10     //
#define MaxSpawnholes 5  //
#define Initial_arrow 10 //
#define loon_colors_no 4 //
typedef struct           //
{
    Vector2 position;
    bool isActive;
    bool isAmmo;
    int looncolorI;

} Balloon;

typedef struct //
{
    Vector2 position;
    Vector2 velocity;
    bool inflight;

} Arrow;

int main(void)
{
    const int screenwidth = 1000;                                                                         //
    const int screenheight = 600;                                                                         //
    InitWindow(screenwidth, screenheight, "HIT'EM ALL");                                                  //
    InitAudioDevice();                                                                                    //
    Music bgost1 = LoadMusicStream("Sounds/Carnival circus music  #shorts #carnival #circus #music.mp3"); //
    Sound GOmusic = LoadSound("Sounds\\16-game-over.mp3");                                                //
    Sound effect = LoadSound("Sounds\\arrow release1.mp3");                                               //
    Sound pop = LoadSound("Sounds\\loonpop.mp3");                                                         //
    PlayMusicStream(bgost1);                                                                              //
    Texture2D background = LoadTexture("Imagespng/bg.png");                                               //
    Rectangle source = {0, 0, (float)background.width, (float)background.height};
    Rectangle dest = {0, 0, (float)screenwidth, (float)screenheight};
    Vector2 origin = {0, 0};
    Texture2D gameover = LoadTexture("Imagespng/GAMEOVER.png");                        //
    Font Customfont = LoadFont("FONTS/carnivalee_freakshow/Carnevalee Freakshow.ttf"); //

    Texture2D looncolors[loon_colors_no];                                                              //
    looncolors[0] = LoadTexture("Imagespng\\Balloons_01\\Balloons_01_64x64_Alt_00_006.png");           //
    looncolors[1] = LoadTexture("Imagespng\\Balloons_01\\Balloons_01_64x64_Alt_00_005.png");           //
    looncolors[2] = LoadTexture("Imagespng\\Balloons_01\\Balloons_01_64x64_Alt_00_007.png");           //
    looncolors[3] = LoadTexture("Imagespng\\Balloons_01\\Balloons_01_64x64_Alt_00_002.png");           //
    Texture2D loonammocolor = LoadTexture("Imagespng\\Balloons_01\\Balloons_01_64x64_Alt_02_004.png"); //
    Texture2D arrowimg = LoadTexture("Imagespng\\arrow2.png");                                         //
    Texture2D bowimg = LoadTexture("Imagespng\\bow 2.png");                                            //

    Vector2 bowtop = {27, -34};//
    Vector2 bowbottom = {27, 34};//
    Vector2 bowtip = {-55, 0};
    Vector2 pivot = {215, 486};//
    Balloon loons[Max_loons];
    for (int i = 0; i < Max_loons; i++)
    {
        loons[i].isActive = false;
    }
    float Spawn_timer = 0.0f;//
    const float Spawn_interval = 2.0f;//
    const float balloon_velocity = 70.0f; //

    Vector2 spawn_holes[MaxSpawnholes];//
    for (int i = 0; i < MaxSpawnholes; i++)
    {
        spawn_holes[i] = (Vector2){620 + i * 80, 450};
    }//
    Arrow arrow1;
    arrow1.inflight = false;
    const float arrow_velocity = 700.0f;
    const float gravity = 400.0f;//
    int arrow_no = Initial_arrow;

    bool GAME_OVER = false;//

    int score = 0;//
    int highscore = 0;//
    FILE *f = fopen("D:\\PROJECT RAYLIB\\raylib_template\\highscore.txt", "r");//
    if (f != NULL)
    {
        fscanf(f, "%d", &highscore);
        fclose(f);
    }//

    SetTargetFPS(60); //

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawTexturePro(background, source, dest, origin, 0.0f, WHITE);
        UpdateMusicStream(bgost1);//

        Vector2 mouse = GetMousePosition();

        Vector2 posarrowleft = {10, 570};
        DrawTextEx(Customfont, TextFormat("Arrow Left: %d", arrow_no), (Vector2){posarrowleft.x + 2, posarrowleft.y + 2}, 32, 2, BLACK);
        DrawTextEx(Customfont, TextFormat("Arrow Left: %d", arrow_no), posarrowleft, 32, 2, WHITE);

        Vector2 posscore = {10, 10};
        DrawTextEx(Customfont, TextFormat("Score: %d", score), (Vector2){posscore.x + 2, posscore.y + 2}, 32, 2, BLACK);
        DrawTextEx(Customfont, TextFormat("Score: %d", score), posscore, 32, 2, WHITE);

        float angle = atan2f(mouse.y - pivot.y, mouse.x - pivot.x);
        Vector2 top = Vector2Add(Vector2Rotate(bowtop, angle), pivot);
        Vector2 bottom = Vector2Add(Vector2Rotate(bowbottom, angle), pivot);
        Vector2 tip = Vector2Add(Vector2Rotate(bowtip, angle), pivot);

        Spawn_timer += GetFrameTime();
        if (Spawn_timer >= Spawn_interval && (!GAME_OVER))
        {
            Spawn_timer = 0.0f;
            for (int i = 0; i < Max_loons; i++)
            {
                if (!loons[i].isActive)
                {
                    loons[i].isActive = true;
                    int holeI = GetRandomValue(0, MaxSpawnholes - 1);
                    loons[i].position = spawn_holes[holeI];
                    loons[i].looncolorI = GetRandomValue(0, loon_colors_no - 1);
                    int ammospawn = GetRandomValue(0, 3);
                    if (ammospawn == 0)
                    {
                        loons[i].isAmmo = true;
                    }
                    else
                        loons[i].isAmmo = false;
                    break;
                }
            }
        }
        for (int i = 0; i < Max_loons; i++)
        {
            if (loons[i].isActive)
            {
                loons[i].position.y -= balloon_velocity * GetFrameTime();
            }
            if (loons[i].position.y < 0)
            {
                loons[i].isActive = false;
            }
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (!arrow1.inflight) && (!GAME_OVER))
        {
            arrow1.position = pivot;
            arrow1.velocity = (Vector2){cosf(angle + PI) * arrow_velocity, sinf(angle + PI) * arrow_velocity};
            arrow1.inflight = true;
            arrow_no--;
            PlaySound(effect);
        }
        // arrow physics + collision(only while flying)
        if (arrow1.inflight)
        {
            arrow1.velocity.y += gravity * GetFrameTime();
            arrow1.position.x += arrow1.velocity.x * GetFrameTime();
            arrow1.position.y += arrow1.velocity.y * GetFrameTime();

            if (arrow1.position.x < 0 || arrow1.position.x > screenwidth || arrow1.position.y < 0 || arrow1.position.y > screenheight)
            {
                arrow1.inflight = false;
            }

            // checking hit against every active balloon
            for (int i = 0; i < Max_loons; i++)
            {
                if (loons[i].isActive)
                {
                    if (CheckCollisionCircles(arrow1.position, 10, loons[i].position, 30))
                    {
                        PlaySound(pop);
                        arrow1.inflight = false;
                        loons[i].isActive = false;

                        if (loons[i].isAmmo)
                        {
                            arrow_no += 2;
                            score += 30;
                        }
                        else
                            score += 20;

                        break;
                    }
                }
            }
        }

        //  game over exactly once, when arrows run out and nothing's in flight
        if (arrow_no <= 0 && (!arrow1.inflight) && (!GAME_OVER))
        {
            GAME_OVER = true;
            StopMusicStream(bgost1);
            PlaySound(GOmusic);
            if (score > highscore)
            {
                highscore = score;
                FILE *f = fopen("D:\\PROJECT RAYLIB\\raylib_template\\highscore.txt", "w");
                fprintf(f, "%d", highscore);
                fclose(f);
            }
        }
        // RESTART after gameover
        if (GAME_OVER && IsKeyPressed(KEY_R))//
        {
            GAME_OVER = false;
            arrow_no = Initial_arrow;
            score = 0;
            arrow1.inflight = false;
            Spawn_timer = 0.0f;
            for (int i = 0; i < Max_loons; i++)
            {
                loons[i].isActive = false;
            }
            StopSound(GOmusic);
            PlayMusicStream(bgost1);
        }//

        // bowimage
        float bowwidth = 150;
        float bowheight = 150;
        Rectangle bowsrc = {0, 0, (float)bowimg.width, (float)bowimg.height};
        Rectangle bowdest = {pivot.x, pivot.y, bowwidth, bowheight};
        Vector2 boworg = {bowwidth / 2, bowheight / 2};
        DrawTexturePro(bowimg, bowsrc, bowdest, boworg, (angle + PI) * RAD2DEG, WHITE);

        // drawing active balloons, gold texture for ammo, random color otherwise
        for (int i = 0; i < Max_loons; i++)
        {
            if (loons[i].isActive)
            {
                Texture2D ballonColor = loons[i].isAmmo ? loonammocolor : looncolors[loons[i].looncolorI];

                Rectangle loonSource = {0, 0, (float)ballonColor.width, (float)ballonColor.height};
                Rectangle loonDest = {loons[i].position.x, loons[i].position.y, (float)ballonColor.width, (float)ballonColor.height};
                Vector2 loonOrigin = {(float)ballonColor.width / 2, (float)ballonColor.height / 2}; // centered so position = center

                DrawTexturePro(ballonColor, loonSource, loonDest, loonOrigin, 0.0f, WHITE);
            }
        }
        // Arrow img needs to be rotated towards its velocity direction
        float arrowangle = atan2f(arrow1.velocity.y, arrow1.velocity.x);

        // Arrow drawing
        if (arrow1.inflight)
        {
            Rectangle arrowsource = {0, 0, (float)arrowimg.width, (float)arrowimg.height};
            Rectangle arrowdest = {arrow1.position.x, arrow1.position.y, 70, 30};
            Vector2 arroworigin = {70 / 2, 30 / 2}; // arrow is rotated wtr to its center
            DrawTexturePro(arrowimg, arrowsource, arrowdest, arroworigin, arrowangle * RAD2DEG, WHITE);
        }
        else if (arrow_no > 0 && !GAME_OVER) // to keep arrow img while at rest
        {
            Vector2 nockOffset = Vector2Rotate((Vector2){5, 0}, angle); // nudge along aim dir
            Vector2 restPos = Vector2Add(pivot, nockOffset);            // so that arrow dont sit exactly at pivot

            Rectangle arrowsource = {0, 0, (float)arrowimg.width, (float)arrowimg.height};
            Rectangle arrowdest = {restPos.x, restPos.y, 70, 30};
            Vector2 arroworigin = {70 / 2, 30 / 2}; // arrow is rotated wtr to its center
            DrawTexturePro(arrowimg, arrowsource, arrowdest, arroworigin, (angle + PI) * RAD2DEG, WHITE);
        }

        // Textures to show after game over
        if (GAME_OVER)
        {
            DrawTexture(gameover, 130, 120, WHITE);

            Vector2 restart_pos = {400, 400};
            DrawTextEx(Customfont, "PRESS R TO RESTART", (Vector2){restart_pos.x + 2, restart_pos.y + 2}, 32, 2, RED);
            DrawTextEx(Customfont, "PRESS R TO RESTART", restart_pos, 32, 2, GOLD);
            // showing scores
            DrawTextEx(Customfont, TextFormat("Your Score: %d", score), (Vector2){300 + 2, 440 + 2}, 32, 2, BLACK);
            DrawTextEx(Customfont, TextFormat("Your Score: %d", score), (Vector2){300, 440}, 32, 2, GOLD);

            DrawTextEx(Customfont, TextFormat("Highest Score: %d", highscore), (Vector2){300 + 2, 480 + 2}, 32, 2, BLACK);
            DrawTextEx(Customfont, TextFormat("Highest Score: %d", highscore), (Vector2){300, 480}, 32, 2, GOLD);
        }

        EndDrawing();//
    }

    UnloadSound(effect);//
    UnloadSound(pop);
    UnloadMusicStream(bgost1);
    UnloadSound(GOmusic);
    UnloadTexture(background);
    UnloadTexture(gameover);
    UnloadFont(Customfont);
    UnloadTexture(loonammocolor);
    for (int i = 0; i < loon_colors_no; i++)
    {
        UnloadTexture(looncolors[i]);
    }
    UnloadTexture(arrowimg);
    UnloadTexture(bowimg);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}