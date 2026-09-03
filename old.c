// all header files
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// all macros needed
#define WIDTH 1600
#define HEIGHT 800
#define OLDMAN_FRAMES 25
#define BOY_FRAMES 20
#define MAX_BALLOONS 10
#define MAX_ARROWS 10
#define MAX_SPAWNHOLES 5

// balloon structure
typedef struct
{
    Vector2 position;
    float speed;
    float radius;
    bool active;
    bool isgold;
} Balloon;
// projectile structure
typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool active;
} Projectile;

// balloon spawning function
void SpawnBalloon(Balloon *b, float balloonradius, Vector2 spawnholes[])
{
    int holeindex = GetRandomValue(0, MAX_SPAWNHOLES - 1);
    b->position = spawnholes[holeindex];
    b->speed = (float)160;
    b->radius = balloonradius;
    b->active = true;
    b->isgold = (GetRandomValue(1, 10) <= 2);
}

// main func
int main(void)
{
    // window and audio init
    InitWindow(WIDTH, HEIGHT, "Hit 'Em All");
    InitAudioDevice();
    SetTargetFPS(60);

    // all music loading and playing and setting volume
    Music gamebgm = LoadMusicStream("assets/audio/Carnival Music (Game Window).mp3");
    PlayMusicStream(gamebgm);
    SetMusicVolume(gamebgm, 0.5f);

    Sound shootsound = LoadSound("assets/audio/Gun shooting.ogg");
    Sound popsound = LoadSound("assets/audio/Balloon Pop.mp3");
    Sound gameoversound = LoadSound("assets/audio/game over.mp3");

    // bg loading
    Texture2D background = LoadTexture("assets/sprites/Gamescreen.png");
    Texture2D gameovertexture = LoadTexture("assets/sprites/gameover.png");

    // loading old man and init for oldman
    char filename[128];
    int oldmanframe = 0;
    float oldmantimer = 0.0f;
    const float oldmanframetime = 0.1f;
    Texture2D oldmanframes[OLDMAN_FRAMES];
    for (int i = 0; i < OLDMAN_FRAMES; i++)
    {
        snprintf(filename, sizeof(filename), "assets/sprites/oldman%03d.png", i);
        oldmanframes[i] = LoadTexture(filename);
    }
    const float spritescale = 2.0f;

    // loading young boy and init
    int boyframe = 0;
    float boytimer = 0.0f;
    float boyframetime = 0.1f;
    Texture2D boyframes[BOY_FRAMES];
    for (int i = 0; i < BOY_FRAMES; i++)
    {
        snprintf(filename, sizeof(filename), "assets/sprites/boy%03d.png", i);
        boyframes[i] = LoadTexture(filename);
    }

    // setting old man position
    Texture2D oldmantexture = oldmanframes[oldmanframe];
    float oldmanwidth = oldmantexture.width * spritescale;
    float oldmanheight = oldmantexture.height * spritescale;
    float oldmanposY = HEIGHT - oldmanheight + 125.0f;
    float oldmanposX = WIDTH - oldmanwidth + 150.0f;

    // setting boy position
    Texture2D boytexture = boyframes[boyframe];
    float boywidth = boytexture.width * spritescale;
    float boyheight = boytexture.height * spritescale;
    float boyposX = -50.0f;
    float boyposY = HEIGHT - boyheight + 125.0f;

    // spawn hole setup for balloons
    Vector2 spawnholes[MAX_SPAWNHOLES];
    for (int i = 0; i < MAX_SPAWNHOLES; i++)
    {
        spawnholes[i] = (Vector2){850.0f + i * 130.0f, HEIGHT + 50.0f};
    }
    float spawntimer = 0.0f;
    const float spawninterval = 1.8f;

    // loading balloons and initializing
    Texture2D normalballoon = LoadTexture("assets/sprites/normalballoon.png");
    Texture2D specialballoon = LoadTexture("assets/sprites/specialballoon.png");
    float balloonradius = (float)normalballoon.width * 0.4f;
    Balloon balloons[MAX_BALLOONS] = {0};
    for (int i = 0; i < MAX_BALLOONS; i++)
    {
        balloons[i].active = false;
    }
    // slingshot setting
    Projectile projectiles[MAX_ARROWS] = {0};
    int ammo = 10;
    bool isaiming = false;
    Vector2 anchorpos = {boyposX + 350.0f, boyposY + 205.0f};
    Vector2 dragpos = anchorpos;
    const float maxpulldistance = 150.0f, launchmult = 7.5f, gravity = 700.0f;

    // game variables setting
    int score = 0;
    bool gameover = 0;

    // each game frame
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // streaming audio
        UpdateMusicStream(gamebgm);

        // timing for animation of old man and boy
        oldmantimer += dt;
        if (oldmantimer >= oldmanframetime)
        {
            oldmantimer = 0.0f;
            oldmanframe = (oldmanframe + 1) % OLDMAN_FRAMES;
        }

        boytimer += dt;
        if (boytimer >= boyframetime)
        {
            boytimer = 0.0f;
            boyframe = (boyframe + 1) % BOY_FRAMES;
        }
        // balloon spawning

        spawntimer += dt;
        if (spawntimer >= spawninterval)
        {
            spawntimer = 0.0f;
            for (int i = 0; i < MAX_BALLOONS; i++)
            {
                if (balloons[i].active == 0)
                {
                    SpawnBalloon(&balloons[i], balloonradius, spawnholes);
                    break;
                }
            }
        }

        // floating of balloon vertically upwards & collision of rock with balloon to pop it (with sound)
        for (int i = 0; i < MAX_BALLOONS; i++)
        {
            if (balloons[i].active == 0)
                continue;

            balloons[i].position.y -= balloons[i].speed * dt;
            if (balloons[i].position.y < -100.0f)
            {
                balloons[i].active = false;
            }
            for (int j = 0; j < MAX_ARROWS; j++)
            {
                if (projectiles[j].active && CheckCollisionCircles(projectiles[j].position, projectiles[j].radius, balloons[i].position, balloons[i].radius))
                {
                    projectiles[j].active = false;
                    if (balloons[i].isgold == 1)
                    {
                        ammo += 2;
                        score += 10;
                    }
                    else
                    {
                        score += 10;
                    }
                    PlaySound(popsound);
                    balloons[i].active = false;
                }
            }
        }
        // slingshot aiming and firing
        if (ammo > 0 && gameover == 0)
        {
            Vector2 mousepos = GetMousePosition();
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointCircle(mousepos, anchorpos, 100.0f))
            {
                isaiming = true;
            }
            if (isaiming)
            {
                Vector2 pullvector = Vector2Subtract(mousepos, anchorpos);
                if (Vector2Length(pullvector) > maxpulldistance)
                {
                    pullvector = Vector2Scale(Vector2Normalize(pullvector), maxpulldistance);
                }
                dragpos = Vector2Add(anchorpos, pullvector);
            }

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                Vector2 launchvector = Vector2Subtract(anchorpos, dragpos);
                if (Vector2Length(launchvector) > 15.0f)
                {
                    for (int i = 0; i < MAX_ARROWS; i++)
                    {
                        if (!projectiles[i].active)
                        {
                            projectiles[i] = (Projectile){
                                anchorpos, Vector2Scale(launchvector, launchmult), 12.0f, true};
                            ammo--;
                            PlaySound(shootsound);
                            break;
                        }
                    }
                }
                isaiming = false;
            }
        }

        // actual projectile mechanics
        bool hasactiveprojectiles = false;
        for (int i = 0; i < MAX_ARROWS; i++)
        {
            if (projectiles[i].active)
            {
                hasactiveprojectiles = true;
                projectiles[i].velocity.y += gravity * dt;
                projectiles[i].position = Vector2Add(projectiles[i].position, Vector2Scale(projectiles[i].velocity, dt));

                if (projectiles[i].position.x > WIDTH + 50 || projectiles[i].position.y > HEIGHT + 50 || projectiles[i].position.x < -50)
                {
                    projectiles[i].active = false;
                }
            }
        }
        // checking game over state
        if (ammo <= 0 && !hasactiveprojectiles && !gameover)
        {
            gameover = true;
            isaiming = 0;
            StopMusicStream(gamebgm);
            PlaySound(gameoversound);
        }
        // drawing portion
        BeginDrawing();
        ClearBackground(RAYWHITE);
        Vector2 origin = {0.0f, 0.0f};
        // drawing bg
        Rectangle bgsource = {0.0f, 0.0f, (float)background.width, (float)background.height};
        Rectangle bgdest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
        DrawTexturePro(background, bgsource, bgdest, origin, 0.0f, WHITE);

        // drawing old man
        oldmantexture = oldmanframes[oldmanframe];
        Rectangle oldmansource = {0.0f, 0.0f, -(float)oldmantexture.width, (float)oldmantexture.height};
        Rectangle oldmandest = {oldmanposX, oldmanposY, oldmanwidth, oldmanheight};
        DrawTexturePro(oldmantexture, oldmansource, oldmandest, origin, 0.0f, WHITE);
        // drawing boy
        boytexture = boyframes[boyframe];
        Rectangle boysource = {0.0f, 0.0f, (float)boytexture.width, (float)boytexture.height};
        Rectangle boydest = {boyposX, boyposY, boywidth, boyheight};
        DrawTexturePro(boytexture, boysource, boydest, origin, 0.0f, WHITE);

        // drawing the balloons
        for (int i = 0; i < MAX_BALLOONS; i++)
        {
            if (balloons[i].active == 1)
            {
                Texture2D balloontexture = balloons[i].isgold ? specialballoon : normalballoon;
                DrawTextureV(balloontexture, (Vector2){balloons[i].position.x - (balloontexture.width / 2.0f), balloons[i].position.y - (balloontexture.height / 2.0f)}, WHITE);
            }
        }

        // aiming of gun
        if (isaiming)
        {
            DrawLineEx(anchorpos, dragpos, 5.0f, RED);
            DrawCircleV(dragpos, 12.0f, DARKGRAY);
        }
        // Active Projectiles
        for (int i = 0; i < MAX_ARROWS; i++)
        {
            if (projectiles[i].active)
            {
                DrawCircleV(projectiles[i].position, projectiles[i].radius, DARKGRAY);
                DrawCircleLinesV(projectiles[i].position, projectiles[i].radius, BLACK);
            }
        }

        // score and ammo display
        DrawText(TextFormat("SCORE: %d", score), 30, 25, 32, WHITE);
        DrawText(TextFormat("AMMO: %d", ammo), 30, 65, 32, GOLD);

        // Game Over img drawing
        if (gameover)
        {
            DrawTexture(gameovertexture, WIDTH / 2 - gameovertexture.width / 2, HEIGHT / 2 - gameovertexture.height / 2, WHITE);
        }
        EndDrawing();
    }

    // unloading all textures/drawings
    UnloadTexture(background);
    for (int i = 0; i < OLDMAN_FRAMES; i++)
        UnloadTexture(oldmanframes[i]);
    for (int i = 0; i < BOY_FRAMES; i++)
        UnloadTexture(boyframes[i]);
    UnloadTexture(normalballoon);
    UnloadTexture(specialballoon);
    UnloadTexture(gameovertexture);

    // unloading all audio
    UnloadSound(shootsound);
    UnloadSound(popsound);
    UnloadMusicStream(gamebgm);
    UnloadSound(gameoversound);
    CloseAudioDevice();

    CloseWindow();
    return 0;
}