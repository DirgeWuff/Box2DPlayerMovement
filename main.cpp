#include <iostream>
#include <vector>
#include <cassert>
#include "raylib.h"
#include "box2d/box2d.h"
#include "box2d/types.h"

// #define DEBUG

constexpr int windowWidth = 640;
constexpr int windowHeight = 480;
constexpr float ppm = 100.0f;
constexpr float timeStep = 1.0f / 60.0f;
constexpr int subStep = 4;

class Player;

Vector2 m2PxVec(const b2Vec2 vec) {
    return Vector2{vec.x * ppm, vec.y * ppm};
}

b2Vec2 px2MVec(const Vector2 vec) {
    return b2Vec2{vec.x / ppm, vec.y / ppm};
}

float m2Px(const float n) {
    return n * ppm;
}

float px2M(const float n) {
    return n / ppm;
}

Vector2 toRayVec2(const b2Vec2 vec) {
    return Vector2{vec.x, vec.y};
}

void drawB2ShapeDebug(const Player& player);

class Platform {
public:

    b2Vec2 m_size{};
    b2Vec2 m_position{};
    b2BodyDef m_bodyDef{};
    b2BodyId m_body{};
    b2Polygon m_boundingBox{};
    b2Vec2 m_boundingBoxSize{};
    b2ShapeDef m_shapeDef{};

    Platform(
        const float X,
        const float Y,
        const float width,
        const float height,
        const b2WorldId world) :
    m_size{px2M(width), px2M(height)},
    m_position{px2M(X), px2M(Y)},
    m_bodyDef(b2DefaultBodyDef())
    {
        m_bodyDef.position = {m_position.x, m_position.y};
        m_bodyDef.type = b2_staticBody;
        m_body = b2CreateBody(world, &m_bodyDef);

        m_boundingBox = b2MakeBox(m_size.x, m_size.y);
        m_shapeDef = b2DefaultShapeDef();
        m_shapeDef.material.friction = 5;
        b2CreatePolygonShape(m_body, &m_shapeDef, &m_boundingBox); // May not be destroyed in unload()???
    }
};

class Player {
public:
    b2Vec2 m_size{};
    b2Vec2 m_position{};
    b2BodyDef m_bodyDef{};
    b2BodyId m_body{};
    b2Polygon m_boundingBox{};
    b2ShapeDef m_shapeDef{};

    Player(const float X, const float Y, const b2WorldId world) :
    m_size{px2M(30), px2M(30)},
    m_position{px2M(X), px2M(Y)},
    m_bodyDef(b2DefaultBodyDef())
    {
        m_bodyDef.type = b2_dynamicBody;
        m_bodyDef.position = {m_position.x + m_size.x / 2, m_position.y + m_size.y};
        m_bodyDef.fixedRotation = true;
        m_bodyDef.linearDamping = 5.0;
        m_body = b2CreateBody(world, &m_bodyDef);

        m_boundingBox = b2MakeBox(m_size.x, m_size.y);
        m_shapeDef = b2DefaultShapeDef();
        m_shapeDef.density = 0.05f;
        m_shapeDef.material.friction = 0.40f;
        m_shapeDef.material.restitution = 0.0f;
        b2CreatePolygonShape(m_body, &m_shapeDef, &m_boundingBox);
    }

    Player() = default;

    void update() {
        m_position = b2Body_GetPosition(m_body);
    }

    void draw() const {
        DrawRectangle(
            static_cast<int>(m2Px(m_position.x)),
            static_cast<int>(m2Px(m_position.y)),
            static_cast<int>(m2Px(m_size.x)) * 2,
            static_cast<int>(m2Px(m_size.y)) * 2,
            RED);

        #ifdef DEBUG
            DrawText(TextFormat("m_position.x: %f", m_position.x), 10, 10, 15, RED);
            DrawText(TextFormat("m_position.y: %f", m_position.y), 10, 30, 15, RED);
        #endif
    }

    void moveRight() const {
        const float mass = b2Body_GetMass(m_body);

        b2Body_ApplyLinearImpulse(
            m_body,
            {mass * .50f, 0.0f},
            b2Body_GetWorldCenterOfMass(m_body),
            true);
    }

    void moveLeft() const {
        const float mass = b2Body_GetMass(m_body);

        b2Body_ApplyLinearImpulse(
            m_body,
            {-(mass * .50f), 0.0f},
            b2Body_GetWorldCenterOfMass(m_body),
            true);
    }

    void jump() const {
        const float mass = b2Body_GetMass(m_body);
        b2Body_ApplyLinearImpulse(
            m_body,
            {0.0f, -(mass * 6.0f)},
            b2Body_GetWorldCenterOfMass(m_body),
            true);
    }

};

class World {
public:
    b2WorldDef m_worldDef{};
    b2WorldId m_worldID{};
    Player m_player{};
    std::vector<Platform> m_platforms{};
    std::vector<Platform> m_invisibleWalls{};

    World() :
        m_worldDef(b2DefaultWorldDef())
    {
        m_worldDef.gravity = {0.0f, 10.0f};
        m_worldID = b2CreateWorld(&m_worldDef);
        m_player = Player(static_cast<float>(windowWidth) / 2, 300, m_worldID);

        m_platforms.emplace_back(0, windowHeight - 30, windowWidth, 30, m_worldID);

        m_invisibleWalls.emplace_back(-30.0f, 0.0f, 1.0f, windowHeight - 30.0f, m_worldID);
        m_invisibleWalls.emplace_back(windowWidth - 30.0f, 0.0f, 1.0f, windowHeight - 30, m_worldID);
        TraceLog(LOG_INFO, "World created.");
    }

    void update() {
        if (IsKeyDown(KEY_D)) {
            m_player.moveRight();
        }
        else if (IsKeyDown(KEY_A)) {
            m_player.moveLeft();
        }

        if (IsKeyPressed(KEY_SPACE)) {
            m_player.jump();
        }

        m_player.update();
        b2World_Step(m_worldID, timeStep, subStep);
    }

    void draw() const {
        for (const auto& platform : m_platforms) {
            DrawRectangle(
                static_cast<int>(m2Px(platform.m_position.x)),
                static_cast<int>(m2Px(platform.m_position.y)) ,
                static_cast<int>(m2Px(platform.m_size.x)),
                static_cast<int>(m2Px(platform.m_size.y)),
                WHITE);
        }

        m_player.draw();
        #ifdef DEBUG
            drawB2ShapeDebug(m_player);
        #endif

    }

    void unload() const {
        for (const auto& platform : m_platforms) {
            b2DestroyBody(platform.m_body);
        }
        b2DestroyBody(m_player.m_body);
        b2DestroyWorld(m_worldID);

        TraceLog(LOG_INFO, "World unloaded.");
    }
};

// Later implementations should take a subclass of Player rather than Player class itself
void drawB2ShapeDebug(const Player& player) {
    assert(b2Body_GetShapeCount(player.m_body) != 0 && "Assertion failed. Body contains no shapes.");

    const int maxShapes = b2Body_GetShapeCount(player.m_body);

    b2ShapeId shapes[maxShapes];
    b2Body_GetShapes(player.m_body, shapes, maxShapes);
    b2Transform rp;

    const b2Vec2 offset = player.m_size;
    rp.q = b2Body_GetRotation(player.m_body);
    rp.p = b2Add(b2Body_GetPosition(player.m_body), offset);

    constexpr Color lineColor = {0, 0, 255, 255};

    // Probably really slow, but fuck it, this is a debug function
    for (const auto& shape : shapes) {
        const b2Polygon polygon = b2Shape_GetPolygon(shape);
        const int numVerts = polygon.count;


        b2Vec2 tfdVerts[numVerts];
        for (size_t i = 0; i < numVerts; i++) {
            tfdVerts[i] = b2TransformPoint(rp, polygon.vertices[i]);
        }

        for (size_t i = 0; i < numVerts; i++) {
            if (i + 1 == numVerts) {
                DrawLineEx(
             m2PxVec(tfdVerts[i]),
             m2PxVec(tfdVerts[0]),
             1.0f,
             lineColor);
            }
            else {
                DrawLineEx(
             m2PxVec(tfdVerts[i]),
             m2PxVec(tfdVerts[i + 1]),
             1.0f,
             lineColor);
            }
        }
    }
}

int main() {
    InitWindow(windowWidth, windowHeight, "Box2DTest with player");
    SetTargetFPS(60);

    World world = World();

    while (!WindowShouldClose()) {
        world.update();

        BeginDrawing();
        ClearBackground(BLACK);
        world.draw();
        EndDrawing();
    }

    world.unload();

    return 0;
}