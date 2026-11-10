#include "raylib.h"
#include "raymath.h"
#include <float.h>
#include <math.h>
#include <stdbool.h>

#define TARGET_COUNT 5
#define PLAYER_HEIGHT 2.0f
#define RESET_TIME 2.0

typedef struct {
    Vector3 position;
    bool hit;
    double lastHit;
} Dummy;

int main(void)
{
    const int screenWidth = 1100;
    const int screenHeight = 700;

    InitWindow(screenWidth, screenHeight, "Raylib - 3D Target Practice");

    SetTargetFPS(60);
    DisableCursor();

    // First-person player camera
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, PLAYER_HEIGHT * 0.8f, 12.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 70.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Camera rotation angles (in degrees)
    float cameraYaw = -90.0f;   // Facing towards negative Z initially
    float cameraPitch = 0.0f;

    // Five rectangular targets
    Dummy dummies[TARGET_COUNT] = {
        { {-8, 1,  0}, false, 0 },
        { {-4, 1, -5}, false, 0 },
        { { 0, 1, -8}, false, 0 },
        { { 4, 1, -5}, false, 0 },
        { { 8, 1,  0}, false, 0 }
    };

    Vector3 dummySize = { 1.0f, 2.0f, 0.5f };

    while (!WindowShouldClose())
    {
        double currentTime = GetTime();
        float deltaTime = GetFrameTime();

        // 1. Smooth Mouse Look
        Vector2 mouseDelta = GetMouseDelta();
        float mouseSensitivity = 0.15f;

        cameraYaw += mouseDelta.x * mouseSensitivity;
        cameraPitch -= mouseDelta.y * mouseSensitivity;

        // Clamp pitch so the player can't flip upside down
        if (cameraPitch > 89.0f) cameraPitch = 89.0f;
        if (cameraPitch < -89.0f) cameraPitch = -89.0f;

        // Convert angles to direction vectors
        float yawRad = cameraYaw * DEG2RAD;
        float pitchRad = cameraPitch * DEG2RAD;

        Vector3 forward = {
            cosf(pitchRad) * cosf(yawRad),
            sinf(pitchRad),
            cosf(pitchRad) * sinf(yawRad)
        };
        forward = Vector3Normalize(forward);

        Vector3 worldUp = { 0.0f, 1.0f, 0.0f };
        Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, worldUp));

        // 2. Frame-rate independent WASD Movement
        float moveSpeed = 6.0f * deltaTime; // Units per second

        if (IsKeyDown(KEY_W)) camera.position = Vector3Add(camera.position, Vector3Scale(forward, moveSpeed));
        if (IsKeyDown(KEY_S)) camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, moveSpeed));
        if (IsKeyDown(KEY_D)) camera.position = Vector3Add(camera.position, Vector3Scale(right, moveSpeed));
        if (IsKeyDown(KEY_A)) camera.position = Vector3Subtract(camera.position, Vector3Scale(right, moveSpeed));

        // Lock player height
        camera.position.y = PLAYER_HEIGHT * 0.8f;

        // Update camera look target
        camera.target = Vector3Add(camera.position, forward);

        // 3. Smooth Camera Zoom (Hold Right Mouse Button to Zoom / ADS)
        float targetFovy = IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ? 30.0f : 70.0f;
        camera.fovy += (targetFovy - camera.fovy) * 12.0f * deltaTime;

        // Construct a ray from the crosshair center
        Vector2 center = { screenWidth / 2.0f, screenHeight / 2.0f };
        Ray ray = GetMouseRay(center, camera);

        // Click to tag the nearest target
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            int selected = -1;
            float nearestDistance = FLT_MAX;

            for (int i = 0; i < TARGET_COUNT; i++)
            {
                Vector3 half = { dummySize.x / 2.0f, dummySize.y / 2.0f, dummySize.z / 2.0f };

                BoundingBox box = {
                    {
                        dummies[i].position.x - half.x,
                        dummies[i].position.y - half.y,
                        dummies[i].position.z - half.z
                    },
                    {
                        dummies[i].position.x + half.x,
                        dummies[i].position.y + half.y,
                        dummies[i].position.z + half.z
                    }
                };

                RayCollision collision = GetRayCollisionBox(ray, box);

                if (collision.hit && collision.distance < nearestDistance)
                {
                    nearestDistance = collision.distance;
                    selected = i;
                }
            }

            if (selected != -1)
            {
                dummies[selected].hit = true;
                dummies[selected].lastHit = currentTime;
            }
        }

        // Reset red targets after five seconds
        for (int i = 0; i < TARGET_COUNT; i++)
        {
            if (dummies[i].hit && currentTime - dummies[i].lastHit >= RESET_TIME)
            {
                dummies[i].hit = false;
            }
        }

        BeginDrawing();
        ClearBackground((Color){ 18, 22, 30, 255 });

        BeginMode3D(camera);

            // Ground grid and plane
            DrawGrid(50, 1.0f);
            DrawPlane((Vector3){ 0, -0.02f, 0 }, (Vector2){ 50, 50 }, (Color){ 35, 40, 48, 255 });

            // Draw targets
            for (int i = 0; i < TARGET_COUNT; i++)
            {
                Color targetColor = dummies[i].hit ? RED : GREEN;

                DrawCubeV(dummies[i].position, dummySize, targetColor);
                DrawCubeWiresV(dummies[i].position, dummySize, BLACK);
            }

        EndMode3D();

        // Crosshair
        int cx = screenWidth / 2;
        int cy = screenHeight / 2;

        DrawLine(cx - 12, cy, cx + 12, cy, WHITE);
        DrawLine(cx, cy - 12, cx, cy + 12, WHITE);
        DrawCircleLines(cx, cy, 4, WHITE);

        // HUD Overlay
        DrawRectangle(15, 15, 300, 115, (Color){ 0, 0, 0, 170 });
        DrawText("TARGET PRACTICE", 30, 25, 20, RAYWHITE);
        DrawText("WASD: Move", 30, 55, 16, LIGHTGRAY);
        DrawText("Mouse: Look | Left Click: Tag", 30, 80, 16, LIGHTGRAY);
        DrawText("Hold Right Click: Zoom / ADS", 30, 105, 16, LIGHTGRAY);

        int activeTargets = 0;
        for (int i = 0; i < TARGET_COUNT; i++)
        {
            if (dummies[i].hit) activeTargets++;
        }

        DrawText(TextFormat("RED TARGETS: %d / 5", activeTargets), 30, 145, 20, RED);
        DrawText(TextFormat("PLAYER HEIGHT: %.1f", PLAYER_HEIGHT), 30, 175, 18, RAYWHITE);
        DrawText("ESC: Exit", screenWidth - 120, 25, 18, LIGHTGRAY);

        EndDrawing();
    }

    EnableCursor();
    CloseWindow();

    return 0;
}
