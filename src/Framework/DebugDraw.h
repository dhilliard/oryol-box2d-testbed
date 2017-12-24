#pragma once

#include "Gfx/Gfx.h"
#include "Box2D\Box2D.h"
#include "glm/mat3x2.hpp"
#include "glm/mat2x2.hpp"
#include "glm/mat4x4.hpp"
#include "Core\Containers\Array.h"

struct CameraSetup {
	glm::vec2 WorldPosition;
	float Rotation;
	float Zoom;
	uint32_t ViewportWidth;
	uint32_t ViewportHeight;
	float zNear;
	float zFar;
	CameraSetup();
	CameraSetup(uint32_t viewportWidth, uint32_t viewportHeight);
};
//Extremely dumb camera class; Update must be called after any changes to the transform params.
//TODO: Potentially expose the camera center parameter to the public interface
struct Camera
{
	//Sets up the camera with a viewport with the specified parameters
	//TODO: Investigate adding a viewport structure
	void Setup(const CameraSetup & setup = CameraSetup());
	//Update must be called once per frame (or at least when the underlying transform is modified)
	void Update();
	glm::vec2 ConvertScreenToWorld(const glm::vec2& screenPoint);
	glm::vec2 ConvertWorldToScreen(const glm::vec2& worldPoint);
	glm::vec2 ConvertScreenToWorldDir(const glm::vec2& screenDir);
	glm::vec2 ConvertWorldToScreenDir(const glm::vec2& worldDir);
	glm::mat4 BuildProjectionViewMatrix(float zBias);
	float GetWidth() const;
	float GetHeight() const;
	void ResizeViewport(uint32_t width, uint32_t height);

	glm::vec2 WorldPosition; //Controls the center of the camera
	float Zoom; //Vertical/Horizontal Zoom are controlled by the same parameter.
	float Rotation; //Rotation is in degrees; +ve = CW, -ve = CCW
private:
	glm::vec2 viewportCenter;
	float zNear;
	float zFar;
	glm::mat2 worldToScreenMatrix;
	glm::mat2 screenToWorldMatrix;
	glm::vec2 translationOffset;
	glm::mat4 projectionMatrix;
};

// This class implements debug drawing callbacks that are invoked
// inside b2World::Step.
class DebugDraw : public b2Draw
{
public:

	void Setup(const Oryol::GfxSetup & setup);
	void Discard();

	void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;

	void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;

	void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) override;

	void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) override;

	void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override;

	void DrawTransform(const b2Transform& xf) override;

	void DrawPoint(const b2Vec2& p, float32 size, const b2Color& color) override;

	void DrawString(int x, int y, const char* string, ...);

	void DrawString(const b2Vec2& p, const char* string, ...);

	void DrawAABB(b2AABB* aabb, const b2Color& color);

	void Render(const glm::mat4 & mvpMatrix);

private:
	struct instance_t {
		float x, y;
		float size;
		uint32_t color;
	};
	struct vertex_t {
		float x, y;
		uint32_t color;
	};
	Oryol::DrawState drawState[3];
	Oryol::ResourceLabel label;
	Oryol::Array<vertex_t> lines;
	Oryol::Array<vertex_t> triangles;
	Oryol::Array<instance_t> points;
	static const int MaxNumLineVertices = 2 * 32 * 1024;
	static const int MaxNumTriangleVertices = 2 * 32 * 1024;
	static const int MaxNumPointVertices = 1 * 32 * 1024;
	void LineVertex(const b2Vec2 & position, const b2Color & color);
	void TriangleVertex(const b2Vec2 & position, const b2Color & color);
	void PointVertex(const b2Vec2 & position, const b2Color & color, float32 size);
};

extern DebugDraw g_debugDraw;
extern Camera g_camera;
