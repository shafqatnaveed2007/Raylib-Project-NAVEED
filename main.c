#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>

#define WIDTH 1600
#define HEIGHT 800
#define MAX_BALLOONS 10
#define SPAWNHOLES 5
#define NORMAL_BALLOONS_NUM 4

typedef struct
{
    Vector2 position;
    float speed;
    float radius;
    bool active;
    bool isgold;
    int textureindex;
} Balloon;

typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool active;
} Arrow;

int main(void)
{
    InitWindow(WIDTH, HEIGHT, "HIT 'EM ALL");
    InitAudioDevice();
    SetTargetFPS(60);

    Music gamebgmusic = LoadMusicStream("assets/audio/Carnival Music (Game Window).mp3");
    PlayMusicStream(gamebgmusic);
    Sound shootsound = LoadSound("assets/audio/Gun shooting.ogg");
    Sound popsound = LoadSound("assets/audio/Balloon Pop.mp3");
    Sound gameoversound = LoadSound("assets/audio/game over.mp3");

    Texture2D background = LoadTexture("assets/sprites/Gamescreen.png");
    Texture2D gameovertexture = LoadTexture("assets/sprites/gameover.png");
    Texture2D bowimage = LoadTexture("assets/sprites/bow.png");
    Texture2D arrowimage = LoadTexture("assets/sprites/arrow.png");

    Font customfont = LoadFont("assets/fonts/Carnival Font.ttf");
    // spawn hole setup for balloons
    Vector2 spawnholes[SPAWNHOLES];
    for (int i = 0; i < SPAWNHOLES; i++)
    {
        spawnholes[i] = (Vector2){1000.0f + i * 130.0f, HEIGHT + 50.0f};
    }

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
    const float balloonradius = (float)normalballoons[0].width * 0.4f;

    // arrow
    Arrow arrow1 = {0};

    // game variables
    int score = 0;
    int highscore = 0;
    bool gameover = false;
    int arrowsleft = 10;
    const float gravity = 980.0f;
    float spawntimer = 0.0f;
    const float spawninterval = 1.8f;
    float pulldistance = 0.0f;

    // highest score
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
        UpdateMusicStream(gamebgmusic);

        Vector2 mouseposition = GetMousePosition();
        Vector2 arrowpivot = {260.0f, 630.0f};

        // initialising arrow settings
        Vector2 aimdirection = {1.0f, 0.0f};
        float aimangle = 0.0f;
        // Pull-back mechanism variables
        const float maxpulldistance = 120.0f;
        const float pullspeed = 120.0f;
        const float minarrowspeed = 400.0f;
        const float maxarrowspeed = 2200.0f;
        // aiming, pulling back and shooting arrow
        if (arrowsleft > 0 && gameover == false)
        {
            // Updating aim angle with mouse position
            float mousepointerangle = atan2f(mouseposition.y - arrowpivot.y, mouseposition.x - arrowpivot.x);
            aimangle = fmaxf(-PI / 4.0f, fminf(PI / 4.0f, mousepointerangle));
            aimdirection = (Vector2){cosf(aimangle), sinf(aimangle)};
            // Pulling back of string
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && arrow1.active == false)
            {
                pulldistance += pullspeed * dt;
                if (pulldistance > maxpulldistance)
                {
                    pulldistance = maxpulldistance;
                }
            }
            // arrow released
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) == true && arrow1.active == false)
            {
                float pullratio = pulldistance / maxpulldistance;
                float arrowspeed = minarrowspeed + pullratio * (maxarrowspeed - minarrowspeed);

                arrow1.position = arrowpivot;
                arrow1.velocity = Vector2Scale(aimdirection, arrowspeed);
                arrow1.radius = 18.0f;
                arrow1.active = true;
                arrowsleft--;
                PlaySound(shootsound);
                pulldistance = 0.0f;
            }
        }
        // projectile formula
        if (arrow1.active == true)
        {
            arrow1.velocity.y += gravity * dt;
            arrow1.position = Vector2Add(arrow1.position, Vector2Scale(arrow1.velocity, dt));

            if (arrow1.position.x > WIDTH + 50 || arrow1.position.y > HEIGHT + 50)
            {
                arrow1.active = false;
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
                        int holeindex = GetRandomValue(0, SPAWNHOLES - 1);
                        balloons[i].position = spawnholes[holeindex];
                        balloons[i].speed = 160.0f;
                        balloons[i].radius = balloonradius;
                        balloons[i].active = true;
                        balloons[i].isgold = (GetRandomValue(1, 10) <= 2);
                        balloons[i].textureindex = GetRandomValue(0, NORMAL_BALLOONS_NUM - 1);
                        break;
                    }
                }
            }
        }
        // floating of balloon & collision of arrow with balloon
        for (int i = 0; i < MAX_BALLOONS; i++)
        {
            if (balloons[i].active == false)
                continue;

            balloons[i].position.y -= balloons[i].speed * dt;
            if (balloons[i].position.y < -100.0f)
            {
                balloons[i].active = false;
            }

            if (arrow1.active == true && CheckCollisionCircles(arrow1.position, arrow1.radius, balloons[i].position, balloons[i].radius) == true)
            {
                arrow1.active = false;
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
        // checking game over
        if (arrowsleft <= 0 && arrow1.active == false && gameover == false)
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
        // RESTART
        if (gameover == true && IsKeyPressed(KEY_R) == true)
        {
            gameover = false;
            arrowsleft = 10;
            score = 0;
            spawntimer = 0.0f;

            arrow1.active = false;

            for (int i = 0; i < MAX_BALLOONS; i++)
            {
                balloons[i].active = false;
            }
            StopSound(gameoversound);
            PlayMusicStream(gamebgmusic);
        }

        // drawing portion
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // drawing bg
        Vector2 origin = {0.0f, 0.0f};
        Rectangle bgsource = {0.0f, 0.0f, (float)background.width, (float)background.height};
        Rectangle bgdest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
        DrawTexturePro(background, bgsource, bgdest, origin, 0.0f, WHITE);
        // drawing bow
        float bowwidth = 240.0f;
        float bowheight = 240.0f;
        Rectangle bowsource = {0.0f, 0.0f, (float)bowimage.width, (float)bowimage.height};
        Rectangle bowdest = {arrowpivot.x, arrowpivot.y, bowwidth, bowheight};
        Vector2 boworg = {bowwidth / 2.0f, bowheight / 2.0f};
        DrawTexturePro(bowimage, bowsource, bowdest, boworg, aimangle * RAD2DEG, WHITE);

        // drawing bow string
        Vector2 bowstringbottom = Vector2Add(arrowpivot, Vector2Rotate((Vector2){0.0f, -95.0f}, aimangle));
        Vector2 bowstringtop = Vector2Add(arrowpivot, Vector2Rotate((Vector2){0.0f, 95.0f}, aimangle));
        Vector2 pullpoint = Vector2Subtract(arrowpivot, Vector2Scale(aimdirection, pulldistance));
        DrawLineEx(bowstringbottom, pullpoint, 3.5f, LIGHTGRAY);
        DrawLineEx(bowstringtop, pullpoint, 3.5f, LIGHTGRAY);

        // drawing arrow at bow
        if (arrowsleft > 0 && gameover == false && arrow1.active == false)
        {
            Rectangle arrowsource = {0.0f, 0.0f, (float)arrowimage.width, (float)arrowimage.height};
            Rectangle arrowdest = {pullpoint.x, pullpoint.y, 110.0f, 45.0f};
            Vector2 arroworigin = {110.0f / 2.0f, 45.0f / 2.0f};
            DrawTexturePro(arrowimage, arrowsource, arrowdest, arroworigin, aimangle * RAD2DEG, WHITE);
        }

        // drawing arrow in air
        if (arrow1.active == true)
        {
            float arrowangle = atan2f(arrow1.velocity.y, arrow1.velocity.x);
            Rectangle arrowsource = {0.0f, 0.0f, (float)arrowimage.width, (float)arrowimage.height};
            Rectangle arrowdest = {arrow1.position.x, arrow1.position.y, 110.0f, 45.0f};
            Vector2 arroworigin = {110.0f / 2.0f, 45.0f / 2.0f};
            DrawTexturePro(arrowimage, arrowsource, arrowdest, arroworigin, arrowangle * RAD2DEG, WHITE);
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
            const char *restarttext = "PRESS R TO RESTART";
            DrawTextEx(customfont, restarttext, (Vector2){650.0f, 500.0f}, 48.0f, 2, BLACK);
            DrawTextEx(customfont, restarttext, (Vector2){648.0f, 498.0f}, 48.0f, 2, RAYWHITE);

            const char *scoretext = TextFormat("YOUR SCORE: %d", score);
            DrawTextEx(customfont, scoretext, (Vector2){650.0f, 600.0f}, 48.0f, 2, BLACK);
            DrawTextEx(customfont, scoretext, (Vector2){648.0f, 598.0f}, 48.0f, 2, RAYWHITE);

            const char *highscoretext = TextFormat("HIGHEST SCORE: %d", highscore);
            DrawTextEx(customfont, highscoretext, (Vector2){650.0f, 650.0f}, 48.0f, 2, BLACK);
            DrawTextEx(customfont, highscoretext, (Vector2){648.0f, 648.0f}, 48.0f, 2, RAYWHITE);
        }

        EndDrawing();
    }
    UnloadTexture(background);
    for (int i = 0; i < NORMAL_BALLOONS_NUM; i++)
    {
        UnloadTexture(normalballoons[i]);
    }
    UnloadTexture(specialballoon);
    UnloadTexture(gameovertexture);
    UnloadTexture(bowimage);
    UnloadTexture(arrowimage);
    UnloadFont(customfont);
    UnloadSound(shootsound);
    UnloadSound(popsound);
    UnloadMusicStream(gamebgmusic);
    UnloadSound(gameoversound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}