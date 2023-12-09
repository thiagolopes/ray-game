#include "raylib.h"
#include "stdlib.h"
#include "stdbool.h"
#include "stdio.h"
#include "math.h"
#include "map.h"

#include "emotional_text.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#define FONTS 10
#define W 1920
#define H 1080

void UpdateCameraRelative(Camera *camera, double deltaTime, int velocity) {
    // Update camera movement/rotation
    float moviment_delta = (deltaTime * velocity);
    float step = 0.1f;

    float foward   = ((IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) * step) * moviment_delta;
    float backward = ((IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) * step) * moviment_delta;
    float right = ((IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) * step) * moviment_delta;
    float left  = ((IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) * step) * moviment_delta;

    UpdateCameraPro(camera,
                    (Vector3){foward - backward,
                               right - left,
                               0.0f // Move up-down
                    }, (Vector3){
                        GetMouseDelta().x * 0.05f, // Rotation: yaw
                        GetMouseDelta().y * 0.05f, // Rotation: pitch
                        0.0f // Rotation: roll
                    },
                    GetMouseWheelMove() * 2.0f);
}

typedef enum {
    RUNNING_GAME,
    PAUSED,
} StatusGlobalGame;

typedef struct {
    Font font;
    float line_spc;
    int letter_spc;
    int size;
} FontGame;

#define FONT_SPACE_RATIO 1.14
#define FONT_SPACE 1
FontGame LoadFontGame(const char* path, int size) {
    float line_spc  = size * FONT_SPACE_RATIO;
    Font font = LoadFontEx(path, size, NULL, 0);

    return (FontGame){font, line_spc, FONT_SPACE, size};
}

static struct {
    float text_box_margin;
    float box_border;

    Sound* sound_click;
} GuiGameStyle = {5.0f, 3.0f, NULL} ;

#define BORDER_THICK 2.5f

Vector2 GuiGameMeasureText(const char* text, FontGame font) {
    SetTextLineSpacing(font.line_spc);
    return MeasureTextEx(font.font, text, font.size , font.letter_spc);
}

Rectangle GuiGameTextBoxMeasure(const char* text, Vector2 pos, FontGame font) {
    Vector2 measure = GuiGameMeasureText(text, font);
    return (Rectangle) {pos.x - GuiGameStyle.text_box_margin,
                        pos.y - GuiGameStyle.text_box_margin,
                        measure.x + (GuiGameStyle.text_box_margin * 2),
                        measure.y + (GuiGameStyle.text_box_margin * 2)};
}

void GuiGameDrawBorder(Rectangle size, Color color) {
    size.x -= GuiGameStyle.box_border;
    size.y -= GuiGameStyle.box_border;
    size.width += GuiGameStyle.box_border * 2;
    size.height += GuiGameStyle.box_border * 2;

    DrawRectangleLinesEx(size, BORDER_THICK, color);
}

void GuiGameDrawTextBox(const char* text, Vector2 pos, FontGame font, Color color, Color color_text) {
    Rectangle text_box = GuiGameTextBoxMeasure(text, pos, font);
    GuiGameDrawBorder(text_box, Fade(color, 0.7f));

    DrawRectangleRec(text_box, color);
    DrawEmotionalText(font.font, text, (Vector2){pos.x, pos.y}, font.size, font.letter_spc, color_text);
};

void GuiGameDrawSubTextBox(const char* text, int position, Vector2 pos, FontGame font, Color color, Color color_text) {
    Rectangle text_box = GuiGameTextBoxMeasure(text, pos, font);
    GuiGameDrawBorder(text_box, Fade(color, 0.7f));

    DrawRectangleRec(text_box, color);
    DrawEmotionalText(font.font, TextSubtext(text, 0, position), (Vector2){pos.x, pos.y}, font.size, font.letter_spc, color_text);
};

bool GuiGameDrawButton(const char* text, FontGame font, Vector2 pos, Color color, Color color_text) {
    bool is_active = false;
    Rectangle text_box = GuiGameTextBoxMeasure(text, pos, font);
    GuiGameDrawBorder(text_box, Fade(color, 0.7f));

    bool is_hover = CheckCollisionPointRec(GetMousePosition(), text_box);
    if (is_hover){
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            PlaySound(*GuiGameStyle.sound_click);
            is_active = true;
        }
        color = ColorBrightness(color, 0.4);
    }

    DrawRectangleRec(text_box, color);
    DrawEmotionalText(font.font, text, pos, font.size, font.letter_spc, color_text);

    return is_active;
}

typedef struct {
    Camera3D camera;
    CameraMode active_mode;
    CameraProjection active_proj;
} CameraGame;

#define DEFAULT_CAMERA_PROJ CAMERA_ORTHOGRAPHIC
#define DEFAULT_CAMERA_MODE CAMERA_THIRD_PERSON

// TODO use this after this infomation became from map;
CameraGame NewCameraGame(Vector3 pos, Vector3 target, int fov) {
    CameraGame camera_game = {0};
    camera_game.camera.projection = DEFAULT_CAMERA_PROJ;
    camera_game.active_mode = DEFAULT_CAMERA_MODE;
    camera_game.camera.position = pos;
    camera_game.camera.target = target;
    camera_game.camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera_game.camera.fovy = (float)fov;
    return camera_game;
}

// hardcoded functions
CameraGame NewCameraGamePerspective(void) {
    // Perspective
    // hardcoded
    CameraGame camera_game = NewCameraGame((Vector3){26.0f, 10.0f, 31.0f}, (Vector3){50.0f, 0.0f, 0.0f}, 45);
    camera_game.camera.projection = CAMERA_PERSPECTIVE;
    camera_game.active_mode = CAMERA_FREE;
    return camera_game;
}
CameraGame NewCameraGameOrtho(void) {
    CameraGame camera_game = NewCameraGame((Vector3){-43.0f, 58.0f, 128.0f}, (Vector3){37.0f, 2.0f, 11.0f}, 18);
    // Ortho
    // hardcoded
    camera_game.camera.projection = CAMERA_ORTHOGRAPHIC;
    camera_game.active_mode = CAMERA_THIRD_PERSON;
    return camera_game;
}

void UpdateCameraGamePerspective(CameraGame* camera, Camera data) {
    camera->active_mode = CAMERA_FREE;
    camera->active_proj = CAMERA_PERSPECTIVE;

    camera->camera.position = data.position;
    camera->camera.target = data.target;
    camera->camera.up = data.up;
    camera->camera.fovy = data.fovy;
    camera->camera.projection = data.projection;
}

void UpdateCameraGameOrtho(CameraGame* camera, Camera data) {
    camera->active_mode = CAMERA_THIRD_PERSON;
    camera->active_proj = CAMERA_ORTHOGRAPHIC;

    // Note: The target distance is related to the render distance in the orthographic projection
    camera->camera.position = data.position;
    camera->camera.target = data.target;
    camera->camera.up = data.up;
    camera->camera.fovy = data.fovy;
    camera->camera.projection = data.projection;
}

FontGame DEBUG_FONT;
void DebugCameraGame(CameraGame *camera, Vector2 pos) {
    GuiGameDrawTextBox(TextFormat("Pos: %.1f %.1f %.1f",
                              camera->camera.position.x,
                              camera->camera.position.y,
                              camera->camera.position.z),
                   pos, DEBUG_FONT, DARKGREEN, WHITE);
    GuiGameDrawTextBox(TextFormat("FOV: %.1f", camera->camera.fovy), (Vector2){pos.x, pos.y + 40}, DEBUG_FONT, DARKGREEN, WHITE);
    GuiGameDrawTextBox(TextFormat("Target: %.1f %1.f %.1f",
                              camera->camera.target.x,
                              camera->camera.target.y,
                              camera->camera.target.z),
                   (Vector2){pos.x, pos.y + 80}, DEBUG_FONT, DARKGREEN, WHITE);
    // add edition
    // add title
}

typedef struct {
    bool visible;
    Texture icon;
    Vector2 pos;
    Vector2 last_pos;
} CursorGame;

int main(void) {
    char *window_title = "Raylon - running";
    SetConfigFlags(FLAG_MSAA_4X_HINT);  // Enable Multi Sampling Anti Aliasing 4x (if available)
    InitWindow(W, H, window_title);
    HideCursor();

    bool show_mouse = true;
    bool fps_cap = true;

    FontGame fonts[FONTS] = { 0 };
    fonts[0] = LoadFontGame("fonts/mono-bold.ttf", 20);
    fonts[1] = LoadFontGame("fonts/alagard.ttf", 20);

    DEBUG_FONT = fonts[1];

    InitAudioDevice();
    Sound click = LoadSound("sounds/click_004.ogg");
    SetSoundVolume(click, 1.0f);
    GuiGameStyle.sound_click = &click;

    unsigned int p = 0;
    const char *message = "**Life** isn't just about passing on your genes. \n"
        "We can leave behind much more than just DNA. \n"
        "Through speech, music, literature and movies... \n"
        "what we've seen, heard, felt anger, joy and sorrow, \n"
        "these are the things I will pass on. \n"
        "~That's what I live for. ~\n"
        "We need to pass the torch, and let our \n"
        "children read our messy and sad history by its light. \n"
        "We have the magic of the digital age to do that \n"
        "with. The human race will probably come to an end \n"
        "some time, and new species may rule over this \n"
        "planet. Earth may not be forever, but we still have \n"
        "the responsibility to leave what trace of life we \n"
        "can. Building the future and keeping the past alive \n"
        "are one in the same thing.";

    CameraGame camera_game = NewCameraGamePerspective();
    CameraGame last_camera_gamer = NewCameraGameOrtho();

    int size = 4;
    Model wall = LoadModel("models/medieval01/wall.obj");
    Model wallDoom = LoadModelFromMesh(wall.meshes[0]);
    Model wallWolf = LoadModelFromMesh(wall.meshes[0]);
    Model wallFortified = LoadModel("models/medieval01/wallFortified.obj");
    Model wallFortifiedGate = LoadModel("models/medieval01/wallFortified_gate.obj");
    Model tower = LoadModel("models/medieval01/tower.obj");
    Model floor = LoadModel("models/medieval01/floor.obj");
    Model column = LoadModel("models/medieval01/column.obj");

    GenTextureMipmaps(&wall.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
    GenTextureMipmaps(&wallFortified.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
    GenTextureMipmaps(&wallFortifiedGate.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
    GenTextureMipmaps(&tower.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
    GenTextureMipmaps(&floor.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
    GenTextureMipmaps(&column.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);

    SetTextureFilter(wall.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_ANISOTROPIC_16X);
    SetTextureFilter(wallFortified.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_ANISOTROPIC_16X);
    SetTextureFilter(wallFortifiedGate.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_ANISOTROPIC_16X);
    SetTextureFilter(tower.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_ANISOTROPIC_16X);
    SetTextureFilter(floor.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_ANISOTROPIC_16X);

    Texture2D doom = LoadTexture("textures/doom.png");
    Texture2D wolf = LoadTexture("textures/wolf.png");
    SetTextureFilter(doom, TEXTURE_FILTER_ANISOTROPIC_16X);
    SetTextureFilter(wolf, TEXTURE_FILTER_ANISOTROPIC_16X);

    wallDoom.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = doom;
    wallWolf.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = wolf;

    // wall.materials[0].shader = shader;
    // floor.materials[0].shader = shader;
    // column.materials[0].shader = shader;
    // light.materials[0].shader = shader;

    Texture2D cross = LoadTexture("textures/crosshair.png");
    Texture2D cursor = LoadTexture("textures/cursor.png");

    // Ray ray = { 0 }; // Picking line ray
    // int velocity = 80;

    int monitor = GetCurrentMonitor();
    int fps = GetMonitorRefreshRate(monitor);
    if (fps_cap) {
        SetTargetFPS(fps);
    } else {
        SetTargetFPS(0);
    }

    Texture heroin = LoadTexture("textures/heroin.png");
    Grid map_file = grid_load("src/map_01");

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_Z)) {
            CameraGame temp;
            if (camera_game.active_proj == CAMERA_ORTHOGRAPHIC) {
                temp = camera_game;
                UpdateCameraGamePerspective(&camera_game, last_camera_gamer.camera);
                last_camera_gamer = temp;
            } else {
                temp = camera_game;
                UpdateCameraGameOrtho(&camera_game, last_camera_gamer.camera);
                last_camera_gamer = temp;
            }
        }

        if (IsKeyPressed(KEY_LEFT_SHIFT)) {
            show_mouse = !show_mouse;
        }
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            UpdateCamera(&camera_game.camera, camera_game.active_mode);
            // UpdateCameraProFPS(&camera, GetFrameTime(), velocity);
            SetMousePosition(W / 2, H / 2);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode3D(camera_game.camera);

        // map
        // TODO generate entities to not draws over map every frame, but over entities (batch rendering).
        // TODO push a smart algorith, for now, with smol number of itens, its ok
        for (size_t x = 0; x < map_file.rows; x++) {
            for (size_t y = 0; y < map_file.cols; y++) {
                Cel cel = map_file.cels[x][y];
                if (cel.raw_value == 0) {
                    DrawModel(floor, (Vector3){x * size, -0.2f, y * size}, size, WHITE);
                } else if (cel.raw_value == 1) {
                    //
                } else if (cel.raw_value == 2) {
                    DrawModel(wallFortified, (Vector3){x * size, 0.0f, y * size}, size, WHITE);
                } else if (cel.raw_value == 3) {
                    DrawModel(floor,(Vector3){x * size, -0.2f, y * size}, size, WHITE);
                    DrawModel(wallFortifiedGate, (Vector3){x * size, 0.0f, y * size}, size, WHITE);
                } else if (cel.raw_value == 4) {
                    DrawModel(floor,(Vector3){x * size, -0.2f, y * size}, size, WHITE);
                    DrawModel(tower, (Vector3){x * size, 0.0f, y * size}, size, WHITE);
                } else if (cel.raw_value == 5) {
                    DrawModel(wallDoom, (Vector3){x * size, 0.0f, y * size}, size, WHITE);
                } else if (cel.raw_value == 6) {
                    DrawModel(floor, (Vector3){x * size, -0.2f, y * size}, size, WHITE);
                    DrawModel(column, (Vector3){x * size, 0.2f, y * size}, size, WHITE);
                } else if (cel.raw_value == 8) {
                    DrawModel(wallWolf, (Vector3){x * size, 0.0f, y * size}, size, WHITE);
                } else if (cel.raw_value == 9) {
                    //
                } else {
                    DrawModel(wall, (Vector3){x * size, 0.0f, y * size}, size, WHITE);
                }
            }
        }

        // billboard
        DrawBillboardPro(camera_game.camera, heroin,
                         (Rectangle){0, 0, heroin.width, heroin.height},
                         (Vector3){37.0f, 1.0f, 13.0f},
                         (Vector3){0.0f, 1.0f, 0.0f},
                         (Vector2){2.5f, 2.5f},
                         (Vector2){0}, 0.0f, WHITE);
        EndMode3D();

        // 2d draw
        if (IsKeyDown(KEY_SPACE) == 1) {
            GuiGameDrawTextBox("Space pressed", (Vector2){ 30, 740} , fonts[1], BLANK, MAGENTA);
            p += 4;
        }else{
            p += 1;
        }

        GuiGameDrawSubTextBox(message, p, (Vector2){30, 30}, fonts[1], Fade(BROWN, 0.9f), WHITE);
        if (GuiGameDrawButton("Click me!!", fonts[1], (Vector2) {470, 380}, GOLD, BLACK)) {
            p = 0;
        }

        GuiGameDrawTextBox(TextFormat("Mouse Pos: %i %i", GetMouseX(), GetMouseY()), (Vector2){ 30, 820} , fonts[1], WHITE, MAGENTA);
        // GuiGameSliderBar((Rectangle){ 30, 730, 80, 10 }, "0", "4.0", &sphere_r, 0.0, 4.0);

        DebugCameraGame(&camera_game, (Vector2){30, 400});

        DrawFPS(0, 0);
        if (camera_game.active_proj == CAMERA_PERSPECTIVE) {
            DrawTexture(cross, W / 2 - cross.width / 2, H / 2 - cross.height / 2, WHITE); // cross
        }

        if (IsKeyUp(KEY_LEFT_SHIFT)) {
            DrawTexture(cursor, GetMouseX(), GetMouseY(), WHITE);
        }
        EndDrawing();
        UpdateEmotionalTextTimer();
    }

    // shutdown
    for (size_t i = 0; i < FONTS; i++) {
        UnloadFont(fonts[i].font);
    }
    CloseWindow();

    return EXIT_SUCCESS;
}
