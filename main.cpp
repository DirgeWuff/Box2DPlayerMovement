#include <vector>
#include <cassert>
#include <complex>
#include <iostream>

#include "raylib.h"
#include "box2d/box2d.h"
#include "box2d/types.h"

#define ENABLE_DEBUG // Comment or uncomment this line to enable or disable debugging features

constexpr int WINDOW_WIDTH = 640;
constexpr int WINDOW_HEIGHT = 480;
constexpr float PPM = 100.0f; // Pixels per meter
constexpr float TIME_STEP = 1.0f / 60.0f; // Time step for world step, 60hz
constexpr int SUB_STEP = 4; // Sub step for world step
constexpr Color DEBUG_COLOR = {0, 0, 255, 255}; // Color used to draw debugging shapes

class BoxBody;
void drawDebugBodyPolygons(const BoxBody& targetBody);
void drawDebugBodyCenter(const BoxBody& targetBody);

// Convert meters to pixels using a b2Vec2
Vector2 m2PxVec(const b2Vec2 vec) {
    return Vector2{vec.x * PPM, vec.y * PPM};
}

// Convert pixels to meters using a Vector2
b2Vec2 px2MVec(const Vector2 vec) {
    return b2Vec2{vec.x / PPM, vec.y / PPM};
}

// Convert meters to pixels using a float
float m2Px(const float n) {
    return n * PPM;
}

// Convert pixels to meters using a float
float px2M(const float n) {
    return n / PPM;
}

// Sloppy comparison function since the == operator doesn't work on b2ShapeID
// Remove if better fix is found later
bool isShapeIdEqual(const b2ShapeId& idOne, const b2ShapeId& idTwo) {
    return idOne.generation == idTwo.generation &&
            idOne.index1 == idTwo.index1 &&
            idOne.world0 == idTwo.world0;
}

// Base object class
class BoxBody {
protected:
    b2Vec2 m_size{};
    b2Vec2 m_centerPostion{};
    b2BodyDef m_bodyDef{};
    b2BodyId m_body{};
    b2ShapeDef m_shapeDef{};
public:
    BoxBody() = default;
    virtual ~BoxBody() = default;

    BoxBody(
        const float centerX,
        const float centerY,
        const float fullWidth,
        const float fullHeight,
        const b2WorldId world) :
        m_size{px2M(fullWidth), px2M(fullHeight)},
        m_centerPostion{px2M(centerX), px2M(centerY)},
        m_bodyDef(b2DefaultBodyDef())
    {
        m_bodyDef.position = m_centerPostion;
        m_bodyDef.type = b2_staticBody;
        m_body = b2CreateBody(world, &m_bodyDef);

        b2Polygon boundingBox = b2MakeBox(m_size.x / 2.0f, m_size.y / 2.0f);
        m_shapeDef = b2DefaultShapeDef();
        m_shapeDef.material.friction = 0.50f;
        m_shapeDef.enableSensorEvents = true;
        b2CreatePolygonShape(m_body, &m_shapeDef, &boundingBox);
    };

    virtual void unload() const {
        b2DestroyBody(m_body);
    };

    virtual void draw() const {
        DrawRectangle(
            static_cast<int>(m2Px(m_centerPostion.x)) - static_cast<int>(m2Px(m_size.x)) / 2,
            static_cast<int>(m2Px(m_centerPostion.y)) - static_cast<int>(m2Px(m_size.y)) / 2,
            static_cast<int>(m2Px(m_size.x)),
            static_cast<int>(m2Px(m_size.y)),
            WHITE
        );
    }

    b2Vec2 getSize() const {
        return m_size;
    }

    b2Vec2 getPosition() const {
        return m_centerPostion;
    }

    b2BodyId getBodyID() const {
        return m_body;
    }
};

// Platform/floor class/invisible wall class
class Platform final : public BoxBody { using BoxBody::BoxBody; };

// Player class
class Player final : public BoxBody {
protected:
    b2Polygon m_footSensorBox{};
    b2ShapeDef m_footSensorShape{};
    b2ShapeId m_footID{};
    bool m_feetOnGround{};
public:
    Player() = default;

    Player(const float centerX, const float centerY, const b2WorldId world) :
    m_footSensorShape(b2DefaultShapeDef())
    {
        // Body def and basic params
        m_size = {px2M(60.0f), px2M(60.0f)};
        m_centerPostion = {px2M(centerX), px2M(centerY)};
        m_bodyDef = b2DefaultBodyDef();
        m_bodyDef.position = m_centerPostion;
        m_bodyDef.type = b2_dynamicBody;
        m_bodyDef.fixedRotation = true;
        m_bodyDef.linearDamping = 8.0f;
        m_body = b2CreateBody(world, &m_bodyDef);

        // Shape def
        b2Capsule boundingCapsule = {
            px2MVec(Vector2{30.0f, 0.0f}),
            px2MVec(Vector2{30.0f, 0.0f}),
            px2M(15.0f)};
        m_shapeDef = b2DefaultShapeDef();
        m_shapeDef.material.friction = 0.40f;
        m_shapeDef.material.restitution = 0.0f;
        b2CreateCapsuleShape(m_body, &m_shapeDef, &boundingCapsule);

        // Foot sensor stuff
        m_footSensorBox = b2MakeOffsetBox(
            px2M(10.0f),
            px2M(10.0f),
            {0.0f, m_size.y / 2.0f},
            b2MakeRot(0.0f));
        m_footSensorShape = b2DefaultShapeDef();
        m_footSensorShape.isSensor = true;
        m_footSensorShape.enableSensorEvents = true;
        m_footID = b2CreatePolygonShape(m_body, &m_footSensorShape, &m_footSensorBox);
        m_feetOnGround = false;
    }

    void update() {
        m_centerPostion = b2Body_GetPosition(m_body);
    }

    void draw() const override {
        DrawRectangle(
            static_cast<int>(m2Px(m_centerPostion.x)) - static_cast<int>(m2Px(m_size.x)) / 2,
            static_cast<int>(m2Px(m_centerPostion.y)) - static_cast<int>(m2Px(m_size.y)) / 2,
            static_cast<int>(m2Px(m_size.x)),
            static_cast<int>(m2Px(m_size.y)),
            RED);

        #ifdef ENABLE_DEBUG
            DrawText(TextFormat("Player center X: %f", m_centerPostion.x), 10, 10, 15, RED);
            DrawText(TextFormat("Player center Y: %f", m_centerPostion.y), 10, 30, 15, RED);
            if (m_feetOnGround) {
                DrawText("Foot sensor in contact with object", 10, 50, 15, RED);
            }
            else {
                DrawText("Foot sensor not in contact with object", 10, 50, 15, RED);
            }
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
        if (m_feetOnGround) {
            b2Body_ApplyLinearImpulse(
            m_body,
            {0.0f, -(mass * 10.0f)},
            b2Body_GetWorldCenterOfMass(m_body),
            true);
        }
    }

    void setFootStatus(const bool status) {
        if (status != m_feetOnGround) {
            m_feetOnGround = status;
        }
    }

    b2ShapeId getFootSensorId() const {
        return m_footID;
    }
};

// World class.
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
        m_worldDef.gravity = {0.0f, 20.0f};
        m_worldID = b2CreateWorld(&m_worldDef);
        m_player = Player(
            30.0f,
            300.0f,
            m_worldID);

        m_platforms.emplace_back(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT - 20.0f, WINDOW_WIDTH, 50.0f, m_worldID);
        m_platforms.emplace_back(WINDOW_WIDTH / 4.0f, 400.0f, 130.0f, 30.0f, m_worldID);
        m_platforms.emplace_back(WINDOW_WIDTH / 2.0f, 360.0f, 130.0f, 30.0f, m_worldID);
        m_platforms.emplace_back(WINDOW_WIDTH * 0.75f, 400.0f, 130.0f, 30, m_worldID);

        m_invisibleWalls.emplace_back(0.0f, WINDOW_HEIGHT / 2.0f, 1.0f, WINDOW_HEIGHT, m_worldID);
        m_invisibleWalls.emplace_back(WINDOW_WIDTH, WINDOW_HEIGHT / 2.0f, 1.0f, WINDOW_HEIGHT, m_worldID);
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
        b2World_Step(m_worldID, TIME_STEP, SUB_STEP);
        handleSensorEvents();
    }

    void draw() const {
        for (const auto& platform : m_platforms) {
            platform.draw();

            #ifdef ENABLE_DEBUG
                drawDebugBodyPolygons(platform);
                drawDebugBodyCenter(platform);
            #endif
        }

        m_player.draw();
        #ifdef ENABLE_DEBUG
            drawDebugBodyPolygons(m_player);
            drawDebugBodyCenter(m_player);
        #endif
    }

    void unload() const {
        for (const auto& platform : m_platforms) {
            platform.unload();
        }

        m_player.unload();
        b2DestroyWorld(m_worldID);

        TraceLog(LOG_INFO, "World destroyed.");
    }

    // Handle events generated by foot sensor contact.
    void handleSensorEvents() {
        const b2SensorEvents sensorContactEvents = b2World_GetSensorEvents(m_worldID);

        for (std::size_t i = 0; i < sensorContactEvents.beginCount; i++) {
            const b2SensorBeginTouchEvent& event = sensorContactEvents.beginEvents[i];
            if (isShapeIdEqual(event.sensorShapeId, m_player.getFootSensorId())) {
                m_player.setFootStatus(true);
            }
        }

        for (std::size_t i = 0; i < sensorContactEvents.endCount; i++) {
            const b2SensorEndTouchEvent& event = sensorContactEvents.endEvents[i];
            if (isShapeIdEqual(event.sensorShapeId, m_player.getFootSensorId())) {
                m_player.setFootStatus(false);
            }
        }
    }
};

// Draw a BoxBody's shape for debugging purposes, based on the b2ShapeID.
void drawDebugBodyPolygons(const BoxBody& targetBody) {
    assert(b2Body_GetShapeCount(targetBody.getBodyID()) != 0 && "Assertion failed. Body contains no shapes.");

    const int maxShapes = b2Body_GetShapeCount(targetBody.getBodyID());

    b2ShapeId shapes[maxShapes];
    b2Body_GetShapes(targetBody.getBodyID(), shapes, maxShapes);

    b2Transform tf;
    tf.q = b2Body_GetRotation(targetBody.getBodyID());
    tf.p = b2Body_GetPosition(targetBody.getBodyID());

    // Probably really slow, but fuck it, this is a debug function
    for (const auto& shape : shapes) {
        switch (b2Shape_GetType(shape)) {
            case b2_polygonShape: {
                const b2Polygon polygon = b2Shape_GetPolygon(shape);
                const int numVerts = polygon.count;

                b2Vec2 tfdVerts[numVerts];
                for (size_t i = 0; i < numVerts; i++) {
                    tfdVerts[i] = b2TransformPoint(tf, polygon.vertices[i]);
                }

                for (size_t i = 0; i < numVerts; i++) {
                    if (i + 1 == numVerts) {
                        DrawLineEx(
                     m2PxVec(tfdVerts[i]),
                      m2PxVec(tfdVerts[0]),
                     1.0f,
                     DEBUG_COLOR);
                    }
                    else {
                        DrawLineEx(
                         m2PxVec(tfdVerts[i]),
                          m2PxVec(tfdVerts[i + 1]),
                          1.0f,
                          DEBUG_COLOR);
                    }
                }
                break;
            }
            case b2_capsuleShape: { // NOT WORKING
                const b2Capsule capsule = b2Shape_GetCapsule(shape);

                const b2Vec2 tfedP1 = b2TransformPoint(tf, capsule.center1);
                const b2Vec2 tfedP2 = b2TransformPoint(tf, capsule.center2);
                const float rad = capsule.radius;

                const b2Vec2 axis = tfedP2 - tfedP1;
                const b2Vec2 direction = b2Normalize(axis);
                const b2Vec2 normals = {-direction.x, direction.y};

                const b2Vec2 quads[4] = {
                    tfedP1 + rad * normals,
                    tfedP2 + rad * normals,
                    tfedP1 - rad * normals,
                    tfedP2 - rad * normals
                };

                DrawLineEx(m2PxVec(quads[0]), m2PxVec(quads[1]), 1.0f, DEBUG_COLOR);
                DrawLineEx(m2PxVec(quads[1]), m2PxVec(quads[2]), 1.0f, DEBUG_COLOR);
                DrawLineEx(m2PxVec(quads[2]), m2PxVec(quads[3]), 1.0f, DEBUG_COLOR);
                DrawLineEx(m2PxVec(quads[3]), m2PxVec(quads[0]), 1.0f, DEBUG_COLOR);

                const float capRadius = atan2f(direction.x, direction.y) * RAD2DEG;

                DrawCircleSectorLines(
                    m2PxVec(tfedP1),
                    rad,
                    capRadius + 90.0f,
                    capRadius - 90.0f,
                    20, DEBUG_COLOR);
                DrawCircleSectorLines(
                    m2PxVec(tfedP2),
                    rad,
                    capRadius - 90.0f,
                    capRadius + 90.0f,
                    20,
                    DEBUG_COLOR);
                break;
            }
            default: {
                std::cout << "Unsupported shape type." << std::endl;
                break;
            }
        }
    }
}

// Draw the center point of a BoxBody for debugging purposes.
void drawDebugBodyCenter(const BoxBody& targetBody) {
    const b2Vec2 origin = b2Body_GetPosition(targetBody.getBodyID());

    DrawCircleLines(
        static_cast<int>(m2Px(origin.x)),
        static_cast<int>(m2Px(origin.y)),
        5.0f,
        DEBUG_COLOR
    );
}

int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Box2D player movement demo");
    SetTargetFPS(60);

    auto world = World();

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