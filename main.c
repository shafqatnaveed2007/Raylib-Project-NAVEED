// all header files
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// all macros needed
#define WIDTH 1600
#define HEIGHT 800
#define MAX_BALLOONS 10
#define MAX_ARROWS 1
#define MAX_SPAWNHOLES 5
#define NORMAL_BALLOONS_NUM 4

// balloon structure
typedef struct
{
    Vector2 position;
    float speed;
    float radius;
    bool active;
    bool isgold;
    int textureindex;
} Balloon;

// arrow structure
typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool active;
} Arrow;

// balloon spawning function
void SpawnBalloon(Balloon *b, float balloonradius, Vector2 spawnholes[])
{
    int holeindex = GetRandomValue(0, MAX_SPAWNHOLES - 1);
    b->position = spawnholes[holeindex];
    b->speed = (float)160;
    b->radius = balloonradius;
    b->active = true;
    b->isgold = (GetRandomValue(1, 10) <= 2);
    b->textureindex = GetRandomValue(0, NORMAL_BALLOONS_NUM - 1);
}
/////////////////////////////////////////////////////////////
// main func
int main(void)
{
    // window and audio init
    InitWindow(WIDTH, HEIGHT, "HIT 'EM ALL");
    InitAudioDevice();
    SetTargetFPS(60);

    // all music loading and playing and setting volume
    Music gamebgmusic = LoadMusicStream("assets/audio/Carnival Music (Game Window).mp3");
    PlayMusicStream(gamebgmusic);

    Sound shootsound = LoadSound("assets/audio/Gun shooting.ogg");
    Sound popsound = LoadSound("assets/audio/Balloon Pop.mp3");
    Sound gameoversound = LoadSound("assets/audio/game over.mp3");

    // bg loading
    Texture2D background = LoadTexture("assets/sprites/Gamescreen.png");
    Texture2D gameovertexture = LoadTexture("assets/sprites/gameover.png");

    // loading custom font
    Font customfont = LoadFont("assets/fonts/Carnival Font.ttf");

    // boy character texture setup
    Texture2D boyimg = LoadTexture("assets/sprites/boy.png");

    // bow and arrow textures setup
    Texture2D bowimage = LoadTexture("assets/sprites/bow.png");
    Texture2D arrowimage = LoadTexture("assets/sprites/arrow.png");
    /////////////////////////////////////////////////////////////////////
    // spawn hole setup for balloons
    Vector2 spawnholes[MAX_SPAWNHOLES];
    for (int i = 0; i < MAX_SPAWNHOLES; i++)
    {
        spawnholes[i] = (Vector2){1000.0f + i * 130.0f, HEIGHT + 50.0f};
    }
    float spawntimer = 0.0f;
    const float spawninterval = 1.8f;

    // loading balloons and initializing
    Texture2D normalballoons[NORMAL_BALLOONS_NUM];
    for (int i = 0; i < NORMAL_BALLOONS_NUM; i++)
    {
        normalballoons[i] = LoadTexture(TextFormat("assets/sprites/normalballoon%d.png", i + 1));
    }
    Texture2D specialballoon = LoadTexture("assets/sprites/specialballoon.png");
    Balloon balloons[MAX_BALLOONS] = {0};
    for (int i = 0; i < MAX_BALLOONS; i++)
    {
        balloons[i].active = false;
    }

    // Calculate radius
    const float balloonradius = (float)normalballoons[0].width * 0.4f;

    // arrow/projectile settings
    Arrow arrows[MAX_ARROWS] = {0};
    int arrowsleft = 10;
    const float gravity = 980.0f;

    // game variables setting
    int score = 0;
    int highscore = 0;
    bool gameover = false;

    // Read Highscore from file before starting game
    FILE *highscorefile = fopen("highscore.txt", "r");
    if (highscorefile != NULL)
    {
        fscanf(highscorefile, "%d", &highscore);
        fclose(highscorefile);
    }

    // each game frame
    while (WindowShouldClose() == false)
    {
        const float dt = GetFrameTime();

        // streaming audio
        UpdateMusicStream(gamebgmusic);

        // RESTART after gameover
        if (gameover == true && IsKeyPressed(KEY_R) == true)
        {
            gameover = false;
            arrowsleft = 10;
            score = 0;
            spawntimer = 0.0f;

            for (int i = 0; i < MAX_ARROWS; i++)
            {
                arrows[i].active = false;
            }

            for (int i = 0; i < MAX_BALLOONS; i++)
            {
                balloons[i].active = false;
            }
            StopSound(gameoversound);
            PlayMusicStream(gamebgmusic);
        }

        Vector2 mouseposition = GetMousePosition();
        Vector2 arrowpivot = {260.0f, 630.0f};

        // Checking if any arrow is currently in flight
        bool hasarrowsinflight = false;
        for (int i = 0; i < MAX_ARROWS; i++)
        {
            if (arrows[i].active == true)
            {
                hasarrowsinflight = true;
                break;
            }
        }
        // initialising arrow settings
        float arrowspeed = 1500.0f;
        Vector2 aimdirection = {1.0f, 0.0f};
        float aimangle = 0.0f;

        // aiming and shooting arrow
        if (arrowsleft > 0 && gameover == false)
        {
            // Continuously update aim angle according to mouse cursor
            float mousepointerangle = atan2f(mouseposition.y - arrowpivot.y, mouseposition.x - arrowpivot.x);
            aimangle = fmaxf(-PI / 4.0f, fminf(PI / 4.0f, mousepointerangle));
            aimdirection = (Vector2){cosf(aimangle), sinf(aimangle)};

            // releasing arrow
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) == true && hasarrowsinflight == false)
            {
                for (int i = 0; i < MAX_ARROWS; i++)
                {
                    if (arrows[i].active == false)
                    {
                        arrows[i].position = arrowpivot;
                        arrows[i].velocity = Vector2Scale(aimdirection, arrowspeed);
                        arrows[i].radius = 18.0f;
                        arrows[i].active = true;
                        arrowsleft--;
                        PlaySound(shootsound);
                        hasarrowsinflight = true;
                        break;
                    }
                }
            }
        }

        // balloon spawning
        if (gameover == false)
        {
            spawntimer += dt;
            if (spawntimer >= spawninterval)
            {
                spawntimer = 0.0f;
                for (int i = 0; i < MAX_BALLOONS; i++)
                {
                    if (balloons[i].active == false)
                    {
                        SpawnBalloon(&balloons[i], balloonradius, spawnholes);
                        break;
                    }
                }
            }
        }

        // floating of balloon vertically upwards & collision of arrow with balloon to pop it (with sound)
        for (int i = 0; i < MAX_BALLOONS; i++)
        {
            if (balloons[i].active == false)
                continue;

            balloons[i].position.y -= balloons[i].speed * dt;
            if (balloons[i].position.y < -100.0f)
            {
                balloons[i].active = false;
            }
            for (int j = 0; j < MAX_ARROWS; j++)
            {
                if (arrows[j].active == true && CheckCollisionCircles(arrows[j].position, arrows[j].radius, balloons[i].position, balloons[i].radius) == true)
                {
                    arrows[j].active = false;
                    balloons[i].active = false;
                    if (balloons[i].isgold == true)
                    {
                        arrowsleft += 2;
                        score += 20;
                    }
                    else
                    {
                        score += 20;
                    }
                    PlaySound(popsound);
                }
            }
        }

        // projectile formula
        hasarrowsinflight = false;
        for (int i = 0; i < MAX_ARROWS; i++)
        {
            if (arrows[i].active == true)
            {
                hasarrowsinflight = true;
                arrows[i].velocity.y += gravity * dt;
                arrows[i].position = Vector2Add(arrows[i].position, Vector2Scale(arrows[i].velocity, dt));

                if (arrows[i].position.x > WIDTH + 50 || arrows[i].position.y > HEIGHT + 50)
                {
                    arrows[i].active = false;
                }
            }
        }

        // checking game over state
        if (arrowsleft <= 0 && hasarrowsinflight == false && gameover == false)
        {
            gameover = true;
            StopMusicStream(gamebgmusic);
            PlaySound(gameoversound);

            if (score > highscore)
            {
                highscore = score;
                FILE *fw = fopen("highscore.txt", "w");
                if (fw != NULL)
                {
                    fprintf(fw, "%d", highscore);
                    fclose(fw);
                }
            }
        }

        // drawing portion
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // drawing bg
        Vector2 origin = {0.0f, 0.0f};
        Rectangle bgsource = {0.0f, 0.0f, (float)background.width, (float)background.height};
        Rectangle bgdest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
        DrawTexturePro(background, bgsource, bgdest, origin, 0.0f, WHITE);

        // drawing boy
        float boywidth = 415.0f;
        float boyheight = 415.0f;
        Rectangle boysource = {0.0f, 0.0f, (float)boyimg.width, (float)boyimg.height};
        Rectangle boydest = {arrowpivot.x - 50.0f, arrowpivot.y + 30.0f, boywidth, boyheight};
        Vector2 boyorigin = {boywidth / 2.0, boyheight / 2.0};
        DrawTexturePro(boyimg, boysource, boydest, boyorigin, 0.0f, WHITE);

        // drawing bow rotated towards mouse pointer
        float bowwidth = 240.0f;
        float bowheight = 240.0f;
        Rectangle bowsource = {0.0f, 0.0f, (float)bowimage.width, (float)bowimage.height};
        Rectangle bowdest = {arrowpivot.x, arrowpivot.y, bowwidth, bowheight};
        Vector2 boworg = {bowwidth / 2.0f, bowheight / 2.0f};
        DrawTexturePro(bowimage, bowsource, bowdest, boworg, aimangle * RAD2DEG, WHITE);

        // Calculate coordinates for bow string top and bottom
        Vector2 bowstringbottom = Vector2Add(arrowpivot, Vector2Rotate((Vector2){0.0f, -95.0f}, aimangle));
        Vector2 bowstringtop = Vector2Add(arrowpivot, Vector2Rotate((Vector2){0.0f, 95.0f}, aimangle));

        // drawing bow string
        DrawLineEx(bowstringbottom, arrowpivot, 3.5f, LIGHTGRAY);
        DrawLineEx(bowstringtop, arrowpivot, 3.5f, LIGHTGRAY);

        // drawing arrow at bow
        if (arrowsleft > 0 && gameover == false && hasarrowsinflight == false)
        {
            Rectangle arrowsource = {0.0f, 0.0f, (float)arrowimage.width, (float)arrowimage.height};
            Rectangle arrowdest = {arrowpivot.x, arrowpivot.y, 110.0f, 45.0f};
            Vector2 arroworigin = {110.0f / 2.0f, 45.0f / 2.0f};
            DrawTexturePro(arrowimage, arrowsource, arrowdest, arroworigin, aimangle * RAD2DEG, WHITE);
        }

        // drawing the balloons
        for (int i = 0; i < MAX_BALLOONS; i++)
        {
            if (balloons[i].active == true)
            {
                Texture2D balloontexture = balloons[i].isgold ? specialballoon : normalballoons[balloons[i].textureindex];
                DrawTextureV(balloontexture, (Vector2){balloons[i].position.x - (balloontexture.width / 2.0f), balloons[i].position.y - (balloontexture.height / 2.0f)}, WHITE);
            }
        }

        // drawing arrows in air
        for (int i = 0; i < MAX_ARROWS; i++)
        {
            if (arrows[i].active)
            {
                float arrowangle = atan2f(arrows[i].velocity.y, arrows[i].velocity.x);
                Rectangle arrowsource = {0.0f, 0.0f, (float)arrowimage.width, (float)arrowimage.height};
                Rectangle arrowdest = {arrows[i].position.x, arrows[i].position.y, 110.0f, 45.0f};
                Vector2 arroworigin = {110.0f / 2.0f, 45.0f / 2.0f};
                DrawTexturePro(arrowimage, arrowsource, arrowdest, arroworigin, arrowangle * RAD2DEG, WHITE);
            }
        }

        // score and ammo display
        DrawTextEx(customfont, TextFormat("SCORE: %d", score), (Vector2){32, 27}, 42, 2, BLACK);
        DrawTextEx(customfont, TextFormat("SCORE: %d", score), (Vector2){30, 25}, 42, 2, WHITE);

        DrawTextEx(customfont, TextFormat("ARROWS: %d", arrowsleft), (Vector2){32, 77}, 42, 2, BLACK);
        DrawTextEx(customfont, TextFormat("ARROWS: %d", arrowsleft), (Vector2){30, 75}, 42, 2, GOLD);

        // game over and final score display
        if (gameover == true)
        {
            float gameoverwidth = (float)gameovertexture.width * 1.8f;
            float gameoverheight = (float)gameovertexture.height * 1.8f;
            Rectangle gameoversource = {0.0f, 0.0f, (float)gameovertexture.width, (float)gameovertexture.height};
            Rectangle gameoverdest = {800.0f, 300.0f, gameoverwidth, gameoverheight};
            Vector2 gameoverorigin = {gameoverwidth / 2.0, gameoverheight / 2.0};
            DrawTexturePro(gameovertexture, gameoversource, gameoverdest, gameoverorigin, 0.0f, WHITE);

            // Settings for Game Over text
            float fontsize = 48.0f;
            float linespacing = 52.0f;

            // Restart Text
            const char *restarttext = "PRESS R TO RESTART";
            DrawTextEx(customfont, restarttext, (Vector2){650.0f, 500.0f}, fontsize, 2, BLACK);
            DrawTextEx(customfont, restarttext, (Vector2){648.0f, 498.0f}, fontsize, 2, RAYWHITE);

            // Final score + high score display
            const char *scoretext = TextFormat("YOUR SCORE: %d", score);
            DrawTextEx(customfont, scoretext, (Vector2){650.0f, 600.0f}, fontsize, 2, BLACK);
            DrawTextEx(customfont, scoretext, (Vector2){648.0f, 598.0f}, fontsize, 2, RAYWHITE);

            const char *highscoretext = TextFormat("HIGHEST SCORE: %d", highscore);
            DrawTextEx(customfont, highscoretext, (Vector2){650.0f, 650.0f}, fontsize, 2, BLACK);
            DrawTextEx(customfont, highscoretext, (Vector2){648.0f, 648.0f}, fontsize, 2, RAYWHITE);
        }

        EndDrawing();
    }
    // unloading all textures/drawings
    UnloadTexture(background);
    for (int i = 0; i < NORMAL_BALLOONS_NUM; i++)
    {
        UnloadTexture(normalballoons[i]);
    }
    UnloadTexture(specialballoon);
    UnloadTexture(gameovertexture);
    UnloadTexture(boyimg);
    UnloadTexture(bowimage);
    UnloadTexture(arrowimage);
    UnloadFont(customfont);

    // unloading all audio
    UnloadSound(shootsound);
    UnloadSound(popsound);
    UnloadMusicStream(gamebgmusic);
    UnloadSound(gameoversound);
    CloseAudioDevice();

    CloseWindow();
    return 0;
}