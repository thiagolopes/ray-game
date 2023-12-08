#include "raylib.h"
#include "stdlib.h"
#include "stdbool.h"
#include <math.h>
#include <stdio.h>

#define RLIGHTS_IMPLEMENTATION
#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
#else // PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION 100
#endif

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define FONTS 2
#define FONT_SPACING_NORMAL 1
#define FONT_SIZE 28
#define W 1920
#define H 1080
#define FPS 144


void UpdateCameraProFPS(Camera *camera, double deltaTime, int velocity) {
  // Update camera movement/rotation
  float moviment_delta = (deltaTime * velocity);

  float foward   = ((IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) * 0.1f) * moviment_delta;
  float backward = ((IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) * 0.1f) * moviment_delta;
  float right = ((IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) * 0.1f) * moviment_delta;
  float left  = ((IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) * 0.1f) * moviment_delta;

  UpdateCameraPro(camera, (Vector3)
                  {foward - backward,
                   right - left,
                   0.0f // Move up-down
                  }, (Vector3){
                    GetMouseDelta().x * 0.05f, // Rotation: yaw
                    GetMouseDelta().y * 0.05f, // Rotation: pitch
                    0.0f // Rotation: roll
                  },
                  GetMouseWheelMove() * 2.0f);
}

typedef struct {
  bool visible;
  Texture icon;
  Vector2 pos;
  Vector2 last_pos;
} Cursor;

int main(void) {
  char *window_title = "Raylon - running";
  InitWindow(W, H, window_title);
  /* SetTargetFPS(FPS); */
  bool show_mouse = true;

  Font fonts[FONTS] = { 0 };
  fonts[0] = LoadFontEx("fonts/mono-bold.ttf", FONT_SIZE, NULL, 0);
  fonts[1] = LoadFontEx("fonts/alagard.ttf", FONT_SIZE, NULL, 0);
  /* SetTextureFilter(fonts[0].texture, TEXTURE_FILTER_POINT); */
  /* SetTextureFilter(fonts[1].texture, TEXTURE_FILTER_POINT); */

  CameraMode camera_mode = CAMERA_FIRST_PERSON;

  unsigned int p = 0;
  char *message = "Life isn't just about passing on your genes. \n"
    "We can leave behind much more than just DNA. \n"
    "Through speech, music, literature and movies... \n"
    "what we've seen, heard, felt anger, joy and sorrow, \n"
    "these are the things I will pass on. \n"
    "That's what I live for. \n"
    "We need to pass the torch, and let our \n"
    "children read our messy and sad history by its light. \n"
    "We have the magic of the digital age to do that \n"
    "with. The human race will probably come to an end \n"
    "some time, and new species may rule over this \n"
    "planet. Earth may not be forever, but we still have \n"
    "the responsibility to leave what trace of life we \n"
    "can. Building the future and keeping the past alive \n"
    "are one in the same thing.";

  Camera3D camera = { 0 };
  camera.position = (Vector3){ 0.0, 2.0, 4.0f };
  camera.target = (Vector3){ 0.0, 2.0, 0.0 };
  camera.up = (Vector3){ 0.0, 1.0, 0.0 };
  camera.fovy = 45.0;
  camera.projection = CAMERA_PERSPECTIVE;

  Shader shader = LoadShader(TextFormat("resources/shaders/glsl%i/lighting.vs", GLSL_VERSION),
                             TextFormat("resources/shaders/glsl%i/lighting.fs", GLSL_VERSION));
  Shader shader_fisheye = LoadShader(0, TextFormat("resources/shaders/glsl%i/fisheye.fs", GLSL_VERSION));
  Shader shader_pixelizer = LoadShader(0, TextFormat("resources/shaders/glsl%i/pixelizer.fs", GLSL_VERSION));

  Vector2 last_mouse_pos = { 0, 0 };

  Vector3 cubeSize = { 4.0f, 4.0f, 4.0f };
  Model cube = LoadModelFromMesh(GenMeshCube(cubeSize.x, cubeSize.y, cubeSize.z));
  Texture2D texture = LoadTexture("textures/wall.png");
  Texture2D atlas = LoadTexture("textures/atlas.png");
  Texture2D cross = LoadTexture("textures/crosshair.png");
  cube.materials[0].shader = shader;
  SetMaterialTexture(&cube.materials[0], MATERIAL_MAP_DIFFUSE, texture); // Set model material map texture
  Vector3 cubePosition = { -5.0f, 2.0f, 0.0f };

  Ray ray = { 0 }; // Picking line ray
  RayCollision collision_box = { 0 }; // Ray collision_box hit info
  RayCollision collision_sphere = { 0 };

  Vector3 sphere = { 4.0, 4.0, 4.0 };
  Vector3 actual_sphere = { 4.0, 4.0, 4.0 };
  float sphere_r = 2.0f;
  float sphere_step_x = 0.1;
  int velocity = 80;

  bool fps_cap = true;
  if (fps_cap) {
    SetTargetFPS(FPS);
  } else {
    SetTargetFPS(0);
  }

  Image image = LoadImage("textures/map01.png");
  ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  ImageColorInvert(&image);

  Mesh map_mesh = GenMeshCubicmap(image, (Vector3){1.0, 2.0, 1.0});
  Model map = LoadModelFromMesh(map_mesh);
  SetMaterialTexture(&map.materials[0], MATERIAL_MAP_DIFFUSE, texture); // Set model material map texture
  map.materials[0].shader = shader;
  Vector3 mapPosition = { -16.0f, 0.0f, -8.0f };

  while (!WindowShouldClose()) {
    // Pack entity + colisions settings;
    ray = GetMouseRay(GetMousePosition(), camera);
    collision_box = GetRayCollisionBox(ray, (BoundingBox){(Vector3){cubePosition.x - cubeSize.x / 2,
                                                                    cubePosition.y - cubeSize.y / 2,
                                                                    cubePosition.z - cubeSize.z / 2 },
                                                          (Vector3){cubePosition.x + cubeSize.x / 2,
                                                                    cubePosition.y + cubeSize.y / 2,
                                                                    cubePosition.z + cubeSize.z / 2 } });
    collision_sphere = GetRayCollisionSphere(ray, actual_sphere, sphere_r);

    if (!collision_sphere.hit) {
      actual_sphere.y = sin(6.0 * GetTime() + (GetFrameTime() * velocity)) + sphere.y;
      actual_sphere.x += sphere_step_x * (GetFrameTime() * velocity);
      if (actual_sphere.x >= 10 || actual_sphere.x <= -10) {
        sphere_step_x = -sphere_step_x;
      }
    }

    BeginDrawing();
    ClearBackground(BLACK);

    DrawTexture(cross, W / 2 - cross.width / 2, H / 2 - cross.height / 2, WHITE);

    BeginMode3D(camera);
    /* DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 32.0f, 32.0f }, Fade(LIGHTGRAY, 0.3)); // Draw ground */
    DrawCubeWires((Vector3){ 0.0, 2.0, 0.0 }, 4.0f, 4.0f, 4.0f, MAROON);
    if (collision_sphere.hit) {
      DrawSphereWires(actual_sphere, sphere_r, 10, 30, MAGENTA);
    } else {
      DrawSphere(actual_sphere, sphere_r, MAGENTA);
    }

    if (collision_box.hit) {
      DrawModelWiresEx(cube, cubePosition, (Vector3){1, 1, 1}, 0, (Vector3){ 1, 1, 1 }, WHITE);
    } else {
        DrawModel(cube, cubePosition, 1.0, WHITE);
    }

    DrawModel(map, mapPosition, 3.0f, WHITE);

    EndMode3D();

    // zoom
    float scroll = GetMouseWheelMove();
    if (scroll) {
      camera.fovy += scroll;
    }

    // update only
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
      UpdateCamera(&camera, camera_mode);
      UpdateCameraProFPS(&camera, GetFrameTime(), velocity);
      HideCursor();
      SetMousePosition(W / 2, H / 2);
    }

    if (IsKeyPressed(KEY_LEFT_SHIFT)) {
      show_mouse = !show_mouse;
    }
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
    }
    if (IsKeyUp(KEY_LEFT_SHIFT)) {
      ShowCursor();
    }

    // text experiemt
    if (IsKeyPressed(KEY_ENTER) == 1) {
      p = 0;
    }
    if (IsKeyDown(KEY_SPACE) == 1) {
      DrawTextEx(fonts[1], "Space pressed", (Vector2){ W - 500, H - 100 }, fonts[1].baseSize, FONT_SPACING_NORMAL, Fade(MAGENTA, 0.4));
      p += 4;
    }else{
      p += 1;
    }
    SetTextLineSpacing(FONT_SIZE);

    Vector2 ui_b = MeasureTextEx(fonts[1], message, fonts[1].baseSize, FONT_SPACING_NORMAL);
    DrawRectangle(30, 30, ui_b.x, ui_b.y, Fade(BLACK, 0.8));
    DrawTextEx(fonts[1], TextSubtext(message, 0, p), (Vector2){ 30, 30 }, fonts[1].baseSize, FONT_SPACING_NORMAL, WHITE);

    // my gui
    // if(Ggui_Button(Rectangle bounds, const char *text, enable)) if clicked... etc

    //gui
    GuiSliderBar((Rectangle){ 40, 400, 80, 10 }, "0", "4.0", &sphere_r, 0.0, 4.0);
    DrawTextEx(fonts[1], TextFormat("Value %f", sphere.x), (Vector2){ 30, 420 }, fonts[1].baseSize, FONT_SPACING_NORMAL, RED);

    // fps
    if (IsKeyPressed(KEY_F)) {
      fps_cap = !fps_cap;
      if (fps_cap) {
        SetTargetFPS(FPS);
      } else {
        SetTargetFPS(0);
      }
    }

    DrawTextEx(fonts[1], TextFormat("FPS Cap: %b", fps_cap), (Vector2){ 30, 440 }, fonts[1].baseSize, FONT_SPACING_NORMAL, RED);
    DrawTextEx(fonts[1], TextFormat("Time: %f", GetFrameTime()), (Vector2){ 30, 460 }, fonts[1].baseSize, FONT_SPACING_NORMAL, RED);
    DrawFPS(0, 0);
    EndDrawing();
  }

  // shutdown
  for (size_t i = 0; i < FONTS; i++) {
    UnloadFont(fonts[i]);
  }
  CloseWindow();

  return EXIT_SUCCESS;
}
