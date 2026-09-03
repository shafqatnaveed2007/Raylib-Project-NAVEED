#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==========================================
// CONSTANTS & ENUMS
// ==========================================
#define WIDTH 1600
#define HEIGHT 800
#define OLDMAN_FRAMES 25
#define BOY_FRAMES 20
#define MAX_BALLOONS 6
#define MAX_ARROWS 10
#define MAX_LEADERBOARD_ENTRIES 10

typedef enum
{
    STATE_MENU,
    STATE_PLAYING,
    STATE_HOW_TO_PLAY,
    STATE_LEADERBOARD,
    STATE_GAME_OVER
} GameState;

// ==========================================
// DATA STRUCTURES
// ==========================================
typedef struct
{
    char name[4];
    int score;
} ScoreEntry;

typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool active;
} Projectile;

typedef struct
{
    Vector2 position;
    float speed;
    float radius;
    bool active;
    bool isGold;
} Balloon;

typedef struct
{
    bool active;
    float timer;
    Texture2D texture;
} RewardNotification;

// Ensures each reward milestone only triggers once per game
typedef struct
{
    bool unlocked50;
    bool unlocked100;
    bool unlocked150;
} RewardFlags;

// ==========================================
// HELPER FUNCTIONS
// ==========================================

// Respawns or initializes a balloon with random velocity & properties
void SpawnBalloon(Balloon *b, float baseRadius)
{
    b->position.x = (float)GetRandomValue(850, WIDTH - 120);
    b->position.y = (float)GetRandomValue(HEIGHT + 80, HEIGHT + 300);
    b->speed = (float)GetRandomValue(120, 200);
    b->radius = baseRadius;
    b->active = true;
    b->isGold = (GetRandomValue(1, 10) <= 2); // 20% chance for golden balloon
}

// Resets dynamic state variables for a new round
void ResetGameState(int *score, int *ammo, bool *isAiming, int *letterCount,
                    char *playerName, Projectile projectiles[],
                    Balloon balloons[], float balloonRadius, RewardFlags *flags, RewardNotification *rewardNotif)
{
    *score = 0;
    *ammo = 5;
    *isAiming = false;
    *letterCount = 0;
    playerName[0] = '\0';

    flags->unlocked50 = false;
    flags->unlocked100 = false;
    flags->unlocked150 = false;
    rewardNotif->active = false;
    rewardNotif->timer = 0.0f;

    for (int i = 0; i < MAX_ARROWS; i++)
    {
        projectiles[i].active = false;
    }

    for (int i = 0; i < MAX_BALLOONS; i++)
    {
        SpawnBalloon(&balloons[i], balloonRadius);
        balloons[i].position.y = (float)GetRandomValue(150, HEIGHT - 150);
    }
}

// Renders an interactive UI button with hover effects and click handling
bool DrawButton(Rectangle rect, const char *text, Sound clickSound)
{
    Vector2 mousePos = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mousePos, rect);

    DrawRectangleRec(rect, hovered ? DARKGRAY : RAYWHITE);
    DrawRectangleLinesEx(rect, 3, BLACK);

    int fontSize = 28;
    int textWidth = MeasureText(text, fontSize);
    DrawText(text, rect.x + (rect.width - textWidth) / 2.0f,
             rect.y + (rect.height - fontSize) / 2.0f, fontSize, hovered ? YELLOW : BLACK);

    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        PlaySound(clickSound);
        return true;
    }
    return false;
}

// Loads score entries from disk file storage
int LoadLeaderboard(ScoreEntry entries[])
{
    FILE *file = fopen("leaderboard.txt", "r");
    if (!file)
        return 0;

    int count = 0;
    while (count < MAX_LEADERBOARD_ENTRIES && fscanf(file, "%3s %d", entries[count].name, &entries[count].score) == 2)
    {
        count++;
    }
    fclose(file);
    return count;
}

// Writes top leaderboard entries to disk storage
void SaveLeaderboard(ScoreEntry entries[], int count)
{
    FILE *file = fopen("leaderboard.txt", "w");
    if (!file)
        return;

    for (int i = 0; i < count; i++)
    {
        fprintf(file, "%s %d\n", entries[i].name, entries[i].score);
    }
    fclose(file);
}

// Adds score entry and sorts the leaderboard in descending order
void AddScoreToLeaderboard(const char *name, int score)
{
    ScoreEntry entries[MAX_LEADERBOARD_ENTRIES + 1];
    int count = LoadLeaderboard(entries);

    strncpy(entries[count].name, name, 3);
    entries[count].name[3] = '\0';
    entries[count].score = score;
    count++;

    // Bubble sort entries descending by score
    for (int i = 0; i < count - 1; i++)
    {
        for (int j = i + 1; j < count; j++)
        {
            if (entries[j].score > entries[i].score)
            {
                ScoreEntry temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }

    if (count > MAX_LEADERBOARD_ENTRIES)
        count = MAX_LEADERBOARD_ENTRIES;
    SaveLeaderboard(entries, count);
}

// ==========================================
// MAIN ENTRY POINT
// ==========================================
int main(void)
{
    InitWindow(WIDTH, HEIGHT, "Hit 'Em All");
    InitAudioDevice();
    SetTargetFPS(60);

    // Audio Assets Loading & Initialization (Relative Paths)
    Music gameBgm = LoadMusicStream("assets/audio/Carnival Music (Game Window).mp3");
    Music menuBgm = LoadMusicStream("assets/audio/Music (Menu Screen).mp3");
    PlayMusicStream(menuBgm);
    SetMusicVolume(menuBgm, 0.4f);
    SetMusicVolume(gameBgm, 0.4f);

    Sound shootSound = LoadSound("assets/audio/Gun shooting.ogg");
    Sound popSound = LoadSound("assets/audio/Balloon Pop.mp3");
    Sound clickSound = LoadSound("assets/audio/Button Clicking.wav");
    Sound rewardSound = LoadSound("assets/audio/Reward winning.mp3");

    // Texture Assets Loading (Relative Paths)
    Texture2D background = LoadTexture("assets/sprites/Gamescreen.png");
    Texture2D menuBg = LoadTexture("assets/sprites/hit-em-all-cover.png");
    Texture2D normalTex = LoadTexture("assets/sprites/normalballoon.png");
    Texture2D specialTex = LoadTexture("assets/sprites/specialballoon.png");

    Texture2D reward50Tex = LoadTexture("assets/sprites/teddy bear.png");
    Texture2D reward100Tex = LoadTexture("assets/sprites/car.png");
    Texture2D reward150Tex = LoadTexture("assets/sprites/watch.png");

    // Load Character Animation Frames (Relative Paths)
    char fileName[256];
    Texture2D oldManFrames[OLDMAN_FRAMES];
    for (int i = 0; i < OLDMAN_FRAMES; i++)
    {
        snprintf(fileName, sizeof(fileName), "assets/sprites/oldman%03d.png", i);
        oldManFrames[i] = LoadTexture(fileName);
    }

    Texture2D boyFrames[BOY_FRAMES];
    for (int i = 0; i < BOY_FRAMES; i++)
    {
        snprintf(fileName, sizeof(fileName), "assets/sprites/boy%03d.png", i);
        boyFrames[i] = LoadTexture(fileName);
    }

    // Animation & Position Properties
    int oldManFrame = 0, boyFrame = 0;
    float oldManTimer = 0.0f, boyTimer = 0.0f;
    const float oldManFrameTime = 0.1f, boyFrameTime = 0.08f;
    const float spriteScale = 2.0f;

    float oldManBaseHeight = oldManFrames[0].height * spriteScale;
    float boyBaseHeight = boyFrames[0].height * spriteScale;
    float oldManPosY = HEIGHT - oldManBaseHeight + 125.0f;
    float boyPosY = HEIGHT - boyBaseHeight + 125.0f;
    float boyPosX = -50.0f;

    // Slingshot Constants & Vectors
    Vector2 anchorPos = {boyPosX + 350.0f, boyPosY + 205.0f};
    Vector2 dragPos = anchorPos;
    const float maxPullDistance = 150.0f, launchMult = 7.5f, gravity = 700.0f;
    float balloonRadius = (float)normalTex.width * 0.4f;

    // Game Dynamic Variables initialization
    GameState currentState = STATE_MENU;
    int score = 0, ammo = 5, letterCount = 0;
    bool isAiming = false;
    char playerName[4] = "\0";

    RewardFlags rewardFlags = {false, false, false};
    RewardNotification currentReward = {false, 0.0f, {0}};

    Projectile projectiles[MAX_ARROWS] = {0};
    Balloon balloons[MAX_BALLOONS];

    ResetGameState(&score, &ammo, &isAiming, &letterCount, playerName, projectiles, balloons, balloonRadius, &rewardFlags, &currentReward);

    // ------------------------------------------
    // MAIN GAME LOOP
    // ------------------------------------------
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        Vector2 mousePos = GetMousePosition();

        // Stream Active Audio
        UpdateMusicStream((currentState == STATE_PLAYING) ? gameBgm : menuBgm);

        // --------------------------------------
        // UPDATE PHASE
        // --------------------------------------
        switch (currentState)
        {
        case STATE_PLAYING:
        {
            // Character Animations
            oldManTimer += dt;
            if (oldManTimer >= oldManFrameTime)
            {
                oldManTimer = 0.0f;
                oldManFrame = (oldManFrame + 1) % OLDMAN_FRAMES;
            }
            boyTimer += dt;
            if (boyTimer >= boyFrameTime)
            {
                boyTimer = 0.0f;
                boyFrame = (boyFrame + 1) % BOY_FRAMES;
            }

            // Reward Popup Display Duration Timer
            if (currentReward.active)
            {
                currentReward.timer -= dt;
                if (currentReward.timer <= 0.0f)
                {
                    currentReward.active = false;
                }
            }

            // Slingshot Input & Firing Controls
            if (ammo > 0)
            {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointCircle(mousePos, anchorPos, 100.0f))
                {
                    isAiming = true;
                }

                if (isAiming)
                {
                    Vector2 pullVector = Vector2Subtract(mousePos, anchorPos);
                    if (Vector2Length(pullVector) > maxPullDistance)
                    {
                        pullVector = Vector2Scale(Vector2Normalize(pullVector), maxPullDistance);
                    }
                    dragPos = Vector2Add(anchorPos, pullVector);

                    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                    {
                        Vector2 launchVector = Vector2Subtract(anchorPos, dragPos);
                        if (Vector2Length(launchVector) > 15.0f)
                        {
                            for (int i = 0; i < MAX_ARROWS; i++)
                            {
                                if (!projectiles[i].active)
                                {
                                    projectiles[i] = (Projectile){anchorPos, Vector2Scale(launchVector, launchMult), 12.0f, true};
                                    ammo--;
                                    PlaySound(shootSound);
                                    break;
                                }
                            }
                        }
                        isAiming = false;
                    }
                }
            }

            // Projectile Movement Physics & Screen Bounds Check
            for (int i = 0; i < MAX_ARROWS; i++)
            {
                if (projectiles[i].active)
                {
                    projectiles[i].velocity.y += gravity * dt;
                    projectiles[i].position = Vector2Add(projectiles[i].position, Vector2Scale(projectiles[i].velocity, dt));

                    if (projectiles[i].position.x > WIDTH + 50 || projectiles[i].position.y > HEIGHT + 50 || projectiles[i].position.x < -50)
                    {
                        projectiles[i].active = false;
                    }
                }
            }

            // Balloon Logic & Collision Resolution
            for (int i = 0; i < MAX_BALLOONS; i++)
            {
                if (!balloons[i].active)
                    continue;

                balloons[i].position.y -= balloons[i].speed * dt;
                if (balloons[i].position.y < -100.0f)
                {
                    SpawnBalloon(&balloons[i], balloonRadius);
                }

                for (int p = 0; p < MAX_ARROWS; p++)
                {
                    if (projectiles[p].active && CheckCollisionCircles(projectiles[p].position, projectiles[p].radius, balloons[i].position, balloons[i].radius))
                    {
                        projectiles[p].active = false;

                        if (balloons[i].isGold)
                        {
                            ammo += 2;
                        }
                        else
                        {
                            score += 10;

                            // Score Milestone Checks
                            if (score >= 150 && !rewardFlags.unlocked150)
                            {
                                rewardFlags.unlocked150 = true;
                                currentReward.active = true;
                                currentReward.timer = 3.0f;
                                currentReward.texture = reward150Tex;
                                PlaySound(rewardSound);
                            }
                            else if (score >= 100 && !rewardFlags.unlocked100)
                            {
                                rewardFlags.unlocked100 = true;
                                currentReward.active = true;
                                currentReward.timer = 3.0f;
                                currentReward.texture = reward100Tex;
                                PlaySound(rewardSound);
                            }
                            else if (score >= 50 && !rewardFlags.unlocked50)
                            {
                                rewardFlags.unlocked50 = true;
                                currentReward.active = true;
                                currentReward.timer = 3.0f;
                                currentReward.texture = reward50Tex;
                                PlaySound(rewardSound);
                            }
                        }

                        PlaySound(popSound);
                        SpawnBalloon(&balloons[i], balloonRadius);
                    }
                }
            }

            // Game Over Trigger Check
            if (ammo == 0 && !isAiming)
            {
                bool projectilesInFlight = false;
                for (int i = 0; i < MAX_ARROWS; i++)
                {
                    if (projectiles[i].active)
                    {
                        projectilesInFlight = true;
                        break;
                    }
                }
                if (!projectilesInFlight)
                {
                    currentState = STATE_GAME_OVER;
                }
            }
            break;
        }

        case STATE_GAME_OVER:
        {
            // Name Input Processing (3-letter initials)
            int key = GetCharPressed();
            while (key > 0)
            {
                if ((key >= 32) && (key <= 125) && (letterCount < 3))
                {
                    playerName[letterCount] = (char)key;
                    playerName[letterCount + 1] = '\0';
                    letterCount++;
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE))
            {
                letterCount = (letterCount > 0) ? letterCount - 1 : 0;
                playerName[letterCount] = '\0';
            }
            break;
        }
        default:
            break;
        }

        // --------------------------------------
        // RENDER / DRAWING PHASE
        // --------------------------------------
        BeginDrawing();
        ClearBackground(RAYWHITE);
        Vector2 origin = {0.0f, 0.0f};

        switch (currentState)
        {
        case STATE_MENU:
        {
            DrawTexturePro(menuBg, (Rectangle){0, 0, (float)menuBg.width, (float)menuBg.height},
                           (Rectangle){0, 0, (float)WIDTH, (float)HEIGHT}, origin, 0.0f, WHITE);

            float leftX = WIDTH / 2.0f - 290.0f, rightX = WIDTH / 2.0f + 30.0f;
            float row1Y = 560.0f, row2Y = 640.0f, w = 260.0f, h = 60.0f;

            if (DrawButton((Rectangle){leftX, row1Y, w, h}, "START", clickSound))
            {
                ResetGameState(&score, &ammo, &isAiming, &letterCount, playerName, projectiles, balloons, balloonRadius, &rewardFlags, &currentReward);
                StopMusicStream(menuBgm);
                PlayMusicStream(gameBgm);
                currentState = STATE_PLAYING;
            }
            if (DrawButton((Rectangle){leftX, row2Y, w, h}, "HOW TO PLAY", clickSound))
                currentState = STATE_HOW_TO_PLAY;
            if (DrawButton((Rectangle){rightX, row1Y, w, h}, "LEADERBOARD", clickSound))
                currentState = STATE_LEADERBOARD;
            if (DrawButton((Rectangle){rightX, row2Y, w, h}, "EXIT", clickSound))
            {
                CloseWindow();
                return 0;
            }
            break;
        }

        case STATE_HOW_TO_PLAY:
        {
            DrawRectangle(0, 0, WIDTH, HEIGHT, ColorAlpha(BLACK, 0.85f));
            DrawText("HOW TO PLAY", WIDTH / 2 - MeasureText("HOW TO PLAY", 50) / 2, 80, 50, GOLD);

            const char *instructions[] = {
                "1. Click and drag back on the slingshot near the boy to set power and angle.",
                "2. Release the mouse button to launch projectiles at floating balloons.",
                "3. Popping NORMAL balloons rewards 10 points.",
                "4. Popping SPECIAL GOLD balloons grants +2 extra AMMO.",
                "5. Projectiles arc downward due to gravity—time and calculate your trajectory!",
                "6. The game ends when you completely run out of ammo."};

            for (int i = 0; i < 6; i++)
            {
                DrawText(instructions[i], 150, 180 + (i * 50), 24, (i == 3) ? YELLOW : (i == 5) ? RED
                                                                                                : WHITE);
            }

            if (DrawButton((Rectangle){WIDTH / 2.0f - 100, 600, 200, 50}, "BACK", clickSound))
            {
                currentState = STATE_MENU;
            }
            break;
        }

        case STATE_LEADERBOARD:
        {
            DrawRectangle(0, 0, WIDTH, HEIGHT, ColorAlpha(BLACK, 0.85f));
            DrawText("LEADERBOARD", WIDTH / 2 - MeasureText("LEADERBOARD", 50) / 2, 80, 50, GOLD);

            ScoreEntry entries[MAX_LEADERBOARD_ENTRIES];
            int count = LoadLeaderboard(entries);

            for (int i = 0; i < count; i++)
            {
                DrawText(TextFormat("%d.  %s  -  %d", i + 1, entries[i].name, entries[i].score),
                         WIDTH / 2 - 100, 180 + (i * 40), 30, WHITE);
            }
            if (count == 0)
            {
                DrawText("NO HIGH SCORES YET", WIDTH / 2 - MeasureText("NO HIGH SCORES YET", 30) / 2, 300, 30, LIGHTGRAY);
            }

            if (DrawButton((Rectangle){WIDTH / 2.0f - 100, 650, 200, 50}, "BACK", clickSound))
            {
                currentState = STATE_MENU;
            }
            break;
        }

        case STATE_PLAYING:
        {
            // Background
            DrawTexturePro(background, (Rectangle){0, 0, (float)background.width, (float)background.height},
                           (Rectangle){0, 0, (float)WIDTH, (float)HEIGHT}, origin, 0.0f, WHITE);

            // Animated Character Rendering
            Texture2D omTex = oldManFrames[oldManFrame];
            float omWidth = omTex.width * spriteScale;
            DrawTexturePro(omTex, (Rectangle){0, 0, -(float)omTex.width, (float)omTex.height},
                           (Rectangle){WIDTH - omWidth + 150.0f, oldManPosY, omWidth, oldManBaseHeight}, origin, 0.0f, WHITE);

            Texture2D bTex = boyFrames[boyFrame];
            float bWidth = bTex.width * spriteScale;
            DrawTexturePro(bTex, (Rectangle){0, 0, (float)bTex.width, (float)bTex.height},
                           (Rectangle){boyPosX, boyPosY, bWidth, boyBaseHeight}, origin, 0.0f, WHITE);

            // Active Balloons
            for (int i = 0; i < MAX_BALLOONS; i++)
            {
                if (balloons[i].active)
                {
                    Texture2D tex = balloons[i].isGold ? specialTex : normalTex;
                    DrawTextureV(tex, (Vector2){balloons[i].position.x - (tex.width / 2.0f), balloons[i].position.y - (tex.height / 2.0f)}, WHITE);
                }
            }

            // Trajectory Prediction Curve
            if (isAiming)
            {
                DrawLineEx(anchorPos, dragPos, 5.0f, RED);
                DrawCircleV(dragPos, 12.0f, DARKGRAY);

                Vector2 launchVector = Vector2Subtract(anchorPos, dragPos);
                Vector2 simVel = Vector2Scale(launchVector, launchMult);
                Vector2 simPos = anchorPos;

                for (int t = 0; t < 25; t++)
                {
                    simVel.y += gravity * 0.035f;
                    Vector2 nextPos = Vector2Add(simPos, Vector2Scale(simVel, 0.035f));
                    DrawCircleV(nextPos, 3.0f, ColorAlpha(RED, 0.8f - (t * 0.03f)));
                    simPos = nextPos;
                }
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

            // Heads-Up Display (HUD)
            DrawText(TextFormat("SCORE: %d", score), 30, 25, 32, WHITE);
            DrawText(TextFormat("AMMO: %d", ammo), 30, 65, 32, (ammo > 0) ? GOLD : RED);

            // Reward Notification Popup Overlay
            if (currentReward.active && currentReward.texture.id > 0)
            {
                float scale = 1.0f;
                float drawW = currentReward.texture.width * scale;
                float drawH = currentReward.texture.height * scale;

                Vector2 pos = {
                    (WIDTH / 2.0f) - (drawW / 2.0f),
                    (HEIGHT / 2.0f) - (drawH / 2.0f) - 50.0f};

                DrawRectangle((int)pos.x - 20, (int)pos.y - 20, (int)drawW + 40, (int)drawH + 80, ColorAlpha(BLACK, 0.7f));
                DrawRectangleLines((int)pos.x - 20, (int)pos.y - 20, (int)drawW + 40, (int)drawH + 80, GOLD);

                DrawTextureEx(currentReward.texture, pos, 0.0f, scale, WHITE);
                DrawText("REWARD UNLOCKED!", WIDTH / 2 - MeasureText("REWARD UNLOCKED!", 24) / 2, (int)(pos.y + drawH + 10), 24, GOLD);
            }
            break;
        }

        case STATE_GAME_OVER:
        {
            DrawRectangle(0, 0, WIDTH, HEIGHT, ColorAlpha(BLACK, 0.75f));
            DrawText("GAME OVER!", WIDTH / 2 - MeasureText("GAME OVER!", 50) / 2, 200, 50, RED);
            DrawText(TextFormat("FINAL SCORE: %d", score), WIDTH / 2 - MeasureText(TextFormat("FINAL SCORE: %d", score), 30) / 2, 270, 30, WHITE);

            DrawText("ENTER YOUR INITIALS (3 CHARS):", WIDTH / 2 - 200, 360, 22, GOLD);
            DrawRectangleLines(WIDTH / 2 - 100, 400, 200, 50, WHITE);
            DrawText(playerName, WIDTH / 2 - MeasureText(playerName, 35) / 2, 410, 35, GREEN);

            if (DrawButton((Rectangle){WIDTH / 2.0f - 125, 500, 250, 50}, "SUBMIT SCORE", clickSound))
            {
                if (strlen(playerName) == 0)
                    strncpy(playerName, "AAA", 3);
                AddScoreToLeaderboard(playerName, score);
                StopMusicStream(gameBgm);
                PlayMusicStream(menuBgm);
                currentState = STATE_LEADERBOARD;
            }
            break;
        }
        }

        EndDrawing();
    }

    // ==========================================
    // RESOURCE UNLOADING & SHUTDOWN
    // ==========================================
    UnloadSound(shootSound);
    UnloadSound(popSound);
    UnloadSound(clickSound);
    UnloadSound(rewardSound);
    UnloadMusicStream(gameBgm);
    UnloadMusicStream(menuBgm);
    CloseAudioDevice();

    UnloadTexture(normalTex);
    UnloadTexture(specialTex);
    UnloadTexture(background);
    UnloadTexture(menuBg);
    UnloadTexture(reward50Tex);
    UnloadTexture(reward100Tex);
    UnloadTexture(reward150Tex);

    for (int i = 0; i < OLDMAN_FRAMES; i++)
        UnloadTexture(oldManFrames[i]);
    for (int i = 0; i < BOY_FRAMES; i++)
        UnloadTexture(boyFrames[i]);

    CloseWindow();
    return 0;
}